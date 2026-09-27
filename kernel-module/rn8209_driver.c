#include "rn8209_driver.h"
#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）
#include "pu_compiler.h"
#include "pu_doublecheck_bitmap.h"
#include "pu_macro.h"
#include "pu_type.h"
#include "pu_util.h"
#ifndef __KERNEL__
#include <inttypes.h> // 用户态专用（内核无此头）
#include <math.h>     // 用户态专用；内核态 pow/fabs 由 pu_port.h 提供
#endif

static size_t communition_faild_counter = 0;

static rn8209_register_config_t register_list_init[] = {{.name = RN8209_REG_SYSCON,
                                                         .address = 0x00,
                                                         .rw = 6,   // R/W (可读可写)
                                                         .size = 2, // 16位寄存器
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0003,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "系统控制寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_EMUCON,
                                                         .address = 0x01,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0003,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "计量控制寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_HFConst,
                                                         .address = 0x02,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x1000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "脉冲频率寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_PStart,
                                                         .address = 0x03,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0060,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "有功起动功率设置,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_DStart,
                                                         .address = 0x04,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0120,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "自定义电能起动功率设置,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_GPQA,
                                                         .address = 0x05,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "通道A功率增益校正寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_GPQB,
                                                         .address = 0x06,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "通道B功率增益校正寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_PhsA,
                                                         .address = 0x07,
                                                         .rw = 6,
                                                         .size = 1,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "通道A相位校正寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_PhsB,
                                                         .address = 0x08,
                                                         .rw = 6,
                                                         .size = 1,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "通道B相位校正寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_QPhsCal,
                                                         .address = 0x09,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "无功相位补偿,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_APOSA,
                                                         .address = 0x0A,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "通道A有功功率Offset校正寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_APOSB,
                                                         .address = 0x0B,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "通道B有功功率Offset校正寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_RPOSA,
                                                         .address = 0x0C,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "通道A无功功率Offset校正寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_RPOSB,
                                                         .address = 0x0D,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "通道B无功功率Offset校正寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_IARMSOS,
                                                         .address = 0x0E,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "电流通道A有效值Offset补偿,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_IBRMSOS,
                                                         .address = 0x0F,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "电流通道B有效值Offset补偿,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_IBGain,
                                                         .address = 0x10,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "电流通道B增益设置,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_D2FPL,
                                                         .address = 0x11,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "自定义功率寄存器D2FP的低16bit,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_D2FPH,
                                                         .address = 0x12,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "自定义功率寄存器D2FP的高16bit,用户需要先写D2FPH,再写D2FPL,然后D2FP才进行电能积分,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_DCIAH,
                                                         .address = 0x13,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "IA通道直流offset校正寄存器的高16bit,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_DCIBH,
                                                         .address = 0x14,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "IB通道直流offset校正寄存器的高16bit,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_DCUH,
                                                         .address = 0x15,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "U通道直流offset校正寄存器的高16bit,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_DCL,
                                                         .address = 0x16,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "三个直流offset校正寄存器的低4bit: DCL[11:0]={DCU[3:0],DCIBL[3:0],DCIAL[3:0]},写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_EMUCON2,
                                                         .address = 0x17,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "计量控制寄存器2,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_PFCnt,
                                                         .address = 0x20,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "快速有功脉冲计数,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_DFcnt,
                                                         .address = 0x21,
                                                         .rw = 6,
                                                         .size = 2,
                                                         .type = RN8209_CALIBRATION_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "自定义电能快速脉冲计数,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_IARMS,
                                                         .address = 0x22,
                                                         .rw = 4, // 只读
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "通道A电流的有效值"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_IBRMS,
                                                         .address = 0x23,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "通道B电流的有效值"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_URMS,
                                                         .address = 0x24,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "电压有效值"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_UFreq,
                                                         .address = 0x25,
                                                         .rw = 4,
                                                         .size = 2,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "电压频率"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_PowerPA,
                                                         .address = 0x26,
                                                         .rw = 4,
                                                         .size = 4,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "有功功率A"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_PowerPB,
                                                         .address = 0x27,
                                                         .rw = 4,
                                                         .size = 4,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "有功功率B"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_PowerQ,
                                                         .address = 0x28,
                                                         .rw = 4,
                                                         .size = 4,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "无功功率"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_EnergyP,
                                                         .address = 0x29,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "有功能量,读后清零、不清零可选,默认为读后不清零,由EnergyCLR寄存器位控制"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_EnergyP2,
                                                         .address = 0x2A,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "有功能量,读后清零寄存器、冻结电能寄存器可选,默认为读后清零寄存器"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_EnergyD,
                                                         .address = 0x2B,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "无功能量或自定义能量,读后清零、不清零可选,默认为读后不清零,由EnergyCLR寄存器位控制"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_EnergyD2,
                                                         .address = 0x2C,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "无功能量或自定义能量,读后清零寄存器、冻结电能寄存器可选,默认是读后清零寄存器"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_EMUStatus,
                                                         .address = 0x2D,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00EE79,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "计量状态及校验和寄存器"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_SPL_IA,
                                                         .address = 0x30,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "IA通道ADC采样值,20位有符号数,采用补码格式,最高位为符号位。数据刷新率为14KHz"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_SPL_IB,
                                                         .address = 0x31,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "IB通道ADC采样值,20位有符号数,采用补码格式,最高位为符号位。数据刷新率为14KHz"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_SPL_U,
                                                         .address = 0x32,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "U通道ADC采样值,20位有符号数,采用补码格式,最高位为符号位。数据刷新率为14KHz"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_UFreq2,
                                                         .address = 0x35,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_MEASURE_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "电压频率寄存器2,扩展了测频范围,50Hz时读出值同UFreq(0x25H)"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_IE,
                                                         .address = 0x40,
                                                         .rw = 6,
                                                         .size = 1,
                                                         .type = RN8209_SYSTEM_REG,
                                                         .is_protect = true,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "中断允许寄存器,写保护"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_IF,
                                                         .address = 0x41,
                                                         .rw = 4,
                                                         .size = 1,
                                                         .type = RN8209_SYSTEM_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "中断标志寄存器,读后清零"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_RIF,
                                                         .address = 0x42,
                                                         .rw = 4,
                                                         .size = 1,
                                                         .type = RN8209_SYSTEM_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "复位中断状态寄存器,读后清零"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_SysStatus,
                                                         .address = 0x43,
                                                         .rw = 4,
                                                         .size = 1,
                                                         .type = RN8209_SYSTEM_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "系统状态寄存器"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_RData,
                                                         .address = 0x44,
                                                         .rw = 4,
                                                         .size = 4,
                                                         .type = RN8209_SYSTEM_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x00000000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "上一次SPI/UART读出的数据"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_WData,
                                                         .address = 0x45,
                                                         .rw = 4,
                                                         .size = 2,
                                                         .type = RN8209_SYSTEM_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x0000,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "上一次SPI/UART写入的数据"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_DeviceID,
                                                         .address = 0x7F,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_SYSTEM_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x820900,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "RN8209 Device ID"
#endif /* #ifdef RN8209_COMMENT */
                                                        },
                                                        {.name = RN8209_REG_DeviceID2,
                                                         .address = 0x7E,
                                                         .rw = 4,
                                                         .size = 3,
                                                         .type = RN8209_SYSTEM_REG,
                                                         .is_protect = false,
#ifdef RN8209_SIMULATE
                                                         .value.dword = 0x820900,
#endif /* #ifdef RN8209_SIMULATE */
#ifdef RN8209_COMMENT
                                                         .comment = "RN8209 Device ID"
#endif /* #ifdef RN8209_COMMENT */
                                                        }};

static uint8_t device_id[3] = {0x82, 0x09, 0x00};

#ifdef RN8209_SIMULATE
/**
 * @brief 运行中的配置文件
 */
static rn8209_register_config_p register_list_running = NULL;
#endif

/**
 * @brief 批量写入RN8209寄存器
 * @param instance RN8209实例指针
 * @param map 寄存器映射表
 * @param count 映射表元素数量
 * @return 成功返回true，失败返回false
 */
static bool rn8209_batch_write_registers(rn8209_instance_p instance, const rn8209_reg_map_t *map, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (rn8209_write_register_by_name(instance, (rn8209_register_name_e)map[i].reg_name, map[i].preset_data_ptr) == false) {
      PU_LOG_ERROR("初始化写入寄存器失败 错误码: %d", i);
      return false;
    }
  }
  return true;
}

/**
 * @brief 初始化
 *
 */
bool rn8209_init(rn8209_instance_p instance, rn8209_preset_p preset) {
#ifdef RN8209_SIMULATE
  if (register_list_running == NULL) {
    register_list_running = pu_malloc(sizeof(register_list_init));
    if (register_list_running == NULL) {
      return false;
    }
  }
  memcpy(register_list_running, register_list_init, sizeof(register_list_init));
#endif

  instance->flag.active_power_register_first_stash_flag = true;
  instance->flag.reactive_power_register_first_stash_flag = true;
  instance->flag.save_pluse_data_first_stash_flag = true;

  // 拷贝preset到对象中
  memcpy(&instance->preset, preset, sizeof(instance->preset));
  // 定义需要写入的寄存器列表及对应的数据指针
  rn8209_reg_map_t reg_preset_map[] = {
      {RN8209_REG_SYSCON, (uext32_t *)&instance->preset.SYSCON},   //
      {RN8209_REG_EMUCON, (uext32_t *)&instance->preset.EMUCON},   //
      {RN8209_REG_EMUCON2, (uext32_t *)&instance->preset.EMUCON2}, //
      {RN8209_REG_D2FPL, (uext32_t *)&instance->preset.D2FPL},     //
      {RN8209_REG_D2FPH, (uext32_t *)&instance->preset.D2FPH},     //
      {RN8209_REG_DCIAH, (uext32_t *)&instance->preset.DCIAH},     //
      {RN8209_REG_DCIBH, (uext32_t *)&instance->preset.DCIBH},     //
      {RN8209_REG_DCUH, (uext32_t *)&instance->preset.DCUH},       //
      {RN8209_REG_DCL, (uext32_t *)&instance->preset.DCL},         //
  };

  if (rn8209_batch_write_registers(instance, reg_preset_map, PU_GET_COUNT(reg_preset_map)) == false) {
    instance->status = METER_CHIP_STATUS_COMM_ERROR;
    return false;
  }

  uint32_t sector_size = instance->flash_desc.sector_size;

  rn8209_calibration_data_t load1, load2;

CALIBRATION_DATA_RELOAD:
  // 从flash中载入校准参数,首先检查魔数和校验和是否合法
  instance->flash_callback.read(sector_size * 0, (uint8_t *)&load1, sizeof(load1));
  instance->flash_callback.read(sector_size * 2, (uint8_t *)&load2, sizeof(load2));

  bool load1_check = false;
  bool load2_check = false;
  uint8_t retry_times = 0;

  // 计算校验和,直接套用8209的校验和公式
  size_t calibration_data_size = offsetof(rn8209_calibration_data_t, checksum);
  uint8_t load1_checksum = rn8209_checksum((uint8_t *)&load1, calibration_data_size);
  // 校验和不一致,代表校准数据出错
  if (load1.checksum == load1_checksum) {
    load1_check = true;
  }

  // 计算校验和,直接套用8209的校验和公式
  uint8_t load2_checksum = rn8209_checksum((uint8_t *)&load1, calibration_data_size);
  // 校验和不一致,代表校准数据出错
  if (load2.checksum == load2_checksum) {
    load2_check = true;
  }

  if (retry_times > RN8209_RETRY_TIMES) {
    // 校验1 校验2 均错误
    instance->status = METER_CHIP_STATUS_CALIBRATION_DATA_ERROR;
    PU_LOG_ERROR("校准数据重载错误,退出");
    return false;
  }

  // 不死鸟的加护,但是如果都被擦除了 那是真没招了
  if (load1_check == true && load2_check == false) {
    // 校验1 正确 校验2 错误
    instance->flash_callback.erase(sector_size * 2);
    instance->flash_callback.write(sector_size * 2, (uint8_t *)&load1, sizeof(load1));
    PU_LOG_ERROR("校准数据2错误");
    retry_times++;
    goto CALIBRATION_DATA_RELOAD;
  } else if (load1_check == false && load2_check == true) {
    // 校验1 错误 校验2 正确
    instance->flash_callback.erase(sector_size * 0);
    instance->flash_callback.write(sector_size * 0, (uint8_t *)&load2, sizeof(load2));
    PU_LOG_ERROR("校准数据1错误");
    retry_times++;
    goto CALIBRATION_DATA_RELOAD;
  } else if (load1_check == true && load2_check == true) {
    // 校验1 校验2 均正确 可以检查魔数之类的
  } else {
    // 校验1 校验2 均错误
    instance->status = METER_CHIP_STATUS_CALIBRATION_DATA_ERROR;
    PU_LOG_ERROR("校准数据全都错误");
    return false;
  }

  // 校准数据正确,载入校准数据,进入计量模式
  memcpy(&instance->calibration_data, &load1, sizeof(instance->calibration_data));

  // 校准参数寄存器映射表（按功能分类）
  const rn8209_reg_map_t calib_map[] = {
      // 1. 基础常数/启动阈值类校准参数
      {RN8209_REG_HFConst, (uext32_t *)&instance->calibration_data.HFConst},
      {RN8209_REG_PStart, (uext32_t *)&instance->calibration_data.PStart},
      {RN8209_REG_DStart, (uext32_t *)&instance->calibration_data.DStart},

      // 2. 功率增益类校准参数
      {RN8209_REG_GPQA, (uext32_t *)&instance->calibration_data.GPQA},
      {RN8209_REG_GPQB, (uext32_t *)&instance->calibration_data.GPQB},

      // 3. 相位校正类参数
      {RN8209_REG_PhsA, (uext32_t *)&instance->calibration_data.PhsA},
      {RN8209_REG_PhsB, (uext32_t *)&instance->calibration_data.PhsB},
      {RN8209_REG_QPhsCal, (uext32_t *)&instance->calibration_data.QPhsCal},

      // 4. 功率偏移量校正参数
      {RN8209_REG_APOSA, (uext32_t *)&instance->calibration_data.APOSA},
      {RN8209_REG_APOSB, (uext32_t *)&instance->calibration_data.APOSB},
      {RN8209_REG_RPOSA, (uext32_t *)&instance->calibration_data.RPOSA},
      {RN8209_REG_RPOSB, (uext32_t *)&instance->calibration_data.RPOSB},

      // 5. 电流测量类校准参数
      {RN8209_REG_IARMSOS, (uext32_t *)&instance->calibration_data.IARMSOS},
      {RN8209_REG_IBRMSOS, (uext32_t *)&instance->calibration_data.IBRMSOS},
      {RN8209_REG_IBGain, (uext32_t *)&instance->calibration_data.IBGain},
  };

  if (rn8209_batch_write_registers(instance, calib_map, PU_GET_COUNT(calib_map)) == false) {
    instance->status = METER_CHIP_STATUS_COMM_ERROR;
    return false;
  }

  size_t retry_count = 0;
  uint16_t reg_checksum;
  while (!rn8209_get_checksum(instance, &reg_checksum) && retry_count < RN8209_RETRY_TIMES) {
    instance->sleep(RN8209_CALIBRATION_SAMPLE_PERIOD);
  }
  instance->reg_checksum = reg_checksum;

  if (rn8209_load_pulse_cnt(instance) == true) {
    instance->status = METER_CHIP_STATUS_MEASURING;
    return true;
  } else {
    PU_LOG_ERROR("载入脉冲信息失败");
    return false;
  }
}

