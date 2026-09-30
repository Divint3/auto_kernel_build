#ifndef METER_CHIP_PORT_DRIVER_H_
#define METER_CHIP_PORT_DRIVER_H_

#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）

#define METER_CHIP_DRIVER_PARAM_COEFFICIENT (float)10000.0

#define POW2_2                              (float)4.0
#define POW2_3                              (float)8.0
#define POW2_4                              (float)16.0
#define POW2_5                              (float)32.0
#define POW2_6                              (float)64.0
#define POW2_7                              (float)128.0
#define POW2_8                              (float)256.0
#define POW2_9                              (float)512.0
#define POW2_10                             (float)1024.0
#define POW2_11                             (float)2048.0
#define POW2_12                             (float)4096.0
#define POW2_13                             (float)8192.0
#define POW2_14                             (float)16384.0
#define POW2_15                             (float)32768.0
#define POW2_16                             (float)65536.0
#define POW2_17                             (float)131072.0
#define POW2_18                             (float)262144.0
#define POW2_19                             (float)524288.0
#define POW2_20                             (float)1048576.0
#define POW2_21                             (float)2097152.0
#define POW2_22                             (float)4194304.0
#define POW2_23                             (float)8388608.0
#define POW2_24                             (float)16777216.0
#define POW2_25                             (float)33554432.0
#define POW2_26                             (float)67108864.0
#define POW2_27                             (float)134217728.0
#define POW2_28                             (float)268435456.0
#define POW2_29                             (float)536870912.0
#define POW2_30                             (float)1073741824.0
#define POW2_31                             (float)2147483648.0
#define POW2_32                             (float)4294967296.0
#define POW2_33                             (float)8589934592.0
#define POW2_34                             (float)17179869184.0
#define POW2_35                             (float)34359738368.0

#define POW10_10                            (float)10000000000.0
#define POW10_11                            (float)100000000000.0
#define POW10_12                            (float)1000000000000.0
#define METER_CHIP_DRIVER_PI                (float)3.1415926
#define METER_CHIP_MAX_TARIFF_COUNT         4
/** 需量类型数量 */
#define METER_CHIP_DEMAND_TYPE_COUNT 8

typedef struct {
  int (*read)(uint8_t *data, uint8_t size);  ///< 串口读
  int (*write)(uint8_t *data, uint8_t size); ///< 串口写
} meter_chip_io_callback_t;

typedef struct {
  uint32_t page_size;     ///< 存储页大小
  uint32_t sector_size;   ///< 存储扇区大小
  uint32_t sector_count;  ///< 总扇区数量
  uint32_t start_address; ///< 存储起始地址
  uint32_t end_address;   ///< 存储结束地址
} meter_chip_flash_desc_t, *meter_chip_flash_desc_p;

///< flash 操作函数
typedef struct {
  int (*read)(uint32_t address, uint8_t *data, uint16_t size);  ///< 持久化存储读
  int (*write)(uint32_t address, uint8_t *data, uint16_t size); ///< 持久化存储写
  int (*erase)(uint32_t address);                               ///< 持久化存擦除
} meter_chip_flash_callback_t;

// 尖峰平谷费率枚举
typedef enum {
  METER_CHIP_TARIFF_TOTAL = 0,
  METER_CHIP_TARIFF_TIP = 1,    // 尖（尖峰时段）
  METER_CHIP_TARIFF_PEAK = 2,   // 峰（高峰时段）
  METER_CHIP_TARIFF_FLAT = 3,   // 平（平段时段）
  METER_CHIP_TARIFF_VALLEY = 4, // 谷（低谷时段）
  METER_CHIP_TARIFF_5 = 5,
  METER_CHIP_TARIFF_6 = 6,
  METER_CHIP_TARIFF_7 = 7,
  METER_CHIP_TARIFF_8 = 8,
  METER_CHIP_TARIFF_9 = 9,
  METER_CHIP_TARIFF_10 = 10,
  METER_CHIP_TARIFF_11 = 11,
  METER_CHIP_TARIFF_12 = 12,
  METER_CHIP_TARIFF_INVALID = 0xFF,
} meter_chip_tariff_e;

