#ifndef RN8209_DRIVER_H_
#define RN8209_DRIVER_H_

#include "meter_chip_port_driver.h"
#include "pu_compiler.h"
#include "pu_doublecheck_bitmap.h"
#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）
#include "pu_type.h"

#define RN8209_CLK_IN                                 ((float)3579545.0)

#define RN8209_READ_CMD(x)                            (x & ~0x80)
#define RN8209_WRITE_CMD(x)                           (x | 0x80)
#define RN8209_GET_CMD(x)                             (x & 0x80)
#define RN8209_GET_ADDRESS(x)                         (x & ~0x80)

#define RN8209_RETRY_TIMES                            5
#define RN8209_CURRENT_CALIBRATION_DATA_BLOCK_VERSION 1
#define RN8209_ERROR_CRC                              0xAA5555AA
#define CHECK_SIZE_STRUCT(s, size)                    typedef char __CHECK_SIZE__##s[(sizeof(s) == (size)) ? 1 : -1]
#if RN8209_CURRENT_CALIBRATION_DATA_BLOCK_VERSION == 1
#define rn8209_pulse_data_block_t rn8209_pulse_data_block_v1_t
#else
#error 切换版本后必须切换rn8209_pulse_data_block_t结构体重定义
#endif

typedef enum {
  TOTAL_INDEX = 0,
  SHARP_INDEX = 1,
  PEAK_INDEX = 2,
  NORMAL_INDEX = 3,
  VALLEY_INDEX = 4
} rn8209_energy_type_index_e;

#define RN8209_CALIBRATION_ACTIVE_POWER_GAIN_AVG    1
#define RN8209_CALIBRATION_ACTIVE_POWER_PHASE_AVG   0
#define RN8209_CALIBRATION_REACTIVE_POWER_PHASE_AVG 0
#define RN8209_CALIBRATION_AVG_TIMES                6
#define RN8209_CALIBRATION_OFFSET_AVG_TIMES         20
#define RN8209_CALIBRATION_SAMPLE_PERIOD            350

typedef enum {
  RN8209_REG_SYSCON,
  RN8209_REG_EMUCON,
  RN8209_REG_HFConst,
  RN8209_REG_PStart,
  RN8209_REG_DStart,
  RN8209_REG_GPQA,
  RN8209_REG_GPQB,
  RN8209_REG_PhsA,
  RN8209_REG_PhsB,
  RN8209_REG_QPhsCal,
  RN8209_REG_APOSA,
  RN8209_REG_APOSB,
  RN8209_REG_RPOSA,
  RN8209_REG_RPOSB,
  RN8209_REG_IARMSOS,
  RN8209_REG_IBRMSOS,
  RN8209_REG_IBGain,
  RN8209_REG_D2FPL,
  RN8209_REG_D2FPH,
  RN8209_REG_DCIAH,
  RN8209_REG_DCIBH,
  RN8209_REG_DCUH,
  RN8209_REG_DCL,
  RN8209_REG_EMUCON2,
  RN8209_REG_PFCnt,
  RN8209_REG_DFcnt,
  RN8209_REG_IARMS,
  RN8209_REG_IBRMS,
  RN8209_REG_URMS,
  RN8209_REG_UFreq,
  RN8209_REG_PowerPA,
  RN8209_REG_PowerPB,
  RN8209_REG_PowerQ,
  RN8209_REG_EnergyP,
  RN8209_REG_EnergyP2,
  RN8209_REG_EnergyD,
  RN8209_REG_EnergyD2,
  RN8209_REG_EMUStatus,
  RN8209_REG_SPL_IA,
  RN8209_REG_SPL_IB,
  RN8209_REG_SPL_U,
  RN8209_REG_UFreq2,
  RN8209_REG_IE,
  RN8209_REG_IF,
  RN8209_REG_RIF,
  RN8209_REG_SysStatus,
  RN8209_REG_RData,
  RN8209_REG_WData,
  RN8209_REG_DeviceID,
  RN8209_REG_DeviceID2,
  RN8209_REG_COUNT
} rn8209_register_name_e;
/**
 * @brief RN8209 校准通道
 *
 */
