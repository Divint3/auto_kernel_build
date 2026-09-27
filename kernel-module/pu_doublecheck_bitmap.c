#include "pu_doublecheck_bitmap.h"
#include "pu_macro.h"
#include <stdint.h>

// 计算置位位数(内部函数)
static void pu_doublecheck_bitmap_calc_bit_count(pu_doublecheck_bitmap_p bitmap) {
  if (!bitmap)
    return;

  bitmap->set_bit_count = 0;
  for (size_t i = 0; i < bitmap->max_bits; i++) {
    if (pu_doublecheck_bitmap_test_bit(bitmap, i)) {
      if (pu_doublecheck_bitmap_test_bit_consistency(bitmap, i)) {
        bitmap->set_bit_count++;
      } else {
        bitmap->bad_bit_count++;
      }
    } else {
      bitmap->unset_bit_count++;
    }
  }
}

// 创建双副本Bitmap对象
pu_doublecheck_bitmap_p pu_doublecheck_bitmap_create(size_t max_bits) {
  if (max_bits == 0) {
    return NULL;
  }

  size_t byte_size = (max_bits + 7) / 8;

  pu_malloc_instance(bitmap, pu_doublecheck_bitmap);
  if (bitmap == NULL) {
    return NULL;
  }

  bitmap->primary_buffer = pu_malloc(byte_size);
  bitmap->secondary_buffer = pu_malloc(byte_size);
  if (!bitmap->primary_buffer || !bitmap->secondary_buffer) {
    if (bitmap->primary_buffer)
      pu_free(bitmap->primary_buffer);
    if (bitmap->secondary_buffer)
      pu_free(bitmap->secondary_buffer);
    pu_free(bitmap);
    return NULL;
  }

  memset(bitmap->primary_buffer, 0, byte_size);
  memset(bitmap->secondary_buffer, 0, byte_size);

  bitmap->buffer_size_bytes = byte_size;
  bitmap->max_bits = max_bits;
  bitmap->set_bit_count = 0;
  bitmap->unset_bit_count = 0;
  bitmap->bad_bit_count = 0;

  return bitmap;
}

// 从数据加载双副本Bitmap
void pu_doublecheck_bitmap_load(pu_doublecheck_bitmap_p bitmap, size_t max_bits, uint8_t *primary_data, uint8_t *backup_data) {
  if (!bitmap || !primary_data || !backup_data) {
    return;
  }

  size_t byte_size = (max_bits + 7) / 8;

  bitmap->primary_buffer = primary_data;
  bitmap->secondary_buffer = backup_data;
  bitmap->buffer_size_bytes = byte_size;
  bitmap->max_bits = max_bits;
  bitmap->set_bit_count = 0;
  bitmap->unset_bit_count = 0;
  bitmap->bad_bit_count = 0;

  pu_doublecheck_bitmap_calc_bit_count(bitmap);
}

// 销毁Bitmap对象
void pu_doublecheck_bitmap_destroy(pu_doublecheck_bitmap_p bitmap) {
  if (bitmap) {
    if (bitmap->primary_buffer) {
      pu_free(bitmap->primary_buffer);
    }
    if (bitmap->secondary_buffer) {
      pu_free(bitmap->secondary_buffer);
    }
    pu_free(bitmap);
  }
}

// 检查两个缓冲区的数据一致性
bool pu_doublecheck_bitmap_check_consistency(const pu_doublecheck_bitmap_p bitmap) {
  if (!bitmap) {
    return false;
  }
  return memcmp(bitmap->primary_buffer, bitmap->secondary_buffer, bitmap->buffer_size_bytes) == 0;
}

// 设置指定位(同时设置两个缓冲区)
bool pu_doublecheck_bitmap_set_bit(pu_doublecheck_bitmap_p bitmap, size_t bit_offset) {
  if (!bitmap || bit_offset >= bitmap->max_bits) {
    return false;
  }

  size_t byte_index = bit_offset / 8;
  size_t bit_index = 7 - (bit_offset % 8);

  // 检查是否已经设置
  if ((bitmap->primary_buffer[byte_index] & (1 << bit_index)) == 0) {
    bitmap->primary_buffer[byte_index] |= (1 << bit_index);
    bitmap->secondary_buffer[byte_index] |= (1 << bit_index);
    bitmap->set_bit_count++;
    return true;
  }
  return false;
}

// 清除指定位(同时清除两个缓冲区)
bool pu_doublecheck_bitmap_clear_bit(pu_doublecheck_bitmap_p bitmap, size_t bit_offset) {
  if (!bitmap || bit_offset >= bitmap->max_bits) {
    return false;
  }

  size_t byte_index = bit_offset / 8;
  size_t bit_index = 7 - (bit_offset % 8);

  // 检查是否已经设置
  if ((bitmap->primary_buffer[byte_index] & (1 << bit_index)) != 0) {
    bitmap->primary_buffer[byte_index] &= ~(1 << bit_index);
    bitmap->secondary_buffer[byte_index] &= ~(1 << bit_index);
    bitmap->set_bit_count--;
    bitmap->unset_bit_count++;
    return true;
  }
  return false;
}