rn8209_register_config_p rn8209_get_reg_by_name(uint8_t name) {
  rn8209_register_config_p result = NULL;
  if (name >= RN8209_REG_COUNT) {
    return NULL;
  }
  for (size_t i = 0; i < PU_GET_COUNT(register_list_init); i++) {
#ifdef RN8209_SIMULATE
    if (register_list_running[i].name == name) {
      result = &register_list_running[i];
      break;
    }
#else
    if (register_list_init[i].name == name) {
      result = &register_list_init[i];
      break;
    }
#endif
  }
  PU_LOG_INFO("初始化成功");
  return result;
}

rn8209_register_config_p rn8209_get_reg_by_address(uint8_t address) {
  rn8209_register_config_p result = NULL;

  for (size_t i = 0; i < PU_GET_COUNT(register_list_init); i++) {
#ifdef RN8209_SIMULATE

    if (register_list_running[i].address == address) {
      result = &register_list_running[i];
      break;
    }
#else
    if (register_list_init[i].address == address) {
      result = &register_list_init[i];
      break;
    }
#endif
  }
  if (result == NULL) {
    PU_LOG_ERROR("8209 address is out of range: %d", address);
  }
  return result;
}

/**
 * @brief 解除写保护
 *
 * @param instance
 * @return true
 * @return false
 */
bool rn8209_write_enable(rn8209_instance_p instance) {
  uint8_t cmd[] = {0xEA, 0xE5, 0x30};
  uext32_t status;
  for (size_t retry = 0; retry < RN8209_RETRY_TIMES; retry++) {
    instance->io_callback.write(cmd, sizeof(cmd));
    if (rn8209_read_register_by_name(instance, RN8209_REG_SysStatus, &status)) {
      if ((status.bit.b04) == 1) {
        return true;
      } else {
        communition_faild_counter++;
        PU_LOG_WARN("8209write_enable failed");
      }
    }
  }
  PU_LOG_WARN("8209write_enable failed");
  return false;
}
/**
 * @brief 开启写保护
 *
 * @param instance
 * @return true
 * @return false
 */
bool rn8209_write_disable(rn8209_instance_p instance) {
  uint8_t cmd[] = {0xEA, 0xDC, 0x39};
  uext32_t status;
  for (size_t retry = 0; retry < RN8209_RETRY_TIMES; retry++) {
    instance->io_callback.write(cmd, sizeof(cmd));
    if (rn8209_read_register_by_name(instance, RN8209_REG_SysStatus, &status)) {
      if ((status.bit.b04) == 0) {
        return true;
      } else {
        communition_faild_counter++;
        PU_LOG_WARN("8209write_disable failed");
      }
    }
  }
  PU_LOG_WARN("8209write_disable failed");
  return false;
}

uint8_t rn8209_get_reg_name_by_address(uint8_t address) {
  for (size_t i = 0; i < PU_GET_COUNT(register_list_init); i++) {
#ifdef RN8209_SIMULATE

    if (register_list_running[i].address == address) {
      return register_list_running[i].name;
    }
#else
    if (register_list_init[i].address == address) {
      return register_list_init[i].name;
    }
#endif
  }
  PU_LOG_ERROR("8209 address is out of range: %d", address);
  return RN8209_REG_COUNT;
}

bool rn8209_read_register_by_address(rn8209_instance_p instance, uint8_t address, uext32_t *buffer) {
  buffer->dword = 0;
  uint8_t reg_length = rn8209_get_reg_length_by_address(address);
  uint8_t cmd;
  uint8_t frame_buffer[8];
  cmd = RN8209_READ_CMD(address);
  for (size_t retry = 0; retry < RN8209_RETRY_TIMES; retry++) {

    instance->io_callback.write(&cmd, sizeof(cmd));
    frame_buffer[0] = cmd;
    uint8_t read_back_count = instance->io_callback.read(frame_buffer + 1, reg_length + 1);
    if (read_back_count == 0) {
      communition_faild_counter++;
      continue;
    }
    frame_buffer[0] = cmd;
    uint8_t calc_checksum = rn8209_checksum(frame_buffer, reg_length + 1);
    if (calc_checksum == frame_buffer[reg_length + 1]) {
      memcpy(buffer->byte, frame_buffer + 1, reg_length);
      pu_reverse_byte_array(buffer->byte, reg_length);
      return true;
    } else {
      communition_faild_counter++;
      PU_LOG_WARN("8209read_register_by_address failed");
    }
  }
  PU_LOG_WARN("8209read_register_by_address failed");
  return false;
}

bool rn8209_write_register_by_address(rn8209_instance_p instance, uint8_t address, uext32_t *buffer) {

  uint8_t reg_length = rn8209_get_reg_length_by_address(address);
  rn8209_register_config_p reg_config = rn8209_get_reg_by_address(address);
  uext32_t read_back;
  uint8_t frame_buffer[8];

  if (reg_config == NULL) {
    PU_LOG_ERROR("8209 reg_config is NULL");
    return false;
  }

  if (reg_config->is_protect && reg_config->rw == 6) {
    if (rn8209_write_enable(instance) == false) {
      PU_LOG_ERROR("8209 write_enable failed");
      return false;
    }
  }
  uint8_t cmd = RN8209_WRITE_CMD(address);
  frame_buffer[0] = cmd;
  // 高字节在前
  for (size_t i = 0; i < reg_length; i++) {
    frame_buffer[i + 1] = buffer->byte[reg_length - 1 - i];
  }
  frame_buffer[reg_length + 1] = rn8209_checksum(frame_buffer, reg_length + 1);
  for (size_t retry = 0; retry < RN8209_RETRY_TIMES; retry++) {
    instance->io_callback.write(frame_buffer, reg_length + 2);
    if (!rn8209_read_register_by_address(instance, address, &read_back)) {
      communition_faild_counter++;
      continue;
    }
    if (memcmp(buffer, &read_back, reg_length) == 0) {
      break;
    }
  }

  if (reg_config->is_protect && reg_config->rw == 6)
    if (rn8209_write_disable(instance) == false) {
      PU_LOG_ERROR("8209 write_disable failed");
      return false;
    }
  return true;
}

bool rn8209_read_register_by_name(rn8209_instance_p instance, rn8209_register_name_e name, uext32_t *buffer) {
  uint8_t address;
  for (size_t i = 0; i < PU_GET_COUNT(register_list_init); i++) {
#ifdef RN8209_SIMULATE
    if (register_list_running[i].name == name) {
      address = register_list_running[i].address;
      break;
    }
#else
    if (register_list_init[i].name == name) {
      address = register_list_init[i].address;
      break;
    }
#endif
  }
  return rn8209_read_register_by_address(instance, address, buffer);
}

bool rn8209_write_register_by_name(rn8209_instance_p instance, rn8209_register_name_e name, uext32_t *buffer) {
  uint8_t address;
  for (size_t i = 0; i < PU_GET_COUNT(register_list_init); i++) {
#ifdef RN8209_SIMULATE
    if (register_list_running[i].name == name) {
      address = register_list_running[i].address;
      break;
    }
#else
    if (register_list_init[i].name == name) {
      address = register_list_init[i].address;
      break;
    }
#endif
  }
  return rn8209_write_register_by_address(instance, address, buffer);
}

uint8_t rn8209_get_reg_length_by_name(uint8_t name) {
  for (size_t i = 0; i < PU_GET_COUNT(register_list_init); i++) {
#ifdef RN8209_SIMULATE
    if (register_list_running[i].name == name) {
      return register_list_running[i].size;
    }
#else
    if (register_list_init[i].name == name) {
      return register_list_init[i].size;
    }
#endif
  }
  PU_LOG_WARN("8209 name is out of range: %d", name);
  return RN8209_REG_COUNT;
}

uint8_t rn8209_get_reg_length_by_address(uint8_t address) {
  for (size_t i = 0; i < PU_GET_COUNT(register_list_init); i++) {
#ifdef RN8209_SIMULATE

    if (register_list_running[i].address == address) {
      return register_list_running[i].size;
    }
#else
    if (register_list_init[i].address == address) {
      return register_list_init[i].size;
    }
#endif
  }
  PU_LOG_WARN("8209 address is out of range: %d", address);
  return RN8209_REG_COUNT;
}

/**
 * @brief 复位命令
 *
 * @param instance
 * @param address
 * @param value
 * @return true
 * @return false
 */
PU_COMPILER_WEAK bool meter_chip_exec_reboot(rn8209_instance_p instance) {
  if (rn8209_write_enable(instance) == false)
    return false;
  uint8_t cmd[] = {0xEA, 0xFA, 0x1B};

  uext32_t WDData_reg;
  for (size_t retry = 0; retry < RN8209_RETRY_TIMES; retry++) {
    instance->io_callback.write(cmd, sizeof(cmd));
    if (rn8209_read_register_by_name(instance, RN8209_REG_WData, &WDData_reg)) {
      return true;
    }
  }
  PU_LOG_ERROR("8209 reboot failed");
  return false;
}
/**
 * @brief 检查复位是否成功
 *
 * @param instance
 * @return true
 * @return false
 */
bool rn8209_check_reboot(rn8209_instance_p instance) {

  uext32_t sys_status_reg;
  if (rn8209_read_register_by_name(instance, RN8209_REG_SysStatus, &sys_status_reg)) {
    if (sys_status_reg.bit.b01 == 1 || sys_status_reg.bit.b00 == 1) {
      return true;
    }
  }
  PU_LOG_WARN("sys_status_reg is not valid");
  return false;
}
uint8_t rn8209_checksum(uint8_t *data, uint8_t data_lenth) {
  uint8_t checksum = 0;

  for (size_t i = 0; i < data_lenth; i++) {
    checksum += data[i];
  }
  return ~checksum;
}

bool rn8209_get_checksum(rn8209_instance_p instance, uint16_t *result) {
  uext32_t register_data;
  if (rn8209_read_register_by_name(instance, RN8209_REG_EMUStatus, &register_data) == false) {
    PU_LOG_ERROR("8209 read EMUStatus failed");
    return false;
  }

  if (register_data.bit.b16 == 1) {
    PU_LOG_ERROR("8209 EMUStatus is not valid");
    return false;
  }

  *result = register_data.word[0];
  return true;
}

bool rn8209_check_checksum(rn8209_instance_p instance) {
  if (instance->status != METER_CHIP_STATUS_MEASURING) {
    return true;
  }
  uext32_t register_data;
  if (rn8209_read_register_by_name(instance, RN8209_REG_EMUStatus, &register_data) == false) {
    PU_LOG_ERROR("8209 read EMUStatus failed");
    return false;
  }

  if (register_data.bit.b16 == 1) {
    return true;
  }
  uint16_t checksum;
  if (rn8209_get_checksum(instance, &checksum)) {
    if (checksum == instance->reg_checksum) {
      return true;
    } else {
      PU_LOG_ERROR("校验和错误");
      return false;
    }
  } else {
    PU_LOG_ERROR("获取校验和失败");
    return false;
  }
}

bool rn8209_calc_current(rn8209_instance_p instance) {
  uext32_t register_data;
  if (rn8209_read_register_by_name(instance, RN8209_REG_IARMS, &register_data) == false) {
    PU_LOG_ERROR("8209 read IARMS failed");
    return false;
  }

  instance->origin_variable_data.channel_a_current = register_data.dword;
  if (register_data.bit.b23) {
    register_data.dword = 0;
  }
  instance->processed_variable_data.channel_a_current = ((float)register_data.dword * instance->calibration_data.Kia);

  if (instance->calibration_data.basic_current * 0.004 > instance->processed_variable_data.channel_a_current) {
    instance->processed_variable_data.channel_a_current = 0;
  }
  return true;
}

bool rn8209_calc_voltage(rn8209_instance_p instance) {
  uext32_t register_data;
  if (rn8209_read_register_by_name(instance, RN8209_REG_URMS, &register_data) == false) {
    PU_LOG_ERROR("8209 read URMS failed");
    return false;
  }
  instance->origin_variable_data.voltage = register_data.dword;

  if (register_data.bit.b23) {
    register_data.dword = 0;
  }

  instance->processed_variable_data.voltage = ((float)register_data.dword * instance->calibration_data.Ku);
  return true;
}

bool rn8209_calc_frequency(rn8209_instance_p instance) {
  uext32_t register_data;
  if (rn8209_read_register_by_name(instance, RN8209_REG_UFreq, &register_data) == false) {
    PU_LOG_ERROR("8209 read UFreq failed");
    return false;
  }

  uint16_t frequency = register_data.word[0];
  instance->processed_variable_data.grid_frequency = RN8209_CLK_IN / ((float)8.0) / ((float)frequency);
  return true;
}

bool rn8209_calc_active_power(rn8209_instance_p instance) {
  uext32_t register_data;
  if (rn8209_read_register_by_name(instance, RN8209_REG_PowerPA, &register_data) == false) {
    PU_LOG_ERROR("8209 read PowerPA failed");
    return false;
  }
  instance->origin_variable_data.active_power = register_data.dword;

  int32_t active_power = register_data.s_dword;
  instance->processed_variable_data.active_power = active_power * instance->calibration_data.Kp;
  return true;
}

bool rn8209_calc_reactive_power(rn8209_instance_p instance) {
  uext32_t register_data;
  if (rn8209_read_register_by_name(instance, RN8209_REG_PowerQ, &register_data) == false) {
    PU_LOG_ERROR("8209 read PowerQ failed");
    return false;
  }
  instance->origin_variable_data.reactive_power = register_data.dword;

  int32_t reactive_power = register_data.s_dword;
  instance->processed_variable_data.reactive_power = reactive_power * instance->calibration_data.Kp;
  return true;
}

bool rn8209_calc_apparent_power(rn8209_instance_p instance) {
  instance->processed_variable_data.apparent_power = instance->processed_variable_data.voltage * instance->processed_variable_data.channel_a_current;
  return true;
}

bool rn8209_calc_power_factor(rn8209_instance_p instance) {
  if (instance->processed_variable_data.apparent_power == 0) {
    instance->processed_variable_data.power_factor = 0;
  } else {
    instance->processed_variable_data.power_factor = instance->processed_variable_data.active_power / instance->processed_variable_data.apparent_power;
  }
  return true;
}

void rn8209_print_register_by_name(rn8209_register_name_e name) {
  uint8_t reg_length = rn8209_get_reg_length_by_name(name);
  rn8209_register_config_p reg_config = rn8209_get_reg_by_name(name);
  if (reg_config == NULL) {
    PU_LOG_ERROR("8209 register config is NULL");
    return;
  }

  uint8_t log_buffer[100];
#ifdef RN8209_COMMENT
  sprintf((char *)log_buffer, "regisitor: %s\tlength: %d\tvalue: 0x", reg_config->comment, reg_config->size);
#endif /* #ifdef RN8209_COMMENT */

  for (size_t i = 0; i < reg_length; i++) {
#ifdef RN8209_SIMULATE
    uint8_t reg_value_buffer[10];
    sprintf((char *)reg_value_buffer, "%02X", reg_config->value.byte[reg_length - 1 - i]);
    strcat((char *)log_buffer, (const char *)reg_value_buffer);
#endif /* #ifdef RN8209_SIMULATE */
  }
  PU_LOG_INFO("%s\r", log_buffer);
}

