#include "nor_flash_simulator.h"

// ==================== 平台适配层 ====================
#ifdef __KERNEL__

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/mm.h>

/* 内核态: 大块内存用 kvmalloc（1MB 数据块 kmalloc 可能失败, 自动回落 vmalloc） */
#define FS_MALLOC(sz)    kvmalloc((sz), GFP_KERNEL)
#define FS_CALLOC(n, sz) kvcalloc((n), (sz), GFP_KERNEL)
#define FS_FREE(p)       kvfree(p)
#define FS_LOG(fmt, ...) pr_info("nor_flash: " fmt, ##__VA_ARGS__)

static struct file *fs_fopen(const char *path, bool is_write) {
  struct file *filp;
  unsigned int flags = is_write ? (O_WRONLY | O_CREAT | O_TRUNC) : O_RDONLY;

  if (path == NULL || path[0] == '\0')
    return NULL;
  filp = filp_open(path, flags, 0644);
  if (IS_ERR(filp))
    return NULL;
  return filp;
}

static void fs_fclose(struct file *filp) {
  if (filp)
    filp_close(filp, NULL);
}

/* 顺序读写: 调用方持有 pos 并传入（模拟 fseek/ftell 的文件位置语义） */
static size_t fs_fwrite(const void *buf, size_t len, struct file *filp, loff_t *pos) {
  ssize_t n = kernel_write(filp, buf, len, pos);
  return (n > 0) ? (size_t)n : 0;
}

static size_t fs_fread(void *buf, size_t len, struct file *filp, loff_t *pos) {
  ssize_t n = kernel_read(filp, buf, len, pos);
  return (n > 0) ? (size_t)n : 0;
}

static long fs_fsize(struct file *filp) {
  return (long)i_size_read(file_inode(filp));
}

#else /* 用户态 */

#define FS_MALLOC(sz)    malloc(sz)
#define FS_CALLOC(n, sz) calloc((n), (sz))
#define FS_FREE(p)       free(p)
#define FS_LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)

#endif /* __KERNEL__ */

// ==================== 工具函数 ====================
static uint32_t addr_to_sector(uint32_t addr) {
  return addr / NOR_SECTOR_SIZE;
}

static uint32_t sector_to_addr(uint32_t sector) {
  return sector * NOR_SECTOR_SIZE;
}

static int is_addr_valid(uint32_t addr) {
  return addr < FLASH_SIZE;
}

static int is_sector_valid(uint32_t sector) {
  return sector < SECTOR_COUNT;
}

static int file_exists(const char *file_path) {
#ifdef __KERNEL__
  struct file *filp;
  if (!file_path || file_path[0] == '\0')
    return 0;
  filp = filp_open(file_path, O_RDONLY, 0);
  if (IS_ERR(filp))
    return 0;
  filp_close(filp, NULL);
  return 1;
#else
  FILE *file;
  if (!file_path || file_path[0] == '\0')
    return 0;
  file = fopen(file_path, "rb");
  if (file) {
    fclose(file);
    return 1;
  }
  return 0;
#endif
}

// ==================== 核心实现 ====================
nor_flash_t *flash_init(const char *data_file_path, const char *meta_file_path) {
  nor_flash_t *flash = (nor_flash_t *)FS_MALLOC(sizeof(nor_flash_t));
  if (!flash)
    return NULL;

  // 初始化文件路径
  if (data_file_path && data_file_path[0] != '\0') {
    strncpy(flash->data_file_path, data_file_path, sizeof(flash->data_file_path) - 1);
    flash->data_file_path[sizeof(flash->data_file_path) - 1] = '\0';
  } else {
    strcpy(flash->data_file_path, DEFAULT_DATA_FILE);
  }

  if (meta_file_path && meta_file_path[0] != '\0') {
    strncpy(flash->meta_file_path, meta_file_path, sizeof(flash->meta_file_path) - 1);
    flash->meta_file_path[sizeof(flash->meta_file_path) - 1] = '\0';
  } else {
    strcpy(flash->meta_file_path, DEFAULT_META_FILE);
  }

  // 分配Flash数据内存（初始为0xFF，模拟已擦除状态）
  flash->data = (uint8_t *)FS_MALLOC(FLASH_SIZE);
  if (!flash->data) {
    FS_FREE(flash);
    return NULL;
  }
  memset(flash->data, 0xFF, FLASH_SIZE);

  // 分配擦除计数数组
  flash->erase_count = (uint32_t *)FS_CALLOC(SECTOR_COUNT, sizeof(uint32_t));
  if (!flash->erase_count) {
    FS_FREE(flash->data);
    FS_FREE(flash);
    return NULL;
  }

  // 分配坏块标记数组
  flash->bad_blocks = (uint8_t *)FS_CALLOC(SECTOR_COUNT, sizeof(uint8_t));
  if (!flash->bad_blocks) {
    FS_FREE(flash->erase_count);
    FS_FREE(flash->data);
    FS_FREE(flash);
    return NULL;
  }

  flash->total_erase_count = 0;
  flash->initialized = 1;

  // 自动加载现有文件
  if (file_exists(flash->data_file_path)) {
    FS_LOG("Loading existing data file: %s\n", flash->data_file_path);
    flash_load_data(flash);
  }

  if (file_exists(flash->meta_file_path)) {
    FS_LOG("Loading existing metadata file: %s\n", flash->meta_file_path);
    flash_load_metadata(flash);
  }

  FS_LOG("Flash initialized: %d KB, %d sectors\n", FLASH_SIZE / 1024, SECTOR_COUNT);
  FS_LOG("Data file: %s\n", flash->data_file_path);
  FS_LOG("Metadata file: %s\n", flash->meta_file_path);

  flash_save_all(flash); // 确保初始状态被保存
  return flash;
}

