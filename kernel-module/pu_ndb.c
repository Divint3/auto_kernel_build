/**
 * 此工具是依据norflash特性实现的数据库,采用了bitmap的方式来管理数据块的使用情况,可用于存储时序数据
 * 如 冻结数据 事件数据等,具备占用小,不跨扇区,写入速度快的优点
 */

#include "pu_ndb.h"

/**
 * @brief ndb数据存储
 * @param ndb 数据库对象
 * @param data 待存储的数据对象指针
 * @param data_size 数据长度
 * @return
 */
bool pu_ndb_load_data(pu_ndb_p ndb, uint8_t *data, uint32_t data_size) {
  // 提取flash_desc中的变量
  uint32_t sector_size = ndb->flash_desc.sector_size;
  uint32_t sector_count = ndb->flash_desc.sector_count;
  if (sector_count < 3) {
    // 扇区数量不足,无法存储数据和位图
    ndb->valid = false;
    return false;
  }

  uint8_t reload_times = 0;
  ndb->current_serial_num = 0;
  static uint8_t data_buffer[PU_NDB_MAX_DATA_SIZE];
  // 加上 魔数字 2 序列号 4 crc32 4 共十字节
  uint8_t pack_data_size = data_size + sizeof(pu_ndb_data_t) + 4;
  uint32_t base_address = ndb->flash_desc.start_address;
  size_t sector_data_block_count = sector_size / pack_data_size;     // 每个扇区可以存储的数据块数量
  uint32_t bit_count = sector_data_block_count * (sector_count - 1); // 位图需要覆盖的总数据块数量,预留一个扇区存储位图

  uint32_t byte_count = (bit_count + 7) / 8;

  if (byte_count > ndb->flash_desc.sector_size) {
    // 位图过大,无法存储
    ndb->valid = false;
    return false;
  }

  uint8_t *bitmap_memory = pu_malloc(byte_count);
  if (bitmap_memory == NULL) {
    ndb->valid = false;
    return false;
  }
  ndb->flash_callback.read(base_address, bitmap_memory, byte_count);

  // 载入数据
  pu_bitmap_load(&ndb->bitmap, bit_count, bitmap_memory);
  int32_t index = pu_bitmap_find_set_bit(&ndb->bitmap); // 查找最后一个未使用的的数据块索引
  int32_t final_index;

  if (index < 0) {
    final_index = ndb->bitmap.max_bits - 1; // 从最后一个数据块开始往前数
  } else if (index > 0) {
    final_index = index - 1;
  } else {
    // 没有任何有效数据,内存初始化过
    ndb->valid = true;
    return true;
  }
BACKUP_DATA_RELOAD:;                                               // label后面不能直接跟变量声明, 加一个空语句, 别删
  int32_t sector_offset = (final_index / sector_data_block_count); // 扇区偏移
  int32_t data_offset = (final_index % sector_data_block_count);   // 数据偏移
  // 从flash中载入数据
  ndb->flash_callback.read(base_address + sector_size * 1 + (sector_size * sector_offset) + (pack_data_size * data_offset), data_buffer, pack_data_size);

  uint32_t crc32_data = *(uint32_t *)(((pu_ndb_data_p)data_buffer)->user_data + data_size);

  if (((pu_ndb_data_p)data_buffer)->magic_num == PU_NDB_MAGIC_NUMBER) {
    if (ndb->crc32_cb) {
      if (crc32_data == ndb->crc32_cb(data_buffer, pack_data_size - 4)) {
        goto READ_SUCCESS;
      } else {
        goto READ_FAILED;
      }
    } else {
      goto READ_SUCCESS;
    }
  } else {
    goto READ_FAILED;
  }
READ_SUCCESS:
  memcpy(data, ((pu_ndb_data_p)data_buffer)->user_data, data_size);
  ndb->current_serial_num = ((pu_ndb_data_p)data_buffer)->serial_num;
  ndb->valid = true;
  return true;
READ_FAILED:
  if (reload_times < PU_NDB_RELOAD_TIMES) { // 最多向前找三个数据块,超过则认为没有有效数据
    reload_times++;
    final_index -= 1;
    if (final_index < 0) {
      final_index = index - 1;
    }
    goto BACKUP_DATA_RELOAD;
  }
  ndb->valid = true;
  return false;
}

