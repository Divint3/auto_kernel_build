// SPDX-License-Identifier: GPL-2.0
/*
 * meter_uart —— 内核态串口传输层实现（仅内核态编译，用户态整文件为空）
 *
 * 接口约定（见 meter_chip_port_driver.h 的 meter_chip_io_callback_t）:
 *   write(data, size) -> 写出字节数（0 = 失败）
 *   read(data, size)  -> 实际读到字节数（0 = 失败，驱动会重试）
 */
#ifdef __KERNEL__

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/delay.h>
#include <linux/errno.h>

#include "meter_uart.h"

/* 读超时（微秒）：9600 波特下 1 字节约 1ms，读一个寄存器(3~5字节)约 4~5ms。
 * 纯 udelay 忙等（不睡眠），保证可在关闭抢占的 FP 保护区内的原子上下文使用。 */
#define METER_UART_READ_TIMEOUT_US 500
#define METER_UART_READ_STEP_US 10
#define METER_UART_WRITE_STEP_US 100

static struct file *uart_filp = NULL;
static loff_t uart_read_pos = 0;  /* tty 读流偏移，持续累加 */
static loff_t uart_write_pos = 0;

int meter_uart_open(const char *dev_path) {
  int ret;

  if (uart_filp != NULL) {
    return 0; /* 已打开 */
  }
  if (dev_path == NULL || dev_path[0] == '\0') {
    return -EINVAL;
  }

  /* O_NOCTTY: 不作为控制终端; O_NONBLOCK: 无数据时 read 返回 -EAGAIN 而非阻塞 */
  uart_filp = filp_open(dev_path, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
  if (IS_ERR(uart_filp)) {
    ret = PTR_ERR(uart_filp);
    uart_filp = NULL;
    pr_err("meter_uart: open %s failed, err=%d\n", dev_path, ret);
    return ret;
  }
  uart_read_pos = 0;
  uart_write_pos = 0;
  pr_info("meter_uart: %s opened (波特率由 init_termios 决定, serial core 默认 9600 8N1)\n",
          dev_path);
  return 0;
}

void meter_uart_close(void) {
  if (uart_filp != NULL) {
    filp_close(uart_filp, NULL);
    uart_filp = NULL;
    pr_info("meter_uart: closed\n");
  }
}

bool meter_uart_is_open(void) {
  return uart_filp != NULL;
}

static int uart_write(uint8_t *data, uint8_t size) {
  ssize_t n;

  if (uart_filp == NULL || size == 0) {
    return 0;
  }
  n = kernel_write(uart_filp, data, size, &uart_write_pos);
  msleep(METER_UART_WRITE_STEP_US);
  return (n > 0) ? (int)n : 0;
}

static int uart_read(uint8_t *data, uint8_t size) {
  size_t got = 0;
  unsigned long waited = 0;
  ssize_t n;

  if (uart_filp == NULL || size == 0) {
    return 0;
  }
  /* 分批凑齐 size 字节; O_NONBLOCK 下无数据返回 -EAGAIN，用 udelay 轮询 */
  while (got < size && waited < METER_UART_READ_TIMEOUT_US) {
    n = kernel_read(uart_filp, data + got, size - got, &uart_read_pos);
    if (n > 0) {
      got += (size_t)n;
      continue;
    }
    if (n == 0) {
      break; /* EOF */
    }
    if (n != -EAGAIN) {
      pr_warn_ratelimited("meter_uart: read err=%zd\n", n);
      break; /* 真实 IO 错误 */
    }
    msleep(METER_UART_READ_STEP_US);
    waited += METER_UART_READ_STEP_US * 1000;
  }
  return (int)got;
}

meter_chip_io_callback_t meter_uart_io(void) {
  meter_chip_io_callback_t io;
  io.read = uart_read;
  io.write = uart_write;
  return io;
}

#endif /* __KERNEL__ */
