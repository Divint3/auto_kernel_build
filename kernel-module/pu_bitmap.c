#include "pu_bitmap.h"
#include "pu_macro.h"
#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）

// 初始化 Bitmap 对象
pu_bitmap_p pu_bitmap_create(size_t max_bits) {
  size_t byte_size = (max_bits + 7) / 8; // 计算需要的字节数

  pu_malloc_instance(bitmap, pu_bitmap);
  if (bitmap == NULL) {
    return NULL;
  }

  bitmap->bitmap_data = pu_malloc(byte_size);
  if (!bitmap->bitmap_data) {
    pu_free(bitmap);
    return NULL;
  }

  bitmap->bitmap_size = byte_size;
  bitmap->max_bits = max_bits;
  bitmap->set_bit_count = 0;

  return bitmap;
}
/**
 * @brief 计算载入bitmap中的置位和非置位标志数量 内部函数
 *
 * @param bitmap
 */
void pu_bitmap_calc_bit_count(const pu_bitmap_p bitmap) {
  bitmap->set_bit_count = 0;
  bitmap->unset_bit_count = 0;
  for (size_t i = 0; i < bitmap->max_bits; i++) {
    if (pu_bitmap_test_bit(bitmap, i)) {
      bitmap->set_bit_count++;
    } else {
      bitmap->unset_bit_count++;
    }
  }
}

void pu_bitmap_load(pu_bitmap_p bitmap, size_t max_bits, uint8_t *data) {
  size_t byte_size = (max_bits + 7) / 8; // 计算需要的字节数

  bitmap->bitmap_data = data;
  bitmap->bitmap_size = byte_size;
  bitmap->max_bits = max_bits;

  pu_bitmap_calc_bit_count(bitmap);
}

// 销毁 Bitmap 对象
void pu_bitmap_destroy(pu_bitmap_p bitmap) {
  if (bitmap) {
    if (bitmap->bitmap_data) {
      pu_free(bitmap->bitmap_data);
    }
    pu_free(bitmap);
  }
}

// 设置指定位(从左到右计算偏移)
bool pu_bitmap_set_bit(pu_bitmap_p bitmap, size_t bit_offset) {
  if (!bitmap || bit_offset >= bitmap->max_bits)
    return false;

  size_t byte_index = bit_offset / 8;
  size_t bit_index = 7 - (bit_offset % 8); // 从左到右计算

  // 检查是否已经设置
  if ((bitmap->bitmap_data[byte_index] & (1 << bit_index)) == 0) {
    bitmap->bitmap_data[byte_index] |= (1 << bit_index);
    bitmap->set_bit_count++;
    return true;
  }
  return false;
}

// 清除指定位
bool pu_bitmap_clear_bit(pu_bitmap_p bitmap, size_t bit_offset) {
  if (!bitmap || bit_offset >= bitmap->max_bits)
    return false;

  size_t byte_index = bit_offset / 8;
  size_t bit_index = 7 - (bit_offset % 8);

  // 检查是否已经设置
  if ((bitmap->bitmap_data[byte_index] & (1 << bit_index)) != 0) {
    bitmap->bitmap_data[byte_index] &= ~(1 << bit_index);
    bitmap->set_bit_count--;
    return true;
  }
  return false;
}

// 检查指定位是否设置
bool pu_bitmap_test_bit(const pu_bitmap_p bitmap, size_t bit_offset) {
  if (!bitmap || bit_offset >= bitmap->max_bits)
    return false;

  size_t byte_index = bit_offset / 8;
  size_t bit_index = 7 - (bit_offset % 8);

  return (bitmap->bitmap_data[byte_index] & (1 << bit_index)) != 0;
}

// 查找第一个空闲位(从左到右)
int32_t pu_bitmap_find_unset_bit(const pu_bitmap_p bitmap) {
  if (!bitmap)
    return -1;

  for (size_t i = 0; i < bitmap->max_bits; i++) {
    size_t byte_index = i / 8;
    size_t bit_index = 7 - (i % 8);

    if ((bitmap->bitmap_data[byte_index] & (1 << bit_index)) == 0) {
      return (int32_t)i;
    }
  }
  return -1; // 没有空闲位
}

// 查找第一个设置位(从左到右)
int32_t pu_bitmap_find_set_bit(const pu_bitmap_p bitmap) {
  if (!bitmap)
    return -1;

  for (size_t i = 0; i < bitmap->max_bits; i++) {
    size_t byte_index = i / 8;
    size_t bit_index = 7 - (i % 8);

    if ((bitmap->bitmap_data[byte_index] & (1 << bit_index)) != 0) {
      return (int32_t)i;
    }
  }
  return -1; // 没有空闲位
}

