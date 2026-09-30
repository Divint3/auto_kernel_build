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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
#include <linux/stdarg.h>
#else
#include <stdarg.h>
#endif
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

/* ---- labs 降级：内核无 <stdlib.h>，无 labs ---- */
static inline long pu_port_labs(long x) { return (x < 0) ? -x : x; }
#define labs(x) pu_port_labs((long)(x))

/* ---- strncpy 映射：新版内核已从 string API 移除 strncpy（fortify 清理），----
 * 用 strscpy 替代：同为最多拷贝 n 字节，且保证 NUL 结尾（strncpy 不保证），
 * 对路径缓冲区等字符串场景语义更安全；仅尾部零填充行为有差异，本库无依赖。
 */
#define strncpy(dst, src, n) strscpy(dst, src, n)

/* strcpy 同样处于移除名单：映射为 strscpy(dst, src, strlen(src)+1)，
 * 与原语义严格等价（完整拷贝含 NUL），杜绝 dst 越界 */
#define strcpy(dst, src) strscpy(dst, src, strlen(src) + 1)

/* ---- asin 降级：内核无 <math.h>，无 asin/sin/cos ----
 * 用途: rn8209 有功相位校正 radian = asin(sin_value)，sin_value 为
 *       功率误差推得的小量（典型 |x| < 0.1），对精度要求不高。
 * 实现: 泰勒级数 sin（先归约到 [-PI, PI]，18 项达双精度极限）
 *       + 牛顿迭代解 sin(y) = x（二次收敛，约 5 次迭代达 1e-12）。
 * 注意: 仅做标量计算，不访问 FP 系统寄存器以外状态；
 *       调用处须位于 PU_FP_BEGIN()/PU_FP_END() 保护区内的前提不变。
 */
#define PU_PORT_PI      3.14159265358979323846
#define PU_PORT_HALF_PI 1.57079632679489661923
static inline double pu_port_sin(double x) {
  const double two_pi = 2.0 * PU_PORT_PI;
  while (x > PU_PORT_PI) {
    x -= two_pi;
  }
  while (x < -PU_PORT_PI) {
    x += two_pi;
  }
  double term = x;
  double sum = x;
  for (int i = 1; i <= 18; i++) {
    term *= -(x * x) / ((2.0 * i) * (2.0 * i + 1.0));
    sum += term;
  }
  return sum;
}
static inline double pu_port_cos(double x) { return pu_port_sin(x + PU_PORT_HALF_PI); }
static inline double pu_port_asin(double x) {
  if (x >= 1.0) {
    return PU_PORT_HALF_PI;
  }
  if (x <= -1.0) {
    return -PU_PORT_HALF_PI;
  }
  double y = x; /* 初值 x, |x| < 1 时牛顿法收敛于 [-PI/2, PI/2] */
  for (int i = 0; i < 24; i++) {
    double c = pu_port_cos(y);
    if (c < 1e-10 && c > -1e-10) {
      break; /* y 已逼近 ±PI/2, 导数趋零即视为收敛 */
    }
    double d = (pu_port_sin(y) - x) / c;
    y -= d;
    if (d < 1e-12 && d > -1e-12) {
      break;
    }
  }
  return y;
}
#define asin(x) pu_port_asin((double)(x))

/* ---- 浮点保护：内核使用 FP 寄存器前必须保存/恢复用户态 FP 状态 ----
 * 用法（三步，PU_FP_STATE 声明的缓冲区供 BEGIN 保存 / END 恢复复用）:
 *   void my_func(void) {
 *     PU_FP_STATE();      // 声明 FP 状态保存缓冲区（栈上局部变量）
 *     PU_FP_BEGIN();
 *     ...浮点运算代码...
 *     PU_FP_END();
 *   }
 * 注意: GUARD 区间内会关闭抢占（kernel_neon_begin/kernel_fpu_begin），
 *       区间内禁止任何可能睡眠的操作（msleep/GFP_KERNEL 分配/阻塞 IO）。
 * 内核 7.2+ 的 arm64 上 kernel_neon_begin/end 签名为
 *   void kernel_neon_begin(struct user_fpsimd_state *);
 *   void kernel_neon_end(struct user_fpsimd_state *);
 * 需要调用方提供保存缓冲区，因此 PU_FP_STATE() 必须与 BEGIN/END 成对出现在同一函数内。
 */
#if defined(CONFIG_ARM64) || defined(CONFIG_ARM)
#include <asm/neon.h>
#define PU_FP_STATE() struct user_fpsimd_state pu_fp_state
#define PU_FP_BEGIN() kernel_neon_begin(&pu_fp_state)
#define PU_FP_END()   kernel_neon_end(&pu_fp_state)
#elif defined(CONFIG_X86) || defined(CONFIG_X86_64)
#include <asm/fpu/api.h>
#define PU_FP_STATE()
#define PU_FP_BEGIN() kernel_fpu_begin()
#define PU_FP_END()   kernel_fpu_end()
#else
#define PU_FP_STATE()
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
