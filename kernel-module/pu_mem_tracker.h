#ifndef PU_MEM_TRACKER_H_
#define PU_MEM_TRACKER_H_

#include <stddef.h>

// 内存分配函数指针类型定义
typedef void *(*pu_calloc_func_t)(size_t size);
typedef void (*pu_free_func_t)(void *ptr);

// 内存分配信息节点
typedef struct pu_mem_node {
  void *ptr;                // 分配的内存指针
  size_t size;              // 分配的大小
  const char *file;         // 分配所在的文件
  int line;                 // 分配所在的行号
  struct pu_mem_node *next; // 下一个节点
} pu_mem_node_t;

// 内存跟踪器
typedef struct {
  pu_mem_node_t *head;    // 链表头
  size_t total_allocated; // 总分配内存
  size_t current_usage;   // 当前使用内存
  size_t peak_usage;      // 峰值内存使用量

  // 自定义内存分配函数
  pu_calloc_func_t calloc_func;
  pu_free_func_t free_func;
} pu_mem_tracker_t;

// 函数声明
void pu_mem_tracker_init(void);
void pu_mem_set_custom_allocator(pu_calloc_func_t calloc_fn, pu_free_func_t free_fn);
void *pu_mem_calloc_debug(size_t size, const char *file, int line);
void pu_mem_free_debug(void *ptr, const char *file, int line);
void pu_mem_tracker_cleanup(void);
void pu_mem_tracker_dump(void);
size_t pu_mem_get_current_usage(void);
size_t pu_mem_get_peak_usage(void);
size_t pu_mem_get_total_allocated(void);

// 宏定义，方便使用
#define pu_malloc(size) pu_mem_calloc_debug(size, __FILE__, __LINE__)
#define pu_free(ptr)    pu_mem_free_debug(ptr, __FILE__, __LINE__)

#endif // PU_MEM_TRACKER_H_