flash_status_t flash_erase_sector(nor_flash_t *flash, uint32_t sector_addr) {
  if (!flash || !flash->initialized)
    return FLASH_ERROR;

  uint32_t sector = addr_to_sector(sector_addr);
  if (!is_sector_valid(sector))
    return FLASH_ADDR_ERROR;

  // 检查是否是坏块
  if (flash->bad_blocks[sector]) {
    return FLASH_BAD_BLOCK;
  }

  // 检查擦除次数是否超限
  if (flash->erase_count[sector] >= MAX_ERASE_COUNT) {
    flash->bad_blocks[sector] = 1;
    return FLASH_WEAR_OUT;
  }

  // 执行擦除操作（设置为全0xFF）
  uint32_t start_addr = sector_to_addr(sector);
  memset(&flash->data[start_addr], 0xFF, NOR_SECTOR_SIZE);

  // 更新擦除计数
  flash->erase_count[sector]++;
  flash->total_erase_count++;

  return FLASH_OK;
}

flash_status_t flash_write_page(nor_flash_t *flash, uint32_t addr, const uint8_t *data, uint32_t len) {
  if (!flash || !data || len == 0)
    return FLASH_ERROR;
  if (!is_addr_valid(addr) || !is_addr_valid(addr + len - 1))
    return FLASH_ADDR_ERROR;

  uint32_t sector = addr_to_sector(addr);
  if (flash->bad_blocks[sector])
    return FLASH_BAD_BLOCK;

  // 检查是否跨扇区写入
  if (addr_to_sector(addr) != addr_to_sector(addr + len - 1)) {
    return FLASH_ERROR;
  }

  // NOR Flash特性检查
  for (uint32_t i = 0; i < len; i++) {
    uint32_t current_addr = addr + i;
    uint8_t current_value = flash->data[current_addr];
    uint8_t new_value = data[i];

    if ((current_value & new_value) != new_value) {
      return FLASH_NOT_ERASED;
    }
  }

  // 执行写入操作
  for (uint32_t i = 0; i < len; i++) {
    flash->data[addr + i] &= data[i];
  }

  return FLASH_OK;
}

flash_status_t flash_write_byte(nor_flash_t *flash, uint32_t addr, uint8_t value) {
  return flash_write_page(flash, addr, &value, 1);
}

flash_status_t flash_read(nor_flash_t *flash, uint32_t addr, uint8_t *buffer, uint32_t len) {
  if (!flash || !buffer || len == 0)
    return FLASH_ERROR;
  if (!is_addr_valid(addr) || !is_addr_valid(addr + len - 1))
    return FLASH_ADDR_ERROR;

  memcpy(buffer, &flash->data[addr], len);
  return FLASH_OK;
}

flash_status_t flash_read_byte(nor_flash_t *flash, uint32_t addr, uint8_t *value) {
  return flash_read(flash, addr, value, 1);
}

flash_status_t flash_save_data(nor_flash_t *flash) {
  if (!flash || !flash->data)
    return FLASH_ERROR;

#ifdef __KERNEL__
  {
    loff_t pos = 0;
    struct file *file = fs_fopen(flash->data_file_path, true);
    if (!file)
      return FLASH_FILE_ERROR;
    {
      size_t written = fs_fwrite(flash->data, FLASH_SIZE, file, &pos);
      fs_fclose(file);
      if (written != FLASH_SIZE)
        return FLASH_FILE_ERROR;
    }
  }
#else
  {
    FILE *file = fopen(flash->data_file_path, "wb");
    if (!file)
      return FLASH_FILE_ERROR;

    size_t written = fwrite(flash->data, 1, FLASH_SIZE, file);
    fclose(file);

    if (written != FLASH_SIZE)
      return FLASH_FILE_ERROR;
  }
#endif
  return FLASH_OK;
}