typedef enum {
  RN8209_PATH_A,
  RN8209_PATH_B,
} rn8209_path_e;

typedef enum {
  RN8209_CALIBRATION_REG, ///< 校表参数
  RN8209_MEASURE_REG,     ///< 计量参数
  RN8209_SYSTEM_REG       ///< 系统寄存器
} rn8209_register_type_e;

typedef struct {
  rn8209_register_name_e name; ///< 寄存器名称(序号)
  uint8_t address;             ///< 地址
  uint8_t rw;                  ///< 可读写 6 写 2 读 4
  uint8_t size;                ///< 寄存器大小
  rn8209_register_type_e type; ///< 寄存器类型
  bool is_protect;             ///< 是否写保护
#ifdef RN8209_SIMULATE
  uext32_t value;
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
  char *comment; ///< 描述信息
#endif           /* #ifdef RN8209_COMMENT */
} rn8209_register_config_t, *rn8209_register_config_p;

/**
 * @brief 硬件/系统预设寄存器,预设配置
 *
 */
typedef struct {
  uext32_t SYSCON;
  uext32_t EMUCON;
  uext32_t EMUCON2;
  uext32_t D2FPL;
  uext32_t D2FPH;
  uext32_t DCIAH;
  uext32_t DCIBH;
  uext32_t DCUH;
  uext32_t DCL;
  float uv; ///< 每1倍标准电压对应的ADC采样电压
  float ui; ///< 每1A对应的ADC采样电压
} rn8209_preset_t, *rn8209_preset_p;

typedef struct {
  float voltage;           ///< 电压实际值 (v)
  float channel_a_current; ///< 通道A电流实际值 (a)
  float channel_b_current; ///< 通道B电流实际值 (a)

  float apparent_power; ///< 视在功率实际值 (va)
  float active_power;   ///< 有功功率实际值 (w)
  float reactive_power; ///< 无功功率实际值 (var)
  float power_factor;   ///< 功率因数实际值

  float avg_power;      ///< 一分钟平均功率实际值 (w)
  float grid_frequency; ///< 电网频率实际值 (hz)

  meter_chip_energy_dir_e activate_power_dir; // 有功电能方向
  meter_chip_energy_dir_e reactive_power_dir; // 无功电能方向

  meter_chip_reactive_quadrant_e quadrant; // 无功象限
  meter_chip_tariff_e tariff;
  bool read_reg_interrupt_flag;
  uext32_t reg_interrupt_flag;
} rn8209_processed_variable_data_t, *rn8209_processed_variable_data_p;

typedef struct {
  uint8_t current_a_channel_gain; ///< 通道A电流增益
  uint8_t current_b_channel_gain; ///< 通道B电流增益
  uint8_t voltage_channel_gain;   ///< 电压增益
  uint32_t channel_a_current;     ///< 通道A电流寄存器值
  uint32_t channel_b_current;     ///< 通道B电流寄存器值
  uint32_t voltage;               ///< 电压寄存器值

  uint32_t active_power;   ///< 有功功率寄存器值
  uint32_t reactive_power; ///< 无功功率寄存器值
} rn8209_origin_variable_data_t, *rn8209_origin_variable_data_p;

#pragma pack(1)

/**
 * @brief 需要持久化存储的计量信息数据,校表时可清零
 *
 */