/**
 * @brief 需量类型枚举
 */
typedef enum {
  METER_CHIP_DEMAND_TYPE_FORWARD_ACTIVE = 0, /**< 正向有功需量 */
  METER_CHIP_DEMAND_TYPE_REVERSE_ACTIVE,     /**< 反向有功需量 */
  METER_CHIP_DEMAND_TYPE_COMB_REACTIVE_1,    /**< 组合无功1需量 */
  METER_CHIP_DEMAND_TYPE_COMB_REACTIVE_2,    /**< 组合无功2需量 */
  METER_CHIP_DEMAND_TYPE_REACTIVE_Q1,        /**< 第一象限无功需量 */
  METER_CHIP_DEMAND_TYPE_REACTIVE_Q2,        /**< 第二象限无功需量 */
  METER_CHIP_DEMAND_TYPE_REACTIVE_Q3,        /**< 第三象限无功需量 */
  METER_CHIP_DEMAND_TYPE_REACTIVE_Q4,        /**< 第四象限无功需量 */
} meter_chip_demand_type_e;

typedef void (*meter_chip_sleep_cb)(uint16_t ms);                                ///< 休眠函数
typedef uint32_t (*meter_chip_data_crc_cb)(uint8_t *data, uint32_t data_size);   ///< CRC计算回调
typedef meter_chip_tariff_e (*meter_chip_get_current_traiff_cb)(void *instance); ///< 获取当前费率函数
typedef bool (*meter_chip_check_reboot_cb)(void *instance);                      ///< 检查复位函数

// 电能方向
typedef enum {
  METER_CHIP_ENERGY_DIR_FORWARD = 0,
  METER_CHIP_ENERGY_DIR_REVERSE = 1,
} meter_chip_energy_dir_e;

// 无功四象限（以电压为参考相量）
typedef enum {
  METER_CHIP_REACTIVE_QUADRANT_I = 0,   // Ⅰ象限：有功+ 无功+ → 感性用电
  METER_CHIP_REACTIVE_QUADRANT_II = 1,  // Ⅱ象限：有功- 无功+ → 感性发电
  METER_CHIP_REACTIVE_QUADRANT_III = 2, // Ⅲ象限：有功- 无功- → 容性发电
  METER_CHIP_REACTIVE_QUADRANT_IV = 3   // Ⅳ象限：有功+ 无功- → 容性用电
} meter_chip_reactive_quadrant_e;

typedef enum {
  METER_CHIP_STATUS_BOOTING,                // 开机状态
  METER_CHIP_STATUS_REBOOTING,              // 复位状态
  METER_CHIP_STATUS_CALIBRATION_DATA_ERROR, // 校准数据错误
  METER_CHIP_STATUS_CALIBRATING,            // 校准状态
  METER_CHIP_STATUS_MEASURING,              // 计量状态
  METER_CHIP_STATUS_STANDBY,                // 待机状态
  METER_CHIP_STATUS_ERROR,                  // 错误状态
  METER_CHIP_STATUS_INIT_ERROR,             // 错误状态
  METER_CHIP_STATUS_COMM_ERROR,             // 通信寄存器无法读写
  METER_CHIP_STATUS_PLUSE_DATA_BLOCK_ERROR, // 脉冲数据块错误
} meter_chip_status_e;

// 组合有功特征字（带 union，可直接读完整字节）
typedef union {
  // 位段访问方式（你原来的代码，完全不变）
  struct {
    uint8_t pos_add :1; /* Bit0: 正向有功加 */
    uint8_t pos_sub :1; /* Bit1: 正向有功减 */
    uint8_t rev_add :1; /* Bit2: 反向有功加 */
    uint8_t rev_sub :1; /* Bit3: 反向有功减 */
    uint8_t reserved:4; /* Bit4~7: 保留 */
  };
  // 直接拿整个 uint8 字节值（新增）
  uint8_t byte;
} meter_chip_combined_active_tagged_word_t;

