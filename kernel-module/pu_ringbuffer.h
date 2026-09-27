#ifndef PU_RINGBUFFER_H
#define PU_RINGBUFFER_H

#include "pu_macro.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// 环形缓冲区类型枚举
typedef enum {
  PU_RINGBUFFER_TYPE_STATIC, // 静态缓冲区(外部传入内存)
  PU_RINGBUFFER_TYPE_DYNAMIC // 动态缓冲区(内部申请内存)
} pu_ringbuffer_type_t;

// 环形缓冲区锁函数类型
typedef void (*pu_ringbuffer_lock_pf)(void);

// 环形缓冲区结构体(外部仅需指针访问)
typedef struct pu_ringbuffer_s {
  pu_ringbuffer_type_t type;        // 缓冲区类型
  uint8_t *buffer;                  // 数据缓冲区
  size_t buffer_mask;               // 缓冲区掩码(size-1,需size为2的幂)
  size_t head_index;                // 写索引(下一个待写位置)
  size_t tail_index;                // 读索引(下一个待读位置)
  pu_ringbuffer_lock_pf fun_lock;   // 锁函数(可选,用于多线程)
  pu_ringbuffer_lock_pf fun_unlock; // 解锁函数(可选)
} pu_ringbuffer_t, *pu_ringbuffer_p;

/**
 * @brief 创建静态环形缓冲区(外部传入内存,替代原 pu_ringbuffer_static_new)
 * @param buf 外部缓冲区指针
 * @param buf_size 缓冲区大小(必须是2的幂)
 * @return 环形缓冲区指针,NULL 表示创建失败
 */
pu_ringbuffer_p pu_ringbuffer_create_static(uint8_t *buf, size_t buf_size);

/**
 * @brief 创建动态环形缓冲区(内部申请内存,替代原 pu_ringbuffer_new)
 * @param buf_size 缓冲区大小(必须是2的幂)
 * @return 环形缓冲区指针,NULL 表示创建失败
 */
pu_ringbuffer_p pu_ringbuffer_create_dynamic(size_t buf_size);

/**
 * @brief 销毁环形缓冲区(释放内存,替代原 pu_ringbuffer_free)
 * @param rb 环形缓冲区指针
 */
void pu_ringbuffer_destroy(pu_ringbuffer_p rb);

/**
 * @brief 配置环形缓冲区的锁函数(多线程场景使用)
 * @param rb 环形缓冲区指针
 * @param lock_fun 锁函数
 * @param unlock_fun 解锁函数
 */
void pu_ringbuffer_config_lock(pu_ringbuffer_p rb, pu_ringbuffer_lock_pf lock_fun, pu_ringbuffer_lock_pf unlock_fun);

/**
 * @brief 单字节入队
 * @param rb 环形缓冲区指针
 * @param data 待入队字节
 */
void pu_ringbuffer_enqueue_byte(pu_ringbuffer_p rb, uint8_t data);

/**
 * @brief 多字节入队
 * @param rb 环形缓冲区指针
 * @param data 待入队数据指针
 * @param size 待入队数据长度
 */
void pu_ringbuffer_enqueue_array(pu_ringbuffer_p rb, const uint8_t *data, size_t size);

/**
 * @brief 单字节出队
 * @param rb 环形缓冲区指针
 * @param data 出队数据存储地址
 * @return 1:成功,0:缓冲区为空
 */
uint8_t pu_ringbuffer_dequeue_byte(pu_ringbuffer_p rb, uint8_t *data);

/**
 * @brief 多字节出队
 * @param rb 环形缓冲区指针
 * @param data 出队数据存储地址
 * @param len 期望出队长度
 * @return 实际出队长度
 */
size_t pu_ringbuffer_dequeue_array(pu_ringbuffer_p rb, uint8_t *data, size_t len);

/**
 * @brief 丢弃单字节数据
 * @param rb 环形缓冲区指针
 * @return 1:成功,0:缓冲区为空
 */
uint8_t pu_ringbuffer_drop_byte(pu_ringbuffer_p rb);

/**
 * @brief 丢弃多字节数据
 * @param rb 环形缓冲区指针
 * @param size 期望丢弃长度
 * @return 实际丢弃长度
 */
uint8_t pu_ringbuffer_drop_array(pu_ringbuffer_p rb, size_t size);

/**
 * @brief  peek 单字节(不删除数据)
 * @param rb 环形缓冲区指针
 * @param data  peek 数据存储地址
 * @param index 偏移索引(从读索引开始)
 * @return 1:成功,0:索引超出有效数据范围
 */
uint8_t pu_ringbuffer_peek_byte(pu_ringbuffer_p rb, uint8_t *data, size_t index);

/**
 * @brief 检查 peek 数据是否匹配(不删除数据)
 * @param rb 环形缓冲区指针
 * @param data 待匹配数据
 * @param index 偏移索引(从读索引开始)
 * @return true:匹配,false:不匹配或索引无效
 */
bool pu_ringbuffer_check_peek(pu_ringbuffer_p rb, uint8_t data, size_t index);

/**
 * @brief  peek 多字节(不删除数据)
 * @param rb 环形缓冲区指针
 * @param data  peek 数据存储地址
 * @param index 偏移索引(从读索引开始)
 * @param count 期望 peek 长度
 * @return 1:成功,0:长度超出有效数据范围
 */
uint8_t pu_ringbuffer_peek_array(pu_ringbuffer_p rb, uint8_t *data, size_t index, size_t count);

/**
 * @brief 复制环形缓冲区数据(不删除数据)
 * @param rb 环形缓冲区指针
 * @param data 复制数据存储地址
 * @param size 期望复制长度
 * @return 实际复制长度
 */
size_t pu_ringbuffer_copy_array(pu_ringbuffer_p rb, uint8_t *data, size_t size);

/**
 * @brief 搜索帧头(从读索引开始,找到后丢弃之前的无效数据)
 * @param rb 环形缓冲区指针
 * @param head 帧头字节
 */
size_t pu_ringbuffer_search_head(pu_ringbuffer_p rb, uint8_t head);

/**
 * @brief 搜索下一个帧头(先丢弃当前读索引数据,再搜索)
 * @param rb 环形缓冲区指针
 * @param head 帧头字节
 */
void pu_ringbuffer_search_next_head(pu_ringbuffer_p rb, uint8_t head);

/**
 * @brief 检查环形缓冲区是否为空
 * @param rb 环形缓冲区指针
 * @return true:空,false:非空
 */
bool pu_ringbuffer_is_empty(pu_ringbuffer_p rb);

/**
 * @brief 检查环形缓冲区是否满
 * @param rb 环形缓冲区指针
 * @return true:满,false:未满
 */
bool pu_ringbuffer_is_full(pu_ringbuffer_p rb);

/**
 * @brief 获取环形缓冲区有效数据长度
 * @param rb 环形缓冲区指针
 * @return 有效数据长度
 */
size_t pu_ringbuffer_get_valid_len(pu_ringbuffer_p rb);

/**
 * @brief 清空环形缓冲区(重置索引,不释放内存)
 * @param rb 环形缓冲区指针
 */
void pu_ringbuffer_clear(pu_ringbuffer_p rb);

// 内部掩码宏(外部无需调用)
#define RINGBUFFER_MASK(rb) (rb->buffer_mask)

size_t pu_ringbuffer_get_contiguous_data(pu_ringbuffer_p rb, uint8_t **ptr);

#endif // PU_RINGBUFFER_H