bool rn8209_load_pulse_cnt(rn8209_instance_p instance) {

  uint8_t backup_data_reload_count = 0;
  static rn8209_pulse_data_block_t test_data;
  uint8_t rollback_times;

  // 提取flash_desc中的变量
  uint32_t sector_size = instance->flash_desc.sector_size;
  uint32_t sector_count = instance->flash_desc.sector_count;

  size_t storage_data_size = sizeof(instance->pulse_data_block);
  size_t sector_data_block_count = sector_size / storage_data_size;
  uint32_t bit_count = sector_data_block_count * (sector_count - 4);

  uint32_t byte_count = (bit_count + 7) / 8;
  uint8_t *primary_bitmap_memory = pu_malloc(byte_count);
  if (primary_bitmap_memory == NULL) {
    return false;
  }
  uint8_t *secondary_bitmap_memory = pu_malloc(byte_count);
  if (secondary_bitmap_memory == NULL) {
    return false;
  }
// BITMAP_RELOAD:
  instance->flash_callback.read(sector_size * 1, primary_bitmap_memory, byte_count);
  instance->flash_callback.read(sector_size * 3, secondary_bitmap_memory, byte_count);

  pu_doublecheck_bitmap_load(&(instance->bitmap), bit_count, primary_bitmap_memory, secondary_bitmap_memory);
  int32_t index = pu_doublecheck_bitmap_find_set_bit(&(instance->bitmap));
BACKUP_DATA_RELOAD:
  if (index > 0) {
    index--;
  } else if (index < 0) {
    // 小于0
    index = instance->bitmap.max_bits - 1;
  }
  // 等于0 时不处理
  // 找到上一个为0的bitmap
  rollback_times = 0;
  // 如果不一致,说明在操作flash时断电,需要向前遍历
  while (!pu_doublecheck_bitmap_test_bit_consistency(&(instance->bitmap), index)) {
    rollback_times++;
    if (rollback_times > 3) {
      return false;
    }
    // 跳过坏块
    index--;
    if (index < 0) {
      index = instance->bitmap.max_bits - 1;
    }
  }
  int32_t sector_offset = (index / sector_data_block_count); // 扇区偏移
  int32_t data_offset = (index % sector_data_block_count);   // 数据偏移
  instance->flash_callback.read(sector_size * 4 + (sector_size * sector_offset) + (storage_data_size * data_offset), (uint8_t *)&test_data, storage_data_size);

  if (instance->data_crc_callback != NULL) {
    uint32_t crc = instance->data_crc_callback((uint8_t *)&test_data, sizeof(test_data) - 4);
    if (crc != test_data.crc) {
      PU_LOG_ERROR("载入计量数据错误");
      backup_data_reload_count++;
      if (backup_data_reload_count < 3) {
        index--;
        goto BACKUP_DATA_RELOAD;
      } else {
        // 计量数据错误
        instance->status = METER_CHIP_STATUS_PLUSE_DATA_BLOCK_ERROR;
        return false;
      }
    } else {
      if (test_data.version != RN8209_CURRENT_CALIBRATION_DATA_BLOCK_VERSION) {
        // TODO 判断版本号,做版本迁移
        PU_LOG_ERROR("计量数据版本不一致");
        instance->status = METER_CHIP_STATUS_PLUSE_DATA_BLOCK_ERROR;
        return false;
      } else {
        memcpy(&instance->pulse_data_block, &test_data, sizeof(test_data));
      }
    }
  }

  return true;
}

bool rn8209_save_pulse_cnt(rn8209_instance_p instance) {
  if (instance->status != METER_CHIP_STATUS_MEASURING) {
    return false;
  }

  static rn8209_pulse_data_block_t test_data;
  static rn8209_pulse_data_block_t stash_data;

  if (instance->flag.save_pluse_data_first_stash_flag == true) {
    instance->flag.save_pluse_data_first_stash_flag = false;
    memcpy(&stash_data, &instance->pulse_data_block, sizeof(rn8209_pulse_data_block_t));
  }

  // 如果电能量没有累计则不存储,避免没有用电浪费flash
  if (memcmp(&stash_data, &instance->pulse_data_block, sizeof(rn8209_pulse_data_block_t) - 4) == 0) {
    return true;
  }
  memcpy(&stash_data, &instance->pulse_data_block, sizeof(rn8209_pulse_data_block_t) - 4);

  if (instance->data_crc_callback != NULL) {
    uint32_t crc = instance->data_crc_callback((uint8_t *)&stash_data, sizeof(stash_data) - 4);
    stash_data.crc = crc;
  } else {
    stash_data.crc = RN8209_ERROR_CRC;
  }
  // 提取flash_desc中的变量
  uint32_t sector_size = instance->flash_desc.sector_size;

  size_t storage_data_size = sizeof(rn8209_pulse_data_block_t);
  size_t sector_data_block_count = sector_size / storage_data_size;
  int32_t index = pu_doublecheck_bitmap_find_set_bit(&(instance->bitmap));

  // 如果找不到可用块,擦除全部分区
  if (index < 0) {
    instance->flash_callback.erase(sector_size * 1);
    instance->flash_callback.erase(sector_size * 3);

    instance->flash_callback.read(sector_size * 1, instance->bitmap.primary_buffer, instance->bitmap.buffer_size_bytes);
    instance->flash_callback.read(sector_size * 3, instance->bitmap.secondary_buffer, instance->bitmap.buffer_size_bytes);

    pu_doublecheck_bitmap_load(&(instance->bitmap), instance->bitmap.max_bits, instance->bitmap.primary_buffer, instance->bitmap.secondary_buffer);
    index = pu_doublecheck_bitmap_find_set_bit(&(instance->bitmap));
  }
  int32_t sector_offset = (index / sector_data_block_count); // 扇区偏移
  int32_t data_offset = (index % sector_data_block_count);   // 数据偏移
  int32_t byte_offset = index / 8;
  int32_t bit_offset = index % 8;
  uint8_t data_byte = 0xFF >> (bit_offset + 1);
TEST_DATA_RELOAD:
  instance->flash_callback.read(sector_size * (4 + sector_offset) + (storage_data_size * data_offset), (uint8_t *)&test_data, storage_data_size);
  for (size_t i = 0; i < storage_data_size; i++) {
    if (((uint8_t *)&test_data)[i] != 0xFF) {
      // 数据不一致,说明当前块是脏块,因为在载入过程中会筛选由于写入坏块导致的数据不一致,所以这里直接擦除当前块,并重新载入数据
      instance->flash_callback.erase(sector_size * (4 + sector_offset));
      // 脏数据块
      PU_LOG_ERROR("saving pluse data found dirty sector");
      goto TEST_DATA_RELOAD;
    }
  }
  instance->flash_callback.write(sector_size * 1 + byte_offset, &data_byte, 1);
  instance->flash_callback.write(sector_size * 4 + (sector_size * sector_offset) + (storage_data_size * data_offset), (uint8_t *)&stash_data, storage_data_size);
  instance->flash_callback.write(sector_size * 3 + byte_offset, &data_byte, 1); // fix: 偏移错误
  pu_doublecheck_bitmap_clear_bit(&instance->bitmap, index);
  return true;
}

// 电能清零
bool rn8209_clear_energy(void *instance) {
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {
    PU_LOG_ERROR("8209 instance status is not MEASURING or CALIBRATING");
    return false;
  }

  // 移除所有校准参数，擦除校准区域
  uint32_t sector_size = rn8209_instance->flash_desc.sector_size;
  uint32_t sector_count = rn8209_instance->flash_desc.sector_count;

  static rn8209_pulse_data_block_t data;
  size_t data_size = sizeof(data);
  data.version = RN8209_CURRENT_CALIBRATION_DATA_BLOCK_VERSION;
#if RN8209_CURRENT_CALIBRATION_DATA_BLOCK_VERSION == 1
  memset(data.pulse_data, 0, sizeof(data.pulse_data));
  memset(data.reverse, 0xFF, sizeof(data.reverse));  
  // 计算校验和
  if (rn8209_instance->data_crc_callback != NULL) {
    uint32_t crc = rn8209_instance->data_crc_callback((uint8_t *)&data, data_size - 4);
    PU_LOG_ERROR("电表清零 数据校验和 %08X", crc);
    data.crc = crc;
  } else {
    data.crc = RN8209_ERROR_CRC;
  }
#else
#error 请根据实际情况修改脉冲数据块初始化数据
#endif
  uint8_t data_byte = 0xFF >> 1;

  // 擦除1 3 扇区的位图数据
  rn8209_instance->flash_callback.erase(sector_size * 1);
  rn8209_instance->flash_callback.erase(sector_size * 3);
  // 后4个扇区数据清零
  for (size_t i = 0; i < sector_count - 4; i++) {
    rn8209_instance->flash_callback.erase(sector_size * (4 + i));
  }
  // 写入1 3 扇区位图, 为1字节0b0111_1111,表示写入了第一块数据
  rn8209_instance->flash_callback.write(sector_size * 1, &data_byte, 1);
  rn8209_instance->flash_callback.write(sector_size * 3, &data_byte, 1);
  // 第一块数据全0
  rn8209_instance->flash_callback.write(sector_size * 4, (uint8_t *)&data, data_size);
  // 载入
  if (rn8209_load_pulse_cnt(rn8209_instance) == true) {
    return true;
  } else {
    PU_LOG_ERROR("载入脉冲信息失败");
    return false;
  }
}

/**
 * @brief 校准初始化
 *
 * @param instance
 * @return true
 * @return false
 */
bool rn8209_calibration_init(void *instance) {
  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;
  rn8209_instance->status = METER_CHIP_STATUS_CALIBRATING;
  // 移除所有校准参数，擦除校准区域
  uint32_t sector_size = rn8209_instance->flash_desc.sector_size;
  uint32_t sector_count = rn8209_instance->flash_desc.sector_count;

  rn8209_instance->flash_callback.erase(sector_size * 0);
  rn8209_instance->flash_callback.erase(sector_size * 1);
  rn8209_instance->flash_callback.erase(sector_size * 2);
  rn8209_instance->flash_callback.erase(sector_size * 3);
  for (size_t i = 0; i < sector_count - 4; i++) {
    rn8209_instance->flash_callback.erase(sector_size * (4 + i));
  }

  uext32_t empty_data;
  empty_data.dword = 0;

  memset(&rn8209_instance->calibration_data, 0x00, sizeof(rn8209_instance->calibration_data));
  uint8_t retry_times = RN8209_RETRY_TIMES;
  retry_times--;
  if (retry_times == 0) {
    rn8209_instance->status = METER_CHIP_STATUS_COMM_ERROR;
    return false;
  }
  // 构建空数据的寄存器映射表（所有寄存器都指向同一个empty_data）
  const rn8209_reg_map_t empty_calib_map[] = {
      {RN8209_REG_HFConst, &empty_data}, //
      {RN8209_REG_PStart, &empty_data},  //
      {RN8209_REG_DStart, &empty_data},  //
      {RN8209_REG_GPQA, &empty_data},    //
      {RN8209_REG_GPQB, &empty_data},    //
      {RN8209_REG_PhsA, &empty_data},    //
      {RN8209_REG_PhsB, &empty_data},    //
      {RN8209_REG_QPhsCal, &empty_data}, //
      {RN8209_REG_APOSA, &empty_data},   //
      {RN8209_REG_APOSB, &empty_data},   //
      {RN8209_REG_RPOSA, &empty_data},   //
      {RN8209_REG_RPOSB, &empty_data},   //
      {RN8209_REG_IARMSOS, &empty_data}, //
      {RN8209_REG_IBRMSOS, &empty_data}, //
      {RN8209_REG_IBGain, &empty_data},  //
  };
  if (rn8209_batch_write_registers(rn8209_instance, empty_calib_map, PU_GET_COUNT(empty_calib_map)) == false) {
    PU_LOG_ERROR("8209 batch write registers failed");
    return false;
  }
  return true;
}

/**
 * @brief 配置参数
 *
 * @param instance
 * @param basic_current 基本电流
 * @param electricity_meter_constant 电表常量
 * @return true
 * @return false
 */
bool rn8209_calibration_one_phase_config_param(void *instance, uint32_t basic_current, uint32_t electricity_meter_constant) {
  // 基础电流值（转换后）
  uint32_t converted_basic_current = (uint32_t)(basic_current / METER_CHIP_DRIVER_PARAM_COEFFICIENT);
  // 临时计算变量
  uint32_t hf_const_calc_value = 0;
  // 启动阈值
  float startup_threshold_value = 0;
  // 寄存器值：高频常数、有功启动、无功启动
  uext16_t hf_const_reg_value, p_start_reg_value, d_start_reg_value;
  // RN8209实例指针（类型转换）
  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;

  // 空指针校验
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  // 基础电流值为0校验
  if (converted_basic_current == 0) {
    PU_LOG_ERROR("8209 basic current is 0");
    return false;
  }

  // 寄存器缓存：电流有效值、电压有效值、系统配置
  static uext32_t i_rms_reg_value;
  static uext32_t u_rms_reg_value;
  static uext32_t sys_con_reg_value;

  // 读取电流有效值寄存器
  if (rn8209_read_register_by_name(rn8209_instance, RN8209_REG_IARMS, &i_rms_reg_value) == false) {
    PU_LOG_ERROR("8209 read register IARMS failed");
    return false;
  }

  // 读取电压有效值寄存器
  if (rn8209_read_register_by_name(rn8209_instance, RN8209_REG_URMS, &u_rms_reg_value) == false) {
    PU_LOG_ERROR("8209 read register URMS failed");
    return false;
  }

  // 读取系统配置寄存器
  if (rn8209_read_register_by_name(rn8209_instance, RN8209_REG_SYSCON, &sys_con_reg_value) == false) {
    PU_LOG_ERROR("8209 read register SYSCON failed");
    return false;
  }

  // 计算A相电流通道增益倍数（从SYSCON寄存器低2位获取）
  uint8_t current_a_channel_gain = 1;
  switch (sys_con_reg_value.dword & 0x0003) {
  case 0:
    current_a_channel_gain = 1;
    break;
  case 1:
    current_a_channel_gain = 2;
    break;
  case 2:
    current_a_channel_gain = 8;
    break;
  case 3:
    current_a_channel_gain = 16;
    break;
  }

  // 计算电压通道增益倍数（从SYSCON寄存器2-3位获取）
  uint8_t voltage_channel_gain = 1;
  switch ((sys_con_reg_value.dword >> 2) & 0x0003) {
  case 0:
    voltage_channel_gain = 1;
    break;
  case 1:
    voltage_channel_gain = 2;
    break;
  case 2:
    voltage_channel_gain = 4;
    break;
  case 3:
    voltage_channel_gain = 4;
    break;
  }

#if defined(RN8209_INVERSE_CALC)
  // 反向计算模式：从寄存器值计算Vu/Vi
  static float voltage_sample_value;
  voltage_sample_value = ((float)i_rms_reg_value.dword / POW2_23);
  static float current_sample_value;
  current_sample_value = ((float)u_rms_reg_value.dword / POW2_23);
#else
  // RN8209 官方手册公式：HFConst=INT[16.1079*Vu*Vi*10^11/(EC*Un*Ib)]
  float voltage_sample_value = (float)rn8209_instance->preset.uv * (float)voltage_channel_gain;
  float current_sample_value = (float)rn8209_instance->preset.ui * (float)converted_basic_current * current_a_channel_gain;
#endif
  PU_LOG_INFO("配置参数 电压增益倍数 %d", voltage_channel_gain);
  PU_LOG_INFO("配置参数 A通道电流增益倍数 %d", current_a_channel_gain);

  PU_LOG_INFO("配置参数 电压通道ADC采样电压有效值 %.17lf", ((float)u_rms_reg_value.dword / voltage_channel_gain) / POW2_23);

  // 计量常数、额定电压、基准电流（浮点型转换）
  float meter_constant = (float)electricity_meter_constant;
  float rated_voltage = (float)220.0;
  float reference_current = (float)converted_basic_current;

  // 计算HFConst寄存器值
  hf_const_calc_value = (uint32_t)(((float)16.1079 * voltage_sample_value * current_sample_value * POW10_11) / (meter_constant * rated_voltage * reference_current));

  // HFConst寄存器值超出16位范围校验
  if (hf_const_calc_value > 0xFFFF) {
    PU_LOG_ERROR("8209 HFConst value is out of range");
    return false;
  }

  // 写入HFConst寄存器
  hf_const_reg_value.word = (uint16_t)hf_const_calc_value;
  if (rn8209_write_register_by_name(rn8209_instance, RN8209_REG_HFConst, (uext32_t *)&hf_const_reg_value) == false) {
    PU_LOG_ERROR("8209 write register HFConst failed");
    return false;
  }

  // 计算有功功率系数Kp
  rn8209_instance->calibration_data.Kp = ((float)3.22155 * POW10_12) / (POW2_32 * (float)hf_const_reg_value.word * (float)electricity_meter_constant);

  // 计算启动阈值
  startup_threshold_value = 0.8 * (float)converted_basic_current * (float)0.004 * (float)220.0 / rn8209_instance->calibration_data.Kp / (float)POW2_8;

  // 赋值有功/无功启动寄存器值
  p_start_reg_value.word = (uint16_t)startup_threshold_value;
  d_start_reg_value.word = (uint16_t)startup_threshold_value;

  // 写入有功/无功启动寄存器（注意：原代码逻辑存在&&错误，已保留原逻辑）
  if (rn8209_write_register_by_name(rn8209_instance, RN8209_REG_PStart, (uext32_t *)&p_start_reg_value) == false || rn8209_write_register_by_name(rn8209_instance, RN8209_REG_DStart, (uext32_t *)&d_start_reg_value) == false) {
    return false;
  }

  // 保存校准数据到实例结构体
  rn8209_instance->calibration_data.basic_current = (uint16_t)converted_basic_current;
  rn8209_instance->calibration_data.meter_const = (uint16_t)electricity_meter_constant;
  rn8209_instance->calibration_data.HFConst.word = hf_const_reg_value.word;
  rn8209_instance->calibration_data.PStart.word = p_start_reg_value.word;
  rn8209_instance->calibration_data.DStart.word = d_start_reg_value.word;

  if (rn8209_calibration_one_phase_effective_value(instance, RN8209_PATH_A) && rn8209_calibration_one_phase_effective_value(instance, RN8209_PATH_B)) {
    return true;
  } else {
    return false;
  }
}