bool pu_ndb_save_data(pu_ndb_p ndb, uint8_t *data, uint32_t data_size) {
  if (ndb->valid == false) {
    return false;
  }
  uint8_t pack_data_size = data_size + sizeof(pu_ndb_data_t) + 4;
  uint8_t reload_times = 0;
  static uint8_t data_buffer[PU_NDB_MAX_DATA_SIZE];

  // 提取flash_desc中的变量
  uint32_t base_address = ndb->flash_desc.start_address;
  uint32_t sector_size = ndb->flash_desc.sector_size;
  size_t sector_data_block_count = sector_size / pack_data_size; // 每个扇区可以存储的数据块数量
  int32_t index = pu_bitmap_find_set_bit(&ndb->bitmap);          // 查找最后一个未使用的的数据块索引

  if (index < 0) {
    // 数据块已满,擦除前两个扇区(第0扇区存储位图,第1扇区存储数据)
    ndb->flash_callback.erase(base_address + sector_size * 0);
    ndb->flash_callback.read(base_address + sector_size * 0, ndb->bitmap.bitmap_data, ndb->bitmap.bitmap_size);

    pu_bitmap_load(&ndb->bitmap, ndb->bitmap.max_bits, ndb->bitmap.bitmap_data);
    index = pu_bitmap_find_set_bit(&ndb->bitmap);
  }

  int32_t sector_offset = index / sector_data_block_count; // 扇区偏移
  int32_t data_offset = index % sector_data_block_count;   // 数据偏移
  int32_t byte_offset = index / 8;
  int32_t bit_offset = index % 8;
  uint8_t data_byte = 0xFF >> (bit_offset + 1);
  uint8_t readback_data_byte = 0;

TEST_DATA_RELOAD:
  ndb->flash_callback.read(base_address + sector_size * (1 + sector_offset) + (pack_data_size * data_offset), data_buffer, pack_data_size);
  for (size_t i = 0; i < pack_data_size; i++) {
    if (data_buffer[i] != 0xFF) {
      if (data_offset != 0) {
        // 放发现不洁净的扇区时,如果数据偏移在扇区之后仍有脏数据则说明erase失败,同时代表flash损坏
        return false;
      }
      // 数据不一致,说明当前块是脏块,因为在载入过程中会筛选由于写入坏块导致的数据不一致,所以这里直接擦除当前块,并重新载入数据
      ndb->flash_callback.erase(base_address + sector_size * (1 + sector_offset));
      if (reload_times < PU_NDB_RELOAD_TIMES) {
        reload_times++;
        goto TEST_DATA_RELOAD;
      } else {
        return false;
      }
    }
  }
  // 组成标准格式
  ndb->current_serial_num++;
  pu_ndb_data_p data_writer = (pu_ndb_data_p)data_buffer;
  data_writer->magic_num = PU_NDB_MAGIC_NUMBER;
  data_writer->serial_num = ndb->current_serial_num;
  uint32_t writein_crc32 = 0xFFFFFFFF;

  memcpy(data_writer->user_data, data, data_size);
  if (ndb->crc32_cb != NULL) {
    writein_crc32 = ndb->crc32_cb(data_buffer, sizeof(pu_ndb_data_t) + data_size);
  }
  memcpy(data_writer->user_data + data_size, &writein_crc32, 4);

  // 写入数据到flash
  ndb->flash_callback.write(base_address + sector_size * 0 + byte_offset, &data_byte, 1);
  ndb->flash_callback.read(base_address + sector_size * 0 + byte_offset, &readback_data_byte, 1);
  if (data_byte != readback_data_byte) {
    return false;
  }
  ndb->flash_callback.write(base_address + sector_size * 1 + (sector_size * sector_offset) + (pack_data_size * data_offset), data_buffer, pack_data_size);
  ndb->flash_callback.read(base_address + sector_size * 1 + (sector_size * sector_offset) + (pack_data_size * data_offset), data_buffer, pack_data_size - 4);
  pu_bitmap_clear_bit(&ndb->bitmap, index); // 更新位图
  uint32_t readback_crc32 = ndb->crc32_cb(data_buffer, sizeof(pu_ndb_data_t) + data_size);
  if (readback_crc32 != writein_crc32) {
    return false;
  }
  return true;
}