// 查找连续的空闲位
int32_t pu_bitmap_find_unset_bits(const pu_bitmap_p bitmap, size_t count) {
  if (!bitmap || count == 0 || count > bitmap->max_bits)
    return -1;

  size_t consecutive = 0;
  for (size_t i = 0; i < bitmap->max_bits; i++) {
    if (!pu_bitmap_test_bit(bitmap, i)) {
      consecutive++;
      if (consecutive == count) {
        return (int32_t)(i - count + 1); // 返回起始位置
      }
    } else {
      consecutive = 0;
    }
  }
  return -1;
}

// 统计已使用的位数
size_t pu_bitmap_count_set(const pu_bitmap_p bitmap) {
  return bitmap ? bitmap->set_bit_count : 0;
}

// 统计空闲的位数
size_t pu_bitmap_count_unset(const pu_bitmap_p bitmap) {
  return bitmap ? (bitmap->max_bits - bitmap->set_bit_count) : 0;
}

// 获取最大位数
size_t pu_bitmap_get_max_bits(const pu_bitmap_p bitmap) {
  return bitmap ? bitmap->max_bits : 0;
}

// 清空所有位
void pu_bitmap_clear_all(pu_bitmap_p bitmap) {
  if (bitmap) {
    memset(bitmap->bitmap_data, 0, bitmap->bitmap_size);
    bitmap->set_bit_count = 0;
  }
}

// 设置所有位
void pu_bitmap_set_all(pu_bitmap_p bitmap) {
  if (bitmap) {
    memset(bitmap->bitmap_data, 0xFF, bitmap->bitmap_size);
    bitmap->set_bit_count = bitmap->max_bits;
  }
}

// 复制 Bitmap
bool pu_bitmap_copy(const pu_bitmap_p src, pu_bitmap_p dest) {
  if (!src || !dest || src->max_bits != dest->max_bits)
    return false;

  memcpy(dest->bitmap_data, src->bitmap_data, src->bitmap_size);
  dest->set_bit_count = src->set_bit_count;
  return true;
}

// Bitmap 与操作
bool pu_bitmap_and(pu_bitmap_p dest, const pu_bitmap_p src1, const pu_bitmap_p src2) {
  if (!dest || !src1 || !src2 || src1->max_bits != src2->max_bits || dest->max_bits != src1->max_bits) {
    return false;
  }

  for (size_t i = 0; i < dest->bitmap_size; i++) {
    dest->bitmap_data[i] = src1->bitmap_data[i] & src2->bitmap_data[i];
  }

  // 重新计算使用计数
  dest->set_bit_count = 0;
  for (size_t i = 0; i < dest->max_bits; i++) {
    if (pu_bitmap_test_bit(dest, i)) {
      dest->set_bit_count++;
    }
  }

  return true;
}

// Bitmap 或操作
bool pu_bitmap_or(pu_bitmap_p dest, const pu_bitmap_p src1, const pu_bitmap_p src2) {
  if (!dest || !src1 || !src2 || src1->max_bits != src2->max_bits || dest->max_bits != src1->max_bits) {
    return false;
  }

  for (size_t i = 0; i < dest->bitmap_size; i++) {
    dest->bitmap_data[i] = src1->bitmap_data[i] | src2->bitmap_data[i];
  }

  // 重新计算使用计数
  dest->set_bit_count = 0;
  for (size_t i = 0; i < dest->max_bits; i++) {
    if (pu_bitmap_test_bit(dest, i)) {
      dest->set_bit_count++;
    }
  }

  return true;
}

// 遍历所有设置的位
void pu_bitmap_foreach_set(const pu_bitmap_p bitmap, pu_bitmap_callback_t callback, void *user_data) {
  if (!bitmap || !callback)
    return;

  for (size_t i = 0; i < bitmap->max_bits; i++) {
    if (pu_bitmap_test_bit(bitmap, i)) {
      callback(i, user_data);
    }
  }
}

// 遍历所有空闲的位
void pu_bitmap_foreach_free(const pu_bitmap_p bitmap, pu_bitmap_callback_t callback, void *user_data) {
  if (!bitmap || !callback)
    return;

  for (size_t i = 0; i < bitmap->max_bits; i++) {
    if (!pu_bitmap_test_bit(bitmap, i)) {
      callback(i, user_data);
    }
  }
}
