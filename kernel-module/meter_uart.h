#ifndef METER_UART_H_
#define METER_UART_H_

#include "pu_port.h"

#ifdef __KERNEL__

/*
 * meter_uart —— 内核态串口传输层
 *
 * 在内核空间打开串口设备字符节点（默认 /dev/ttyAMA1），
 * 通过 filp_open + kernel_write/kernel_read 收发原始字节，
 * 并封装成 rn8209 驱动要求的 meter_chip_io_callback_t 接口。
 *
 * 特性:
 *   - O_NONBLOCK 打开 + 轮询式读（udelay 短等待），全程不睡眠，
 *     因此可以在 PU_FP_BEGIN()/PU_FP_END() 保护区内安全调用
 *   - read 返回实际读到的字节数（0 = 超时未凑齐），符合驱动重试约定
 *
 * 注意: 波特率由串口驱动 init_termios 决定（serial core 默认 9600 8N1，
 *       正好是 rn8209 的常用配置）；如需修改波特率需在用户态用 stty
 *       配置一次，或后续用 serdev/uart 层 API 扩展。
 */

#include "meter_chip_port_driver.h"

/* 打开串口设备，成功返回 0，失败返回负错误码 */
int meter_uart_open(const char *dev_path);

/* 关闭串口 */
void meter_uart_close(void);

/* 串口是否已打开 */
bool meter_uart_is_open(void);

/* 返回符合 rn8209 驱动 io_callback 约定的回调集合 */
meter_chip_io_callback_t meter_uart_io(void);

#endif /* __KERNEL__ */

#endif /* METER_UART_H_ */