typedef struct {
  uint8_t magic_number[3]; ///< 魔数,等同于deviceID 0x820900
  uint16_t basic_current;  ///< 基本电流
  uint16_t meter_const;    ///< 电表常数
  uext16_t HFConst;        ///< 常量计算值
  uext16_t PStart;         ///< 有功启动功率
  uext16_t DStart;         ///< 无功启动功率
  uext16_t GPQA;           ///< A通道功率增益校正
  uext16_t GPQB;           ///< B通道功率增益校正
  uext8_t PhsA;            ///< A通道相位校正
  uext8_t PhsB;            ///< B通道相位校正
  uext16_t QPhsCal;        ///< 无功相位校正
  uext16_t APOSA;          ///< A通道有功功率offset校正
  uext16_t APOSB;          ///< B通道有功功率offset校正
  uext16_t RPOSA;          ///< A通道无功功率offset校正
  uext16_t RPOSB;          ///< B通道无功功率offset校正
  uext16_t IARMSOS;        ///< A通道电流有效值offset校正
  uext16_t IBRMSOS;        ///< B通道电流有效值offset校正
  uext16_t IBGain;         ///< B通道电流增益
  float Ku;                ///< 电压转换系数
  float Kia;               ///< A通道电流转换系数
  float Kib;               ///< B通道电流转换系数
  float Kp;                ///< 有功功率转换系数

  uint8_t checksum;

} rn8209_calibration_data_t, *rn8209_calibration_data_p;

/**
 * @brief 存储到flash的数据
 *
 */

// 版本模板,用于以后升级
// typedef struct {
//   uint32_t version;                                                ///< 脉冲格式版本

//   uint32_t crc;                                                    ///< 保留字段 确定四字节对齐
// } rn8209_pulse_data_block_v2_t, *rn8209_pulse_data_block_v2_p;

// 使用

#pragma pack()

typedef struct {
  uint32_t forward_active_pulse_count;   ///< 正向 有功脉冲数量
  uint32_t reverse_active_pulse_count;   ///< 反向 有功脉冲数量
  uint32_t forward_reactive_pulse_count; ///< 正向 无功脉冲数量
  uint32_t reverse_reactive_pulse_count; ///< 反向 无功脉冲数量
  uint32_t reactive_quadrant_count[4];   ///< 四象限无功脉冲数量
} rn8209_pulse_data_t, *rn8209_pulse_data_p;

// 规定长度固定为256
typedef struct {
  uint32_t version;                                                ///< 脉冲格式版本
  rn8209_pulse_data_t pulse_data[METER_CHIP_MAX_TARIFF_COUNT + 1]; ///< 脉冲数据 总 尖峰平谷费率
  uint32_t reverse[22];                                            ///< 保留字段 确定四字节对齐
  uint32_t crc;                                                    ///< 保留字段 确定四字节对齐
} rn8209_pulse_data_block_v1_t, *rn8209_pulse_data_block_v1_p;

CHECK_SIZE_STRUCT(rn8209_pulse_data_block_t, 256);

/**
 * @brief 与电表常数计算后的用电量信息 (双精度浮点格式)
 *
 */
typedef struct {
  float forward_active_energy;             ///< 正向有功电能 (单位: kWh)
  float reverse_active_energy;             ///< 反向有功电能 (单位: kWh)
  float total_active_energy;               ///< 有功总电能 (单位: kWh)
  float combined_active_energy;            ///< 组合有功电能 (单位： kWh)
  float forward_reactive_energy;           ///< 正向无功电能 (单位: kvarh)
  float reverse_reactive_energy;           ///< 反向无功电能 (单位: kvarh)
  float total_reactive_energy;             ///< 无功总电能 (单位: kvarh)
  float total_reactive_quadrant_energy[4]; ///< 四象限无功电能 (单位: kvarh)
  float combined_reactive_energy_1;        ///< 组合无功1电能(单位: kvarh)
  float combined_reactive_energy_2;        ///< 组合无功2电能(单位: kvarh)
} rn8209_processed_pwr_data_t, *rn8209_processed_pwr_data_p;

typedef struct {
  bool active_power_register_first_stash_flag;   // 有功电能脉冲寄存器第一次暂存标志
  bool reactive_power_register_first_stash_flag; // 无功电能脉冲寄存器第一次暂存标志
  bool save_pluse_data_first_stash_flag;         // 保存脉冲第一次暂存标志
} rn8209_calc_cycle_flag_t, *rn8209_calc_cycle_flag_p;

