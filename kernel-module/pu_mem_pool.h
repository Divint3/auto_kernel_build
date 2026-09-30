#ifndef PU_MEM_POOL_H_
#define PU_MEM_POOL_H_

#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）

#ifdef __cplusplus
extern "C" {
#endif

// 内存块指针
#define PU_DEFINE_MEMPOOL(size)           \
  typedef struct pu_mem_pool_##size##_t { \
    uint32_t prefix;                      \
    uint8_t data[size];                   \
  } pu_mem_pool_##size##_t;

/**
 * @brief 内存池控制块结构
 */
typedef struct pu_mem_pool_s {
  void *free_list;      /**< 空闲块链表 */
  void *mem_start;      /**< 内存池起始地址 */
  uint32_t block_size;  /**< 每个内存块的大小（字节） */
  uint32_t block_count; /**< 内存块总数 */
  uint32_t free_count;  /**< 当前空闲块数量 */
  uint32_t max_used;    /**< 历史最大使用块数（用于统计） */
} pu_mem_pool_t, *pu_mem_pool_p;

/**
 * @brief 错误码定义
 */
typedef enum {
  PU_MEM_POOL_ERR_NONE = 0,    /**< 操作成功 */
  PU_MEM_POOL_ERR_NULL = -1,   /**< 空指针错误 */
  PU_MEM_POOL_ERR_SIZE = -2,   /**< 大小错误 */
  PU_MEM_POOL_ERR_EMPTY = -3,  /**< 内存池为空 */
  PU_MEM_POOL_ERR_FULL = -4,   /**< 内存池已满 */
  PU_MEM_POOL_ERR_ALIGN = -5,  /**< 内存对齐错误 */
  PU_MEM_POOL_ERR_INVALID = -6 /**< 无效参数或操作 */
} pu_mem_p_ool_err_e;

/**
 * @brief 创建内存池
 * @param pool       内存池对象指针
 * @param mem_start  内存块指针
 * @param block_size 每个内存块的大小（字节）
 * @param block_count 内存块数量
 * @return 错误码
 *
 * @note 内存对齐要求：
 *       1. mem_start 需要对齐到 sizeof(void*) 的倍数
 *       2. block_size 需要 >= sizeof(void*)
 */
int pu_mem_pool_create(pu_mem_pool_t *pool, void *mem_start, uint32_t block_size, uint32_t block_count);

/**
 * @brief 从内存池获取一个内存块
 * @param pool 内存池对象指针
 * @param[out] mem_ptr 获取的内存块指针
 * @return 错误码
 */
int pu_mem_pool_get(pu_mem_pool_t *pool, void **mem_ptr);

/**
 * @brief 释放内存块回内存池
 * @param pool 内存池对象指针
 * @param mem_ptr 要释放的内存块指针
 * @return 错误码
 */
int pu_mem_pool_put(pu_mem_pool_t *pool, void *mem_ptr);

#ifdef __cplusplus
}
#endif

#endif // PU_MEM_POOL_H_