/**
 * @brief 有功功率增益 Ib 1.0 功率因数
 *
 * @param instance
 * @param path
 * @param standard_voltage 标准电压
 * @param standard_current 标准电路
 * @return true
 * @return false
 */
bool rn8209_calibration_one_phase_active_power_gain(void *instance, uint8_t path, uint32_t standard_voltage, uint32_t standard_current) {
  float S_sample, S_stand;
  static float standard_voltage_ref, standard_current_ref;
  standard_voltage_ref = ((float)standard_voltage) / METER_CHIP_DRIVER_PARAM_COEFFICIENT;
  standard_current_ref = ((float)standard_current) / METER_CHIP_DRIVER_PARAM_COEFFICIENT;
#if RN8209_CALIBRATION_ACTIVE_POWER_GAIN_AVG
  uint32_t current_reg_sample_temp[RN8209_CALIBRATION_AVG_TIMES];
  uint32_t voltage_reg_sample_temp[RN8209_CALIBRATION_AVG_TIMES];
  uint32_t current_reg_sample_sum = 0;
  uint32_t voltage_reg_sample_sum = 0;
#endif
  uext32_t register_data_voltage;
  uext32_t register_data_current;

  float err, pGain = 0;
  uint16_t GPQx;

  // 声明寄存器枚举变量
  rn8209_register_name_e current_reg_enum;
  rn8209_register_name_e gpq_reg_enum;
  rn8209_register_name_e power_reg_enum;

  uext32_t active_power_calculate_register_data;
  uext32_t reactive_power_calculate_register_data;

  float current_avg;
  float voltage_avg;

  if (instance == NULL) {
    return false;
  }
  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;
  uext32_t clear_gpq = {0};

  // 集中赋值：根据path统一配置寄存器和校准数据存储地址，保持逻辑完全一致
  if ((rn8209_path_e)path == RN8209_PATH_A) {
    PU_LOG_INFO(">>>> 有功功率增益校正 A通道");

    current_reg_enum = RN8209_REG_IARMS;
    gpq_reg_enum = RN8209_REG_GPQA;
    power_reg_enum = RN8209_REG_PowerPA;
  } else {
    PU_LOG_INFO(">>>> 有功功率增益校正 B通道");

    current_reg_enum = RN8209_REG_IBRMS;
    gpq_reg_enum = RN8209_REG_GPQB;
    power_reg_enum = RN8209_REG_PowerPB;
  }

  if (rn8209_write_register_by_name(instance, gpq_reg_enum, &clear_gpq) == false) {
    PU_LOG_ERROR("8209 write register GPQA failed");
    return false;
  }

  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);

  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &active_power_calculate_register_data) == false || rn8209_read_register_by_name(rn8209_instance, RN8209_REG_PowerQ, &reactive_power_calculate_register_data) == false) {
    PU_LOG_ERROR("8209 read register PowerPA or PowerQ failed");
    return false;
  }
  PU_LOG_INFO(">>>> 有功功率增益校正 校准前有功功率 %.17lf", active_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO(">>>> 有功功率增益校正 校准后无功功率 %.17lf", reactive_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);

#if RN8209_CALIBRATION_ACTIVE_POWER_GAIN_AVG
  for (int i = 0; i < RN8209_CALIBRATION_AVG_TIMES; i++) {
    if (rn8209_read_register_by_name(instance, RN8209_REG_URMS, &register_data_voltage) == false || rn8209_read_register_by_name(instance, current_reg_enum, &register_data_current) == false) {
      PU_LOG_ERROR("8209 read register URMS or IARMS failed");
      return false;
    }
    if (register_data_current.s_dword <= 0 || register_data_voltage.s_dword <= 0) {
      PU_LOG_ERROR("8209 register value is out of range");
      return false;
    }
    current_reg_sample_temp[i] = register_data_current.dword;
    voltage_reg_sample_temp[i] = register_data_voltage.dword;
    rn8209_instance->sleep(RN8209_CALIBRATION_SAMPLE_PERIOD);
  }

  for (int i = 0; i < RN8209_CALIBRATION_AVG_TIMES; i++) {
    current_reg_sample_sum += current_reg_sample_temp[i];
    voltage_reg_sample_sum += voltage_reg_sample_temp[i];
  }

  current_avg = (float)current_reg_sample_sum / RN8209_CALIBRATION_AVG_TIMES;
  voltage_avg = (float)voltage_reg_sample_sum / RN8209_CALIBRATION_AVG_TIMES;
#else
  if (rn8209_read_register_by_name(instance, RN8209_REG_URMS, &register_data_voltage) == false || rn8209_read_register_by_name(instance, current_reg_enum, &register_data_current) == false) {
    return false;
  }
  if (register_data_current.s_dword <= 0 || register_data_voltage.s_dword <= 0) {
    return false;
  }
  current_avg = register_data_current.dword;
  voltage_avg = register_data_voltage.dword;
#endif

  S_sample = (current_avg / POW2_23) * (voltage_avg / POW2_23);
  S_stand = standard_current_ref * standard_voltage_ref / rn8209_instance->calibration_data.Kp / POW2_31;
  err = (S_sample - S_stand) / S_stand;
  pGain = ((float)1.0 + err);
  pGain = -err / pGain;

  PU_LOG_INFO("有功功率增益校正 标准表电流转换值 %.17lf", standard_current_ref);
  PU_LOG_INFO("有功功率增益校正 标准表电压转换值 %.17lf", standard_voltage_ref);
  PU_LOG_INFO("有功功率增益校正 电流平均值转换值 %.17lf", current_avg);
  PU_LOG_INFO("有功功率增益校正 电压平均值转换值 %.17lf", voltage_avg);
  PU_LOG_INFO("有功功率增益校正 标准表视在功率 %.17lf", S_stand);
  PU_LOG_INFO("有功功率增益校正 计量芯片采样视在功率 %.17lf", S_sample);
  PU_LOG_INFO("有功功率增益校正 误差 %.17lf", err);
  PU_LOG_INFO("有功功率增益校正 增益 %.17lf", pGain);

  if (pGain > 0) {
    GPQx = (uint16_t)(pGain * POW2_15);
  } else {
    GPQx = (uint16_t)((pGain * POW2_15) + POW2_16);
  }
  uext32_t active_power_gain_reg_value;
  active_power_gain_reg_value.word[0] = (uint16_t)GPQx;

  if (rn8209_write_register_by_name(instance, gpq_reg_enum, &active_power_gain_reg_value) == false) {
    return false;
  }

  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);
  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &active_power_calculate_register_data) == false || rn8209_read_register_by_name(rn8209_instance, RN8209_REG_PowerQ, &reactive_power_calculate_register_data) == false) {
    return false;
  }
  PU_LOG_INFO(">>>> 有功功率增益校正 校准后有功功率 %.17lf", active_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO(">>>> 有功功率增益校正 校准后无功功率 %.17lf", reactive_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);

  if ((rn8209_path_e)path == RN8209_PATH_A) {
    rn8209_instance->calibration_data.GPQA.word = GPQx;
  } else {
    rn8209_instance->calibration_data.GPQB.word = GPQx;
  }

  return rn8209_calibration_one_phase_voltage_current_conversion_coefficient(instance, path, standard_voltage, standard_current);
}

/**
 * @brief 有功相位校正 Ib 0.5L 功率因数
 *
 * @param instance
 * @param path
 * @param active_power
 * @return true
 * @return false
 */
bool rn8209_calibration_one_phase_active_power_phase(void *instance, uint8_t path, uint32_t active_power) {
  // 声明寄存器枚举变量，统一通道A/B的寄存器映射
  rn8209_register_name_e phs_reg_enum;
  rn8209_register_name_e power_reg_enum;

  uext32_t reg_data;
  float active_power_sample_avg;
  float sin_value;
  static float active_power_ref;
#if RN8209_CALIBRATION_ACTIVE_POWER_PHASE_AVG
  int32_t active_power_reg_sample_temp[RN8209_CALIBRATION_AVG_TIMES];
  int32_t active_power_reg_sample_sum = 0;
#endif
  active_power_ref = ((float)active_power) / METER_CHIP_DRIVER_PARAM_COEFFICIENT;
  static float error_rate;
  uext32_t clear_phs_reg = {0};
  static float active_power_standard;
  float radian;
  float degree;
  float phase_correction_value;
  uext32_t phase_reg_val;
  uint8_t phase_correction_val;
  uext32_t active_power_calculate_register_data;
  uext32_t reactive_power_calculate_register_data;

  if (instance == NULL) {
    return false;
  }
  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;

  // 集中赋值：根据path统一配置寄存器和校准数据存储地址，消除分支冗余
  if ((rn8209_path_e)path == RN8209_PATH_A) {
    PU_LOG_INFO(">>>> 有功相位校正 A通道");

    phs_reg_enum = RN8209_REG_PhsA;
    power_reg_enum = RN8209_REG_PowerPA;
  } else {
    PU_LOG_INFO(">>>> 有功相位校正 B通道");

    phs_reg_enum = RN8209_REG_PhsB;
    power_reg_enum = RN8209_REG_PowerPB;
  }

  if (rn8209_write_register_by_name(rn8209_instance, phs_reg_enum, &clear_phs_reg) == false) {
    PU_LOG_ERROR("8209 write register PhsA or PhsB failed");
    return false;
  }

  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);
  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &active_power_calculate_register_data) == false || rn8209_read_register_by_name(rn8209_instance, RN8209_REG_PowerQ, &reactive_power_calculate_register_data) == false) {
    PU_LOG_ERROR("8209 read register PowerPA or PowerQ failed");
    return false;
  }
  PU_LOG_INFO(">>>> 有功相位校正 标准有功功率 %.17lf", active_power_ref);
  PU_LOG_INFO(">>>> 有功相位校正 校准前有功功率 %.17lf", active_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO(">>>> 有功相位校正 校准前无功功率 %.17lf", reactive_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);

  active_power_standard = active_power_ref / rn8209_instance->calibration_data.Kp;
#if RN8209_CALIBRATION_ACTIVE_POWER_PHASE_AVG
  for (int i = 0; i < RN8209_CALIBRATION_AVG_TIMES; i++) {
    if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &reg_data) == false) {
      return false;
    }
    active_power_reg_sample_temp[i] = reg_data.s_dword;
    rn8209_instance->sleep(RN8209_CALIBRATION_SAMPLE_PERIOD);
  }

  for (int i = 0; i < RN8209_CALIBRATION_AVG_TIMES; i++) {
    active_power_reg_sample_sum += active_power_reg_sample_temp[i];
  }

  active_power_sample_avg = (float)active_power_reg_sample_sum / RN8209_CALIBRATION_AVG_TIMES;
#else
  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &reg_data) == false) {
    PU_LOG_ERROR("8209 read register PowerPA or PowerPB failed");
    return false;
  }
  active_power_sample_avg = reg_data.s_dword;
#endif

  error_rate = (active_power_sample_avg - active_power_standard) / active_power_standard;
  sin_value = -error_rate / 1.732;
  radian = asin(sin_value);
  degree = radian * 180 / METER_CHIP_DRIVER_PI;
  phase_correction_value = degree / 0.02;

  if (phase_correction_value > 0) {
    phase_correction_val = (uint8_t)phase_correction_value;
  } else {
    phase_correction_val = (uint8_t)(phase_correction_value + POW2_8);
  }

  PU_LOG_INFO("有功相位校正 标准有功功率 %.17lf", active_power_standard);
  PU_LOG_INFO("有功相位校正 采样有功功率 %.17lf", active_power_sample_avg);
  PU_LOG_INFO("有功相位校正 误差%.17lf", error_rate);
  PU_LOG_INFO("有功相位校正 误差弧度%.17lf", radian);
  PU_LOG_INFO("有功相位校正 误差角度%.17lf", degree);
  PU_LOG_INFO("有功相位校正 误差校正值%.17lf", phase_correction_value);
  PU_LOG_INFO("有功相位校正 寄存器设定值%d 0x%02X", phase_correction_val, phase_correction_val);
  PU_LOG_INFO("有功相位校正 有功功率转换系数Kp %.17lf", rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO("有功相位校正 [结果]:标准功率*Kp %.17lf 校准前测量功率*Kp %.17lf", active_power_standard * rn8209_instance->calibration_data.Kp, active_power_sample_avg * rn8209_instance->calibration_data.Kp);

  phase_reg_val.byte[0] = phase_correction_val;
  if (rn8209_write_register_by_name(instance, phs_reg_enum, &phase_reg_val) == false) {
    PU_LOG_ERROR("8209 write register PhsA or PhsB failed");
    return false;
  }

  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);

  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &reg_data) == false) {
    PU_LOG_ERROR("8209 read register PowerPA or PowerPB failed");
    return false;
  }
  PU_LOG_INFO(">>>> 有功相位校正 校准后采样功率 %.17lf", reg_data.s_dword * rn8209_instance->calibration_data.Kp);

  if (rn8209_read_register_by_name(instance, phs_reg_enum, &phase_reg_val) == false) {
    PU_LOG_ERROR("8209 read register PhsA or PhsB failed");
    return false;
  }
  PU_LOG_INFO(">>>> 有功相位校正 校准后寄存器读回数据 %d 0x%02X", phase_reg_val.byte[0], phase_reg_val.byte[0]);

  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);
  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &active_power_calculate_register_data) == false || rn8209_read_register_by_name(rn8209_instance, RN8209_REG_PowerQ, &reactive_power_calculate_register_data) == false) {
    PU_LOG_ERROR("8209 read register PowerPA or PowerQ failed");
    return false;
  }
  PU_LOG_INFO(">>>> 有功相位校正 标准有功功率 %.17lf", active_power_ref);
  PU_LOG_INFO(">>>> 有功相位校正 校准后有功功率 %.17lf", active_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO(">>>> 有功相位校正 校准后无功功率 %.17lf", reactive_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);

  if ((rn8209_path_e)path == RN8209_PATH_A) {
    rn8209_instance->calibration_data.PhsA.byte = phase_correction_val;
  } else {
    rn8209_instance->calibration_data.PhsB.byte = phase_correction_val;
  }

  return true;
}
/**
 * @brief 有功功率偏置校正 5%Ib 0.5L 功率因数
 *
 * @param instance
 * @param path
 * @param active_power
 * @return true
 * @return false
 */
