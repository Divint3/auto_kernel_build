#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）
#include "pu_util.h"
#include "pu_compiler.h"
#include "pu_macro.h"
#include "pu_lock.h"

const char pu_log_file_name[128];
FILE *pu_log_file_fd = NULL;

static pu_log_level_e g_log_level = PU_LOG_LEVEL_INFO;

// void pu_log_init(const char *file_name_suffix) {
//   time_t rawtime;
//   struct tm *timeinfo;
//   char time_buffer[80];

//  time(&rawtime);                 // 获取当前时间
//  timeinfo = localtime(&rawtime); // 转换为本地时间

//  strftime(time_buffer, sizeof(time_buffer), "_%Y-%m-%d_%H-%M-%S.log", timeinfo);
//  sprintf((char *)pu_log_file_name, "%s_%s", file_name_suffix, time_buffer);
//  pu_log_file_fd = fopen(pu_log_file_name, "a");
//  if (pu_log_file_fd == NULL) {
//    perror("Unable to create file");
//  }
//}

PU_COMPILER_NOINIT pu_backtrace_context_t g_backtrace_context;

void pu_reverse_byte_array(uint8_t *arr, size_t size) {
  if (arr == NULL || size <= 1) {
    return;
  }
  // 使用高效算法
  switch (size) {
#if defined(_MSC_VER)
    // MSVC 的实现
  case 2: {
    uint16_t *temp = (uint16_t *)arr;
    *temp = _byteswap_ushort(*temp);
  } break;

  case 4: {
    uint32_t *temp = (uint32_t *)arr;
    *temp = _byteswap_ulong(*temp);
  } break;

  case 8: {
    uint64_t *temp = (uint64_t *)arr;
    *temp = _byteswap_uint64(*temp);
  } break;
#elif defined(__GNUC__) || defined(__clang__)

#if !defined(__CC_ARM)
  case 2: {
    uint16_t *temp = (uint16_t *)arr;
    *temp = __builtin_bswap16(*temp);
  } break;
#endif
  case 4: {
    uint32_t *temp = (uint32_t *)arr;
    *temp = __builtin_bswap32(*temp);
  } break;

  case 8: {
    uint64_t *temp = (uint64_t *)arr;
    *temp = __builtin_bswap64(*temp);
  } break;
#endif
  default: {
    // 通用情况：使用循环交换字节
    size_t start = 0;
    size_t end = size - 1;
    uint8_t temp;

    while (start < end) {
      // 交换元素
      temp = arr[start];
      arr[start] = arr[end];
      arr[end] = temp;

      // 移动指针
      start++;
      end--;
    }
  }
  }
}

void pu_remove_spaces(char *str) {
  char *src = str, *dst = str; // src 用于遍历,dst 用于写入
  while (*src) {
    if (!isspace((unsigned char)*src)) {
      *dst++ = *src; // 如果不是空格复制字符到 dst
    }
    src++;
  }
  *dst = '\0'; // 结束字符串
}

static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
pu_data_p pu_base64_decode(const char *input) {

  size_t len = strlen(input);
  if (len % 4 != 0) {
    return NULL; // 非法的Base64字符串
  }

  // 计算输出缓冲区大小
  size_t padding = 0;
  if (input[len - 1] == '=')
    padding++;
  if (input[len - 2] == '=')
    padding++;

  size_t output_len = (len / 4) * 3 - padding;
  uint8_t *output = (uint8_t *)pu_malloc(output_len);
  if (output == NULL) {
    return NULL;
  }

  for (size_t i = 0, j = 0; i < len;) {
    int64_t a = (input[i] == '=') ? 0 : (strchr(base64_table, input[i]) - base64_table);
    int64_t b = (input[i + 1] == '=') ? 0 : (strchr(base64_table, input[i + 1]) - base64_table);
    int64_t c = (input[i + 2] == '=') ? 0 : (strchr(base64_table, input[i + 2]) - base64_table);
    int64_t d = (input[i + 3] == '=') ? 0 : (strchr(base64_table, input[i + 3]) - base64_table);

    int64_t triple = (a << 18) + (b << 12) + (c << 6) + d;

    if (j < output_len)
      (output)[j++] = (triple >> 16) & 0xFF;
    if (j < output_len)
      (output)[j++] = (triple >> 8) & 0xFF;
    if (j < output_len)
      (output)[j++] = triple & 0xFF;

    i += 4;
  }

  pu_malloc_instance(result, pu_data);
  if (result == NULL) {
    return NULL;
  }
  result->data = output;
  result->size = output_len;

  return result;
}

