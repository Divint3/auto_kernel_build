#ifndef PU_UTIL_H_
#define PU_UTIL_H_

#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）
#include "pu_compiler.h"
#include "pu_macro.h"
#include "pu_type.h"

// 颜色宏定义
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD_RED   "\033[1;31m"
#define COLOR_BOLD_GREEN "\033[1;32m"
#define COLOR_BOLD_YELLOW "\033[1;33m"


// 日志级别定义
typedef enum {
  PU_LOG_LEVEL_DEBUG = 0,
  PU_LOG_LEVEL_INFO = 1,
  PU_LOG_LEVEL_WARN = 2,
  PU_LOG_LEVEL_ERROR = 3,
  PU_LOG_LEVEL_FATAL = 4,
} pu_log_level_e;

#define PU_VAILD_MAGIC 0x584C4357 // 芯珑错误

typedef enum {
  PU_FAULT_REASON_NONE = 0,        /* 0：无异常（正常状态） */
  PU_FAULT_REASON_MEM_MANAGE = 1,  /* 1：存储管理异常 - 内存访问违规（空指针、MPU权限、栈/堆越界） */
  PU_FAULT_REASON_BUS_FAULT = 2,   /* 2：总线故障异常 - 总线访问错误（外设访问失效、取指总线错误） */
  PU_FAULT_REASON_USAGE_FAULT = 3, /* 3：使用故障异常 - 指令执行错误（未定义指令、除数为0、对齐错误） */
  PU_FAULT_REASON_HARD_FAULT = 4,  /* 4：硬故障异常 - 严重致命错误（无法被其他异常捕获的底层错误） */
  PU_FAULT_REASON_WATCH_DOG = 5,   /* 5. 看门狗复位 */
  PU_FAULT_REASON_UNKNOWN = 0xFF   /* 预留：未知异常（兜底类型） */
} pu_fault_reason_e;

typedef enum {
  PU_FAULT_STACK_PSP,
  PU_FAULT_STACK_MSP,
} pu_fault_stack_e;

// ref https://www.cnblogs.com/utank/p/11251721.html
typedef struct {
  pu_fault_stack_e fault_stack;
  uint32_t R0;
  uint32_t R1;
  uint32_t R2;
  uint32_t R3;
  uint32_t R12; // 参数指针寄存器
  uint32_t LR;
  uint32_t PC;
  uint32_t xPSR;
  uint32_t fault_reason;
  uint32_t vaild_magic_number;
  char info[64];
} pu_backtrace_context_t, *pu_backtrace_context_p;

extern pu_backtrace_context_t g_backtrace_context;

/**
 * @brief 初始化日志系统
 * @param[in] file_name_suffix 日志文件名后缀
 * @details 根据当前时间生成带时间戳的日志文件名并打开文件
 * @note 如果文件创建失败,会通过 perror 输出错误信息
 */
void pu_log_init(const char *file_name_suffix);

/**
 * @brief 反转字节数组
 * @param[in,out] arr 要反转的字节数组
 * @param[in] size 数组大小
 * @warning 如果传入 NULL 指针,函数会直接返回
 */
void pu_reverse_byte_array(uint8_t *arr, size_t size);

/**
 * @brief 移除字符串中的所有空格
 * @param[in,out] str 要处理的字符串
 * @details 原地修改字符串,移除所有空白字符
 */
void pu_remove_spaces(char *str);

/**
 * @brief Base64 解码
 * @param[in] input Base64 编码的字符串
 * @return 解码后的数据指针,使用后需要调用 pu_data_free 释放内存
 * @retval NULL 解码失败或输入无效
 * @note 输入字符串长度必须是4的倍数
 */
pu_data_p pu_base64_decode(const char *input);

/**
 * @brief Base64 编码
 * @param[in] data 要编码的原始数据
 * @param[in] input_length 数据长度
 * @return 编码后的数据指针,使用后需要调用 pu_data_free 释放内存
 */
pu_data_p pu_base64_encode(const unsigned char *data, uint16_t input_length);

/**
 * @brief 错误处理函数
 * @param[in] msg 错误消息
 * @details 输出错误信息到 stderr 并退出程序
 * @note 此函数不会返回,会调用 exit(EXIT_FAILURE)
 */
void pu_error(const char *msg);

/**
 * @brief 字节数组转十六进制字符串
 * @param[in] input_data 输入的字节数组数据
 * @return 十六进制字符串数据指针,使用后需要调用 pu_data_free 释放内存
 * @retval NULL 输入数据无效
 */