pu_ndb_error_code_e pu_ndb_get_last_x_data(pu_ndb_p ndb, uint32_t x, uint8_t *data, uint32_t data_size) {
  uint8_t pack_data_size = data_size + sizeof(pu_ndb_data_t) + 4;
  uint32_t base_address = ndb->flash_desc.start_address;
  uint32_t sector_size = ndb->flash_desc.sector_size;
  size_t sector_data_block_count = sector_size / pack_data_size; // 每个扇区可以存储的数据块数量
  int32_t index = pu_bitmap_find_set_bit(&ndb->bitmap);          // 查找最后一个未使用的的数据块索引
  int32_t final_index;
  static uint8_t data_buffer[PU_NDB_MAX_DATA_SIZE];
  uint32_t crc32_data;
  if (index < 0) {
    if (x > ndb->bitmap.max_bits) {
      // x超过最大位数,无法获取
      return PU_NDB_ERR_QUERY_OVER_CAPACITY_MAX;
    } else {
      final_index = ndb->bitmap.max_bits - 1 - x; // 从最后一个数据块开始往前数x个数据块
    }
  } else if (index > 0) {
    if (x > (uint32_t)(index - 1)) {
      // 要判断第一个和最后一个的序列号哪个大才能确认方向
      int32_t sector_offset;
      int32_t data_offset;
      uint32_t older_serial_number;

      int32_t newer_index = index - 1;                         // 从最后一个数据块开始往前数第一个数据块
      sector_offset = (newer_index / sector_data_block_count); // 扇区偏移
      data_offset = (newer_index % sector_data_block_count);   // 数据偏移
      ndb->flash_callback.read(base_address + sector_size * 1 + (sector_size * sector_offset) + (pack_data_size * data_offset), data_buffer, pack_data_size);
      if (((pu_ndb_data_p)(data_buffer))->magic_num != PU_NDB_MAGIC_NUMBER) {
        // 最新的数据无效
        return PU_NDB_ERR_NOT_ENOUGH_DATA;
      }

      int32_t older_index = ndb->bitmap.max_bits - 1 - (x - index); // 从最后一个数据块开始往前数x个数据块
      sector_offset = (older_index / sector_data_block_count);      // 扇区偏移
      data_offset = (older_index % sector_data_block_count);        // 数据偏移
      ndb->flash_callback.read(base_address + sector_size * 1 + (sector_size * sector_offset) + (pack_data_size * data_offset), data_buffer, pack_data_size);
      if (((pu_ndb_data_p)(data_buffer))->magic_num == PU_NDB_MAGIC_NUMBER) {
        // 说明是有效数据 需要判断
        older_serial_number = ((pu_ndb_data_p)(data_buffer))->serial_num;
        if (ndb->current_serial_num > older_serial_number) {
          // final_index = (older_serial_number % ndb->bitmap.max_bits);
          crc32_data = *((uint32_t *)(((pu_ndb_data_p)data_buffer)->user_data + data_size));

          if (ndb->crc32_cb) {
            if (crc32_data == ndb->crc32_cb(data_buffer, pack_data_size - 4)) {
              memcpy(data, ((pu_ndb_data_p)data_buffer)->user_data, data_size);
              return PU_NDB_ERR_NONE;
            } else {
              return PU_NDB_ERR_FLASH_READ_DATA_ERROR;
            }
          } else {
            memcpy(data, ((pu_ndb_data_p)data_buffer)->user_data, data_size);
            return PU_NDB_ERR_NONE;
          }

        } else {
          return PU_NDB_ERR_NOT_ENOUGH_DATA;
        }
      } else {
        // 没有数据
        return PU_NDB_ERR_NOT_ENOUGH_DATA;
      }
    } else {
      final_index = index - 1 - x; // 从最后一个数据块开始往前数x个数据块
    }
  } else {
    return PU_NDB_ERR_NOT_ENOUGH_DATA;
  }
  int32_t sector_offset = (final_index / sector_data_block_count); // 扇区偏移
  int32_t data_offset = (final_index % sector_data_block_count);   // 数据偏移
  // 从flash中载入数据
  ndb->flash_callback.read(base_address + sector_size * 1 + (sector_size * sector_offset) + (pack_data_size * data_offset), data_buffer, pack_data_size);
  if (((pu_ndb_data_p)(data_buffer))->magic_num == PU_NDB_MAGIC_NUMBER) {
    crc32_data = *((uint32_t *)(((pu_ndb_data_p)data_buffer)->user_data + data_size));

    if (ndb->crc32_cb) {
      if (crc32_data == ndb->crc32_cb(data_buffer, pack_data_size - 4)) {
        memcpy(data, ((pu_ndb_data_p)data_buffer)->user_data, data_size);
        return PU_NDB_ERR_NONE;
      } else {
        return PU_NDB_ERR_FLASH_READ_DATA_ERROR;
      }
    } else {
      memcpy(data, ((pu_ndb_data_p)data_buffer)->user_data, data_size);
      return PU_NDB_ERR_NONE;
    }
  } else {
    return PU_NDB_ERR_FLASH_READ_DATA_ERROR;
  }
}

uint32_t pu_ndb_get_total_serial(pu_ndb_p ndb) {
  return ndb->current_serial_num;
}
