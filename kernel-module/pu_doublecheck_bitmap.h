/**
 * @file pu_doublecheck_bitmap.h
 * @brief 双副本校验的 Bitmap 管理库
 * @author Power Utility Team
 * @date 2024
 * @version 1.0
 */

#ifndef PU_DOUBLECHECK_BITMAP_H_
#define PU_DOUBLECHECK_BITMAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @defgroup doublecheck_bitmap_group 双副本Bitmap管理模块
 * @{
 */

/**
 * @brief 双副本Bitmap对象结构体
 */
typedef struct {
  uint8_t *primary_buffer;   /**< 主数据缓冲区 */
  uint8_t *secondary_buffer; /**< 备份数据缓冲区 */
  size_t buffer_size_bytes;  /**< 每个缓冲区的字节大小 */
  size_t max_bits;           /**< 最大支持的位数 */
  size_t bad_bit_count;      /**损坏数据块数量 */
  size_t set_bit_count;      /**< 已置位位数统计 */
  size_t unset_bit_count;    /**< 未置位位数统计 */
} pu_doublecheck_bitmap_t, *pu_doublecheck_bitmap_p;

/**
 * @brief 创建新的双副本Bitmap对象
 *
 * @param[in] max_bits 最大支持的位数
 * @return pu_doublecheck_bitmap_p 成功返回Bitmap对象指针,失败返回NULL
 */
pu_doublecheck_bitmap_p pu_doublecheck_bitmap_create(size_t max_bits);

/**
 * @brief 从数据加载双副本Bitmap
 *
 * @param[in] bitmap Bitmap对象指针
 * @param[in] max_bits 最大支持的位数
 * @param[in] primary_data 主数据指针
 * @param[in] backup_data 备份数据指针
 */
void pu_doublecheck_bitmap_load(pu_doublecheck_bitmap_p bitmap, size_t max_bits, uint8_t *primary_data, uint8_t *backup_data);

/**
 * @brief 销毁Bitmap对象并释放资源
 *
 * @param[in] bitmap Bitmap对象指针
 */
void pu_doublecheck_bitmap_destroy(pu_doublecheck_bitmap_p bitmap);

/**
 * @brief 检查两个缓冲区的数据一致性
 *
 * @param[in] bitmap Bitmap对象指针
 * @return bool 一致返回true,不一致返回false
 */
bool pu_doublecheck_bitmap_check_consistency(const pu_doublecheck_bitmap_p bitmap);

/**
 * @brief 设置指定位(同时设置两个缓冲区)
 *
 * @param[in] bitmap Bitmap对象指针
 * @param[in] bit_offset 位偏移量
 * @return bool 设置成功返回true,失败返回false
 */
bool pu_doublecheck_bitmap_set_bit(pu_doublecheck_bitmap_p bitmap, size_t bit_offset);

/**
 * @brief 清除指定位(同时清除两个缓冲区)
 *
 * @param[in] bitmap Bitmap对象指针
 * @param[in] bit_offset 位偏移量
 * @return bool 清除成功返回true,失败返回false
 */
bool pu_doublecheck_bitmap_clear_bit(pu_doublecheck_bitmap_p bitmap, size_t bit_offset);

/**
 * @brief 检查指定位是否设置(检查主缓冲区)
 *
 * @param[in] bitmap Bitmap对象指针
 * @param[in] bit_offset 位偏移量
 * @return bool 已设置返回true,未设置或参数错误返回false
 */
bool pu_doublecheck_bitmap_test_bit(const pu_doublecheck_bitmap_p bitmap, size_t bit_offset);

/**
 * @brief 检查指定位在两个缓冲区中是否一致
 *
 * @param[in] bitmap Bitmap对象指针
 * @param[in] bit_offset 位偏移量
 * @return bool 一致返回true,不一致或参数错误返回false
 */
bool pu_doublecheck_bitmap_test_bit_consistency(const pu_doublecheck_bitmap_p bitmap, size_t bit_offset);

/**
 * @brief 查找第一个空闲位
 *
 * @param[in] bitmap Bitmap对象指针
 * @return int32_t 成功返回空闲位偏移量,失败返回-1
 */
int32_t pu_doublecheck_bitmap_find_unset_bit(const pu_doublecheck_bitmap_p bitmap);

/**
 * @brief 查找第一个设置位
 *
 * @param[in] bitmap Bitmap对象指针
 * @return int32_t 成功返回设置位偏移量,失败返回-1
 */
int32_t pu_doublecheck_bitmap_find_set_bit(const pu_doublecheck_bitmap_p bitmap);

/**
 * @brief 统计已使用的位数
 *
 * @param[in] bitmap Bitmap对象指针
 * @return size_t 已使用的位数
 */
size_t pu_doublecheck_bitmap_count_set(const pu_doublecheck_bitmap_p bitmap);

/**
 * @brief 统计空闲的位数
 *
 * @param[in] bitmap Bitmap对象指针
 * @return size_t 空闲的位数
 */
size_t pu_doublecheck_bitmap_count_unset(const pu_doublecheck_bitmap_p bitmap);

/**
 * @brief 获取最大支持的位数
 *
 * @param[in] bitmap Bitmap对象指针
 * @return size_t 最大位数
 */
size_t pu_doublecheck_bitmap_get_max_bits(const pu_doublecheck_bitmap_p bitmap);

/**
 * @brief 清空所有位(同时清空两个缓冲区)
 *
 * @param[in] bitmap Bitmap对象指针
 */
void pu_doublecheck_bitmap_clear_all(pu_doublecheck_bitmap_p bitmap);

/**
 * @brief 设置所有位(同时设置两个缓冲区)
 *
 * @param[in] bitmap Bitmap对象指针
 */
void pu_doublecheck_bitmap_set_all(pu_doublecheck_bitmap_p bitmap);

/** @} */ // end of doublecheck_bitmap_group

#ifdef __cplusplus
}
#endif

#endif /* PU_DOUBLECHECK_BITMAP_H_ */