pu_data_p pu_byte_array_to_hex_string(pu_data_p input_data);

/**
 * @brief 十六进制字符串转字节数组
 * @param[in] input_data 输入的十六进制字符串数据
 * @return 字节数组数据指针,使用后需要调用 pu_data_free 释放内存
 * @retval NULL 输入字符串长度不是偶数或转换失败
 */
pu_data_p pu_hex_string_to_byte_array(pu_data_p input_data);

/**
 * @brief 将32位IPv4地址转换为字符串格式
 * @param[in] addr 网络字节序的32位IP地址
 * @return IP地址字符串指针(静态缓冲区,不需要释放)
 * @note 使用静态缓冲区,非线程安全
 */
char *pu_inet_ntoa(uint32_t addr);

/**
 * @brief 将IPv4地址字符串转换为32位格式
 * @param[in] ip IP地址字符串(格式:xxx.xxx.xxx.xxx)
 * @return 网络字节序的32位IP地址
 */
uint32_t pu_inet_aton(const char *ip);

/**
 * @brief 检查内存区域是否全为零
 * @param[in] memory 要检查的内存起始地址
 * @param[in] size 要检查的内存大小
 * @retval true 内存区域全为零
 * @retval false 内存区域包含非零值
 */
bool check_memory_is_zero(uint8_t *memory, uint16_t size);

/**
 * @brief 以十六进制格式输出数据内容
 * @param[in] ptr 要输出的数据指针
 * @details 使用 BACKEND_LOG 宏输出数据内容
 */
void pu_data_dump(pu_data_p ptr);

/**
 * @brief 释放 pu_data 结构体内存
 * @param[in] ptr 要释放的数据指针
 * @note 会同时释放 data 字段指向的内存
 */
void pu_data_free(pu_data_p ptr);

/**
 * @brief 释放 pu_data_t 结构体内存
 * @param[in] data 要释放的数据结构
 * @note 仅释放 data 字段指向的内存,不释放结构体本身
 */
void pu_data_t_free(pu_data_t data);

/**
 * @brief 移动数据所有权
 * @param[out] dest 目标数据指针
 * @param[in] src 源数据指针
 * @details 将 src 的数据移动到 dest,并释放 src 结构体
 * @warning 调用后 src 指针不再有效
 */
void pu_data_move(pu_data_p dest, pu_data_p src);

/**
 * @brief 复制数据
 * @param[in] src 源数据指针
 * @return 新分配的数据副本指针,使用后需要调用 pu_data_free 释放
 * @retval NULL 内存分配失败
 */
pu_data_p pu_data_copy(pu_data_p src);

/**
 * @brief 反转数据字节顺序
 * @param[in] data 要反转的数据指针
 * @return 反转后的新数据指针,使用后需要调用 pu_data_free 释放
 */
pu_data_p pu_data_reverse(pu_data_p data);

/**
 * @brief 8位十六进制数转BCD码
 * @param[in] hex 十六进制数(0-99)
 * @return BCD编码结果
 */
uint8_t hex_to_bcd8(uint8_t hex);

/**
 * @brief 8位BCD码转十六进制数
 * @param[in] bcd BCD编码数据
 * @return 十六进制结果
 */
uint8_t bcd_to_hex8(uint8_t bcd);

/**
 * @brief 16位十六进制数转BCD码
 * @param[in] hex 十六进制数(0-9999)
 * @return BCD编码结果
 */
uint16_t hex_to_bcd16(uint16_t hex);

/**
 * @brief 16位BCD码转十六进制数
 * @param[in] bcd BCD编码数据
 * @return 十六进制结果
 */
uint16_t bcd_to_hex16(uint16_t bcd);

/**
 * @brief 32位十六进制数转BCD码
 * @param[in] hex 十六进制数(0-99999999)
 * @return BCD编码结果
 */
uint32_t hex_to_bcd32(uint32_t hex);

/**
 * @brief 32位BCD码转十六进制数
 * @param[in] bcd BCD编码数据
 * @return 十六进制结果
 */
uint32_t bcd_to_hex32(uint32_t bcd);

// 定义端序枚举类型
typedef enum {
  PU_ENDIAN_LITTLE, // 小端模式
  PU_ENDIAN_BIG     // 大端模式
} pu_endian_type_e;

/**
 * @brief 检测当前系统的字节序(大小端)
 * @return 返回当前系统的端序类型(ENDIAN_LITTLE 或 ENDIAN_BIG)
 */
