#include "pu_ringbuffer.h"
#include "pu_macro.h"
#include <stdint.h>
#include <stdlib.h>

// 静态缓冲区创建
pu_ringbuffer_p pu_ringbuffer_create_static(uint8_t *buf, size_t buf_size) {
  if (buf == NULL || (buf_size & (buf_size - 1)) != 0) { // 检查是否为2的幂
    return NULL;
  }

  pu_ringbuffer_p rb = (pu_ringbuffer_p)pu_malloc(sizeof(pu_ringbuffer_t));
  if (rb == NULL)
    return NULL;

  rb->type = PU_RINGBUFFER_TYPE_STATIC;
  rb->buffer = buf;
  rb->buffer_mask = buf_size - 1;
  rb->head_index = 0;
  rb->tail_index = 0;
  rb->fun_lock = NULL;
  rb->fun_unlock = NULL;

  return rb;
}

// 动态缓冲区创建
pu_ringbuffer_p pu_ringbuffer_create_dynamic(size_t buf_size) {
  if ((buf_size & (buf_size - 1)) != 0) { // 检查是否为2的幂
    return NULL;
  }

  uint8_t *buf = (uint8_t *)pu_malloc(buf_size);
  if (buf == NULL)
    return NULL;

  pu_ringbuffer_p rb = pu_ringbuffer_create_static(buf, buf_size);
  if (rb != NULL) {
    rb->type = PU_RINGBUFFER_TYPE_DYNAMIC;
  } else {
    pu_free(buf);
  }
  return rb;
}

// 销毁缓冲区
void pu_ringbuffer_destroy(pu_ringbuffer_p rb) {
  if (rb == NULL)
    return;

  // 动态缓冲区需释放内部内存
  if (rb->type == PU_RINGBUFFER_TYPE_DYNAMIC && rb->buffer != NULL) {
    pu_free(rb->buffer);
    rb->buffer = NULL;
  }

  pu_free(rb);
}

// 配置锁函数
void pu_ringbuffer_config_lock(pu_ringbuffer_p rb, pu_ringbuffer_lock_pf lock_fun, pu_ringbuffer_lock_pf unlock_fun) {
  if (rb == NULL)
    return;
  rb->fun_lock = lock_fun;
  rb->fun_unlock = unlock_fun;
}

// 内部锁操作
static void pu_ringbuffer_lock(pu_ringbuffer_p rb) {
  if (rb != NULL && rb->fun_lock != NULL) {
    rb->fun_lock();
  }
}

// 内部解锁操作
static void pu_ringbuffer_unlock(pu_ringbuffer_p rb) {
  if (rb != NULL && rb->fun_unlock != NULL) {
    rb->fun_unlock();
  }
}

// 单字节入队
void pu_ringbuffer_enqueue_byte(pu_ringbuffer_p rb, uint8_t data) {
  if (rb == NULL || pu_ringbuffer_is_full(rb)) {
    return;
  }

  pu_ringbuffer_lock(rb);
  rb->buffer[rb->head_index] = data;
  rb->head_index = (rb->head_index + 1) & RINGBUFFER_MASK(rb);
  pu_ringbuffer_unlock(rb);
}

// 多字节入队
void pu_ringbuffer_enqueue_array(pu_ringbuffer_p rb, const uint8_t *data, size_t size) {
  if (rb == NULL || data == NULL || size == 0) {
    return;
  }

  for (size_t i = 0; i < size; i++) {
    pu_ringbuffer_enqueue_byte(rb, data[i]);
  }
}

// 单字节出队
uint8_t pu_ringbuffer_dequeue_byte(pu_ringbuffer_p rb, uint8_t *data) {
  if (rb == NULL || data == NULL || pu_ringbuffer_is_empty(rb)) {
    return 0;
  }

  pu_ringbuffer_lock(rb);
  *data = rb->buffer[rb->tail_index];
  rb->tail_index = (rb->tail_index + 1) & RINGBUFFER_MASK(rb);
  pu_ringbuffer_unlock(rb);
  return 1;
}

// 多字节出队
size_t pu_ringbuffer_dequeue_array(pu_ringbuffer_p rb, uint8_t *data, size_t len) {
  if (rb == NULL || data == NULL || len == 0 || pu_ringbuffer_is_empty(rb)) {
    return 0;
  }

  size_t dequeued_len = 0;
  while (dequeued_len < len && pu_ringbuffer_dequeue_byte(rb, &data[dequeued_len])) {
    dequeued_len++;
  }
  return dequeued_len;
}

// 丢弃单字节
uint8_t pu_ringbuffer_drop_byte(pu_ringbuffer_p rb) {
  uint8_t dummy;
  return pu_ringbuffer_dequeue_byte(rb, &dummy);
}