bool rn8209_calibration_one_phase_active_power_offset(void *instance, uint8_t path, uint32_t active_power) {
  // 1. 声明寄存器枚举/数据指针，统一通道A/B映射
  rn8209_register_name_e apos_reg_enum;
  rn8209_register_name_e power_reg_enum;

  // 2. 语义化命名变量，保持原有计算逻辑
  uext32_t active_power_reg_data;
  int32_t active_power_reg_sample_temp[RN8209_CALIBRATION_OFFSET_AVG_TIMES];
  int32_t active_power_reg_sample_sum = 0;
  float active_power_sample_avg;
  float active_power_standard;
  float active_power_ref = ((float)active_power) / METER_CHIP_DRIVER_PARAM_COEFFICIENT;
  float offset_calc;
  uint16_t offset_val;
  uext32_t clear_apos_reg = {0};
  uext16_t apos_reg_val;

  uext32_t active_power_calculate_register_data;
  uext32_t reactive_power_calculate_register_data;

  if (fabs(active_power_ref) < 1e-10) { // 避免除零
    PU_LOG_INFO(">>>> 有功功率偏置校正 标准功率为0 无法校正");
    return false;
  }
  // 空指针校验
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;

  // 3. 集中赋值：统一通道A/B的寄存器和数据映射，消除冗余分支
  if ((rn8209_path_e)path == RN8209_PATH_A) {
    PU_LOG_INFO(">>>> 有功功率偏置校正 A通道");

    apos_reg_enum = RN8209_REG_APOSA;
    power_reg_enum = RN8209_REG_PowerPA;
  } else {
    PU_LOG_INFO(">>>> 有功功率偏置校正 B通道");

    apos_reg_enum = RN8209_REG_APOSB;
    power_reg_enum = RN8209_REG_PowerPB;
  }

  if (rn8209_write_register_by_name(instance, apos_reg_enum, &clear_apos_reg) == false) {
    return false;
  }
  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);

  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &active_power_calculate_register_data) == false || rn8209_read_register_by_name(rn8209_instance, RN8209_REG_PowerQ, &reactive_power_calculate_register_data) == false) {
    return false;
  }
  PU_LOG_INFO(">>>> 有功功率偏置校正 标准有功功率 %.17lf", active_power_ref);
  PU_LOG_INFO(">>>> 有功功率偏置校正 校准前有功功率 %.17lf", active_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO(">>>> 有功功率偏置校正 校准前无功功率 %.17lf", reactive_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);

  for (int i = 0; i < RN8209_CALIBRATION_OFFSET_AVG_TIMES; i++) {
    if (rn8209_read_register_by_name(instance, power_reg_enum, &active_power_reg_data) == false) {
      PU_LOG_ERROR("8209 read register PowerPA or PowerPB failed");
      return false;
    }
    active_power_reg_sample_temp[i] = active_power_reg_data.s_dword;
    rn8209_instance->sleep(RN8209_CALIBRATION_SAMPLE_PERIOD);
  }

  for (int i = 0; i < RN8209_CALIBRATION_OFFSET_AVG_TIMES; i++) {
    active_power_reg_sample_sum += active_power_reg_sample_temp[i];
  }
  active_power_sample_avg = (float)active_power_reg_sample_sum / RN8209_CALIBRATION_OFFSET_AVG_TIMES;
  active_power_standard = active_power_ref / rn8209_instance->calibration_data.Kp;

  offset_calc = (float)(active_power_sample_avg - active_power_standard) / (float)active_power_standard;
  offset_calc = ((float)active_power_standard * (-offset_calc));
  PU_LOG_INFO("有功功率偏置校正 有功误差 %.17lf", offset_calc);

  if (offset_calc > 0) {
    offset_val = (uint16_t)offset_calc;
  } else {
    offset_val = (uint16_t)(offset_calc + POW2_16);
  }

  apos_reg_val.word = offset_val;

  if (rn8209_write_register_by_name(instance, apos_reg_enum, (uext32_t *)&apos_reg_val) == false) {
    return false;
  }
  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);
  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &active_power_calculate_register_data) == false || rn8209_read_register_by_name(rn8209_instance, RN8209_REG_PowerQ, &reactive_power_calculate_register_data) == false) {
    return false;
  }
  PU_LOG_INFO(">>>> 有功功率增益校正 标准有功功率 %.17lf", active_power_ref);
  PU_LOG_INFO(">>>> 有功功率增益校正 校准后有功功率 %.17lf", active_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO(">>>> 有功功率增益校正 校准后无功功率 %.17lf", reactive_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);

  if ((rn8209_path_e)path == RN8209_PATH_A) {
    rn8209_instance->calibration_data.APOSA.word = offset_val;
  } else {
    rn8209_instance->calibration_data.APOSB.word = offset_val;
  }

  return true;
}

/**
 * @brief 无功相位校正 Ib 0.5L 功率因数
 *
 * @param instance
 * @param path
 * @param active_power
 * @return true
 * @return false
 */
// bool rn8209_calibration_one_phase_reactive_power_phase(void *instance, uint8_t path, uint32_t active_power, uint32_t reactive_power) {
bool rn8209_calibration_one_phase_reactive_power_phase(void *instance, uint8_t path, uint32_t reactive_power) {
  // 1. 声明寄存器枚举变量，统一通道A/B的功率寄存器映射
  rn8209_register_name_e power_reg_enum;
  // 语义化命名变量，保持原有计算逻辑
  uext32_t active_power_reg_data;
  uext32_t reactive_power_reg_data;
#if RN8209_CALIBRATION_REACTIVE_POWER_PHASE_AVG
  int32_t active_power_reg_sample_temp[RN8209_CALIBRATION_AVG_TIMES];
  int32_t reactive_power_reg_sample_temp[RN8209_CALIBRATION_AVG_TIMES];
  int32_t active_power_reg_sample_sum = 0;
  int32_t reactive_power_reg_sample_sum = 0;
#endif
  float active_power_sample_avg;
  float reactive_power_sample_avg;
  // float active_power_ref = ((float)active_power) / METER_CHIP_DRIVER_PARAM_COEFFICIENT;
  float active_power_ref = 1902.13; // 10A测试
  float reactive_power_ref = ((float)reactive_power) / METER_CHIP_DRIVER_PARAM_COEFFICIENT;
  // float reactive_power_ref = 952.6; // 5A测试
  // float reactive_power_ref = 1905.76; // 10A测试
  float error_rate;
  float offical_error_rate;
  float active_power_calc;
  float reactive_power_calc;
  uint8_t reactive_phase_val;
  uext32_t clear_qphscal_reg = {0};

  uext32_t active_power_calculate_register_data;
  uext32_t reactive_power_calculate_register_data;

  // 空指针校验
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;

  // 2. 集中赋值：根据path配置功率寄存器枚举，消除采样分支冗余
  if ((rn8209_path_e)path == RN8209_PATH_A) {
    PU_LOG_INFO(">>>> 无功相位校正 A通道");
    power_reg_enum = RN8209_REG_PowerPA;
  } else {
    PU_LOG_INFO(">>>> 无功相位校正 B通道");
    power_reg_enum = RN8209_REG_PowerPA;
  }

  if (rn8209_write_register_by_name(instance, RN8209_REG_QPhsCal, &clear_qphscal_reg) == false) {
    PU_LOG_ERROR("8209 write register QPhsCal failed");
    return false;
  }

  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);

  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &active_power_calculate_register_data) == false || rn8209_read_register_by_name(rn8209_instance, RN8209_REG_PowerQ, &reactive_power_calculate_register_data) == false) {
    PU_LOG_ERROR("8209 read register PowerPA or PowerQ failed");
    return false;
  }
  PU_LOG_INFO(">>>> 无功相位校正 标准无功功率 %.17lf", active_power_ref);
  PU_LOG_INFO(">>>> 无功相位校正 校准前有功功率 %.17lf", active_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO(">>>> 无功相位校正 校准前无功功率 %.17lf", reactive_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);

#if RN8209_CALIBRATION_REACTIVE_POWER_PHASE_AVG
  for (int i = 0; i < RN8209_CALIBRATION_AVG_TIMES; i++) {
    if (rn8209_read_register_by_name(instance, power_reg_enum, &active_power_reg_data) == false || rn8209_read_register_by_name(instance, RN8209_REG_PowerQ, &reactive_power_reg_data) == false) {
      return false;
    }
    active_power_reg_sample_temp[i] = active_power_reg_data.s_dword;
    reactive_power_reg_sample_temp[i] = reactive_power_reg_data.s_dword;
    rn8209_instance->sleep(RN8209_CALIBRATION_SAMPLE_PERIOD);
  }

  for (int i = 0; i < RN8209_CALIBRATION_AVG_TIMES; i++) {
    active_power_reg_sample_sum += active_power_reg_sample_temp[i];
    reactive_power_reg_sample_sum += reactive_power_reg_sample_temp[i];
  }
  active_power_sample_avg = (float)active_power_reg_sample_sum / RN8209_CALIBRATION_AVG_TIMES;
  reactive_power_sample_avg = (float)reactive_power_reg_sample_sum / RN8209_CALIBRATION_AVG_TIMES;
#else
  if (rn8209_read_register_by_name(instance, power_reg_enum, &active_power_reg_data) == false || rn8209_read_register_by_name(instance, RN8209_REG_PowerQ, &reactive_power_reg_data) == false) {
    return false;
  }
  active_power_sample_avg = active_power_reg_data.s_dword;
  reactive_power_sample_avg = reactive_power_reg_data.s_dword;

#endif
  active_power_calc = active_power_sample_avg * rn8209_instance->calibration_data.Kp;
  reactive_power_calc = reactive_power_sample_avg * rn8209_instance->calibration_data.Kp;

  PU_LOG_INFO("无功相位校正 标准有功功率 %.17lf", active_power_ref);
  PU_LOG_INFO("无功相位校正 标准无功功率 %.17lfw", reactive_power_ref);

  PU_LOG_INFO("无功相位校正 采样有功功率 %.17lf", active_power_calc);
  PU_LOG_INFO("无功相位校正 采样无功功率 %.17lfw", reactive_power_calc);

  // error_rate = (active_power_calc - active_power_ref) / active_power_ref;
  // PU_LOG_INFO("无功相位校正 应用手册计算误差 %.17lf", error_rate);

  offical_error_rate = (reactive_power_calc - reactive_power_ref) / reactive_power_ref;
  PU_LOG_INFO("无功相位校正 官方代码计算误差 %.17lf", offical_error_rate);

  // error_rate = error_rate * (float)0.5774;
  error_rate = offical_error_rate * (float)0.5774;

  if (error_rate > 0) {
    reactive_phase_val = (uint16_t)(error_rate * POW2_15);
  } else {
    reactive_phase_val = (uint16_t)((error_rate * POW2_15) + POW2_16);
  }

  uext16_t qphs_reg_val;
  uint16_t offset_uint = (uint16_t)reactive_phase_val;
  qphs_reg_val.word = offset_uint;
  if (rn8209_write_register_by_name(instance, RN8209_REG_QPhsCal, (uext32_t *)&qphs_reg_val) == false) {
    PU_LOG_ERROR("8209 write register QPhsCal failed");
    return false;
  }
  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);

  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &active_power_calculate_register_data) == false || rn8209_read_register_by_name(rn8209_instance, RN8209_REG_PowerQ, &reactive_power_calculate_register_data) == false) {
    PU_LOG_ERROR("8209 read register PowerPA or PowerQ failed");
    return false;
  }
  PU_LOG_INFO(">>>> 无功相位校正 标准无功功率 %.17lf", active_power_ref);
  PU_LOG_INFO(">>>> 无功相位校正 校准后有功功率 %.17lf", active_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO(">>>> 无功相位校正 校准后无功功率 %.17lf", reactive_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);

  rn8209_instance->calibration_data.QPhsCal.word = offset_uint;

  return true;
}

/**
 * @brief 无功功率偏置校正 5%Ib 0.5L 功率因数
 *
 * @param instance
 * @param path
 * @param active_power
 * @return true
 * @return false
 */
bool rn8209_calibration_one_phase_reactive_power_offset(void *instance, uint8_t path, uint32_t reactive_power) {
  // 1. 声明寄存器枚举/数据指针，统一通道A/B映射
  rn8209_register_name_e power_reg_enum;
  rn8209_register_name_e rpos_reg_enum; // Rective Power Offset Register

  // 2. 语义化命名变量，保持原有计算逻辑
  uext32_t reactive_power_reg_data;
  int32_t reactive_power_reg_sample_temp[RN8209_CALIBRATION_OFFSET_AVG_TIMES];
  int32_t reactive_power_reg_sample_sum = 0;
  float reactive_power_sample_avg;
  float reactive_power_standard;
  float reactive_power_ref = ((float)reactive_power) / METER_CHIP_DRIVER_PARAM_COEFFICIENT;
  float offset_calc;
  // float gpq_normalization_val;
  // uext16_t gpqx;
  uint16_t offset_val;
  uext32_t clear_apos_reg = {0};
  uext16_t apos_reg_val;
  uext32_t active_power_calculate_register_data;
  uext32_t reactive_power_calculate_register_data;
  // 空指针校验
  if (instance == NULL) {
    return false;
  }
  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;

  // 3. 集中赋值：统一通道A/B的寄存器和数据映射，消除冗余分支
  if ((rn8209_path_e)path == RN8209_PATH_A) {
    PU_LOG_INFO(">>>> 无功功率偏置校正 A通道");
    power_reg_enum = RN8209_REG_PowerPA;
    rpos_reg_enum = RN8209_REG_RPOSA;
  } else {
    PU_LOG_INFO(">>>> 无功功率偏置校正 B通道");
    power_reg_enum = RN8209_REG_PowerPB;
    rpos_reg_enum = RN8209_REG_RPOSB;
  }

  if (rn8209_write_register_by_name(instance, rpos_reg_enum, &clear_apos_reg) == false) {
    PU_LOG_ERROR("8209 write register RPOSA or RPOSB failed");
    return false;
  }
  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);

  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &active_power_calculate_register_data) == false || rn8209_read_register_by_name(rn8209_instance, RN8209_REG_PowerQ, &reactive_power_calculate_register_data) == false) {
    PU_LOG_ERROR("8209 read register PowerPA or PowerQ failed");
    return false;
  }
  PU_LOG_INFO(">>>> 无功功率偏置校正 标准无功功率 %.17lf", reactive_power_ref);
  PU_LOG_INFO(">>>> 无功功率偏置校正 校准前有功功率 %.17lf", active_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO(">>>> 无功功率偏置校正 校准前无功功率 %.17lf", reactive_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);

  for (int i = 0; i < RN8209_CALIBRATION_OFFSET_AVG_TIMES; i++) {
    if (rn8209_read_register_by_name(instance, RN8209_REG_PowerQ, &reactive_power_reg_data) == false) {
      return false;
    }
    reactive_power_reg_sample_temp[i] = reactive_power_reg_data.s_dword;
    rn8209_instance->sleep(RN8209_CALIBRATION_SAMPLE_PERIOD);
  }

  for (int i = 0; i < RN8209_CALIBRATION_OFFSET_AVG_TIMES; i++) {
    reactive_power_reg_sample_sum += reactive_power_reg_sample_temp[i];
  }
  reactive_power_sample_avg = (float)reactive_power_reg_sample_sum / RN8209_CALIBRATION_OFFSET_AVG_TIMES;
  reactive_power_standard = reactive_power_ref / rn8209_instance->calibration_data.Kp;

  offset_calc = (float)(reactive_power_sample_avg - reactive_power_standard) / (float)reactive_power_standard;
  offset_calc = ((float)reactive_power_standard * (-offset_calc));
  PU_LOG_INFO("无功功率偏置校正 无功误差 %.17lf", offset_calc);

  if (offset_calc > 0) {
    offset_val = (uint16_t)offset_calc;
  } else {
    offset_val = (uint16_t)(offset_calc + POW2_16);
  }

  apos_reg_val.word = offset_val;

  if (rn8209_write_register_by_name(instance, rpos_reg_enum, (uext32_t *)&apos_reg_val) == false) {
    PU_LOG_ERROR("8209 write register RPOSA or RPOSB failed");
    return false;
  }

  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);

  if (rn8209_read_register_by_name(rn8209_instance, power_reg_enum, &active_power_calculate_register_data) == false || rn8209_read_register_by_name(rn8209_instance, RN8209_REG_PowerQ, &reactive_power_calculate_register_data) == false) {
    PU_LOG_ERROR("8209 read register PowerPA or PowerQ failed");
    return false;
  }

  PU_LOG_INFO(">>>> 无功功率偏置校正 标准无功功率 %.17lf", reactive_power_ref);
  PU_LOG_INFO(">>>> 无功功率偏置校正 校准前有功功率 %.17lf", active_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);
  PU_LOG_INFO(">>>> 无功功率偏置校正 校准前无功功率 %.17lf", reactive_power_calculate_register_data.s_dword * rn8209_instance->calibration_data.Kp);

  if ((rn8209_path_e)path == RN8209_PATH_A) {
    rn8209_instance->calibration_data.RPOSA.word = offset_val;
  } else {
    rn8209_instance->calibration_data.RPOSB.word = offset_val;
  }
  return true;
}