typedef struct {
  meter_chip_status_e status;                                                      ///< 校准状态寄存器
  meter_chip_check_reboot_cb reboot_check_callback;                                ///< 检查复位函数
  meter_chip_sleep_cb sleep;                                                       ///< 休眠函数
  meter_chip_io_callback_t io_callback;                                            ///< 串口操作回调
  meter_chip_data_crc_cb data_crc_callback;                                        ///< 数据CRC计算回调
  meter_chip_flash_desc_t flash_desc;                                              ///< flash描述符
  meter_chip_flash_callback_t flash_callback;                                      ///< 持久化存储回调
  rn8209_preset_t preset;                                                          ///< 系统预设,硬件相关,和实例绑定
  rn8209_calibration_data_t calibration_data;                                      ///< 校准参数,从flash中读取,根据flash选型修改此结构体的对齐方式
  rn8209_origin_variable_data_t origin_variable_data;                              ///< 寄存器内部的变量数据
  rn8209_processed_variable_data_t processed_variable_data;                        ///< 浮点数 格式的变量数据
  pu_doublecheck_bitmap_t bitmap;                                                  ///< 位图
  meter_chip_get_current_traiff_cb get_current_traiff_callback;                    ///< 获取当前费率回调
  rn8209_calc_cycle_flag_t flag;                                                   ///< 初始化标志位
  rn8209_pulse_data_block_t pulse_data_block;                                      ///< 脉冲数据块
  rn8209_processed_pwr_data_t processed_pwr_data[METER_CHIP_MAX_TARIFF_COUNT + 1]; ///< 处理后的电能数据
  meter_chip_tagged_words_t tagged_words;
  uint16_t reg_checksum;
} rn8209_instance_t, *rn8209_instance_p;

typedef bool (*rn8209_calc_func_t)(rn8209_instance_t *);
typedef struct {
  rn8209_calc_func_t func;
  const char *name;
} rn8209_calc_func_entry_t;

typedef struct {
  uint16_t reg_name;         // 寄存器名称
  uext32_t *preset_data_ptr; // 对应的预设数据指针
} rn8209_reg_map_t, *rn8209_preset_map_p;

////< 内部工具函数,不向外暴露
bool rn8209_init(rn8209_instance_p instance, rn8209_preset_p preset); ///< 寄存器初始化
PU_COMPILER_WEAK bool meter_chip_exec_reboot(rn8209_instance_p instance); ///< 复位命令(弱符号,可被具体平台覆盖)
uint8_t rn8209_checksum(uint8_t *data, uint8_t length);               ///< 校验和函数
bool rn8209_check_reboot(rn8209_instance_p instance);
uint8_t rn8209_get_reg_length_by_name(uint8_t name);
uint8_t rn8209_get_reg_length_by_address(uint8_t address);
uint8_t rn8209_get_reg_name_by_address(uint8_t address);
rn8209_register_config_p rn8209_get_reg_by_name(uint8_t name);
rn8209_register_config_p rn8209_get_reg_by_address(uint8_t address);
void rn8209_print_register_by_name(rn8209_register_name_e name);
bool rn8209_get_checksum(rn8209_instance_p instance, uint16_t *result);
///< 寄存器控制函数
bool rn8209_write_enable(rn8209_instance_p instance);                                                          ///< 解除写保护
bool rn8209_write_disable(rn8209_instance_p instance);                                                         ///< 解除写保护
bool rn8209_read_register_by_address(rn8209_instance_p instance, uint8_t address, uext32_t *buffer);           ///< 通过寄存器地址读寄存器
bool rn8209_write_register_by_address(rn8209_instance_p instance, uint8_t address, uext32_t *buffer);          ///< 通过寄存器地址写寄存器
bool rn8209_read_register_by_name(rn8209_instance_p instance, rn8209_register_name_e name, uext32_t *buffer);  ///< 读寄存器
bool rn8209_write_register_by_name(rn8209_instance_p instance, rn8209_register_name_e name, uext32_t *buffer); ///< 写寄存器
///< 持久存储电能量
bool rn8209_load_pulse_cnt(rn8209_instance_p instance); ///< 开机载入脉冲计数
bool rn8209_save_pulse_cnt(rn8209_instance_p instance); ///< 保存脉冲计数
///< 计算函数
bool rn8209_calc_cycle(rn8209_instance_p instance, size_t index);
bool rn8209_calc_current(rn8209_instance_p instance);
bool rn8209_calc_voltage(rn8209_instance_p instance);
bool rn8209_calc_frequency(rn8209_instance_p instance);
bool rn8209_calc_active_power(rn8209_instance_p instance);
bool rn8209_calc_reactive_power(rn8209_instance_p instance);
bool rn8209_calc_apparent_power(rn8209_instance_p instance);
bool rn8209_calc_power_factor(rn8209_instance_p instance);
bool rn8209_calc_active_energy(rn8209_instance_p instance);
bool rn8209_calc_reactive_energy(rn8209_instance_p instance);
bool rn8209_calc_total_active_energy(rn8209_instance_p instance);
bool rn8209_calc_total_reactive_energy(rn8209_instance_p instance);
bool rn8209_calc_combined_active_energy(rn8209_instance_p instance);
bool rn8209_calc_combined_reactive_energy_1(rn8209_instance_p instance);
bool rn8209_calc_combined_reactive_energy_2(rn8209_instance_p instance);
bool rn8209_update_quadrant(rn8209_instance_p instance);
bool rn8209_update_tariff(rn8209_instance_p instance);
bool rn8209_read_reg(rn8209_instance_p instance);