// 组合无功方式特征字（带 union，可直接读完整字节）
typedef union {
  // 位段访问方式（你原来的代码，完全不变）
  struct {
    uint8_t q1_add:1; /* Bit0: 象限1无功加 */
    uint8_t q1_sub:1; /* Bit1: 象限1无功减 */
    uint8_t q2_add:1; /* Bit2: 象限2无功加 */
    uint8_t q2_sub:1; /* Bit3: 象限2无功减 */
    uint8_t q3_add:1; /* Bit4: 象限3无功加 */
    uint8_t q3_sub:1; /* Bit5: 象限3无功减 */
    uint8_t q4_add:1; /* Bit6: 象限4无功加 */
    uint8_t q4_sub:1; /* Bit7: 象限4无功减 */
  };
  // 直接拿整个 uint8 字节值（新增）
  uint8_t byte;
} meter_chip_combined_reactive_tagged_word_t;

typedef struct {
  meter_chip_combined_active_tagged_word_t combined_active_tagged_word;        ///< 组合有功特征字
  meter_chip_combined_reactive_tagged_word_t combined_reactive_tagged_word[2]; ///< 组合无功特征字
} meter_chip_tagged_words_t;

/**
 * @brief 计算有功组合
 *
 * @param tagged_words  特征字
 * @param w_pos 正向有功累计
 * @param w_rev 反向有功累计
 * @return float
 */
static inline float calc_active_combined(meter_chip_tagged_words_t tagged_words, float w_pos, float w_rev) {
  float result = 0;

  if (tagged_words.combined_active_tagged_word.pos_add) {
    result += w_pos;
  }
  if (tagged_words.combined_active_tagged_word.pos_sub) {
    result -= w_pos;
  }
  if (tagged_words.combined_active_tagged_word.rev_add) {
    result += w_rev;
  }
  if (tagged_words.combined_active_tagged_word.rev_sub) {
    result -= w_rev;
  }
  return result;
}
#define CALC_ACTIVE_COMBINED(tariff) calc_active_combined(instance->tagged_words, instance->processed_pwr_data[tariff].forward_active_energy, instance->processed_pwr_data[tariff].reverse_active_energy)

/**
 * @brief 计算组合无功
 *
 * @param tagged_words 特征字
 * @param num 组合无功1 or 组合无功2
 * @param q1 象限1无功累计
 * @param q2 象限2无功累计
 * @param q3 象限3无功累计
 * @param q4 象限4无功累计
 * @return float
 */
static inline float calc_reactive_combined(meter_chip_tagged_words_t tagged_words, uint8_t num, float q1, float q2, float q3, float q4) {
  float result = 0;
  meter_chip_combined_reactive_tagged_word_t word;
  if (num == 1) {
    word = tagged_words.combined_reactive_tagged_word[0];
  } else if (num == 2) {
    word = tagged_words.combined_reactive_tagged_word[1];
  }

  if (word.q1_add) {
    result += q1;
  }
  if (word.q1_sub) {
    result -= q1;
  }

  if (word.q2_add) {
    result += q2;
  }
  if (word.q2_sub) {
    result -= q2;
  }

  if (word.q3_add) {
    result += q3;
  }
  if (word.q3_sub) {
    result -= q3;
  }

  if (word.q4_add) {
    result += q4;
  }
  if (word.q4_sub) {
    result -= q4;
  }

  return result;
}

// 计算组合无功：参数 = 费率索引(0=总), 组合号(1=组合无功1 / 2=组合无功2)
#define CALC_REACTIVE_COMBINED(rate_index, comb_num) calc_reactive_combined(instance->tagged_words, comb_num, instance->processed_pwr_data[rate_index].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_I], instance->processed_pwr_data[rate_index].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_II], instance->processed_pwr_data[rate_index].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_III], instance->processed_pwr_data[rate_index].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_IV])

#endif // METER_CHIP_PORT_DRIVER_H_