/**
 * @brief 电流有效值校正 仅电压 无电流
 *
 * @param instance
 */
bool rn8209_calibration_one_phase_effective_value(void *instance, uint8_t path) {
  // 声明寄存器枚举变量，统一通道A/B的寄存器映射
  rn8209_register_name_e current_os_reg_enum;
  rn8209_register_name_e current_rms_reg_enum;

  uext32_t reg_data;
  uint32_t current_reg_sample_temp[RN8209_CALIBRATION_AVG_TIMES];
  uint32_t current_reg_sample_sum = 0;
  uint32_t current_reg_sample_avg;
  uext16_t current_os_reg_val;
  uext32_t clear_current_os_reg = {0};
  uint16_t offset;

  // 空指针校验
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;

  // 集中赋值：根据path统一配置寄存器和校准数据存储地址，消除分支冗余
  if ((rn8209_path_e)path == RN8209_PATH_A) {
    current_os_reg_enum = RN8209_REG_IARMSOS;
    current_rms_reg_enum = RN8209_REG_IARMS;
  } else {
    current_os_reg_enum = RN8209_REG_IBRMSOS;
    current_rms_reg_enum = RN8209_REG_IBRMS;
  }

  if (rn8209_write_register_by_name(rn8209_instance, current_os_reg_enum, &clear_current_os_reg) == false) {
    PU_LOG_ERROR("8209 write register IARMSOS or IBBRMSOS failed");
    return false;
  }
  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);
  if (rn8209_read_register_by_name(rn8209_instance, current_rms_reg_enum, &reg_data) == false) {
    PU_LOG_ERROR("8209 read register IARMS or IBBRMS failed");
    return false;
  }
  PU_LOG_INFO(">>>> 电流有效值校正 校准前采样电流寄存器值 %06X", reg_data.dword);

  for (int i = 0; i < RN8209_CALIBRATION_AVG_TIMES; i++) {
    if (rn8209_read_register_by_name(rn8209_instance, current_rms_reg_enum, &reg_data) == false) {
      PU_LOG_ERROR("8209 read register IARMS or IBBRMS failed");
      return false;
    }
    current_reg_sample_temp[i] = reg_data.dword;
    rn8209_instance->sleep(RN8209_CALIBRATION_SAMPLE_PERIOD);
  }

  for (int i = 0; i < RN8209_CALIBRATION_AVG_TIMES; i++) {
    current_reg_sample_sum += current_reg_sample_temp[i];
  }

  current_reg_sample_avg = current_reg_sample_sum / RN8209_CALIBRATION_AVG_TIMES;
  current_reg_sample_avg = current_reg_sample_avg * current_reg_sample_avg;
  current_reg_sample_avg = ~current_reg_sample_avg;
  offset = (uint16_t)(current_reg_sample_avg >> 8);

  current_os_reg_val.word = offset;
  if (rn8209_write_register_by_name(instance, current_os_reg_enum, (uext32_t *)&current_os_reg_val) == false) {
    PU_LOG_ERROR("8209 write register IARMSOS or IBBRMSOS failed");
    return false;
  }

  rn8209_instance->sleep(2 * RN8209_CALIBRATION_SAMPLE_PERIOD);
  if (rn8209_read_register_by_name(rn8209_instance, current_rms_reg_enum, &reg_data) == false) {
    PU_LOG_ERROR("8209 read register IARMS or IBBRMS failed");
    return false;
  }
  PU_LOG_INFO(">>>> 电流有效值校正 采样电流寄存器值 %06X", reg_data.dword);

  if ((rn8209_path_e)path == RN8209_PATH_A) {
    rn8209_instance->calibration_data.IARMSOS.word = offset;
  } else {
    rn8209_instance->calibration_data.IBRMSOS.word = offset;
  }
  return true;
}

/**
 * @brief 电压、电流转换系数 Ib
 *
 * @param instance
 * @param standard_voltage
 * @param standard_current
 * @return true
 * @return false
 */
bool rn8209_calibration_one_phase_voltage_current_conversion_coefficient(void *instance, uint8_t path, uint32_t standard_voltage, uint32_t standard_current) {
  float standard_voltage_ref = ((float)standard_voltage) / METER_CHIP_DRIVER_PARAM_COEFFICIENT;
  float standard_current_ref = ((float)standard_current) / METER_CHIP_DRIVER_PARAM_COEFFICIENT;

  uint32_t current_reg_sample_temp[RN8209_CALIBRATION_AVG_TIMES];
  uint32_t voltage_reg_sample_temp[RN8209_CALIBRATION_AVG_TIMES];

  uext32_t register_data_voltage;
  uext32_t register_data_current;

  uint32_t current_reg_sample_sum = 0;
  uint32_t voltage_reg_sample_sum = 0;

  if (instance == NULL) {
    return false;
  }
  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;
  rn8209_register_name_e current_i_reg;
  if ((rn8209_path_e)path == RN8209_PATH_A) {
    current_i_reg = RN8209_REG_IARMS;
  } else {
    current_i_reg = RN8209_REG_IBRMS;
  }
  for (int i = 0; i < RN8209_CALIBRATION_AVG_TIMES; i++) {
    if (rn8209_read_register_by_name(instance, RN8209_REG_URMS, &register_data_voltage) == false || rn8209_read_register_by_name(instance, current_i_reg, &register_data_current) == false) {
      return false;
    }
    if (register_data_current.s_dword <= 0 || register_data_voltage.s_dword <= 0) {
      PU_LOG_INFO("电压/电流采样数据无效 重新校准");
      return false;
    }
    current_reg_sample_temp[i] = register_data_current.dword;
    voltage_reg_sample_temp[i] = register_data_voltage.dword;
    rn8209_instance->sleep(RN8209_CALIBRATION_SAMPLE_PERIOD);
  }

  for (int i = 0; i < RN8209_CALIBRATION_AVG_TIMES; i++) {
    current_reg_sample_sum += current_reg_sample_temp[i];
    voltage_reg_sample_sum += voltage_reg_sample_temp[i];
  }

  float current_avg = (float)current_reg_sample_sum / RN8209_CALIBRATION_AVG_TIMES;
  float voltage_avg = (float)voltage_reg_sample_sum / RN8209_CALIBRATION_AVG_TIMES;

  rn8209_instance->calibration_data.Ku = standard_voltage_ref / voltage_avg;

  if ((rn8209_path_e)path == RN8209_PATH_A) {
    rn8209_instance->calibration_data.Kia = standard_current_ref / current_avg;
  } else {
    rn8209_instance->calibration_data.Kib = standard_current_ref / current_avg;
  }

  return true;
}

bool rn8209_calibration_finish(void *instance) {
  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;
  // 提取flash_desc中的变量
  uint32_t sector_size = rn8209_instance->flash_desc.sector_size;

  memcpy(rn8209_instance->calibration_data.magic_number, device_id, sizeof(device_id));

  // 计算校验和,直接套用8209的校验和公式
  size_t calibration_data_size = offsetof(rn8209_calibration_data_t, checksum);
  uint8_t checksum = rn8209_checksum((uint8_t *)&rn8209_instance->calibration_data, calibration_data_size);
  rn8209_instance->calibration_data.checksum = checksum;

  rn8209_instance->flash_callback.write(sector_size * 0, (uint8_t *)&rn8209_instance->calibration_data, sizeof(rn8209_instance->calibration_data));
  rn8209_instance->flash_callback.write(sector_size * 2, (uint8_t *)&rn8209_instance->calibration_data, sizeof(rn8209_instance->calibration_data));

  // 打印魔数（16进制格式，对应0x820900）
  PU_LOG_INFO("========== 电表校准参数 ==========");
  PU_LOG_INFO("魔数(magic_number)      : 0x%02X%02X%02X", rn8209_instance->calibration_data.magic_number[0], rn8209_instance->calibration_data.magic_number[1], rn8209_instance->calibration_data.magic_number[2]);

  // 整数类型参数（十进制）
  PU_LOG_INFO("基本电流(basic_current) : %d", rn8209_instance->calibration_data.basic_current);
  PU_LOG_INFO("电表常数(meter_const)   : %d", rn8209_instance->calibration_data.meter_const);
  PU_LOG_INFO("常量计算值(HFConst)     : %04X", rn8209_instance->calibration_data.HFConst.word);
  PU_LOG_INFO("有功启动功率(PStart)    : %04X", rn8209_instance->calibration_data.PStart.word);
  PU_LOG_INFO("无功启动功率(DStart)    : %04X", rn8209_instance->calibration_data.DStart.word);
  PU_LOG_INFO("A通道功率增益(GPQA)     : %04X", rn8209_instance->calibration_data.GPQA.word);
  PU_LOG_INFO("B通道功率增益(GPQB)     : %04X", rn8209_instance->calibration_data.GPQB.word);
  PU_LOG_INFO("A通道相位校正(PhsA)     : %02X", rn8209_instance->calibration_data.PhsA.byte);
  PU_LOG_INFO("B通道相位校正(PhsB)     : %02X", rn8209_instance->calibration_data.PhsB.byte);
  PU_LOG_INFO("无功相位校正(QPhsCal)   : %04X", rn8209_instance->calibration_data.QPhsCal.word);
  PU_LOG_INFO("A通道有功offset(APOSA)  : %04X", rn8209_instance->calibration_data.APOSA.word);
  PU_LOG_INFO("B通道有功offset(APOSB)  : %04X", rn8209_instance->calibration_data.APOSB.word);
  PU_LOG_INFO("A通道无功offset(RPOSA)  : %04X", rn8209_instance->calibration_data.RPOSA.word);
  PU_LOG_INFO("B通道无功offset(RPOSB)  : %04X", rn8209_instance->calibration_data.RPOSB.word);
  PU_LOG_INFO("A通道电流有效值offset   : %04X", rn8209_instance->calibration_data.IARMSOS.word);
  PU_LOG_INFO("B通道电流有效值offset   : %04X", rn8209_instance->calibration_data.IBRMSOS.word);
  PU_LOG_INFO("B通道电流增益(IBGain)   : %04X", rn8209_instance->calibration_data.IBGain.word);

  // 浮点类型参数（保留6位小数，适配精度需求）
  PU_LOG_INFO("电压转换系数(Ku)        : %.17lf", rn8209_instance->calibration_data.Ku);
  PU_LOG_INFO("A通道电流转换系数(Kia)  : %.17lf", rn8209_instance->calibration_data.Kia);
  PU_LOG_INFO("B通道电流转换系数(Kib)  : %.17lf", rn8209_instance->calibration_data.Kib);
  PU_LOG_INFO("有功功率转换系数(Kp)    : %.17lf", rn8209_instance->calibration_data.Kp);

  // 校验和（16进制）
  PU_LOG_INFO("校准参数校验和(checksum)        : 0x%02X", rn8209_instance->calibration_data.checksum);
  PU_LOG_INFO("==================================");

  // 构建校准参数寄存器映射表（与原始代码一一对应）
  const rn8209_reg_map_t calib_map[] = {
      {RN8209_REG_HFConst, (uext32_t *)&rn8209_instance->calibration_data.HFConst}, //
      {RN8209_REG_PStart, (uext32_t *)&rn8209_instance->calibration_data.PStart},   //
      {RN8209_REG_DStart, (uext32_t *)&rn8209_instance->calibration_data.DStart},   //
      {RN8209_REG_GPQA, (uext32_t *)&rn8209_instance->calibration_data.GPQA},       //
      {RN8209_REG_GPQB, (uext32_t *)&rn8209_instance->calibration_data.GPQB},       //
      {RN8209_REG_PhsA, (uext32_t *)&rn8209_instance->calibration_data.PhsA},       //
      {RN8209_REG_PhsB, (uext32_t *)&rn8209_instance->calibration_data.PhsB},       //
      {RN8209_REG_QPhsCal, (uext32_t *)&rn8209_instance->calibration_data.QPhsCal}, //
      {RN8209_REG_APOSA, (uext32_t *)&rn8209_instance->calibration_data.APOSA},     //
      {RN8209_REG_APOSB, (uext32_t *)&rn8209_instance->calibration_data.APOSB},     //
      {RN8209_REG_RPOSA, (uext32_t *)&rn8209_instance->calibration_data.RPOSA},     //
      {RN8209_REG_RPOSB, (uext32_t *)&rn8209_instance->calibration_data.RPOSB},     //
      {RN8209_REG_IARMSOS, (uext32_t *)&rn8209_instance->calibration_data.IARMSOS}, //
      {RN8209_REG_IBRMSOS, (uext32_t *)&rn8209_instance->calibration_data.IBRMSOS}, //
      {RN8209_REG_IBGain, (uext32_t *)&rn8209_instance->calibration_data.IBGain},   //
  };

  if (rn8209_batch_write_registers(rn8209_instance, calib_map, PU_GET_COUNT(calib_map)) == false) {
    rn8209_instance->status = METER_CHIP_STATUS_COMM_ERROR;
    return false;
  }

  static rn8209_pulse_data_block_t data;
  size_t data_size = sizeof(data);
  
  data.version = RN8209_CURRENT_CALIBRATION_DATA_BLOCK_VERSION;
#if RN8209_CURRENT_CALIBRATION_DATA_BLOCK_VERSION == 1
  memset(data.pulse_data, 0, sizeof(data.pulse_data));
  memset(data.reverse, 0xFF, sizeof(data.reverse)); 
  // 计算校验和
  if (rn8209_instance->data_crc_callback != NULL) {
    uint32_t crc = rn8209_instance->data_crc_callback((uint8_t *)&data, data_size - 4);
    PU_LOG_INFO("校准完成 数据校验和 %08X", crc);
    data.crc = crc;
  } else {
    data.crc = RN8209_ERROR_CRC;
  }
#else
#error 请根据实际情况修改脉冲数据块初始化数据
#endif
  uint8_t data_byte = 0xFF >> 1;

  rn8209_instance->flash_callback.write(sector_size * 1, &data_byte, 1);
  rn8209_instance->flash_callback.write(sector_size * 4, (uint8_t *)&data, data_size);
  rn8209_instance->flash_callback.write(sector_size * 3, &data_byte, 1);

  uint16_t reg_checksum;
  size_t retry_count = 0;
  while (!rn8209_get_checksum(rn8209_instance, &reg_checksum) && retry_count < RN8209_RETRY_TIMES) {
    // 重试读取校验和
    retry_count++;
  }
  if (retry_count == RN8209_RETRY_TIMES) {
    rn8209_instance->status = METER_CHIP_STATUS_COMM_ERROR;
    return false;
  }
  rn8209_instance->reg_checksum = reg_checksum;
  if (rn8209_load_pulse_cnt(rn8209_instance) == true) {
    rn8209_instance->status = METER_CHIP_STATUS_BOOTING;
    return true;
  } else {
    PU_LOG_ERROR("载入脉冲信息失败");
    return false;
  }
}