pu_data_p pu_base64_encode(const unsigned char *data, uint16_t input_length) {
  size_t output_length = 4 * ((input_length + 2) / 3);  // 计算输出长度
  uint8_t *encoded_data = pu_malloc(output_length + 1); // 分配内存
  if (encoded_data == NULL) {
    return NULL;
  }

  size_t i, j;
  for (i = 0, j = 0; i < input_length;) {
    uint32_t octet_a = i < input_length ? data[i++] : 0;
    uint32_t octet_b = i < input_length ? data[i++] : 0;
    uint32_t octet_c = i < input_length ? data[i++] : 0;

    uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

    encoded_data[j++] = base64_table[(triple >> 18) & 0x3F];
    encoded_data[j++] = base64_table[(triple >> 12) & 0x3F];
    encoded_data[j++] = base64_table[(triple >> 6) & 0x3F];
    encoded_data[j++] = base64_table[triple & 0x3F];
  }

  // 处理填充
  for (size_t k = 0; k < (size_t)((3 - input_length % 3) % 3); k++) {
    encoded_data[output_length - 1 - k] = '=';
  }

  encoded_data[output_length] = '\0'; // 添加字符串结束符
  // return encoded_data;

  pu_malloc_instance(result, pu_data);
  if (result == NULL) {
    return NULL;
  }
  result->data = encoded_data;
  result->size = output_length;
  return result;
}

// void pu_error(const char *msg) {
//   fprintf(stderr, "%s\n", msg);
//   exit(EXIT_FAILURE);
// }

pu_data_p pu_byte_array_to_hex_string(pu_data_p input_data) {
  pu_malloc_instance(result, pu_data);
  if (result == NULL) {
    return NULL;
  }
  result->size = input_data->size * 2 + 1;
  result->data = pu_malloc(result->size * 2 + 1);
  if (result->data == NULL) {
    pu_free(result);
    return NULL;
  }
  for (size_t i = 0; i < input_data->size; i++) {
    snprintf((char *)result->data + (i * 2), result->size - (i * 2), "%02X", input_data->data[i]);
  }
  return result;
}

pu_data_p pu_hex_string_to_byte_array(pu_data_p input_data) {
  // 检查输入长度是否为偶数,因为每两个字符表示一个十六进制字节

  if (strlen((char *)input_data->data) % 2 != 0) {
    return NULL; // 或者处理错误
  }

  pu_malloc_instance(result, pu_data);
  if (result == NULL) {
    return NULL;
  }
  result->size = strlen((char *)input_data->data) / 2;
  result->data = pu_malloc(result->size);
  if (result->data == NULL) {
    pu_free(result);
    return NULL;
  }

  for (size_t i = 0; i < result->size; i++) {
    unsigned int byte;
    // 将两个字符转换为一个字节（sscanf_s 仅 MSVC 提供，其余统一走 sscanf）
#if defined(_MSC_VER)
    sscanf_s((char *)input_data->data + (i * 2), "%02x", &byte);
#else
    sscanf((char *)input_data->data + (i * 2), "%02x", &byte);
#endif
    result->data[i] = (unsigned char)byte;
  }

  return result;
}