///< 检查函数

// 校准函数
bool rn8209_calibration_init(void *instance);
bool rn8209_clear_energy(void *instance);                                                                                                         // 电能清零
bool rn8209_calibration_three_phase_voltage_current_conversion_coefficient(void *instance, uint32_t standard_voltage, uint32_t standard_current); ///< 电压电流转换系数
bool rn8209_calibration_three_phase_effective_value(void *instance);                                                                              ///< 电流有效值校正
bool rn8209_calibration_three_phase_active_power_gain(void *instance, uint32_t standard_voltage, uint32_t standard_current);                      ///< 校准有功功率增益
bool rn8209_calibration_three_phase_config_param(void *instance, uint32_t basic_current, uint32_t electricity_meter_constant);                    ///< 校表参数设置
bool rn8209_calibration_three_phase_active_power_offset(void *instance, uint32_t active_power);                                                   ///< AB相位有功功率offset校正
bool rn8209_calibration_three_phase_reactive_power_phase(void *instance, uint32_t active_power);                                                  ///< AB相位无功功率相位补偿
bool rn8209_calibration_three_phase_reaactive_power_offset(void *instance, uint32_t active_power);                                                ///< AB相位无功功率offset校正
bool rn8209_calibration_three_phase_active_power_phase(void *instance, uint32_t active_power);                                                    ///< AB相位有功功率相位补偿

bool rn8209_calibration_one_phase_voltage_current_conversion_coefficient(void *instance, uint8_t path, uint32_t standard_voltage, uint32_t standard_current); ///< 电压电流转换系数
bool rn8209_calibration_one_phase_effective_value(void *instance, uint8_t path);                                                                              ///< 电流有效值校正
bool rn8209_calibration_one_phase_active_power_gain(void *instance, uint8_t path, uint32_t standard_voltage, uint32_t standard_current);                      ///< 校准有功功率增益
bool rn8209_calibration_one_phase_config_param(void *instance, uint32_t basic_current, uint32_t electricity_meter_constant);                                  ///< 校表参数设置
bool rn8209_calibration_one_phase_active_power_offset(void *instance, uint8_t path, uint32_t active_power);                                                   ///< AB相位有功功率offset校正
bool rn8209_calibration_one_phase_reactive_power_phase(void *instance, uint8_t path, uint32_t active_power);                                                  ///< AB相位无功功率相位补偿
bool rn8209_calibration_one_phase_reactive_power_offset(void *instance, uint8_t path, uint32_t active_power);                                                 ///< AB相位无功功率offset校正
bool rn8209_calibration_one_phase_active_power_phase(void *instance, uint8_t path, uint32_t active_power);
bool rn8209_calibration_finish(void *instance);