bool rn8209_calc_active_energy(rn8209_instance_p instance) {
  uext32_t energy_p_register_data;
  uext32_t emustatus_register_data;

  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  if (rn8209_read_register_by_name(instance, RN8209_REG_EnergyP, &energy_p_register_data) == false) {
    PU_LOG_ERROR("8209 read register EnergyP failed");
    return false;
  }

  if (rn8209_read_register_by_name(instance, RN8209_REG_EMUStatus, &emustatus_register_data) == false) {
    PU_LOG_ERROR("8209 read register EMUStatus failed");
    return false;
  }

  uint32_t add_pulse_count = 0;

  if (instance->preset.EMUCON.bit.b15 == 1) {
    // 读后清零型
    add_pulse_count = energy_p_register_data.dword;
  } else {
    // 累加型
    static uint32_t last_pulse_count = 0;
    if (instance->flag.active_power_register_first_stash_flag == true) {
      instance->flag.active_power_register_first_stash_flag = false;
      last_pulse_count = energy_p_register_data.dword;
    }

    if (instance->processed_variable_data.read_reg_interrupt_flag == true && instance->processed_variable_data.reg_interrupt_flag.bit.b03 == 1) {
      // 发生溢出
      add_pulse_count += POW2_24 - last_pulse_count;
      add_pulse_count += energy_p_register_data.dword;
    } else {
      // 未发生溢出
      add_pulse_count += energy_p_register_data.dword - last_pulse_count;
    }
    last_pulse_count = energy_p_register_data.dword;
  }

  if (emustatus_register_data.bit.b17 == 0) {
    instance->pulse_data_block.pulse_data[0].forward_active_pulse_count += add_pulse_count;
    if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
      instance->pulse_data_block.pulse_data[instance->processed_variable_data.tariff].forward_active_pulse_count += add_pulse_count;
    }

    instance->processed_variable_data.activate_power_dir = METER_CHIP_ENERGY_DIR_FORWARD;
  } else {
    instance->pulse_data_block.pulse_data[0].reverse_active_pulse_count += add_pulse_count;
    if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
      instance->pulse_data_block.pulse_data[instance->processed_variable_data.tariff].reverse_active_pulse_count += add_pulse_count;
    }
    instance->processed_variable_data.activate_power_dir = METER_CHIP_ENERGY_DIR_REVERSE;
  }
  instance->processed_pwr_data[0].forward_active_energy = (float)(instance->pulse_data_block.pulse_data[0].forward_active_pulse_count) / (float)instance->calibration_data.meter_const;
  instance->processed_pwr_data[0].reverse_active_energy = (float)(instance->pulse_data_block.pulse_data[0].reverse_active_pulse_count) / (float)instance->calibration_data.meter_const;

  if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
    instance->processed_pwr_data[instance->processed_variable_data.tariff].forward_active_energy = (float)(instance->pulse_data_block.pulse_data[instance->processed_variable_data.tariff].forward_active_pulse_count) / (float)instance->calibration_data.meter_const;
    instance->processed_pwr_data[instance->processed_variable_data.tariff].reverse_active_energy = (float)(instance->pulse_data_block.pulse_data[instance->processed_variable_data.tariff].reverse_active_pulse_count) / (float)instance->calibration_data.meter_const;
  }

  return true;
}

bool rn8209_calc_reactive_energy(rn8209_instance_p instance) {
  uext32_t energy_d_register_data;
  uext32_t emustatus_register_data;

  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  if (rn8209_read_register_by_name(instance, RN8209_REG_EnergyD, &energy_d_register_data) == false) {
    PU_LOG_ERROR("8209 read register EnergyD failed");
    return false;
  }

  if (rn8209_read_register_by_name(instance, RN8209_REG_EMUStatus, &emustatus_register_data) == false) {
    PU_LOG_ERROR("8209 read register EMUStatus failed");
    return false;
  }
  uint32_t add_pulse_count = 0;
  static uint32_t last_pulse_count = 0;

  if (instance->flag.reactive_power_register_first_stash_flag == true) {
    instance->flag.reactive_power_register_first_stash_flag = false;
    last_pulse_count = energy_d_register_data.dword;
  }

  if (instance->preset.EMUCON.bit.b15 == 1) {
    // 读后清零型
    add_pulse_count = energy_d_register_data.dword;
  } else {
    // 累加型
    if (instance->processed_variable_data.read_reg_interrupt_flag == true && instance->processed_variable_data.reg_interrupt_flag.bit.b04 == 1) {
      // 发生溢出
      add_pulse_count += POW2_24 - last_pulse_count;
      add_pulse_count += energy_d_register_data.dword;
    } else {
      // 未发生溢出
      add_pulse_count += energy_d_register_data.dword - last_pulse_count;
    }
    last_pulse_count = energy_d_register_data.dword;
  }

  // 四象限无功电能累计
  instance->pulse_data_block.pulse_data[0].reactive_quadrant_count[instance->processed_variable_data.quadrant] += add_pulse_count;
  if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
    instance->pulse_data_block.pulse_data[instance->processed_variable_data.tariff].reactive_quadrant_count[instance->processed_variable_data.quadrant] += add_pulse_count;
  }

  if (emustatus_register_data.bit.b18 == 0) {
    instance->pulse_data_block.pulse_data[0].forward_reactive_pulse_count += add_pulse_count;
    if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
      instance->pulse_data_block.pulse_data[instance->processed_variable_data.tariff].forward_reactive_pulse_count += add_pulse_count;
    }
    instance->processed_variable_data.reactive_power_dir = METER_CHIP_ENERGY_DIR_FORWARD;
  } else {
    instance->pulse_data_block.pulse_data[0].reverse_reactive_pulse_count += add_pulse_count;
    if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
      instance->pulse_data_block.pulse_data[instance->processed_variable_data.tariff].reverse_reactive_pulse_count += add_pulse_count;
    }
    instance->processed_variable_data.reactive_power_dir = METER_CHIP_ENERGY_DIR_REVERSE;
  }
  instance->processed_pwr_data[0].forward_reactive_energy = (float)(instance->pulse_data_block.pulse_data[0].forward_reactive_pulse_count) / (float)(instance->calibration_data.meter_const);
  instance->processed_pwr_data[0].reverse_reactive_energy = (float)(instance->pulse_data_block.pulse_data[0].reverse_reactive_pulse_count) / (float)(instance->calibration_data.meter_const);
  instance->processed_pwr_data[0].total_reactive_quadrant_energy[instance->processed_variable_data.quadrant] = (float)(instance->pulse_data_block.pulse_data[0].reactive_quadrant_count[instance->processed_variable_data.quadrant]) / (float)(instance->calibration_data.meter_const);
  if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
    instance->processed_pwr_data[instance->processed_variable_data.tariff].forward_reactive_energy = (float)(instance->pulse_data_block.pulse_data[instance->processed_variable_data.tariff].forward_reactive_pulse_count) / (float)(instance->calibration_data.meter_const);
    instance->processed_pwr_data[instance->processed_variable_data.tariff].reverse_reactive_energy = (float)(instance->pulse_data_block.pulse_data[instance->processed_variable_data.tariff].reverse_reactive_pulse_count) / (float)(instance->calibration_data.meter_const);
    instance->processed_pwr_data[instance->processed_variable_data.tariff].total_reactive_quadrant_energy[instance->processed_variable_data.quadrant] = (float)(instance->pulse_data_block.pulse_data[instance->processed_variable_data.tariff].reactive_quadrant_count[instance->processed_variable_data.quadrant]) / (float)(instance->calibration_data.meter_const);
  }

  return true;
}

bool rn8209_calc_total_active_energy(rn8209_instance_p instance) {

  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  instance->processed_pwr_data[0].total_active_energy = instance->processed_pwr_data[0].forward_active_energy + instance->processed_pwr_data[0].reverse_active_energy;
  if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
    instance->processed_pwr_data[instance->processed_variable_data.tariff].total_active_energy = instance->processed_pwr_data[instance->processed_variable_data.tariff].forward_active_energy + instance->processed_pwr_data[0].reverse_active_energy;
  }

  return true;
}

bool rn8209_calc_total_reactive_energy(rn8209_instance_p instance) {

  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  instance->processed_pwr_data[0].total_reactive_energy = instance->processed_pwr_data[0].forward_reactive_energy + instance->processed_pwr_data[0].reverse_reactive_energy;
  if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
    instance->processed_pwr_data[instance->processed_variable_data.tariff].total_reactive_energy = instance->processed_pwr_data[instance->processed_variable_data.tariff].forward_reactive_energy + instance->processed_pwr_data[instance->processed_variable_data.tariff].reverse_reactive_energy;
  }

  return true;
}

bool rn8209_calc_combined_active_energy(rn8209_instance_p instance) {

  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  // instance->processed_pwr_data[0].combined_active_energy = instance->processed_pwr_data[0].forward_active_energy - instance->processed_pwr_data[0].reverse_active_energy;
  instance->processed_pwr_data[0].combined_active_energy = CALC_ACTIVE_COMBINED(0);

  if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
    // instance->processed_pwr_data[instance->processed_variable_data.tariff].combined_active_energy = instance->processed_pwr_data[instance->processed_variable_data.tariff].forward_active_energy - instance->processed_pwr_data[0].reverse_active_energy;
    instance->processed_pwr_data[instance->processed_variable_data.tariff].combined_active_energy = CALC_ACTIVE_COMBINED(instance->processed_variable_data.tariff);
  }

  return true;
}

bool rn8209_calc_combined_reactive_energy_1(rn8209_instance_p instance) {

  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  // 组合无功1 = I象限+II象限(感性)
  // instance->processed_pwr_data[0].combined_reactive_energy_1 = instance->processed_pwr_data[0].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_I] + instance->processed_pwr_data[0].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_II];
  instance->processed_pwr_data[0].combined_reactive_energy_1 = CALC_REACTIVE_COMBINED(0, 1);

  if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
    // instance->processed_pwr_data[instance->processed_variable_data.tariff].combined_reactive_energy_1 = instance->processed_pwr_data[instance->processed_variable_data.tariff].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_I] + instance->processed_pwr_data[instance->processed_variable_data.tariff].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_II];
    instance->processed_pwr_data[instance->processed_variable_data.tariff].combined_reactive_energy_1 = CALC_REACTIVE_COMBINED(instance->processed_variable_data.tariff, 1);
  }

  return true;
}

bool rn8209_calc_combined_reactive_energy_2(rn8209_instance_p instance) {

  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  // 组合无功2 = III象限+IV象限(容性)
  // instance->processed_pwr_data[0].combined_reactive_energy_2 = instance->processed_pwr_data[0].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_III] + instance->processed_pwr_data[0].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_IV];
  instance->processed_pwr_data[0].combined_reactive_energy_2 = CALC_REACTIVE_COMBINED(0, 2);
  if (instance->processed_variable_data.tariff <= METER_CHIP_MAX_TARIFF_COUNT) {
    // instance->processed_pwr_data[instance->processed_variable_data.tariff].combined_reactive_energy_2 = instance->processed_pwr_data[instance->processed_variable_data.tariff].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_III] + instance->processed_pwr_data[instance->processed_variable_data.tariff].total_reactive_quadrant_energy[METER_CHIP_REACTIVE_QUADRANT_IV];
    instance->processed_pwr_data[instance->processed_variable_data.tariff].combined_reactive_energy_2 = CALC_REACTIVE_COMBINED(instance->processed_variable_data.tariff, 2);
  }

  return true;
}

bool rn8209_check_syscon(rn8209_instance_p instance) {
  static bool init_val = false;
  static struct {
    uint8_t current_a_channel_gain;
    uint8_t current_b_channel_gain;
    uint8_t voltage_channel_gain;
  } stash_gain_value;
  uext32_t sys_con_reg_value;

  // 读取系统配置寄存器
  if (rn8209_read_register_by_name(instance, RN8209_REG_SYSCON, &sys_con_reg_value) == false) {
    PU_LOG_ERROR("8209 read register SYSCON failed");
    return false;
  }

  // 计算A相电流通道增益倍数（从SYSCON寄存器低2位获取）
  switch (sys_con_reg_value.dword & 0x0003) {
  case 0:
    instance->origin_variable_data.current_a_channel_gain = 1;
    break;
  case 1:
    instance->origin_variable_data.current_a_channel_gain = 2;
    break;
  case 2:
    instance->origin_variable_data.current_a_channel_gain = 8;
    break;
  case 3:
    instance->origin_variable_data.current_a_channel_gain = 16;
    break;
  }

  // 计算电压通道增益倍数（从SYSCON寄存器2-3位获取）
  switch ((sys_con_reg_value.dword >> 2) & 0x0003) {
  case 0:
    instance->origin_variable_data.voltage_channel_gain = 1;
    break;
  case 1:
    instance->origin_variable_data.voltage_channel_gain = 2;
    break;
  case 2:
    instance->origin_variable_data.voltage_channel_gain = 4;
    break;
  case 3:
    instance->origin_variable_data.voltage_channel_gain = 4;
    break;
  }

  if (instance->status != METER_CHIP_STATUS_MEASURING) {
    return true;
  }

  if (init_val == false) {
    stash_gain_value.current_a_channel_gain = instance->origin_variable_data.current_a_channel_gain;
    stash_gain_value.current_b_channel_gain = instance->origin_variable_data.current_b_channel_gain;
    stash_gain_value.voltage_channel_gain = instance->origin_variable_data.voltage_channel_gain;
    init_val = true;
    return true;
  }

  if (stash_gain_value.current_a_channel_gain != instance->origin_variable_data.current_a_channel_gain || stash_gain_value.current_b_channel_gain != instance->origin_variable_data.current_b_channel_gain || stash_gain_value.voltage_channel_gain != instance->origin_variable_data.voltage_channel_gain) {
    PU_LOG_ERROR("8209 syscon change");
    return false;
  }
  return true;
}

bool rn8209_update_energy_storage_time(rn8209_instance_p instance);

