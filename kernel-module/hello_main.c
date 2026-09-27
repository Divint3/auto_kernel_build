// SPDX-License-Identifier: GPL-2.0
/*
 * hello —— 双环境入口模块
 *
 * 内核态 (__KERNEL__)：树外内核模块
 *   - pu 库冒烟自测（bitmap / queue / ringbuffer / mem tracker）
 *   - rn8209 计量芯片驱动测试：通过 meter_uart 打开串口（默认 /dev/ttyAMA1，
 *     可用模块参数 uart_dev=... 修改），经 io_callback 与芯片通信
 * 用户态        ：普通 C 程序，同一套自测（make test 显式编译）
 */
#include "pu_bitmap.h"
#include "pu_mem_tracker.h"
#include "pu_port.h"
#include "pu_queue.h"
#include "pu_ringbuffer.h"
#include "rn8209_driver.h"
#include "nor_flash_simulator.h"

#ifdef __KERNEL__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>

#include "meter_uart.h"

/* 串口设备路径（insmod hello.ko uart_dev=/dev/ttyAMA1 可覆盖） */
static char *uart_dev = "/dev/ttyAMA1";
module_param(uart_dev, charp, 0444);
MODULE_PARM_DESC(uart_dev, "rn8209 计量芯片所接的串口设备 (默认 /dev/ttyAMA1)");

/* NOR Flash 仿真器持久化文件（相对路径基于模块加载时的进程 cwd，QEMU 里即 /） */
static char *flash_data_file = "flash_data.bin";
static char *flash_meta_file = "flash_meta.bin";
module_param(flash_data_file, charp, 0444);
module_param(flash_meta_file, charp, 0444);
MODULE_PARM_DESC(flash_data_file, "NOR Flash 仿真数据文件 (默认 flash_data.bin)");
MODULE_PARM_DESC(flash_meta_file, "NOR Flash 仿真元数据文件 (默认 flash_meta.bin)");

static nor_flash_t *meter_flash = NULL;

/* ---- NOR Flash 仿真器 -> rn8209 flash_callback 适配 ----
 * 注意: 内核态下仿真器的文件读写(flash_save_all)可能睡眠,
 *       因此 flash 回调不能在 PU_FP_BEGIN/END 保护区内的调用路径触发;
 *       rn8209 的脉冲持久化(rn8209_save_pulse_cnt 等)应在普通进程上下文调用。 */

static int flash_read_cb(uint32_t address, uint8_t *data, uint16_t size) {
  if (meter_flash == NULL)
    return -1;
  return (flash_read(meter_flash, address, data, size) == FLASH_OK) ? 0 : -1;
}

static int flash_write_cb(uint32_t address, uint8_t *data, uint16_t size) {
  if (meter_flash == NULL)
    return -1;
  /* 仿真器限制单次写不得跨扇区, 按 NOR_PAGE_SIZE 对齐边界拆分 */
  uint32_t off = 0;
  while (off < (uint32_t)size) {
    uint32_t chunk = NOR_PAGE_SIZE - ((address + off) % NOR_PAGE_SIZE);
    if (chunk > (uint32_t)(size - off))
      chunk = size - off;
    if (flash_write_page(meter_flash, address + off, data + off, chunk) != FLASH_OK)
      return -1;
    off += chunk;
  }
  return 0;
}

static int flash_erase_cb(uint32_t address) {
  if (meter_flash == NULL)
    return -1;
  return (flash_erase_sector(meter_flash, address) == FLASH_OK) ? 0 : -1;
}

/* NOR Flash 仿真器写读回环自测 */
static void flash_smoke_test(void) {
  uint8_t wbuf[16];
  uint8_t rbuf[16] = {0};
  const uint32_t test_addr = 0;

  memcpy(wbuf, "RN8209-FLASH-OK", sizeof("RN8209-FLASH-OK"));

  if (flash_erase_cb(test_addr) != 0) {
    pr_warn("hello: nor flash erase failed\n");
    return;
  }
  if (flash_write_cb(test_addr, wbuf, sizeof(wbuf)) != 0) {
    pr_warn("hello: nor flash write failed\n");
    return;
  }
  if (flash_read(meter_flash, test_addr, rbuf, sizeof(rbuf)) != FLASH_OK ||
      memcmp(wbuf, rbuf, sizeof(wbuf)) != 0) {
    pr_warn("hello: nor flash verify FAILED\n");
    return;
  }
  pr_info("hello: nor flash roundtrip OK: %s\n", rbuf);
  flash_save_all(meter_flash); // 立即持久化, 验证文件落盘链路
}

