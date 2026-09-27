#ifndef PU_PORT_H_
#define PU_PORT_H_

/*
 * pu_port.h —— 普通(C标准库) / 内核(__KERNEL__) 双环境可移植层
 *
 * 约定：kernel-module 下所有文件的平台相关头文件一律经由本文件引入，
 *       用 __KERNEL__ 宏区分两种环境，禁止直接 #include <stdio.h> 等 libc 头。
 *
 * 内核态（kbuild 构建，__KERNEL__ 由内核构建系统自动定义）：
 *   - 整型/布尔类型用 linux/types.h 映射（内核无 <stdint.h>/<stdbool.h>）
 *   - stdio 用 FILE 兼容层，输出走 printk（pu_util.c 中提供强符号实现）
 *   - 数学函数 pow/fabs 提供标量降级实现（无需 libm）
 *   - 含浮点运算的源文件（rn8209_driver.c）编入内核时需单独放开
 *     -mgeneral-regs-only（见 Makefile），且运行期浮点调用必须包在
 *     PU_FP_BEGIN()/PU_FP_END() 内（见下方定义）
 * 用户态：直接使用 C 标准库。
 */

#ifdef __KERNEL__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/stdarg.h>
#include <linux/errno.h>

/* ---- 整型：内核无 <stdint.h>，用 linux/types.h 映射 ---- */
typedef __u8 uint8_t;
typedef __s8 int8_t;
typedef __u16 uint16_t;
typedef __s16 int16_t;
typedef __u32 uint32_t;
typedef __s32 int32_t;
typedef __u64 uint64_t;
typedef __s64 int64_t;

/* ---- bool/true/false：linux/types.h 已提供 _Bool 与 true/false，兜底保护 ---- */
#ifndef true
#define true 1
#define false 0
#endif

/* ---- stdio 兼容层：内核无 FILE，用占位类型让既有签名保持不变 ---- */
typedef int FILE;
#define stderr ((FILE)0)
#define stdout ((FILE)1)
#define stdin  ((FILE)2)

/* ---- math 兼容层：仅为头文件级兼容，内核态不要依赖浮点计算 ---- */
#define fabs(x)   pu_port_fabs((double)(x))
#define pow(x, y) pu_port_pow((double)(x), (double)(y))
static inline double pu_port_fabs(double x) { return (x < 0.0) ? -x : x; }
static inline double pu_port_pow(double base, double exp) {
  double result = 1.0;
  long n = (long)exp;
  int neg = (n < 0);
  unsigned long k = neg ? (unsigned long)(-n) : (unsigned long)n;
  while (k--) {
    result *= base;
  }
  return neg ? (1.0 / result) : result;
}

/* ---- 浮点保护：内核使用 FP 寄存器前必须保存/恢复用户态 FP 状态 ----
 * 用法: 包住所有会执行浮点运算的代码段（如 rn8209 驱动的浮点 API 调用）。
 * 注意: GUARD 区间内会关闭抢占（kernel_neon_begin/kernel_fpu_begin），
 *       区间内禁止任何可能睡眠的操作（msleep/GFP_KERNEL 分配/阻塞 IO）。
 */
#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
#include <asm/neon.h>
#define PU_FP_BEGIN() kernel_neon_begin()
#define PU_FP_END()   kernel_neon_end()
#elif defined(CONFIG_X86) || defined(CONFIG_X86_64)
#include <asm/fpu/api.h>
#define PU_FP_BEGIN() kernel_fpu_begin()
#define PU_FP_END()   kernel_fpu_end()
#else
#define PU_FP_BEGIN()
#define PU_FP_END()
#endif

#else /* 用户态 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

#endif /* __KERNEL__ */

#endif /* PU_PORT_H_ */
