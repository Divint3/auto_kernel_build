#ifndef PU_MACRO_H_
#define PU_MACRO_H_
#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）
#include "pu_mem_tracker.h"
#include "pu_type.h"
#include "pu_util.h"

extern FILE *pu_log_file_fd;
extern const char pu_log_file_name[128];

/**
 * @brief 宏函数
 *
 */
#define PU_UNUSED(x)    (void)(x)
#define PU_STR(x)       #x
#define PU_GET_STR(x)   PU_STR(x)
#define PU_GET_COUNT(x) (sizeof(x) / sizeof(x[0]))

/**
 * @brief 位相关
 *
 */
#define PU_SET_BIT(num, pos)   ((num) |= (1 << (pos)))  // 置位宏: 将 num 的 pos 位设置为 1
#define PU_RESET_BIT(num, pos) ((num) &= ~(1 << (pos))) // 取消置位宏: 将 num 的 pos 位设置为 0
#define PU_GET_BIT(num, pos)   (((num) >> (pos)) & 1)   // 获取 num 的 pos 位值

/**
 * @brief 日志相关
 *
 */
#define BACKEND_LOG_FAULT(format, ...)                                                                           \
  do {                                                                                                           \
    pu_fprintf(stderr, "\033[32;1m file %s function: %s line :%d\r\n\033[0m", __FILE__, __FUNCTION__, __LINE__); \
    pu_fprintf(stderr, format, ##__VA_ARGS__);                                                                   \
    pu_fflush(NULL);                                                                                             \
  } while (0)

#define BACKEND_LOG(format, ...)                                       \
  do {                                                                 \
    if (pu_log_file_fd) {                                              \
      pu_fprintf(pu_log_file_fd, (char const *)format, ##__VA_ARGS__); \
      pu_fflush(NULL);                                                 \
    } else {                                                           \
      pu_fprintf(NULL, format, ##__VA_ARGS__);                         \
      pu_fflush(NULL);                                                 \
    }                                                                  \
  } while (0)

/**不安全的宏函数 */
#define pu_malloc_instance(result, type)                type##_p result = (type##_p)pu_mem_calloc_debug(sizeof(type##_t), __FILE__, __LINE__)
#define pu_malloc_flexible_instance(result, type, size) type##_p result = (type##_p)pu_mem_calloc_debug(sizeof(type##_t) + size, __FILE__, __LINE__)

static inline pu_data_p PU_DATA_MALLOC(size_t length, const char *file, int line) {
  pu_data_p ptr = (pu_data_p)pu_mem_calloc_debug(sizeof(pu_data_t), file, line);
  if (ptr != NULL) {
    ptr->size = length;
    if (length == 0) {
      ptr->data = NULL;
      return ptr;
    }
    ptr->data = pu_mem_calloc_debug(length, file, line);
    // 如果数据内存分配失败，需要清理
    if (ptr->data == NULL) {
      pu_free(ptr);
      ptr = NULL;
    }
  }
  return ptr;
}

#define pu_data_malloc(length) PU_DATA_MALLOC(length, __FILE__, __LINE__)

static inline pu_data_p PU_DATA_PACK(const void *input_data, size_t length, const char *file, int line) {
  pu_data_p ptr = PU_DATA_MALLOC(length, file, line);
  if (ptr != NULL && input_data != NULL) {
    memcpy(ptr->data, input_data, length);
  }
  return ptr;
}
#define pu_data_pack(input_data, length) PU_DATA_PACK(input_data, length, __FILE__, __LINE__)

// 封装宏，自动传递文件名、行号，简化调用
#if defined(__DEBUG_) && __DEBUG_
#define PU_LOG_DEBUG(fmt, ...)            pu_log_printf(PU_LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define PU_LOG_INFO(fmt, ...)             pu_log_printf(PU_LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define PU_LOG_WARN(fmt, ...)             pu_log_printf(PU_LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define PU_LOG_ERROR(fmt, ...)            pu_log_printf(PU_LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define PU_LOG_FATAL(fmt, ...)            pu_log_printf(PU_LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define PU_LOG_HEX_FATAL(data, len, desc) pu_log_hex(PU_LOG_LEVEL_FATAL, __FILE__, __LINE__, data, len, desc)
#else
#define PU_LOG_DEBUG(fmt, ...)            
#define PU_LOG_INFO(fmt, ...)             
#define PU_LOG_WARN(fmt, ...)             
#define PU_LOG_ERROR(fmt, ...)            
#define PU_LOG_FATAL(fmt, ...)            
#define PU_LOG_HEX_FATAL(data, len, desc) 
#endif
#endif /* #ifndef PU_MACRO_H_ */