flash_status_t flash_save_metadata(nor_flash_t *flash) {
  if (!flash)
    return FLASH_ERROR;

#ifdef __KERNEL__
  {
    loff_t pos = 0;
    struct file *file = fs_fopen(flash->meta_file_path, true);
    if (!file)
      return FLASH_FILE_ERROR;

    // 写入元数据标识头
    const char *header = "FLASH_META_v1";
    fs_fwrite(header, strlen(header), file, &pos);

    // 写入擦除计数
    fs_fwrite(flash->erase_count, sizeof(uint32_t) * SECTOR_COUNT, file, &pos);

    // 写入坏块信息
    fs_fwrite(flash->bad_blocks, sizeof(uint8_t) * SECTOR_COUNT, file, &pos);

    // 写入总擦除计数
    fs_fwrite(&flash->total_erase_count, sizeof(uint32_t), file, &pos);

    fs_fclose(file);
  }
#else
  {
    FILE *file = fopen(flash->meta_file_path, "wb");
    if (!file)
      return FLASH_FILE_ERROR;

    // 写入元数据标识头
    const char *header = "FLASH_META_v1";
    fwrite(header, 1, strlen(header), file);

    // 写入擦除计数
    fwrite(flash->erase_count, sizeof(uint32_t), SECTOR_COUNT, file);

    // 写入坏块信息
    fwrite(flash->bad_blocks, sizeof(uint8_t), SECTOR_COUNT, file);

    // 写入总擦除计数
    fwrite(&flash->total_erase_count, sizeof(uint32_t), 1, file);

    fclose(file);
  }
#endif
  return FLASH_OK;
}

flash_status_t flash_save_all(nor_flash_t *flash) {
  flash_status_t data_status = flash_save_data(flash);
  flash_status_t meta_status = flash_save_metadata(flash);

  if (data_status != FLASH_OK)
    return data_status;
  if (meta_status != FLASH_OK)
    return meta_status;

  return FLASH_OK;
}

flash_status_t flash_load_data(nor_flash_t *flash) {
  if (!flash || !flash->data)
    return FLASH_ERROR;

#ifdef __KERNEL__
  {
    loff_t pos = 0;
    struct file *file = fs_fopen(flash->data_file_path, false);
    if (!file)
      return FLASH_FILE_ERROR;
    {
      long file_size = fs_fsize(file);
      size_t read_size = (file_size > FLASH_SIZE) ? FLASH_SIZE : (size_t)file_size;
      size_t rd = fs_fread(flash->data, read_size, file, &pos);
      fs_fclose(file);
      if (rd != read_size)
        return FLASH_FILE_ERROR;
    }
  }
#else
  {
    FILE *file = fopen(flash->data_file_path, "rb");
    if (!file)
      return FLASH_FILE_ERROR;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    size_t read_size = (file_size > FLASH_SIZE) ? FLASH_SIZE : file_size;
    size_t read = fread(flash->data, 1, read_size, file);
    fclose(file);

    if (read != read_size)
      return FLASH_FILE_ERROR;
  }
#endif
  return FLASH_OK;
}