// 检查指定位是否设置(检查主缓冲区)
bool pu_doublecheck_bitmap_test_bit(const pu_doublecheck_bitmap_p bitmap, size_t bit_offset) {
  if (!bitmap || bit_offset >= bitmap->max_bits) {
    return false;
  }

  size_t byte_index = bit_offset / 8;
  size_t bit_index = 7 - (bit_offset % 8);

  return (bitmap->primary_buffer[byte_index] & (1 << bit_index)) != 0;
}

// 检查指定位在两个缓冲区中是否一致
bool pu_doublecheck_bitmap_test_bit_consistency(const pu_doublecheck_bitmap_p bitmap, size_t bit_offset) {
  if (!bitmap || bit_offset >= bitmap->max_bits) {
    return false;
  }

  size_t byte_index = bit_offset / 8;
  size_t bit_index = 7 - (bit_offset % 8);

  uint8_t primary_bit = (bitmap->primary_buffer[byte_index] >> bit_index) & 0x01;
  uint8_t backup_bit = (bitmap->secondary_buffer[byte_index] >> bit_index) & 0x01;

  return primary_bit == backup_bit;
}

// 查找第一个空闲位
int32_t pu_doublecheck_bitmap_find_unset_bit(const pu_doublecheck_bitmap_p bitmap) {
  if (!bitmap) {
    return -1;
  }

  // 首先按四字节块处理
  size_t max_bits = bitmap->max_bits;
  size_t word_count = (max_bits + 31) / 32; // 计算完整的32位块数量

  for (size_t word_idx = 0; word_idx < word_count; word_idx++) {
    size_t byte_index = word_idx * 4; // 每个字4字节

    // 检查当前32位块是否全为1(没有空闲位)
    uint32_t word_value = *(uint32_t *)(&bitmap->primary_buffer[byte_index]);

    // 如果整个字都是1,则跳过
    if (word_value == 0xFFFFFFFF) {
      continue;
    }

    // 在当前字中查找第一个未设置的位
    size_t start_bit = word_idx * 32;
    size_t end_bit = (start_bit + 32) < max_bits ? (start_bit + 32) : max_bits;

    for (size_t i = start_bit; i < end_bit; i++) {
      size_t byte_idx = i / 8;
      size_t bit_idx = 7 - (i % 8);

      if ((bitmap->primary_buffer[byte_idx] & (1 << bit_idx)) == 0) {
        return (int32_t)i;
      }
    }
  }

  return -1;
}

// 查找第一个设置位
int32_t pu_doublecheck_bitmap_find_set_bit(const pu_doublecheck_bitmap_p bitmap) {
  if (!bitmap) {
    return -1;
  }

  // 首先按四字节块处理
  size_t max_bits = bitmap->max_bits;
  size_t word_count = (max_bits + 31) / 32; // 计算完整的32位块数量

  for (size_t word_idx = 0; word_idx < word_count; word_idx++) {
    size_t byte_index = word_idx * 4; // 每个字4字节

    // 检查当前32位块是否全为0(没有设置位)
    uint32_t word_value = *(uint32_t *)(&bitmap->primary_buffer[byte_index]);

    // 如果整个字都是0,则跳过
    if (word_value == 0x00000000) {
      continue;
    }

    // 在当前字中查找第一个设置的位
    size_t start_bit = word_idx * 32;
    size_t end_bit = (start_bit + 32) < max_bits ? (start_bit + 32) : max_bits;

    for (size_t i = start_bit; i < end_bit; i++) {
      size_t byte_idx = i / 8;
      size_t bit_idx = 7 - (i % 8);

      if ((bitmap->primary_buffer[byte_idx] & (1 << bit_idx)) != 0) {
        return (int32_t)i;
      }
    }
  }

  return -1;
}

// 统计已使用的位数
size_t pu_doublecheck_bitmap_count_set(const pu_doublecheck_bitmap_p bitmap) {
  return bitmap ? bitmap->set_bit_count : 0;
}

// 统计空闲的位数
size_t pu_doublecheck_bitmap_count_unset(const pu_doublecheck_bitmap_p bitmap) {
  return bitmap ? (bitmap->max_bits - bitmap->set_bit_count) : 0;
}

// 获取最大支持的位数
size_t pu_doublecheck_bitmap_get_max_bits(const pu_doublecheck_bitmap_p bitmap) {
  return bitmap ? bitmap->max_bits : 0;
}

// 清空所有位(同时清空两个缓冲区)
void pu_doublecheck_bitmap_clear_all(pu_doublecheck_bitmap_p bitmap) {
  if (bitmap) {
    memset(bitmap->primary_buffer, 0, bitmap->buffer_size_bytes);
    memset(bitmap->secondary_buffer, 0, bitmap->buffer_size_bytes);
    bitmap->set_bit_count = 0;
  }
}

// 设置所有位(同时设置两个缓冲区)
void pu_doublecheck_bitmap_set_all(pu_doublecheck_bitmap_p bitmap) {
  if (bitmap) {
    memset(bitmap->primary_buffer, 0xFF, bitmap->buffer_size_bytes);
    memset(bitmap->secondary_buffer, 0xFF, bitmap->buffer_size_bytes);
    bitmap->set_bit_count = bitmap->max_bits;
  }
}