// 丢弃多字节
uint8_t pu_ringbuffer_drop_array(pu_ringbuffer_p rb, size_t size) {
  size_t dropped_len = 0;
  while (dropped_len < size && pu_ringbuffer_drop_byte(rb)) {
    dropped_len++;
  }
  return (uint8_t)dropped_len;
}

// peek 单字节
uint8_t pu_ringbuffer_peek_byte(pu_ringbuffer_p rb, uint8_t *data, size_t index) {
  if (rb == NULL || data == NULL || index >= pu_ringbuffer_get_valid_len(rb)) {
    return 0;
  }

  pu_ringbuffer_lock(rb);
  size_t data_idx = (rb->tail_index + index) & RINGBUFFER_MASK(rb);
  *data = rb->buffer[data_idx];
  pu_ringbuffer_unlock(rb);
  return 1;
}

// 检查 peek 数据
bool pu_ringbuffer_check_peek(pu_ringbuffer_p rb, uint8_t data, size_t index) {
  uint8_t peek_data;
  if (pu_ringbuffer_peek_byte(rb, &peek_data, index) == 0) {
    return false;
  }
  return (peek_data == data);
}

// peek 多字节
uint8_t pu_ringbuffer_peek_array(pu_ringbuffer_p rb, uint8_t *data, size_t index, size_t count) {
  if (rb == NULL || data == NULL || count == 0) {
    return 0;
  }

  for (size_t i = 0; i < count; i++) {
    if (pu_ringbuffer_peek_byte(rb, &data[i], index + i) == 0) {
      return 0;
    }
  }
  return 1;
}

// 复制数据(不删除)
size_t pu_ringbuffer_copy_array(pu_ringbuffer_p rb, uint8_t *data, size_t size) {
  if (rb == NULL || data == NULL || size == 0 || pu_ringbuffer_is_empty(rb)) {
    return 0;
  }

  size_t valid_len = pu_ringbuffer_get_valid_len(rb);
  size_t copy_len = (valid_len < size) ? valid_len : size;

  for (size_t i = 0; i < copy_len; i++) {
    pu_ringbuffer_peek_byte(rb, &data[i], i);
  }
  return copy_len;
}

// 搜索帧头
size_t pu_ringbuffer_search_head(pu_ringbuffer_p rb, uint8_t head) {
  if (rb == NULL || pu_ringbuffer_is_empty(rb)) {
    return 0;
  }

  size_t valid_len = pu_ringbuffer_get_valid_len(rb);
  for (size_t i = 0; i < valid_len; i++) {
    uint8_t curr_byte;
    if (pu_ringbuffer_peek_byte(rb, &curr_byte, i) && curr_byte == head) {
      if (i > 0) {
        pu_ringbuffer_drop_array(rb, i); // 丢弃帧头前的无效数据
      }
      return i;
    }
  }
  size_t result = pu_ringbuffer_get_valid_len(rb);
  // 未找到帧头,丢弃所有数据
  pu_ringbuffer_clear(rb);
  return result;
}

// 搜索下一个帧头
void pu_ringbuffer_search_next_head(pu_ringbuffer_p rb, uint8_t head) {
  if (rb == NULL)
    return;
  pu_ringbuffer_drop_byte(rb); // 丢弃当前读索引数据
  pu_ringbuffer_search_head(rb, head);
}

// 检查是否为空
bool pu_ringbuffer_is_empty(pu_ringbuffer_p rb) {
  if (rb == NULL)
    return true;
  return (rb->head_index == rb->tail_index);
}

// 检查是否满
bool pu_ringbuffer_is_full(pu_ringbuffer_p rb) {
  if (rb == NULL)
    return true;
  return ((rb->head_index - rb->tail_index) & RINGBUFFER_MASK(rb)) == RINGBUFFER_MASK(rb);
}

// 获取有效数据长度
size_t pu_ringbuffer_get_valid_len(pu_ringbuffer_p rb) {
  if (rb == NULL)
    return 0;
  return ((rb->head_index - rb->tail_index) & RINGBUFFER_MASK(rb));
}

// 清空缓冲区
void pu_ringbuffer_clear(pu_ringbuffer_p rb) {
  if (rb == NULL)
    return;

  pu_ringbuffer_lock(rb);
  rb->head_index = rb->tail_index;
  pu_ringbuffer_unlock(rb);
}

size_t pu_ringbuffer_get_contiguous_data(pu_ringbuffer_p rb, uint8_t **ptr) {
  size_t tail = rb->tail_index;
  size_t head = rb->head_index;
  size_t mask = rb->buffer_mask;

  if (head > tail) {
    // 未回绕：数据连续
    *ptr = &rb->buffer[tail];
    return head - tail;
  } else if (head < tail) {
    // 已回绕：只能发到缓冲区末尾
    *ptr = &rb->buffer[tail];
    return (mask + 1) - tail;
  } else {
    return 0; // 空
  }
}