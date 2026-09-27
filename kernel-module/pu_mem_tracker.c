#include "pu_mem_tracker.h"
#include "pu_macro.h"

#include <linux/slab.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static pu_mem_tracker_t pu_tracker = {0};

// 默认使用C库内存函数
static void *pu_default_calloc(size_t size) {
  // return calloc(size, 1);
  return kmalloc(size, GFP_KERNEL);
}
static void pu_default_free(void *ptr) {
  // free(ptr);
  kfree(prt);
}

// 初始化内存跟踪器
void pu_mem_tracker_init(void) {
  pu_tracker.head = NULL;
  pu_tracker.total_allocated = 0;
  pu_tracker.current_usage = 0;
  pu_tracker.peak_usage = 0;

  // 设置默认内存分配函数
  pu_tracker.calloc_func = pu_default_calloc;
  pu_tracker.free_func = pu_default_free;

  // PU_LOG_DEBUG("pu_mem_tracker: Memory tracker initialized\n");
}

// 设置自定义内存分配器
void pu_mem_set_custom_allocator(pu_calloc_func_t calloc_fn, pu_free_func_t free_fn) {
  if (calloc_fn)
    pu_tracker.calloc_func = calloc_fn;
  if (free_fn)
    pu_tracker.free_func = free_fn;

  // PU_LOG_DEBUG("pu_mem_tracker: Custom memory allocator set\n");
}

#define PU_MEM_NODE

#ifdef PU_MEM_NODE
// 创建新的内存节点
static pu_mem_node_t *pu_create_mem_node(void *ptr, size_t size, const char *file, int line) {
  pu_mem_node_t *node = (pu_mem_node_t *)pu_tracker.calloc_func(sizeof(pu_mem_node_t));
  if (!node) {
    PU_LOG_DEBUG("pu_mem_tracker ERROR: Failed to create memory tracker node\n");
    return NULL;
  }

  node->ptr = ptr;
  node->size = size;
  node->file = file;
  node->line = line;
  node->next = NULL;

  return node;
}

// 添加内存节点到链表
static void pu_add_mem_node(pu_mem_node_t *node) {
  node->next = pu_tracker.head;
  pu_tracker.head = node;
  pu_tracker.total_allocated += node->size;
  pu_tracker.current_usage += node->size;

  // 更新峰值使用量
  if (pu_tracker.current_usage > pu_tracker.peak_usage) {
    pu_tracker.peak_usage = pu_tracker.current_usage;
  }
}

// 从链表中删除内存节点
static int pu_remove_mem_node(void *ptr) {
  pu_mem_node_t *current = pu_tracker.head;
  pu_mem_node_t *prev = NULL;

  while (current != NULL) {
    if (current->ptr == ptr) {
      // 找到节点，从链表中移除
      if (prev == NULL) {
        pu_tracker.head = current->next;
      } else {
        prev->next = current->next;
      }

      pu_tracker.current_usage -= current->size;
      pu_tracker.free_func(current);
      return 1; // 成功删除
    }
    prev = current;
    current = current->next;
  }

  return 0; // 未找到节点
}
#endif

// 带清零的内存分配函数
void *pu_mem_calloc_debug(size_t size, const char *file, int line) {
  if (pu_tracker.calloc_func == NULL) {
    // lazy load
    pu_mem_tracker_init();
  }
  if (size == 0) {
    PU_LOG_DEBUG("pu_mem_tracker WARNING: Attempt to calloc 0 bytes at %s:%d\n", file, line);
    return NULL;
  }

  void *ptr = pu_tracker.calloc_func(size);

  if (!ptr) {
    PU_LOG_DEBUG("pu_mem_tracker ERROR: Calloc failed - %zu bytes at %s:%d\n", size, file, line);
    return NULL;
  }
  // 如果不是系统默认实现的calloc 那么就会内存进行初始化,设置为全0
  if (pu_tracker.calloc_func != pu_default_calloc) {
    memset(ptr, 0x00, size);
  }
#ifdef PU_MEM_NODE
  // 创建跟踪节点
  pu_mem_node_t *node = pu_create_mem_node(ptr, size, file, line);
  if (!node) {
    pu_tracker.free_func(ptr);
    PU_LOG_DEBUG("pu_mem_tracker ERROR: Failed to create tracker node for %p at %s:%d\n", ptr, file, line);
    return NULL;
  }
  pu_add_mem_node(node);
#endif
  return ptr;
}

