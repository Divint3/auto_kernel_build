#ifndef PU_QUEUE_H
#define PU_QUEUE_H

#include "pu_macro.h"
#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）

// 节点结构体(内部使用,外部无需暴露)
typedef struct pu_node_s {
  void *data;
  struct pu_node_s *prev;
  struct pu_node_s *next;
} pu_node_t, *pu_node_p;

// 队列结构体(外部仅需指针访问)
typedef struct pu_queue_s {
  pu_node_p head;
  pu_node_p tail;
  size_t size;     // 队列元素数量
  size_t capacity; // 容量
} pu_queue_t, *pu_queue_p;

/**
 * @brief 创建队列(替代原 pu_queue_new)
 * @return 队列指针,NULL 表示创建失败
 */
pu_queue_p pu_queue_create(size_t capacity);

/**
 * @brief 清空队列(释放所有节点,保留队列对象)
 * @param queue 队列指针
 */
void pu_queue_clear(pu_queue_p queue);

/**
 * @brief 销毁队列(释放节点+队列对象,替代原 pu_queue_free)
 * @param queue 队列指针
 */
void pu_queue_destroy(pu_queue_p queue);

/**
 * @brief 入队操作(尾部插入)
 * @param queue 队列指针
 * @param data 入队数据(外部保证数据有效性)
 */
bool pu_queue_enqueue(pu_queue_p queue, void *data);

/**
 * @brief 出队操作(头部取出)
 * @param queue 队列指针
 * @return 出队数据,NULL 表示队列为空
 */
void *pu_queue_dequeue(pu_queue_p queue);

/**
 * @brief 获取队列的链表头(用于遍历)
 * @param queue 队列指针
 * @return 链表头节点指针
 */
pu_node_p pu_queue_get_link_head(pu_queue_p queue);

/**
 * @brief 获取队列大小
 * @param queue 队列指针
 * @return 队列元素数量
 */
PU_COMPILER_ALWAYS_INLINE inline size_t pu_queue_get_size(pu_queue_p queue) {
  return (queue != NULL) ? queue->size : 0;
}

PU_COMPILER_ALWAYS_INLINE inline size_t pu_queue_get_capacity(pu_queue_p queue) {
  if (queue == NULL) {
    return 0;
  } else {
    return queue->capacity;
  }
}

/**
 * @brief 检查队列是否为空
 * @param queue 队列指针
 * @return true:空,false:非空
 */
bool pu_queue_is_empty(pu_queue_p queue);

/**
 * @brief 检查队列是否非空
 * @param queue 队列指针
 * @return true:非空,false:空
 */
bool pu_queue_is_not_empty(pu_queue_p queue);

#endif // PU_QUEUE_H