flash_status_t flash_load_metadata(nor_flash_t *flash) {
  if (!flash)
    return FLASH_ERROR;

#ifdef __KERNEL__
  {
    loff_t pos = 0;
    char header[14];
    struct file *file = fs_fopen(flash->meta_file_path, false);
    if (!file)
      return FLASH_FILE_ERROR;

    // 检查文件头
    if (fs_fread(header, 13, file, &pos) != 13) {
      fs_fclose(file);
      return FLASH_FILE_ERROR;
    }
    header[13] = '\0';

    if (strcmp(header, "FLASH_META_v1") != 0) {
      fs_fclose(file);
      return FLASH_FILE_ERROR;
    }

    // 读取擦除计数
    if (fs_fread(flash->erase_count, sizeof(uint32_t) * SECTOR_COUNT, file, &pos) !=
        sizeof(uint32_t) * SECTOR_COUNT) {
      fs_fclose(file);
      return FLASH_FILE_ERROR;
    }

    // 读取坏块信息
    if (fs_fread(flash->bad_blocks, sizeof(uint8_t) * SECTOR_COUNT, file, &pos) !=
        sizeof(uint8_t) * SECTOR_COUNT) {
      fs_fclose(file);
      return FLASH_FILE_ERROR;
    }

    // 读取总擦除计数
    if (fs_fread(&flash->total_erase_count, sizeof(uint32_t), file, &pos) != sizeof(uint32_t)) {
      fs_fclose(file);
      return FLASH_FILE_ERROR;
    }

    fs_fclose(file);
  }
#else
  {
    FILE *file = fopen(flash->meta_file_path, "rb");
    if (!file)
      return FLASH_FILE_ERROR;

    // 检查文件头
    char header[14];
    fread(header, 1, 13, file);
    header[13] = '\0';

    if (strcmp(header, "FLASH_META_v1") != 0) {
      fclose(file);
      return FLASH_FILE_ERROR;
    }

    // 读取擦除计数
    fread(flash->erase_count, sizeof(uint32_t), SECTOR_COUNT, file);

    // 读取坏块信息
    fread(flash->bad_blocks, sizeof(uint8_t), SECTOR_COUNT, file);

    // 读取总擦除计数
    fread(&flash->total_erase_count, sizeof(uint32_t), 1, file);

    fclose(file);
  }
#endif
  return FLASH_OK;
}

void flash_deinit(nor_flash_t *flash) {
  if (!flash)
    return;

  // 自动保存所有数据
  flash_save_all(flash);

  if (flash->data)
    FS_FREE(flash->data);
  if (flash->erase_count)
    FS_FREE(flash->erase_count);
  if (flash->bad_blocks)
    FS_FREE(flash->bad_blocks);

  FS_FREE(flash);
}

void flash_dump_info(nor_flash_t *flash) {
  if (!flash)
    return;

  FS_LOG("Flash Info: Size=%dKB, Sectors=%d\n", FLASH_SIZE / 1024, SECTOR_COUNT);
  FS_LOG("Data File: %s\n", flash->data_file_path);
  FS_LOG("Meta File: %s\n", flash->meta_file_path);
  FS_LOG("Total Erase Count: %u\n", flash->total_erase_count);

  int bad_blocks = 0;
  for (uint32_t i = 0; i < SECTOR_COUNT; i++) {
    if (flash->bad_blocks[i])
      bad_blocks++;
  }
  FS_LOG("Bad Blocks: %d/%d\n", bad_blocks, SECTOR_COUNT);
}

void flash_dump_sector(nor_flash_t *flash, uint32_t sector, uint32_t bytes_to_dump) {
  if (!flash || !is_sector_valid(sector))
    return;

  uint32_t start_addr = sector_to_addr(sector);
  bytes_to_dump = (bytes_to_dump > NOR_SECTOR_SIZE) ? NOR_SECTOR_SIZE : bytes_to_dump;

  FS_LOG("Sector %u (0x%08X-0x%08X)\n", sector, start_addr, start_addr + bytes_to_dump - 1);
  FS_LOG("Erase Count: %u, Bad Block: %s\n", flash->erase_count[sector],
         flash->bad_blocks[sector] ? "YES" : "NO");

  for (uint32_t i = 0; i < bytes_to_dump; i += 16) {
    FS_LOG("0x%08X: ", start_addr + i);
    for (uint32_t j = 0; j < 16; j++) {
      if (i + j < bytes_to_dump) {
        FS_LOG("%02X ", flash->data[start_addr + i + j]);
      }
    }
    FS_LOG("\n");
  }
}

void flash_fill_pattern(nor_flash_t *flash, uint32_t start_addr, uint32_t len, uint8_t pattern) {
  if (!flash || !is_addr_valid(start_addr) || !is_addr_valid(start_addr + len - 1)) {
    return;
  }

  // 确保区域已擦除
  uint32_t start_sector = addr_to_sector(start_addr);
  uint32_t end_sector = addr_to_sector(start_addr + len - 1);

  for (uint32_t sector = start_sector; sector <= end_sector; sector++) {
    if (flash->data[sector_to_addr(sector)] != 0xFF) {
      return;
    }
  }

  // 写入模式
  for (uint32_t i = 0; i < len; i++) {
    flash->data[start_addr + i] &= pattern;
  }
}

uint32_t flash_get_sector_erase_count(nor_flash_t *flash, uint32_t sector) {
  if (!flash || !is_sector_valid(sector))
    return 0;
  return flash->erase_count[sector];
}

uint8_t flash_is_bad_block(nor_flash_t *flash, uint32_t sector) {
  if (!flash || !is_sector_valid(sector))
    return 0;
  return flash->bad_blocks[sector];
}