// 内存释放函数
void pu_mem_free_debug(void *ptr, const char *file, int line) {
  if (ptr == NULL) {
    PU_LOG_DEBUG("pu_mem_tracker FREE: NULL pointer at %s:%d (safe to free NULL)\n", file, line);
    return;
  }
#ifdef PU_MEM_NODE
  if (!pu_remove_mem_node(ptr)) {
    PU_LOG_DEBUG("pu_mem_tracker WARNING: Attempt to free untracked memory %p at %s:%d\n", ptr, file, line);
    // 在嵌入式环境中，可以选择不释放或者仍然释放
    pu_tracker.free_func(ptr); // 仍然释放，但给出警告
  } else
#endif
  {
    pu_tracker.free_func(ptr);
  }
}

// 获取当前内存使用量
size_t pu_mem_get_current_usage(void) {
  return pu_tracker.current_usage;
}

// 获取峰值内存使用量
size_t pu_mem_get_peak_usage(void) {
  return pu_tracker.peak_usage;
}

// 获取总分配内存量
size_t pu_mem_get_total_allocated(void) {
  return pu_tracker.total_allocated;
}

// 打印内存使用情况
void pu_mem_tracker_dump(void) {
  PU_LOG_DEBUG("\n=== pu_mem_tracker Dump ===\n");
  PU_LOG_DEBUG("Total allocated: %zu bytes\n", pu_tracker.total_allocated);
  PU_LOG_DEBUG("Current usage: %zu bytes\n", pu_tracker.current_usage);
  PU_LOG_DEBUG("Peak usage: %zu bytes\n", pu_tracker.peak_usage);
  PU_LOG_DEBUG("Memory leaks:\n");

  pu_mem_node_t *current = pu_tracker.head;
  int leak_count = 0;
  size_t leak_size = 0;

  while (current != NULL) {
    PU_LOG_ERROR("  Leak: %p, %zu bytes at %s:%d\n", current->ptr, current->size, current->file, current->line);
    leak_size += current->size;
    current = current->next;
    leak_count++;
  }

  if (leak_count == 0) {
    PU_LOG_ERROR("  No memory leaks detected\n");
  } else {
    PU_LOG_ERROR("  Total leaks: %d, Total leak size: %zu bytes\n", leak_count, leak_size);
  }
  PU_LOG_DEBUG("============================\n");
}

// 清理内存跟踪器
void pu_mem_tracker_cleanup(void) {
  pu_mem_node_t *current = pu_tracker.head;
  pu_mem_node_t *next;
  int leak_count = 0;
  size_t leak_size = 0;

  while (current != NULL) {
    next = current->next;
    PU_LOG_DEBUG("pu_mem_tracker WARNING: Memory leak - %p, %zu bytes at %s:%d\n", current->ptr, current->size, current->file, current->line);
    leak_size += current->size;
    pu_tracker.free_func(current->ptr); // 释放泄漏的内存
    pu_tracker.free_func(current);      // 释放节点本身
    current = next;
    leak_count++;
  }

  pu_tracker.head = NULL;
  pu_tracker.current_usage = 0;

  if (leak_count > 0) {
    PU_LOG_DEBUG("pu_mem_tracker: Cleaned up %d memory leaks, total %zu bytes\n", leak_count, leak_size);
  }

  PU_LOG_DEBUG("pu_mem_tracker: Memory tracker cleanup completed\n");
}