char *pu_inet_ntoa(uint32_t addr) {
#define IP_STR_LEN 16 // 最大的 IPv4 地址字符串长度(xxx.xxx.xxx.xxx)

  static char buffer[IP_STR_LEN]; // 静态缓冲区以存储字符串
  snprintf(buffer, IP_STR_LEN, "%u.%u.%u.%u",
           (addr >> 24) & 0xFF, // 提取第一个字节
           (addr >> 16) & 0xFF, // 提取第二个字节
           (addr >> 8) & 0xFF,  // 提取第三个字节
           addr & 0xFF);        // 提取第四个字节
  return buffer;                // 返回指向字符串的指针
}

uint32_t pu_inet_aton(const char *ip) {
  uint32_t result = 0;
  int b1, b2, b3, b4;

  // 使用 sscanf 来解析每个部分（sscanf_s 仅 MSVC 提供，其余统一走 sscanf）
#if defined(_MSC_VER)
  sscanf_s(ip, "%d.%d.%d.%d", &b1, &b2, &b3, &b4);
#else
  sscanf(ip, "%d.%d.%d.%d", &b1, &b2, &b3, &b4);
#endif
  // 转换为 uint32_t 类型
  result = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;

  return result;
}

bool check_memory_is_zero(uint8_t *memory, uint16_t size) {
  for (size_t i = 0; i < size; i++) {
    if (memory[i] != 0) {
      return false;
    }
  }
  return true;
}

void pu_data_dump(pu_data_p ptr) {
  if (ptr != NULL) {
    for (size_t i = 0; i < ptr->size; i++) {
      PU_LOG_DEBUG("%02X ", ptr->data[i]);
    }
    PU_LOG_DEBUG("\n");
  }
}

void pu_data_free(pu_data_p ptr) {
  if (ptr != NULL) {
    if (ptr->data != NULL)
      pu_free(ptr->data);
    pu_free(ptr);
  }
}

bool check_memory_is_zero(uint8_t *memory, uint16_t size);

void pu_data_t_free(pu_data_t data) {
  if (data.data != NULL)
    pu_free(data.data);
}

void pu_data_move(pu_data_p dest, pu_data_p src) {
  dest->data = src->data;
  dest->size = src->size;

  pu_free(src);
}

pu_data_p pu_data_copy(pu_data_p src) {
  pu_malloc_instance(result, pu_data);
  if (result == NULL) {
    return NULL;
  }
  result->data = pu_malloc(src->size);
  if (result->data == NULL) {
    pu_free(result);
    return NULL;
  }
  result->size = src->size;
  memcpy(result->data, src->data, src->size);
  return result;
}

// 翻转数据
pu_data_p pu_data_reverse(pu_data_p data) {
  pu_data_p result = pu_data_copy(data);
  pu_reverse_byte_array((uint8_t *)result->data, result->size);
  return result;
}

/**
 * @brief 8位十六进制数转BCD码
 * @param hex 十六进制数(0-99)
 * @return uint8_t BCD编码结果
 */
uint8_t hex_to_bcd8(uint8_t hex) {
  return ((hex / 10) << 4) | (hex % 10);
}

/**
 * @brief 8位BCD码转十六进制数
 * @param bcd BCD编码数据
 * @return uint8_t 十六进制结果
 */