/* 驱动实例较大（脉冲块 256B + 多费率数据），放静态存储区 */
static rn8209_instance_t rn8209_inst;

/* ---- 驱动平台回调 ---- */

/* 休眠回调: 驱动在重试/校准流程中使用（注意: 不能在 FP 保护区内的调用路径触发） */
static void kernel_sleep_cb(uint16_t ms) {
  msleep(ms);
}

/* CRC32 (IEEE 802.3 反射算法) —— 纯软件实现，不依赖内核 CONFIG_CRC32 */
static uint32_t kernel_crc_cb(uint8_t *data, uint32_t size) {
  uint32_t crc = 0xFFFFFFFFu;
  while (size--) {
    crc ^= *data++;
    for (int i = 0; i < 8; i++) {
      crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
  }
  return ~crc;
}

/*
 * rn8209 驱动冒烟测试
 *
 * 浮点说明: rn8209_driver.c 含 float 运算（arm64 内核默认禁浮点，该文件已在
 * Makefile 中单独放开），按内核规则，执行浮点指令前必须用 PU_FP_BEGIN/END
 * 保存/恢复用户态 FP 状态。保护区会关闭抢占 -> 区间内禁止睡眠，
 * meter_uart 实现为纯 udelay 轮询，可安全使用。
 * 今后调用其他 rn8209 浮点 API（calc_cycle/校准/读取类）同样必须包在
 * PU_FP_BEGIN()/PU_FP_END() 内。
 */
static void rn8209_smoke_test(void) {
  rn8209_preset_t preset;
  uext32_t device_id;
  bool ok;

  memset(&rn8209_inst, 0, sizeof(rn8209_inst));
  memset(&preset, 0, sizeof(preset));
  /* TODO: 按芯片手册/原理图填写 SYSCON/EMUCON/... 预设寄存器与 uv/ui 转换系数 */
  preset.uv = 1.0f;
  preset.ui = 1.0f;

  rn8209_inst.sleep              = kernel_sleep_cb;
  rn8209_inst.data_crc_callback  = kernel_crc_cb;
  rn8209_inst.io_callback        = meter_uart_io();
  /* NOR Flash 仿真器 -> 脉冲数据持久化(掉电保存), flash 回调在 hello_init 中注入 */
  rn8209_inst.flash_desc.page_size    = NOR_PAGE_SIZE;
  rn8209_inst.flash_desc.sector_size  = NOR_SECTOR_SIZE;
  rn8209_inst.flash_desc.sector_count = SECTOR_COUNT;
  rn8209_inst.flash_desc.start_address = 0;
  rn8209_inst.flash_desc.end_address   = FLASH_SIZE - 1;
  rn8209_inst.flash_callback.read  = flash_read_cb;
  rn8209_inst.flash_callback.write = flash_write_cb;
  rn8209_inst.flash_callback.erase = flash_erase_cb;

  PU_FP_STATE(); // FP 状态保存缓冲区（arm64 内核 7.2+ kernel_neon_begin 需要调用方提供）
  PU_FP_BEGIN();
  ok = rn8209_init(&rn8209_inst, &preset);
  if (ok) {
    ok = rn8209_read_register_by_name(&rn8209_inst, RN8209_REG_DeviceID, &device_id);
  }
  PU_FP_END();

  if (ok) {
    pr_info("hello: rn8209 init OK, DeviceID=0x%06X\n", device_id.dword);
  } else {
    pr_warn("hello: rn8209 通信失败(芯片未接/预设寄存器未配置), 设备=%s\n", uart_dev);
  }
}

static int __init hello_init(void) {
  pr_info("hello: module loaded (built against running kernel)\n");

  /* pu 库冒烟自测：bitmap / queue / ringbuffer / mem tracker */
  pu_bitmap_p bm = pu_bitmap_create(64);
  if (bm != NULL) {
    pu_bitmap_set_bit(bm, 3);
    pu_bitmap_set_bit(bm, 63);
    pr_info("hello: bitmap bit3=%d bit63=%d set_count=%zu\n",
            pu_bitmap_test_bit(bm, 3) ? 1 : 0,
            pu_bitmap_test_bit(bm, 63) ? 1 : 0,
            pu_bitmap_count_set(bm));
    pu_bitmap_destroy(bm);
  }

  pu_queue_p q = pu_queue_create(4);
  if (q != NULL) {
    static int item = 42;
    pu_queue_enqueue(q, &item);
    int *out = (int *)pu_queue_dequeue(q);
    pr_info("hello: queue dequeue=%d\n", out ? *out : -1);
    pu_queue_destroy(q);
  }

  pu_ringbuffer_p rb = pu_ringbuffer_create_dynamic(16);
  if (rb != NULL) {
    pu_ringbuffer_enqueue_byte(rb, 0xAA);
    uint8_t byte = 0;
    pu_ringbuffer_dequeue_byte(rb, &byte);
    pr_info("hello: ringbuffer roundtrip=0x%02X\n", byte);
    pu_ringbuffer_destroy(rb);
  }

  pr_info("hello: mem peak=%zu current=%zu bytes\n",
          pu_mem_get_peak_usage(), pu_mem_get_current_usage());

  /* NOR Flash 仿真器（文件持久化）。此后 rn8209_inst.flash_callback 可用,
   * 可在业务代码中调用 rn8209_load_pulse_cnt/rn8209_save_pulse_cnt 做掉电保存 */
  meter_flash = flash_init(flash_data_file, flash_meta_file);
  if (meter_flash != NULL) {
    flash_smoke_test();
  } else {
    pr_warn("hello: nor flash init failed, pulse persistence disabled\n");
  }

  /* 打开串口并跑 rn8209 冒烟测试。
   * 打开失败（如 QEMU virt 默认只有 ttyAMA0）只告警，不影响模块加载。 */
  int ret = meter_uart_open(uart_dev);
  if (ret == 0) {
    rn8209_smoke_test();
  } else {
    pr_warn("hello: open %s failed (err=%d), skip rn8209 test\n", uart_dev, ret);
  }
  return 0;
}

static void __exit hello_exit(void) {
  if (meter_flash != NULL) {
    flash_deinit(meter_flash); // 自动 save_all 后释放
    meter_flash = NULL;
  }
  meter_uart_close();
  pr_info("hello: module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WorkBuddy");
MODULE_DESCRIPTION("rn8209 metering chip driver + pu library, UART via /dev/ttyAMA1");

#else /* 用户态 */

#include <stdio.h>

int main(void) {
  pu_mem_tracker_init();
  printf("hello: userspace selftest start\n");

  pu_bitmap_p bm = pu_bitmap_create(64);
  if (bm != NULL) {
    pu_bitmap_set_bit(bm, 3);
    pu_bitmap_set_bit(bm, 63);
    printf("hello: bitmap bit3=%d bit63=%d set_count=%zu\n",
           pu_bitmap_test_bit(bm, 3), pu_bitmap_test_bit(bm, 63),
           pu_bitmap_count_set(bm));
    pu_bitmap_destroy(bm);
  }

  pu_queue_p q = pu_queue_create(4);
  if (q != NULL) {
    static int item = 42;
    pu_queue_enqueue(q, &item);
    int *out = (int *)pu_queue_dequeue(q);
    printf("hello: queue dequeue=%d\n", out ? *out : -1);
    pu_queue_destroy(q);
  }

  pu_ringbuffer_p rb = pu_ringbuffer_create_dynamic(16);
  if (rb != NULL) {
    pu_ringbuffer_enqueue_byte(rb, 0xAA);
    uint8_t byte = 0;
    pu_ringbuffer_dequeue_byte(rb, &byte);
    printf("hello: ringbuffer roundtrip=0x%02X\n", byte);
    pu_ringbuffer_destroy(rb);
  }

  printf("hello: mem peak=%zu current=%zu bytes\n",
         pu_mem_get_peak_usage(), pu_mem_get_current_usage());
  printf("=== userspace selftest OK ===\n");

  pu_mem_tracker_dump();
  pu_mem_tracker_cleanup();
  return 0;
}

#endif /* __KERNEL__ */