bool rn8209_update_quadrant(rn8209_instance_p instance) {
  if (instance->processed_variable_data.active_power > 0 && instance->processed_variable_data.reactive_power > 0) {
    instance->processed_variable_data.quadrant = METER_CHIP_REACTIVE_QUADRANT_I;
  } else if (instance->processed_variable_data.active_power < 0 && instance->processed_variable_data.reactive_power > 0) {
    instance->processed_variable_data.quadrant = METER_CHIP_REACTIVE_QUADRANT_II;
  } else if (instance->processed_variable_data.active_power < 0 && instance->processed_variable_data.reactive_power < 0) {
    instance->processed_variable_data.quadrant = METER_CHIP_REACTIVE_QUADRANT_III;
  } else if (instance->processed_variable_data.active_power > 0 && instance->processed_variable_data.reactive_power < 0) {
    instance->processed_variable_data.quadrant = METER_CHIP_REACTIVE_QUADRANT_IV;
  } else {
    instance->processed_variable_data.quadrant = METER_CHIP_REACTIVE_QUADRANT_I;
  }

  return true;
}

bool rn8209_update_tariff(rn8209_instance_p instance) {
  if (instance->get_current_traiff_callback == NULL) {
    instance->processed_variable_data.tariff = METER_CHIP_TARIFF_INVALID; // 无效费率
    return false;
  }
  instance->processed_variable_data.tariff = instance->get_current_traiff_callback(instance);
  return true;
}

bool rn8209_read_reg(rn8209_instance_p instance) {
  instance->processed_variable_data.read_reg_interrupt_flag = rn8209_read_register_by_name(instance, RN8209_REG_IF, &instance->processed_variable_data.reg_interrupt_flag);
  return instance->processed_variable_data.read_reg_interrupt_flag;
}

// 定义函数指针数组
static rn8209_calc_func_entry_t rn8209_calc_functions[] = {
    {rn8209_calc_current, "current"},                                       // 计算A相电流
    {rn8209_calc_voltage, "voltage"},                                       // 计算A相电压
    {rn8209_calc_frequency, "frequency"},                                   // 计算A相频率
    {rn8209_calc_active_power, "active power"},                             // 计算A相有功功率
    {rn8209_calc_reactive_power, "reactive power"},                         // 计算A相无功功率
    {rn8209_calc_apparent_power, "apparent power"},                         // 计算A相视在功率
    {rn8209_read_reg, "read flag reg"},                                     // 读取中断状态标志
    {rn8209_update_quadrant, "updata reactive quadrant"},                   // 更新无功象限
    {rn8209_update_tariff, "update tariff"},                                // 更新费率
    {rn8209_calc_power_factor, "power factor"},                             // 计算A相功率因数
    {rn8209_calc_active_energy, "active energy"},                           // 计算A相有功电能
    {rn8209_calc_reactive_energy, "reactive energy"},                       // 计算A相无功电能
    {rn8209_calc_total_active_energy, "total_active_energy"},               // 计算有功总电能
    {rn8209_calc_total_reactive_energy, "total_reactive_energy"},           // 计算无功总电能
    {rn8209_calc_combined_active_energy, "combined_active_energy"},         // 计算组合有功电能
    {rn8209_calc_combined_reactive_energy_1, "combined_reactive_energy_1"}, // 计算组合无功1电能
    {rn8209_calc_combined_reactive_energy_2, "combined_reactive_energy_2"}, // 计算组合无功2电能
};

size_t rn8209_calc_functions_count = PU_GET_COUNT(rn8209_calc_functions);

/**
 * @brief rn8209 计算循环
 * @note 为了防止多个计算导致IO阻塞芯片,所以使用循环进行计算,每次只计算其中一个数据
 * @param instance
 * @param index
 * @return true
 * @return false
 */
bool rn8209_calc_cycle(rn8209_instance_p instance, size_t index) {
  if (instance->status != METER_CHIP_STATUS_MEASURING && instance->status != METER_CHIP_STATUS_CALIBRATING) {
    return false;
  }
  if (rn8209_calc_functions[index % rn8209_calc_functions_count].func(instance) == false) {
    instance->status = METER_CHIP_STATUS_COMM_ERROR;
    PU_LOG_ERROR("计量芯片计算错误:%s", rn8209_calc_functions[index % rn8209_calc_functions_count].name);
  }
  return true;
}

/**
 * @brief 获取A相电流
 *
 * @param instance 计量芯片实例
 * @param value 返回的数据
 * @return true
 * @return false
 */
bool rn8209_get_phase_a_current(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;
  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_variable_data.channel_a_current, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_voltage(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_variable_data.voltage, sizeof(float));
  return true;
}

bool rn8209_get_tagged_words(void *instance, uint8_t *value, uint8_t which) {
  rn8209_instance_p rn8209_instance = instance;
  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {
    return false;
  }
  switch (which) {
  case 1: {
    memcpy(value, &rn8209_instance->tagged_words.combined_active_tagged_word.byte, sizeof(uint8_t));
  } break;
  case 2: {
    memcpy(value, &rn8209_instance->tagged_words.combined_reactive_tagged_word[0].byte, sizeof(uint8_t));
  } break;
  case 3: {
    memcpy(value, &rn8209_instance->tagged_words.combined_reactive_tagged_word[1].byte, sizeof(uint8_t));
  } break;
  }
  return true;
}

bool rn8209_set_tagged_words(void *instance, uint8_t value, uint8_t which) {
  rn8209_instance_p rn8209_instance = instance;
  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {
    return false;
  }

  switch (which) {
  case 1: {
    memset(&rn8209_instance->tagged_words.combined_active_tagged_word.byte, value, sizeof(uint8_t));
  } break;
  case 2: {
    memset(&rn8209_instance->tagged_words.combined_reactive_tagged_word[0].byte, value, sizeof(uint8_t));
  } break;
  case 3: {
    memset(&rn8209_instance->tagged_words.combined_reactive_tagged_word[1].byte, value, sizeof(uint8_t));
  } break;
  }
  return true;
}

bool rn8209_get_phase_a_frequency(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_variable_data.grid_frequency, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_active_power(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_variable_data.active_power, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_reactive_power(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_variable_data.reactive_power, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_apparent_power(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_variable_data.apparent_power, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_power_factor(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_variable_data.power_factor, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_forward_active_energy(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_pwr_data[0].forward_active_energy, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_forward_reactive_energy(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_pwr_data[0].forward_reactive_energy, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_reverse_active_energy(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_pwr_data[0].reverse_active_energy, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_reverse_reactive_energy(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_pwr_data[0].reverse_reactive_energy, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_total_active_energy(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_pwr_data[0].total_active_energy, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_total_reactive_energy(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_pwr_data[0].total_reactive_energy, sizeof(float));
  return true;
}

/**
 * @brief 获取对应BCD格式的值
 *
 * @param instance 芯片实例
 * @param value 输出值(存放BCD值)
 * @param size value的大小(单位: 字节)
 * @param conversion 换算倍率
 * @return true
 * @return false
 */
bool rn8209_get_phase_a_current_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  // 计算返回值
  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_variable_data.channel_a_current * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_phase_a_voltage_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;
  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }
  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_variable_data.voltage * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_tagged_words_bcd(void *instance, uint8_t *value, uint8_t which_word, uint8_t size, int conversion) {
  rn8209_instance_p rn8209_instance = instance;
  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {
    return false;
  }
  uint8_t word;
  switch (which_word) {
  case 1: {
    memcpy(&word, &rn8209_instance->tagged_words.combined_active_tagged_word.byte, sizeof(uint8_t));
  } break;
  case 2: {
    memcpy(&word, &rn8209_instance->tagged_words.combined_reactive_tagged_word[0].byte, sizeof(uint8_t));
  } break;
  case 3: {
    memcpy(&word, &rn8209_instance->tagged_words.combined_reactive_tagged_word[1].byte, sizeof(uint8_t));
  } break;
  }
  return pu_convert_to_bcd(word, value, size);
}

bool rn8209_get_phase_a_frequency_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_variable_data.grid_frequency * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_phase_a_active_power_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uext32_t calc_data;
  calc_data.s_dword = (int32_t)(rn8209_instance->processed_variable_data.active_power * (float)pow(10, -conversion));
  calc_data.s_dword = labs(calc_data.s_dword);
  pu_convert_to_bcd(calc_data.s_dword, value, size);

  if (rn8209_instance->processed_variable_data.active_power < 0) {
    PU_SET_BIT(*(value + size - 1), 7);
  }
  return true;
}

bool rn8209_get_phase_a_reactive_power_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uext32_t calc_data;
  calc_data.s_dword = (int32_t)(rn8209_instance->processed_variable_data.reactive_power * (float)pow(10, -conversion));
  calc_data.s_dword = labs(calc_data.s_dword);
  pu_convert_to_bcd(calc_data.s_dword, value, size);

  if (rn8209_instance->processed_variable_data.reactive_power < 0) {
    PU_SET_BIT(*(value + size - 1), 7);
  }
  return true;
}

bool rn8209_get_phase_a_apparent_power_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uext32_t calc_data;
  calc_data.s_dword = (int32_t)(rn8209_instance->processed_variable_data.apparent_power * (float)pow(10, -conversion));
  calc_data.s_dword = labs(calc_data.s_dword);
  pu_convert_to_bcd(calc_data.s_dword, value, size);

  if (rn8209_instance->processed_variable_data.apparent_power < 0) {
    PU_SET_BIT(*(value + size - 1), 7);
  }
  return true;
}

bool rn8209_get_phase_a_power_factor_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uext32_t calc_data;
  calc_data.s_dword = (int32_t)(rn8209_instance->processed_variable_data.power_factor * (float)pow(10, -conversion));
  calc_data.s_dword = labs(calc_data.s_dword);
  pu_convert_to_bcd(calc_data.s_dword, value, size);

  if (rn8209_instance->processed_variable_data.power_factor < 0) {
    PU_SET_BIT(*(value + size - 1), 7);
  }
  return true;
}

bool rn8209_get_phase_a_forward_active_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[0].forward_active_energy * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_phase_a_forward_reactive_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[0].forward_reactive_energy * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_phase_a_reverse_active_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[0].reverse_active_energy * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_phase_a_reverse_reactive_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[0].reverse_reactive_energy * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_phase_a_active_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[0].total_active_energy * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_phase_a_total_active_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[0].total_active_energy * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_phase_a_total_reactive_energy_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[0].total_reactive_energy * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_phase_a_combined_active_energy(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_pwr_data[0].combined_active_energy, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_combined_reactive_energy_1(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }
  memcpy(value, &rn8209_instance->processed_pwr_data[0].combined_reactive_energy_1, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_combined_reactive_energy_2(void *instance, float *value) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }
  memcpy(value, &rn8209_instance->processed_pwr_data[0].combined_reactive_energy_2, sizeof(float));
  return true;
}

bool rn8209_get_phase_a_quadrant_reactive_energy(void *instance, float *value, uint8_t quadrant) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }
  memcpy(value, &rn8209_instance->processed_pwr_data[0].total_reactive_quadrant_energy[quadrant], sizeof(float));
  return true;
}

bool rn8209_get_phase_a_quadrant_reactive_energy_bcd(void *instance, float *value, uint8_t quadrant, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[0].total_reactive_quadrant_energy[quadrant] * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, (uint8_t *)value, size);
}

bool rn8209_get_phase_a_combined_reactive_energy_1_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[0].combined_reactive_energy_1 * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}
bool rn8209_get_phase_a_combined_reactive_energy_2_bcd(void *instance, uint8_t *value, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[0].combined_reactive_energy_2 * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, value, size);
}

bool rn8209_get_combined_active_energy_by_tariff(void *instance, float *value, uint8_t tariff) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  if (tariff >= METER_CHIP_MAX_TARIFF_COUNT + 1) {
    PU_LOG_ERROR("暂不支持该费率");
    return false;
  }

  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_pwr_data[tariff].combined_active_energy, sizeof(float));
  return true;
}

bool rn8209_get_forward_active_energy_by_tariff(void *instance, float *value, uint8_t tariff) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  if (tariff >= METER_CHIP_MAX_TARIFF_COUNT + 1) {
    PU_LOG_ERROR("暂不支持该费率");
    return false;
  }

  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  memcpy(value, &rn8209_instance->processed_pwr_data[tariff].forward_active_energy, sizeof(float));
  return true;
}

bool rn8209_get_reverse_active_energy_by_tariff(void *instance, float *value, uint8_t tariff) {
  if (instance == NULL || value == NULL) {
    PU_LOG_ERROR("rn8209 instance or value pointer is NULL");
    return false;
  }

  if (tariff >= METER_CHIP_MAX_TARIFF_COUNT) {
    PU_LOG_ERROR("暂不支持该费率");
    return false;
  }

  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {
    return false;
  }

  memcpy(value, &rn8209_instance->processed_pwr_data[tariff].reverse_active_energy, sizeof(float));
  return true;
}

bool rn8209_get_combined_reactive_energy_1_by_tariff(void *instance, float *value, uint8_t tariff) {
  if (instance == NULL || value == NULL) {
    PU_LOG_ERROR("rn8209 instance or value pointer is NULL");
    return false;
  }

  if (tariff >= METER_CHIP_MAX_TARIFF_COUNT) {
    PU_LOG_ERROR("暂不支持该费率");
    return false;
  }

  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;
  memcpy(value, &rn8209_instance->processed_pwr_data[tariff].combined_reactive_energy_1, sizeof(float));
  return true;
}

bool rn8209_get_combined_reactive_energy_2_by_tariff(void *instance, float *value, uint8_t tariff) {
  if (instance == NULL || value == NULL) {
    return false;
  }

  if (tariff >= METER_CHIP_MAX_TARIFF_COUNT) {
    PU_LOG_ERROR("暂不支持该费率");
    return false;
  }

  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;
  memcpy(value, &rn8209_instance->processed_pwr_data[tariff].combined_reactive_energy_2, sizeof(float));
  return true;
}

bool rn8209_get_quadrant_reactive_energy_by_tariff(void *instance, float *value, uint8_t tariff) {
  if (instance == NULL || value == NULL) {
    return false;
  }

  if (tariff >= METER_CHIP_MAX_TARIFF_COUNT) {
    return false;
  }

  rn8209_instance_p rn8209_instance = (rn8209_instance_p)instance;
  memcpy(value, &rn8209_instance->processed_pwr_data[tariff].total_reactive_quadrant_energy[rn8209_instance->processed_variable_data.quadrant], sizeof(float));
  return true;
}
bool rn8209_get_combined_active_energy_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[tariff].combined_active_energy * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, (uint8_t *)value, size);
}

bool rn8209_get_forward_active_energy_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }
  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {

    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[tariff].forward_active_energy * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, (uint8_t *)value, size);
}

bool rn8209_get_reverse_active_energy_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {
    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[tariff].reverse_active_energy * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, (uint8_t *)value, size);
}

bool rn8209_get_combined_reactive_energy_1_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {
    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[tariff].combined_reactive_energy_1 * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, (uint8_t *)value, size);
}

bool rn8209_get_combined_reactive_energy_2_by_tariff_bcd(void *instance, float *value, uint8_t size, uint8_t tariff, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {
    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[tariff].combined_reactive_energy_2 * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, (uint8_t *)value, size);
}

bool rn8209_get_quadrant_reactive_energy_by_tariff_bcd(void *instance, float *value, uint8_t tariff, uint8_t quadrant, uint8_t size, int conversion) {
  if (instance == NULL) {
    PU_LOG_ERROR("8209 instance is NULL");
    return false;
  }

  rn8209_instance_p rn8209_instance = instance;

  if (rn8209_instance->status != METER_CHIP_STATUS_MEASURING && rn8209_instance->status != METER_CHIP_STATUS_CALIBRATING) {
    return false;
  }

  uint32_t calc_data = (uint32_t)(rn8209_instance->processed_pwr_data[tariff].total_reactive_quadrant_energy[quadrant] * (float)pow(10, -conversion));
  return pu_convert_to_bcd(calc_data, (uint8_t *)value, size);
}