uint8_t bcd_to_hex8(uint8_t bcd) {
  return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/**
 * @brief 16位十六进制数转BCD码
 * @param hex 十六进制数(0-9999)
 * @return uint16_t BCD编码结果
 */
uint16_t hex_to_bcd16(uint16_t hex) {
  uint16_t result = 0;

  for (size_t i = 0; i < 4; i++) {
    result = (result >> 4) | ((hex % 10) << 12);
    hex /= 10;
  }

  return result;
}

/**
 * @brief 16位BCD码转十六进制数
 * @param bcd BCD编码数据
 * @return uint16_t 十六进制结果
 */
uint16_t bcd_to_hex16(uint16_t bcd) {
  uint16_t result = 0;

  for (size_t i = 0; i < 4; i++) {
    result = result * 10 + ((bcd >> 12) & 0x0F);
    bcd <<= 4;
  }

  return result;
}

/**
 * @brief 32位十六进制数转BCD码
 * @param hex 十六进制数(0-99999999)
 * @return uint32_t BCD编码结果
 */
uint32_t hex_to_bcd32(uint32_t hex) {
  uint32_t result = 0;

  for (size_t i = 0; i < 8; i++) {
    result = (result >> 4) | ((hex % 10) << 28);
    hex /= 10;
  }

  return result;
}

/**
 * @brief 32位BCD码转十六进制数
 * @param bcd BCD编码数据
 * @return uint32_t 十六进制结果
 */
uint32_t bcd_to_hex32(uint32_t bcd) {
  uint32_t result = 0;

  for (size_t i = 0; i < 8; i++) {
    result = result * 10 + ((bcd >> 28) & 0x0F);
    bcd <<= 4;
  }

  return result;
}

pu_endian_type_e pu_get_current_endian(void) {
  union {
    uint32_t i;
    uint8_t c[4];
  } test = {.i = 0x12345678};

  // 小端模式下,低地址存储低字节(0x78)
  return (test.c[0] == 0x78) ? PU_ENDIAN_LITTLE : PU_ENDIAN_BIG;
}

/**
 * @brief 打印当前系统的字节序信息
 */
void pu_print_endian_info(void) {
  if (pu_get_current_endian() == PU_ENDIAN_LITTLE) {
    PU_LOG_DEBUG("(Little-Endian)\n");
  } else {
    PU_LOG_DEBUG("(Big-Endian)\n");
  }
}

/**
 * @brief 根据目标端序对字节数组进行转换(原地转换)
 * @param data 指向要转换的字节数组
 * @param length 数组的长度(字节数)
 * @param target_endian 目标端序(ENDIAN_LITTLE 或 ENDIAN_BIG)
 * @note 如果当前系统端序与目标端序一致,则不进行转换
 */
void pu_reverse_byte_array_by_endian(void *data, uint16_t length, pu_endian_type_e target_endian) {
  // 空指针或长度小于等于1无需转换
  if (data == NULL || length <= 1) {
    return;
  }

  // 获取当前系统端序
  pu_endian_type_e current_endian = pu_get_current_endian();

  // 如果当前端序与目标端序一致,则不转换
  if (current_endian == target_endian) {
    return;
  }
  pu_reverse_byte_array(data, length);
}

void *pu_get_stream_position_pointer(pu_stream_p stream) {
  if (stream == NULL)
    return NULL;
  return (void *)(stream->buffer + stream->position);
}

/**
 * @brief 将流的位置向后偏移指定字节数
 * @param stream AXDR流指针
 * @param offset 要偏移的字节数
 * @return 成功返回0,失败返回-1(偏移超出缓冲区范围)
 */
int pu_seek_stream(pu_stream_p stream, uint16_t offset) {
  if (stream == NULL) {
    return -1;
  }

  if (stream->position + offset > stream->size) {
    // 偏移超出缓冲区范围
    return -1;
  }

  stream->position += offset;
  return 0;
}

/**
 * @brief 向AXDR流中写入数据
 * @param stream AXDR流指针
 * @param data 要写入的数据指针
 * @param data_size 要写入的数据大小
 * @return 成功写入的字节数,失败返回0
 */
uint16_t pu_write_stream(pu_stream_p stream, const void *data, uint16_t data_size) {
  if (stream == NULL || data == NULL || data_size == 0) {
    return 0;
  }

  if (!stream->is_encoding) {
    // 当前流不是编码模式
    return 0;
  }
  // 增加尝试写入数据
  stream->try_to_write += data_size;
  // 检查缓冲区是否足够
  if (stream->position + data_size > stream->size) {
    return 0;
  }

  // 拷贝数据到缓冲区
  memcpy(stream->buffer + stream->position, data, data_size);

  // 更新位置
  stream->position += data_size;

  return data_size;
}

/**
 * @brief bcd数据转换
 *
 * @param value 输入的数据
 * @param buffer 输出的数据
 * @param size 数据数据的大小
 * @return true
 * @return false
 */
bool pu_convert_to_bcd(uint32_t value, uint8_t *buffer, int size) {
  // 根据size确定最大值
  uint32_t max_value;
  // 缓冲区清零
  memset(buffer, 0, size);

  switch (size) {
  case 4:
    max_value = 99999999U;
    break;
  case 3:
    max_value = 999999U;
    break;
  case 2:
    max_value = 9999U;
    break;
  case 1:
    max_value = 99U;
    break;
  default:
    return false;
  }

  // 钳位到最大值
  if (value > max_value) {
    return false;
  }

  // 清空缓冲区
  for (int i = 0; i < size; i++) {
    buffer[i] = 0;
  }

  // BCD转换
  uint32_t temp = value;
  for (int i = size - 1; i >= 0; i--) {
    uint8_t low_digit = temp % 10;
    temp /= 10;
    uint8_t high_digit = temp % 10;
    temp /= 10;
    buffer[i] = (high_digit << 4) | low_digit;
  }
  pu_reverse_byte_array(buffer, size);
  return true;
}

#ifdef __KERNEL__
/*
 * 内核态强符号实现：覆盖下方用户态弱符号的职责，输出走 printk。
 * pu_fprintf 忽略 stream 参数（stderr/stdout/stdin 均统一到内核日志）。
 */
int pu_vfprintf(FILE *stream, const char *format, va_list args) {
  char buf[256]; // 内核栈有限，限制单条日志长度
  int ret = vsnprintf(buf, sizeof(buf), format, args);
  printk("%s", buf);
  return ret;
}

int pu_fprintf(FILE *stream, const char *format, ...) {
  va_list args;
  int ret;
  PU_UNUSED(stream);
  va_start(args, format);
  ret = pu_vfprintf(stream, format, args);
  va_end(args);
  return ret;
}

int pu_fflush(FILE *stream) {
  PU_UNUSED(stream);
  return 0; // printk 无需 flush
}

void pu_get_log_time(char *time_buf, int buf_len) {
  if (time_buf != NULL && buf_len > 0) {
    time_buf[0] = '\0'; // 内核态暂不提供时间戳，可按需用 do_gettimeofday 扩展
  }
}

#else /* 用户态：弱符号，允许使用者自行覆盖 */

/**
 * @brief 禁止修改此代码,此代码用于提醒用户自定义自己的pu_fprintf 避免使用默认定义,避免频繁修改库函数
 *
 * @param stream
 * @param format
 * @param ...
 * @return PU_COMPILER_WEAK
 */
PU_COMPILER_WEAK int pu_vfprintf(FILE *stream, const char *format, va_list args) {
  PU_UNUSED(stream);
  PU_UNUSED(format);
  PU_UNUSED(args);
  return 0;
}

/**
 * @brief 禁止修改此代码,此代码用于提醒用户自定义自己的pu_fprintf 避免使用默认定义,避免频繁修改库函数
 *
 * @param stream
 * @param format
 * @param ...
 * @return PU_COMPILER_WEAK
 */
PU_COMPILER_WEAK int pu_fprintf(FILE *stream, const char *format, ...) {
  PU_UNUSED(stream);
  PU_UNUSED(format);
  return 0;
}
/**
 * @brief 禁止修改此代码,此代码用于提醒用户自定义自己的pu_fflush 避免使用默认定义,避免频繁修改库函数
 *
 * @param stream
 * @return PU_COMPILER_WEAK
 */
PU_COMPILER_WEAK int pu_fflush(FILE *stream) {
  PU_UNUSED(stream);
  return 0;
}

/**
 * @brief 获取当前时间戳（格式：YYYY-MM-DD HH:MM:SS）
 *
 * @param time_buf
 * @param buf_len
 */
PU_COMPILER_WEAK void pu_get_log_time(char *time_buf, int buf_len) {
  PU_UNUSED(time_buf);
  PU_UNUSED(buf_len);
}

#endif /* __KERNEL__ */

/**
 * @brief 获取日志级别字符串
 *
 * @param level 日志级别
 * @return const char*
 */
const char *pu_get_level_str(pu_log_level_e level) {
  switch (level) {
  case PU_LOG_LEVEL_DEBUG:
    return "DEBUG";
  case PU_LOG_LEVEL_INFO:
    return "INFO";
  case PU_LOG_LEVEL_WARN:
    return "WARN";
  case PU_LOG_LEVEL_ERROR:
    return "ERROR";
  case PU_LOG_LEVEL_FATAL:
    return "FATAL";
  default:
    return "UNKNOWN";
  }
}

/**
 * @brief 封装的日志打印函数
 *
 * @param level 日志级别
 * @param file 调用文件  __FILE__
 * @param line 调用行号  __LINE__
 * @param fmt  格式化字符串
 * @param ...  可变参数
 */
void pu_log_printf(pu_log_level_e level, const char *file, int line, const char *fmt, ...) {
  PU_UNUSED(file); // 避免编译器警告
  PU_UNUSED(line); // 避免编译器警告

  if (level < g_log_level) {
    return;
  }
#if defined(PU_LOCK_ALLOC)
  PU_LOCK_ALLOC();
#endif
#if defined(PU_LOCK_ON)
  PU_LOCK_ON();
#endif
  // 根据日志级别选择颜色
  const char *color = COLOR_RESET;
  switch (level) {
  case PU_LOG_LEVEL_DEBUG:
    color = COLOR_CYAN;
    break;
  case PU_LOG_LEVEL_INFO:
    color = COLOR_GREEN;
    break;
  case PU_LOG_LEVEL_WARN:
    color = COLOR_YELLOW;
    break;
  case PU_LOG_LEVEL_ERROR:
    color = COLOR_BOLD_RED;
    break;
  case PU_LOG_LEVEL_FATAL:
    color = COLOR_RED;
    break;
  default:
    color = COLOR_WHITE;
    break;
  }
  pu_fprintf(stderr, "%s", color);
  if (level >= PU_LOG_LEVEL_ERROR) {
    pu_fprintf(stderr, "File: %s Line: %d\n[%s]\n", file, line, pu_get_level_str(level));
  } else {
    pu_fprintf(stderr, "[%s]", pu_get_level_str(level));
  }
  va_list args;
  va_start(args, fmt);
  pu_vfprintf(stderr, fmt, args);
  va_end(args);
  pu_fprintf(stderr, "\n");
  pu_fprintf(stderr, "%s\n", COLOR_RESET);
  pu_fflush(stderr);
#if defined(PU_LOCK_OFF)
  PU_LOCK_OFF();
#endif
}

/**
 * @brief 封装的16进制打印函数
 *
 * @param level 日志级别
 * @param file 调用文件
 * @param line 调用行号
 * @param data 打印的数据
 * @param len 数据长度
 * @param desc 数据描述
 */
void pu_log_hex(pu_log_level_e level, const char *file, int line, const void *data, size_t len, const char *desc) {
  PU_UNUSED(desc); // 目前未使用desc参数,避免编译器警告
  if (level < g_log_level) {
    return; // 只有在ERROR和FATAL级别打印
  }
#if defined(PU_LOCK_ALLOC)
  PU_LOCK_ALLOC();
#endif
#if defined(PU_LOCK_ON)
  PU_LOCK_ON();
#endif
  // 根据日志级别选择颜色
  const char *color = COLOR_RESET;
  switch (level) {
  case PU_LOG_LEVEL_DEBUG:
    color = COLOR_CYAN;
    break;
  case PU_LOG_LEVEL_INFO:
    color = COLOR_GREEN;
    break;
  case PU_LOG_LEVEL_WARN:
    color = COLOR_YELLOW;
    break;
  case PU_LOG_LEVEL_ERROR:
    color = COLOR_BOLD_RED;
    break;
  case PU_LOG_LEVEL_FATAL:
    color = COLOR_RED;
    break;
  default:
    color = COLOR_WHITE;
    break;
  }
  pu_fprintf(stderr, "%s", color);

  if (level >= PU_LOG_LEVEL_ERROR) {
    pu_fprintf(stderr, "File: %s Line: %d\n[%s]", file, line, pu_get_level_str(level));
  } else {
    pu_fprintf(stderr, "[%s]", pu_get_level_str(level));
  }

  const uint8_t *bytes = (const uint8_t *)data;
  static char hex_buf[3 * 16 + 2] = {0};
  size_t hex_buf_offset; // 缓冲区偏移

  for (size_t i = 0; i < len; i += 16) {
    hex_buf_offset = 0;
    size_t line_len = (len - i) >= 16 ? 16 : (len - i);
    for (size_t j = 0; j < line_len; j++) {
      sprintf(hex_buf + hex_buf_offset, "%02X ", bytes[i + j]);
      hex_buf_offset += 3;
    }
    if (hex_buf_offset > 0) {
      hex_buf[hex_buf_offset - 1] = '\n';
    }
    pu_fprintf(stderr, "%s", hex_buf);
    memset(hex_buf, 0, sizeof(hex_buf));
  }
  pu_fprintf(stderr, "%s\n", COLOR_RESET);
  pu_fflush(stderr);
#if defined(PU_LOCK_OFF)
  PU_LOCK_OFF();
#endif
}

void pu_set_log_level(pu_log_level_e level) {
  g_log_level = level;
}
pu_log_level_e pu_get_log_level() {
  return g_log_level;
}

void pu_crash_info(FILE *stream) {
  if (g_backtrace_context.vaild_magic_number == PU_VAILD_MAGIC) {
    switch (g_backtrace_context.fault_reason) {
    case PU_FAULT_REASON_NONE: {
      pu_fprintf(stream, "未发生异常\n");
      return;
    } break;
    case PU_FAULT_REASON_MEM_MANAGE: {
      pu_fprintf(stream, "内存管理异常\n");
    } break;
    case PU_FAULT_REASON_BUS_FAULT: {
      pu_fprintf(stream, "总线故障异常\n");
    } break;
    case PU_FAULT_REASON_USAGE_FAULT: {
      pu_fprintf(stream, "指令异常\n");
    } break;
    case PU_FAULT_REASON_HARD_FAULT: {
      pu_fprintf(stream, "硬件异常\n");
    } break;
    case PU_FAULT_REASON_WATCH_DOG: {
      pu_fprintf(stream, "硬件异常\n");
    } break;
    default: {
      pu_fprintf(stream, "未知原因复位\n");
    }
    }
    pu_fprintf(stream, "LR: 0x%08X (返回地址)\n", g_backtrace_context.LR);
    pu_fprintf(stream, "PC: 0x%08X (故障指令地址)\n", g_backtrace_context.PC);
    if (strlen(g_backtrace_context.info)) {
      pu_fprintf(stream, "异常信息: %s\n", g_backtrace_context.info);
    }
  }
  memset(&g_backtrace_context, 0xAA, sizeof(g_backtrace_context));
}

PU_COMPILER_WEAK void pu_stash_thread_info(uint32_t *exception_stack_ptr) {
  PU_UNUSED(exception_stack_ptr);
  memset(g_backtrace_context.info, 0x00, sizeof(g_backtrace_context.info));
}
