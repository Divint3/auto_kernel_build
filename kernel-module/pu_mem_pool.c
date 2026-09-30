#include "pu_mem_pool.h"
#include "pu_compiler.h"
#include "pu_macro.h"
#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）

// 内存对齐检查宏
#define PU_IS_ALIGNED(addr, align) (((uintptr_t)(addr) & ((align) - 1)) == 0)

int pu_mem_pool_create(pu_mem_pool_t *pool, void *mem_start, uint32_t block_size, uint32_t block_count) {
  void **p_link;
  uint8_t *p_blk;
  size_t loops = block_count - 1;

  p_link = (void **)mem_start;
  p_blk = (uint8_t *)mem_start;

  // 参数检查
  if (pool == NULL || mem_start == NULL) {
    return PU_MEM_POOL_ERR_NULL;
  }

  if (block_size < sizeof(void *) || block_count == 0) {
    return PU_MEM_POOL_ERR_SIZE;
  }
  // 检查对齐
  if (!PU_IS_ALIGNED(mem_start, sizeof(void *))) {
    return PU_MEM_POOL_ERR_ALIGN;
  }

  // 初始化内存池控制块
  memset(pool, 0, sizeof(pu_mem_pool_t));

  for (size_t i = 0u; i < loops; i++) {
    p_blk += block_size;
    *p_link = (void *)p_blk;         /* Save pointer to NEXT block in CURRENT block            */
    p_link = (void **)(void *)p_blk; /* Position     to NEXT block                             */
  }
  *p_link = (void *)0;

  pool->mem_start = mem_start;
  pool->free_list = mem_start;
  pool->block_size = block_size;
  pool->block_count = block_count;
  pool->free_count = block_count;
  pool->max_used = 0;

  return PU_MEM_POOL_ERR_NONE;
}

int pu_mem_pool_get(pu_mem_pool_t *pool, void **mem_ptr) {
  void *block;

  if (pool == NULL || mem_ptr == NULL) {
    return PU_MEM_POOL_ERR_NULL;
  }

  // 检查是否有空闲块
  if (pool->free_count == 0) {
    *mem_ptr = NULL;
    return PU_MEM_POOL_ERR_EMPTY;
  }

  // 从链表头部取一个块
  block = pool->free_list;           /* Yes, point to next free memory block                   */
  pool->free_list = *(void **)block; /*      Adjust pointer to new free list                   */
  pool->free_count--;

  uint32_t used_count = pool->block_count - pool->free_count;
  if (used_count > pool->max_used) {
    pool->max_used = used_count;
  }

  *mem_ptr = block;
  return PU_MEM_POOL_ERR_NONE;
}

int pu_mem_pool_put(pu_mem_pool_t *pool, void *mem_ptr) {
  if (pool == NULL || mem_ptr == NULL) {
    return PU_MEM_POOL_ERR_NULL;
  }

  *(void **)mem_ptr = pool->free_list;
  pool->free_list = mem_ptr;
  pool->free_count++;

  return PU_MEM_POOL_ERR_NONE;
}
