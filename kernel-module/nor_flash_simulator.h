#ifndef FLASH_SIMULATOR_H_
#define FLASH_SIMULATOR_H_

/*
 * NOR Flash 仿真器 —— 普通(C标准库) / 内核(__KERNEL__) 双环境
 *
 * 平台头文件统一经 pu_port.h 引入（stdint/stddef/stdio/stdlib/string 均已映射）:
 *   - 内核态: kvmalloc/kvcalloc 分配（1MB 大块自动回落 vmalloc），
 *     文件持久化走 filp_open + kernel_write/kernel_read
 *   - 用户态: malloc/calloc + stdio 文件操作
 */

#include "pu_port.h"

// ==================== 配置区域 ====================
#define FLASH_SIZE      (1024 * 1024) // 1MB Flash
#define SECTOR_SIZE     (4 * 1024)    // 4KB 扇区大小
#define PAGE_SIZE       256           // 256字节 页大小
#define MAX_ERASE_COUNT 100000        // 最大擦除次数

// 默认文件名
#define DEFAULT_DATA_FILE "flash_data.bin"
#define DEFAULT_META_FILE "flash_meta.bin"

// 计算扇区数量
#define SECTOR_COUNT (FLASH_SIZE / SECTOR_SIZE)

// Flash状态码
typedef enum {
  FLASH_OK = 0,
  FLASH_ERROR,
  FLASH_NOT_ERASED,
  FLASH_BAD_BLOCK,
  FLASH_WEAR_OUT,
  FLASH_ADDR_ERROR,
  FLASH_FILE_ERROR
} flash_status_t;

// ==================== 数据结构 ====================
typedef struct {
  uint8_t *data;              // Flash数据存储
  uint32_t *erase_count;      // 每个扇区的擦除计数
  uint8_t *bad_blocks;        // 坏块标记
  uint32_t total_erase_count; // 总擦除次数
  uint32_t initialized;       // 初始化标志

  char data_file_path[256]; // 数据文件路径
  char meta_file_path[256]; // 元数据文件路径
} nor_flash_t;

// ==================== 函数声明 ====================
nor_flash_t *flash_init(const char *data_file_path, const char *meta_file_path);
flash_status_t flash_erase_sector(nor_flash_t *flash, uint32_t sector_addr);
flash_status_t flash_write_page(nor_flash_t *flash, uint32_t addr, const uint8_t *data, uint32_t len);
flash_status_t flash_write_byte(nor_flash_t *flash, uint32_t addr, uint8_t value);
flash_status_t flash_read(nor_flash_t *flash, uint32_t addr, uint8_t *buffer, uint32_t len);
flash_status_t flash_read_byte(nor_flash_t *flash, uint32_t addr, uint8_t *value);
flash_status_t flash_verify(nor_flash_t *flash, uint32_t addr, const uint8_t *data, uint32_t len);
flash_status_t flash_save_data(nor_flash_t *flash);
flash_status_t flash_save_metadata(nor_flash_t *flash);
flash_status_t flash_save_all(nor_flash_t *flash);
flash_status_t flash_load_data(nor_flash_t *flash);
flash_status_t flash_load_metadata(nor_flash_t *flash);
void flash_deinit(nor_flash_t *flash);
void flash_dump_info(nor_flash_t *flash);
void flash_dump_sector(nor_flash_t *flash, uint32_t sector, uint32_t bytes_to_dump);
void flash_fill_pattern(nor_flash_t *flash, uint32_t start_addr, uint32_t len, uint8_t pattern);
uint32_t flash_get_sector_erase_count(nor_flash_t *flash, uint32_t sector);
uint8_t flash_is_bad_block(nor_flash_t *flash, uint32_t sector);

#endif // FLASH_SIMULATOR_H_