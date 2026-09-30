/**
 * @file pu_bitmap.h
 * @brief 面向对象的 Bitmap 管理库
 * @author Power Utility Team
 * @date 2024
 * @version 1.0
 */

#ifndef PU_BITMAP_H_
#define PU_BITMAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）

/**
 * @defgroup bitmap_group Bitmap 管理模块
 * @{
 */

/**
 * @brief Bitmap 对象结构体
 *
 * 用于管理位图数据,支持高效的位操作和状态管理
 */
typedef struct {
  uint8_t *bitmap_data;   /**< bitmap 数据指针 */
  size_t bitmap_size;     /**< bitmap 字节大小 */
  size_t max_bits;        /**< 最大支持的位数 */
  size_t set_bit_count;   /**< 已置位位数统计 */
  size_t unset_bit_count; /**< 未置位位数统计 */
} pu_bitmap_t, *pu_bitmap_p;

/**
 * @brief 回调函数类型定义
 * @param bit_offset 位偏移量
 * @param user_data 用户自定义数据指针
 */
typedef void (*pu_bitmap_callback_t)(size_t bit_offset, void *user_data);

/**
 * @brief 创建新的 Bitmap 对象
 *
 * @param[in] max_bits 最大支持的位数
 * @return pu_bitmap_t* 成功返回 Bitmap 对象指针,失败返回 NULL
 *
 * @note 使用完成后必须调用 pu_bitmap_destroy() 释放资源
 */
pu_bitmap_p pu_bitmap_create(size_t max_bits);

void pu_bitmap_load(pu_bitmap_p bitmap, size_t max_bits, uint8_t *data);

/**
 * @brief 销毁 Bitmap 对象并释放资源
 *
 * @param[in] bitmap Bitmap 对象指针
 */
void pu_bitmap_destroy(pu_bitmap_p bitmap);

/**
 * @brief 设置指定位(从左到右计算偏移)
 *
 * @param[in] bitmap Bitmap 对象指针
 * @param[in] bit_offset 位偏移量(0 到 max_bits-1)
 * @return true 设置成功
 * @return false 设置失败(参数错误或位已设置)
 */
bool pu_bitmap_set_bit(pu_bitmap_p bitmap, size_t bit_offset);

/**
 * @brief 清除指定位
 *
 * @param[in] bitmap Bitmap 对象指针
 * @param[in] bit_offset 位偏移量(0 到 max_bits-1)
 * @return true 清除成功
 * @return false 清除失败(参数错误或位已清除)
 */
bool pu_bitmap_clear_bit(pu_bitmap_p bitmap, size_t bit_offset);

/**
 * @brief 检查指定位是否设置
 *
 * @param[in] bitmap Bitmap 对象指针
 * @param[in] bit_offset 位偏移量(0 到 max_bits-1)
 * @return true 位已设置
 * @return false 位未设置或参数错误
 */
bool pu_bitmap_test_bit(const pu_bitmap_p bitmap, size_t bit_offset);

/**
 * @brief 查找第一个空闲位(从左到右)
 *
 * @param[in] bitmap Bitmap 对象指针
 * @return int32_t 成功返回空闲位偏移量,失败返回 -1
 */
int32_t pu_bitmap_find_unset_bit(const pu_bitmap_p bitmap);
int32_t pu_bitmap_find_set_bit(const pu_bitmap_p bitmap);
/**
 * @brief 查找连续的空闲位
 *
 * @param[in] bitmap Bitmap 对象指针
 * @param[in] count 需要连续的空闲位数量
 * @return int32_t 成功返回起始位偏移量,失败返回 -1
 */
int32_t pu_bitmap_find_unset_bits(const pu_bitmap_p bitmap, size_t count);

/**
 * @brief 统计已使用的位数
 *
 * @param[in] bitmap Bitmap 对象指针
 * @return size_t 已使用的位数
 */
void pu_bitmap_calc_bit_count(const pu_bitmap_p bitmap);
size_t pu_bitmap_count_set(const pu_bitmap_p bitmap);

/**
 * @brief 统计空闲的位数
 *
 * @param[in] bitmap Bitmap 对象指针
 * @return size_t 空闲的位数
 */
size_t pu_bitmap_count_unset(const pu_bitmap_p bitmap);

/**
 * @brief 获取最大支持的位数
 *
 * @param[in] bitmap Bitmap 对象指针
 * @return size_t 最大位数
 */
size_t pu_bitmap_get_max_bits(const pu_bitmap_p bitmap);

/**
 * @brief 清空所有位
 *
 * @param[in] bitmap Bitmap 对象指针
 */
void pu_bitmap_clear_all(pu_bitmap_p bitmap);

/**
 * @brief 设置所有位
 *
 * @param[in] bitmap Bitmap 对象指针
 */
void pu_bitmap_set_all(pu_bitmap_p bitmap);

/**
 * @brief 复制 Bitmap 内容
 *
 * @param[in] src 源 Bitmap 对象指针
 * @param[out] dest 目标 Bitmap 对象指针
 * @return true 复制成功
 * @return false 复制失败(参数错误或大小不匹配)
 */
bool pu_bitmap_copy(const pu_bitmap_p src, pu_bitmap_p dest);

/**
 * @brief Bitmap 与操作(dest = src1 AND src2)
 *
 * @param[out] dest 目标 Bitmap 对象指针
 * @param[in] src1 第一个源 Bitmap 对象指针
 * @param[in] src2 第二个源 Bitmap 对象指针
 * @return true 操作成功
 * @return false 操作失败(参数错误或大小不匹配)
 */
bool pu_bitmap_and(pu_bitmap_p dest, const pu_bitmap_p src1, const pu_bitmap_p src2);

/**
 * @brief Bitmap 或操作(dest = src1 OR src2)
 *
 * @param[out] dest 目标 Bitmap 对象指针
 * @param[in] src1 第一个源 Bitmap 对象指针
 * @param[in] src2 第二个源 Bitmap 对象指针
 * @return true 操作成功
 * @return false 操作失败(参数错误或大小不匹配)
 */
bool pu_bitmap_or(pu_bitmap_p dest, const pu_bitmap_p src1, const pu_bitmap_p src2);

/**
 * @brief 遍历所有设置的位
 *
 * @param[in] bitmap Bitmap 对象指针
 * @param[in] callback 回调函数
 * @param[in] user_data 用户自定义数据指针
 */
void pu_bitmap_foreach_set(const pu_bitmap_p bitmap, pu_bitmap_callback_t callback, void *user_data);

/**
 * @brief 遍历所有空闲的位
 *
 * @param[in] bitmap Bitmap 对象指针
 * @param[in] callback 回调函数
 * @param[in] user_data 用户自定义数据指针
 */
void pu_bitmap_foreach_free(const pu_bitmap_p bitmap, pu_bitmap_callback_t callback, void *user_data);

/** @} */ // end of bitmap_group

#ifdef __cplusplus
}
#endif

#endif /* PU_BITMAP_H_ */