///< 获取函数
bool rn8209_get_tagged_words(void *instance, uint8_t *value, uint8_t which);
bool rn8209_set_tagged_words(void *instance, uint8_t value, uint8_t which);
bool rn8209_get_tagged_words_bcd(void *instance, uint8_t *value, uint8_t which_word, uint8_t size, int conversion);
bool rn8209_get_phase_a_current(void *instance, float *value);
bool rn8209_get_phase_a_voltage(void *instance, float *value);
bool rn8209_get_phase_a_frequency(void *instance, float *value);
bool rn8209_get_phase_a_active_power(void *instance, float *value);
bool rn8209_get_phase_a_reactive_power(void *instance, float *value);
bool rn8209_get_phase_a_apparent_power(void *instance, float *value);
bool rn8209_get_phase_a_power_factor(void *instance, float *value);
bool rn8209_get_phase_a_forward_active_energy(void *instance, float *value);
bool rn8209_get_phase_a_forward_reactive_energy(void *instance, float *value);
bool rn8209_get_phase_a_reverse_active_energy(void *instance, float *value);
bool rn8209_get_phase_a_reverse_reactive_energy(void *instance, float *value);
bool rn8209_get_phase_a_total_active_energy(void *instance, float *value);
bool rn8209_get_phase_a_total_reactive_energy(void *instance, float *value);
bool rn8209_get_phase_a_combined_active_energy(void *instance, float *value);
bool rn8209_get_phase_a_combined_reactive_energy_1(void *instance, float *value);
bool rn8209_get_phase_a_combined_reactive_energy_2(void *instance, float *value);
bool rn8209_get_phase_a_quadrant_reactive_energy(void *instance, float *value, uint8_t quadrant);
// 电能量费率
bool rn8209_get_combined_active_energy_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_forward_active_energy_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_reverse_active_energy_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_combined_reactive_energy_1_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_combined_reactive_energy_2_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_quadrant_reactive_energy_by_tariff(void *instance, float *value, uint8_t tariff);
// 最大需量费率
bool rn8209_get_forward_active_max_demand_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_reverse_active_max_demand_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_combined_reactive_1_max_demand_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_combined_reactive_2_max_demand_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_quadrant_max_demand_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_forward_apparent_max_demand_by_tariff(void *instance, float *value, uint8_t tariff);
bool rn8209_get_reverse_apparent_max_demand_by_tariff(void *instance, float *value, uint8_t tariff);

bool rn8209_get_phase_a_current_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_voltage_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_frequency_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_active_power_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_reactive_power_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_apparent_power_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_power_factor_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_active_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_forward_active_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_forward_reactive_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_reverse_active_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_reverse_reactive_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_total_active_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_total_reactive_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_combined_reactive_energy_1_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_combined_reactive_energy_2_bcd(void *instance, uint8_t *value, uint8_t size, int conversion);
bool rn8209_get_phase_a_quadrant_reactive_energy_bcd(void *instance, float *value, uint8_t quadrant, uint8_t size, int conversion);
bool rn8209_get_combined_active_energy_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_forward_active_energy_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_reverse_active_energy_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_combined_reactive_energy_1_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_combined_reactive_energy_2_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_quadrant_reactive_energy_by_tariff_bcd(void *instance, float *value, uint8_t tariff, uint8_t quadrant, uint8_t size, int conversion);
bool rn8209_get_forward_active_max_demand_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_reverse_active_max_demand_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_combined_reactive_1_max_demand_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_combined_reactive_2_max_demand_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_quadrant_max_demand_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_forward_apparent_max_demand_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);
bool rn8209_get_reverse_apparent_max_demand_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion);

bool rn8209_check_checksum(rn8209_instance_p instance);
bool rn8209_check_syscon(rn8209_instance_p instance);

#endif // RN8209_DRIVER_H_
