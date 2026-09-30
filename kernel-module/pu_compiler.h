#ifndef PU_COMPILER_H_
#define PU_COMPILER_H_

// 编译器特性宏：补充静态断言 + 完善缺失的属性（如 noreturn/unused 等）
#if defined(_MSC_VER)
// Microsoft Visual C++ (MSVC)
#define PU_COMPILER_ALIGN(n)      __declspec(align(n))
#define PU_COMPILER_PACKED        __pragma(pack(push, 1)) // 补充PACKED的完整实现
#define PU_COMPILER_PACKED_END    __pragma(pack(pop))
#define PU_COMPILER_SECTION(s)    __declspec(allocate(s))
#define PU_COMPILER_WEAK          __declspec(selectany) // MSVC无weak，用selectany模拟
#define PU_COMPILER_USED          __declspec(used)
#define PU_COMPILER_UNUSED        __pragma(warning(suppress:4100)) // 抑制未使用警告
#define PU_COMPILER_NOINLINE      __declspec(noinline)
#define PU_COMPILER_ALWAYS_INLINE __forceinline
#define PU_COMPILER_DEPRECATED    __declspec(deprecated)
#define PU_COMPILER_NORETURN      __declspec(noreturn)
#define PU_COMPILER_SELECTANY     __declspec(selectany)
// MSVC 无原生noinit，通过section指定到未初始化段+编译器优化实现（需链接器配合）
#define PU_COMPILER_NOINIT __declspec(allocate(".noinit"))
// MSVC静态断言：C11前用 _STATIC_ASSERT，C11后用 static_assert
#if _MSC_VER >= 1900 // VS2015+ 支持 C11 static_assert
#define PU_COMPILER_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
#else // 旧版MSVC
#define PU_COMPILER_STATIC_ASSERT(expr, msg) _STATIC_ASSERT(expr, msg)
#endif

#elif defined(__GNUC__) || defined(__clang__)
// GCC 和 Clang (含ARMCLANG)
#define PU_COMPILER_ALIGN(n) __attribute__((aligned(n)))
#define PU_COMPILER_PACKED   __attribute__((packed))
#define PU_COMPILER_PACKED_BEGIN
#define PU_COMPILER_PACKED_END
#define PU_COMPILER_SECTION(s) __attribute__((section(s)))
#if defined(__MINGW64__) || defined(__MINGW32__)
#define PU_COMPILER_WEAK
#define PU_COMPILER_SELECTANY
#else
#define PU_COMPILER_WEAK      __attribute__((weak))
#define PU_COMPILER_SELECTANY __attribute__((weak))
#endif
#define PU_COMPILER_USED          __attribute__((used))
#define PU_COMPILER_UNUSED        __attribute__((unused))
#define PU_COMPILER_NOINLINE      __attribute__((noinline))
#define PU_COMPILER_ALWAYS_INLINE __attribute__((always_inline))
#define PU_COMPILER_DEPRECATED    __attribute__((deprecated))
#define PU_COMPILER_NORETURN      __attribute__((noreturn))
// GCC通过section指定.noinit段实现非初始化，需结合链接脚本NOLOAD
#define PU_COMPILER_NOINIT        __attribute__((section(".noinit"), used))
// GCC/Clang 静态断言：C11 static_assert 或 _Static_assert
#if __STDC_VERSION__ >= 201112L
#define PU_COMPILER_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
#else
#define PU_COMPILER_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)
#endif

#elif defined(__ICCARM__)
// IAR Embedded Workbench (ARM)
#define PU_COMPILER_ALIGN(n) _Pragma("data_alignment=" #n) // 修正语法：需字符串化
#define PU_COMPILER_PACKED   __packed
#define PU_COMPILER_PACKED_BEGIN
#define PU_COMPILER_PACKED_END
#define PU_COMPILER_SECTION(s)    _Pragma("location=\"" s "\"") // 修正：需引号包裹段名
#define PU_COMPILER_WEAK          __weak
#define PU_COMPILER_USED          __root
#define PU_COMPILER_UNUSED        __attribute__((unused)) // IAR兼容GCC的unused
#define PU_COMPILER_NOINLINE      _Pragma("inline=never")
#define PU_COMPILER_ALWAYS_INLINE _Pragma("inline=forced")
#define PU_COMPILER_DEPRECATED    __deprecated // IAR专属deprecated
#define PU_COMPILER_NORETURN      __noreturn   // IAR专属noreturn
#define PU_COMPILER_SELECTANY     __weak       // IAR用weak模拟selectany
// IAR原生支持__no_init关键字，直接映射
#define PU_COMPILER_NOINIT        __no_init
// IAR 静态断言：C11支持static_assert，旧版用数组长度实现
#if __IAR_SYSTEMS_ICC__ >= 800 // IAR 8.0+ 支持 C11
#define PU_COMPILER_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
#else
#define PU_COMPILER_STATIC_ASSERT(expr, msg) typedef char pu_static_assert_##msg[(expr) ? 1 : -1];
#endif

#elif defined(__CC_ARM) || defined(__ARMCC_VERSION__)
// Keil ARMCC (v5/v6，兼容旧版__CC_ARM)
#define PU_COMPILER_ALIGN(n)     __align(n)
#define PU_COMPILER_PACKED       __packed
#define PU_COMPILER_PACKED_BEGIN __pragma(pack(push, 1))
#define PU_COMPILER_PACKED_END    __pragma(pop))
#define PU_COMPILER_SECTION(s)    __attribute__((section(s)))
#define PU_COMPILER_WEAK          __weak
#define PU_COMPILER_USED          __attribute__((used))
#define PU_COMPILER_UNUSED        __attribute__((unused))
#define PU_COMPILER_NOINLINE      __attribute__((noinline))
#define PU_COMPILER_ALWAYS_INLINE __attribute__((always_inline))
#define PU_COMPILER_DEPRECATED    __attribute__((deprecated))
#define PU_COMPILER_NORETURN      __attribute__((noreturn)) // 补充noreturn
#define PU_COMPILER_SELECTANY     __attribute__((weak))     // ARMCC用weak模拟selectany
// Keil ARMCC通过section指定.noinit段，结合链接脚本NOLOAD
#define PU_COMPILER_NOINIT        __attribute__((section(".noinit"), used))
// Keil ARMCC 静态断言：v5用_Static_assert，v6+支持static_assert
#if __ARMCC_VERSION__ >= 6000000 // ARMCLANG (v6+)
#define PU_COMPILER_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
#else // ARMCC v5
#define PU_COMPILER_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)
#endif

#else
#error "Unsupported PU_compiler: Please add compiler-specific macros!"
#endif

// 常用对齐大小的定义
#define ALIGN_1          PU_COMPILER_ALIGN(1)
#define ALIGN_2          PU_COMPILER_ALIGN(2)
#define ALIGN_4          PU_COMPILER_ALIGN(4)
#define ALIGN_8          PU_COMPILER_ALIGN(8)
#define ALIGN_16         PU_COMPILER_ALIGN(16)
#define ALIGN_32         PU_COMPILER_ALIGN(32)
#define ALIGN_64         PU_COMPILER_ALIGN(64)
#define ALIGN_CACHE_LINE PU_COMPILER_ALIGN(32) // 通常缓存行大小

// 补充noinit段相关的辅助宏：统一段名，便于跨编译器维护
#define PU_NOINIT_SECTION_NAME     ".noinit"                                   // 统一noinit段名
#define PU_COMPILER_NOINIT_SECTION PU_COMPILER_SECTION(PU_NOINIT_SECTION_NAME) // 快速指定noinit段

#endif // PU_COMPILER_H_