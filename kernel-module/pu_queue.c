#include "pu_queue.h"
#include "pu_macro.h"
#include <stdlib.h>

pu_queue_p pu_queue_create(size_t capacity) {
  pu_malloc_instance(result, pu_queue); // 复用您的内存分配宏
  if (result == NULL)
    return NULL;

  result->head = NULL;
  result->tail = NULL;
  result->size = 0;
  result->capacity = capacity;
  return result;
}

void pu_queue_clear(pu_queue_p queue) {
  if (queue == NULL)
    return;

  while (pu_queue_is_not_empty(queue)) {
    pu_node_p node = (pu_node_p)pu_queue_dequeue(queue);
    if (node != NULL) {
      pu_free(node);
    }
  }
}

void pu_queue_destroy(pu_queue_p queue) {
  if (queue == NULL)
    return;

  pu_queue_clear(queue);
  pu_free(queue);
}

bool pu_queue_enqueue(pu_queue_p queue, void *data) {
  if (queue == NULL)
    return false;
  // 不允许超过容量
  size_t curent_size = pu_queue_get_size(queue) + 1;
  size_t current_capacity = pu_queue_get_capacity(queue);
  if (curent_size > current_capacity) {
    return false;
  }

  pu_malloc_instance(node, pu_node);
  if (node == NULL)
    return false;

  node->data = data;
  node->prev = NULL;
  node->next = NULL;

  if (pu_queue_is_empty(queue)) {
    queue->head = node;
    queue->tail = node;
  } else {
    queue->tail->next = node;
    node->prev = queue->tail;
    queue->tail = node;
  }
  queue->size++;
  return true;
}

void *pu_queue_dequeue(pu_queue_p queue) {
  if (queue == NULL || pu_queue_is_empty(queue)) {
    return NULL;
  }

  pu_node_p temp_node = queue->head;
  void *temp_data = temp_node->data;

  // 更新头节点
  queue->head = queue->head->next;
  if (queue->head != NULL) {
    queue->head->prev = NULL;
  } else {
    queue->tail = NULL; // 队列为空时,尾节点也置空
  }

  pu_free(temp_node);
  queue->size--;
  return temp_data;
}

pu_node_p pu_queue_get_link_head(pu_queue_p queue) {
  return (queue != NULL) ? queue->head : NULL;
}

bool pu_queue_is_empty(pu_queue_p queue) {
  return (queue != NULL) ? (queue->size == 0) : true;
}

bool pu_queue_is_not_empty(pu_queue_p queue) {
  return !pu_queue_is_empty(queue);
}