pu_endian_type_e pu_get_current_endian(void);

/**
 * @brief 打印当前系统的字节序信息
 */
void pu_print_endian_info(void);

/**
 * @brief 根据目标端序对字节数组进行转换(原地转换)
 * @param data 指向要转换的字节数组
 * @param length 数组的长度(字节数)
 * @param target_endian 目标端序(ENDIAN_LITTLE 或 ENDIAN_BIG)
 * @note 如果当前系统端序与目标端序一致,则不进行转换
 */
void pu_reverse_byte_array_by_endian(void *data, uint16_t length, pu_endian_type_e target_endian);

typedef struct {
  uint8_t *buffer;       // 数据缓冲区
  uint32_t size;         // 缓冲区总大小
  uint32_t position;     // 当前读写位置
  uint32_t try_to_write; // 尝试写入大小
  int is_encoding;       // 1=编码, 0=解码
} pu_stream_t, *pu_stream_p;

void *pu_get_stream_position_pointer(pu_stream_p stream);
int pu_seek_stream(pu_stream_p stream, uint16_t offset);
uint16_t pu_write_stream(pu_stream_p stream, const void *data, uint16_t data_size);

bool pu_convert_to_bcd(uint32_t value, uint8_t *buffer, int size);

int pu_fprintf(FILE *stream, const char *format, ...);
int pu_vfprintf(FILE *stream, const char *format, va_list args);

#define pu_printf(format, ...) pu_fprintf(NULL, format, ##__VA_ARGS__)

int pu_fflush(FILE *stream);

/**
 * @brief 获取当前时间戳（格式：YYYY-MM-DD HH:MM:SS）
 *
 * @param time_buf
 * @param buf_len
 */
void pu_get_log_time(char *time_buf, int buf_len);

/**
 * @brief 获取日志级别字符串
 *
 * @param level 日志级别
 * @return const char*
 */
const char *pu_get_level_str(pu_log_level_e level);

/**
 * @brief 封装的日志打印函数
 *
 * @param level 日志级别
 * @param file 调用文件  __FILE__
 * @param line 调用行号  __LINE__
 * @param fmt  格式化字符串
 * @param ...  可变参数
 */
void pu_log_printf(pu_log_level_e level, const char *file, int line, const char *fmt, ...);
void pu_log_hex(pu_log_level_e level, const char *file, int line, const void *data, size_t len, const char *desc);

void pu_stash_thread_info(uint32_t *exception_stack_ptr);

// 设置全局日志级别（动态过滤）

void pu_set_log_level(pu_log_level_e level);
pu_log_level_e pu_get_log_level(void);

#ifdef __ICCARM__
#define PU_BACKTRACE_SAVECONTEXT()                           \
  do {                                                       \
    static uint32_t msp, psp, *exception_stack_ptr;          \
    static uext32_t lr;                                      \
    msp = __arm_rsr("MSP");                                  \
    psp = __arm_rsr("PSP");                                  \
    __asm volatile("MOV %0, LR" : "=r"(lr.dword));           \
    if (lr.bit.b02) {                                        \
      exception_stack_ptr = (uint32_t *)psp;                 \
      g_backtrace_context.fault_stack = PU_FAULT_STACK_PSP;  \
    } else {                                                 \
      exception_stack_ptr = (uint32_t *)msp;                 \
      g_backtrace_context.fault_stack = PU_FAULT_STACK_MSP;  \
    }                                                        \
    g_backtrace_context.R0 = exception_stack_ptr[0];         \
    g_backtrace_context.R1 = exception_stack_ptr[1];         \
    g_backtrace_context.R2 = exception_stack_ptr[2];         \
    g_backtrace_context.R3 = exception_stack_ptr[3];         \
    g_backtrace_context.R12 = exception_stack_ptr[4];        \
    g_backtrace_context.LR = exception_stack_ptr[5];         \
    g_backtrace_context.PC = exception_stack_ptr[6];         \
    g_backtrace_context.xPSR = exception_stack_ptr[7];       \
    g_backtrace_context.vaild_magic_number = PU_VAILD_MAGIC; \
    pu_stash_thread_info(exception_stack_ptr);               \
  } while (0)
#else
#define PU_BACKTRACE_SAVECONTEXT() \
  do {                             \
  } while (0);
#endif

void pu_crash_info(FILE *stream);

#endif // PU_UTIL_H_
