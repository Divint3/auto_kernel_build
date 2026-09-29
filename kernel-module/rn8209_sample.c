#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>

#include "meter_uart.h"
#include "nor_flash_simulator.h"
#include "pu_port.h"
#include "pu_type.h"
#include "rn8209_driver.h"
static nor_flash_t *meter_flash = NULL;

static char *uart_dev = "/dev/ttyAMA1";
module_param(uart_dev, charp, 0444);
MODULE_PARM_DESC(uart_dev, "rn8209 计量芯片所接的串口设备 (默认 /dev/ttyAMA1)");
/* NOR Flash 仿真器持久化文件（相对路径基于模块加载时的进程 cwd，QEMU 里即 /）
 */
static char *flash_data_file = "flash_data.bin";
static char *flash_meta_file = "flash_meta.bin";
module_param(flash_data_file, charp, 0444);
module_param(flash_meta_file, charp, 0444);
MODULE_PARM_DESC(flash_data_file,
                 "NOR Flash 仿真数据文件 (默认 flash_data.bin)");
MODULE_PARM_DESC(flash_meta_file,
                 "NOR Flash 仿真元数据文件 (默认 flash_meta.bin)");

typedef enum {
  RN8209_DEVICE_STATUS_DUMP = 0,
  RN8209_DEVICE_STATUS_VOLTAGE,
  RN8209_DEVICE_STATUS_CURRENT,
} rn8209_device_status_e;

static rn8209_device_status_e status = RN8209_DEVICE_STATUS_DUMP;
static char rn8209_device_buff[64] = "voltage 220.0 current 5.0";

static void kernel_sleep_cb(uint16_t ms) { msleep(ms); }
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
    if (flash_write_page(meter_flash, address + off, data + off, chunk) !=
        FLASH_OK)
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

static rn8209_instance_t rn8209_inst;
static void rn8209_register_init(void) {
  rn8209_preset_t preset;
  uext32_t device_id;
  bool ok;

  memset(&rn8209_inst, 0, sizeof(rn8209_inst));
  memset(&preset, 0, sizeof(preset));
  /* TODO: 按芯片手册/原理图填写 SYSCON/EMUCON/... 预设寄存器与 uv/ui 转换系数
   */
  preset.uv = 1.0f;
  preset.ui = 1.0f;
  preset.SYSCON.byte[0] = 0x16;
  preset.SYSCON.byte[1] = 0x03;

  preset.EMUCON.byte[0] = 0x80;
  preset.EMUCON.byte[1] = 0x03;

  preset.EMUCON2.byte[0] = 0x01;
  preset.EMUCON2.byte[1] = 0x80;

  rn8209_inst.sleep = kernel_sleep_cb;
  rn8209_inst.data_crc_callback = kernel_crc_cb;
  rn8209_inst.io_callback = meter_uart_io();
  /* NOR Flash 仿真器 -> 脉冲数据持久化(掉电保存), flash 回调在 hello_init
   * 中注入 */
  rn8209_inst.flash_desc.page_size = NOR_PAGE_SIZE;
  rn8209_inst.flash_desc.sector_size = NOR_SECTOR_SIZE;
  rn8209_inst.flash_desc.sector_count = SECTOR_COUNT;
  rn8209_inst.flash_desc.start_address = 0;
  rn8209_inst.flash_desc.end_address = FLASH_SIZE - 1;
  rn8209_inst.flash_callback.read = flash_read_cb;
  rn8209_inst.flash_callback.write = flash_write_cb;
  rn8209_inst.flash_callback.erase = flash_erase_cb;

  PU_FP_STATE(); // FP 状态保存缓冲区（arm64 内核 7.2+ kernel_neon_begin
                 // 需要调用方提供）
  PU_FP_BEGIN();
  rn8209_init(&rn8209_inst, &preset);
  rn8209_inst.status = METER_CHIP_STATUS_MEASURING;

  ok = rn8209_read_register_by_name(&rn8209_inst, RN8209_REG_DeviceID,
                                    &device_id);

  PU_FP_END();

  if (ok) {
    pr_info("hello: rn8209 init OK, DeviceID=0x%06X\n", device_id.dword);
  } else {
    pr_warn("hello: rn8209 通信失败(芯片未接/预设寄存器未配置), 设备=%s\n",
            uart_dev);
  }
}

// cat命令时,将会调用该函数
static ssize_t cat_rn8209_device(struct device *dev,
                                 struct device_attribute *attr, char *buf) {
  return sysfs_emit(buf, "%s\n", rn8209_device_buff);
}

// echo命令时,将会调用该函数
static ssize_t echo_rn8209_device(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t len) {

  uext32_t register_data;
  if (sysfs_streq(buf, "voltage")) {
    status = RN8209_DEVICE_STATUS_VOLTAGE;

    if (!rn8209_read_register_by_name(&rn8209_inst, RN8209_REG_URMS,
                                     &register_data)) {
      scnprintf(rn8209_device_buff, sizeof(rn8209_device_buff), "ERR");
    } else {
      scnprintf(rn8209_device_buff, sizeof(rn8209_device_buff),
                "voltage raw=%d", register_data.dword);
    }

  } else if (sysfs_streq(buf, "current")) {
    status = RN8209_DEVICE_STATUS_CURRENT;
    if (!rn8209_read_register_by_name(&rn8209_inst, RN8209_REG_IARMS,
                                     &register_data)) {
      scnprintf(rn8209_device_buff, sizeof(rn8209_device_buff), "ERR");
    } else {
      scnprintf(rn8209_device_buff, sizeof(rn8209_device_buff),
                "current raw=%d", register_data.dword);
    }
  } else {
    status = RN8209_DEVICE_STATUS_DUMP;
    scnprintf(rn8209_device_buff, sizeof(rn8209_device_buff),
              "voltage %d.%d\n current %d.%d", 220, 0, 5, 0);
  }
  return len;
}

static DEVICE_ATTR(rn8209_device_test, S_IWUSR | S_IRUSR, cat_rn8209_device,
                   echo_rn8209_device);

struct file_operations rn8209_device_ops = {
    .owner = THIS_MODULE,
};

static int major;
static struct class *cls;
static int rn8209_device_init(void) {
  meter_flash = flash_init(flash_data_file, flash_meta_file);
  meter_uart_open(uart_dev);
  rn8209_register_init();

  struct device *rn8209_device;
  major = register_chrdev(0, "rn8209", &rn8209_device_ops);
  cls = class_create("rn8209_class");
  // 创建mytest_device设备
  rn8209_device = device_create(cls, 0, MKDEV(major, 0), NULL, "rn8209_device");

  // 在mytest_device设备目录下创建一个my_device_test属性文件
  if (sysfs_create_file(&(rn8209_device->kobj),
                        &dev_attr_rn8209_device_test.attr)) {
    return -1;
  }

  return 0;
}

static void rn8209_device_exit(void) {
  meter_uart_close();
  if (meter_flash != NULL) {
    flash_deinit(meter_flash); // 自动 save_all 后释放
    meter_flash = NULL;
  }
  device_destroy(cls, MKDEV(major, 0));
  class_destroy(cls);
  unregister_chrdev(major, "rn8209");
}

module_init(rn8209_device_init);
module_exit(rn8209_device_exit);
MODULE_LICENSE("GPL");