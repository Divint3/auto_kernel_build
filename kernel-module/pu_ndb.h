#ifndef PU_NDB_H_
#define PU_NDB_H_

#include "pu_bitmap.h"
#include "pu_util.h"

#define PU_NDB_MAX_DATA_SIZE 256 // 包括魔数 序列号 用户数据和校验和的总长度不能超过这个
#define PU_NDB_RELOAD_TIMES  3
#define PU_NDB_MAGIC_NUMBER  0xAA5555AA

typedef enum {
  PU_NDB_ERR_NONE,                    // 无错误
  PU_NDB_ERR_FLASH_READ_DATA_ERROR,   // 读取flash数据时出错
  PU_NDB_ERR_QUERY_OVER_CAPACITY_MAX, // 查询时超出容量最大值
  PU_NDB_ERR_NOT_ENOUGH_DATA,         // 数据不足
} pu_ndb_error_code_e;

typedef struct {
  uint32_t page_size;     ///< 存储页大小
  uint32_t sector_size;   ///< 存储扇区大小
  uint32_t sector_count;  ///< 总扇区数量
  uint32_t start_address; ///< 存储起始地址
} pu_flash_desc_t, *pu_flash_desc_p;

///< flash 操作函数
typedef struct {
  int (*read)(uint32_t address, uint8_t *data, size_t size);  ///< 持久化存储读
  int (*write)(uint32_t address, uint8_t *data, size_t size); ///< 持久化存储写
  int (*erase)(uint32_t address);                             ///< 持久化存擦除
} pu_flash_callback_t;

typedef uint32_t (*pu_ndb_crc32_cb)(uint8_t *data, size_t data_size);

typedef struct {
  bool valid;                         // 数据库数据是否有效
  uint32_t data_size;                 // 数据长度
  pu_flash_desc_t flash_desc;         // flash描述
  pu_flash_callback_t flash_callback; ///< 持久化存储回调
  pu_bitmap_t bitmap;                 ///< 数据位图
  pu_ndb_crc32_cb crc32_cb;
  uint32_t current_serial_num;
} pu_ndb_t, *pu_ndb_p;

typedef struct {
  uint32_t magic_num;   ///< 魔数字段
  uint32_t serial_num;  ///< 序列号字段
  uint8_t user_data[0]; ///< 自定义数据
}  pu_ndb_data_t, *pu_ndb_data_p;

// 通用数据帧结构体宏
// magic:   魔数字段 (如 0xAA55)
// seq:     序列号字段
// data_struct: 自定义数据结构体定义
// name:    定义的结构体变量名
#define PU_NDB_DATA_STRUCT(data_struct) \
  typedef struct {                      \
    uint32_t magic_num;                 \
    uint32_t serial_num;                \
    data_struct user_data;              \
    uint32_t check_sum;                 \
  }  pu_ndb_##data_struct
#define PU_NDB_DATA_NAME(data_struct) pu_ndb_##data_struct

bool pu_ndb_load_data(pu_ndb_p ndb, uint8_t *data, uint32_t data_size);
bool pu_ndb_save_data(pu_ndb_p ndb, uint8_t *data, uint32_t data_size);
pu_ndb_error_code_e pu_ndb_get_last_x_data(pu_ndb_p ndb, uint32_t x, uint8_t *data, uint32_t data_size);

#endif // PU_NDB_H_