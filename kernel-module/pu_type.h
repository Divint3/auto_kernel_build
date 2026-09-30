#ifndef PU_TYPE_H_
#define PU_TYPE_H_

#include "pu_port.h" // 平台可移植层（__KERNEL__ 区分内核/用户态）

#pragma pack(1)

// char型数据位定义
typedef struct {
  uint8_t b00:1;
  uint8_t b01:1;
  uint8_t b02:1;
  uint8_t b03:1;
  uint8_t b04:1;
  uint8_t b05:1;
  uint8_t b06:1;
  uint8_t b07:1;
} bits8_t;

// short型数据位定义
typedef struct {
  uint16_t b00:1;
  uint16_t b01:1;
  uint16_t b02:1;
  uint16_t b03:1;
  uint16_t b04:1;
  uint16_t b05:1;
  uint16_t b06:1;
  uint16_t b07:1;
  uint16_t b08:1;
  uint16_t b09:1;
  uint16_t b10:1;
  uint16_t b11:1;
  uint16_t b12:1;
  uint16_t b13:1;
  uint16_t b14:1;
  uint16_t b15:1;
} bits16_t;

// long型数据位定义
typedef struct {
  uint32_t b00:1;
  uint32_t b01:1;
  uint32_t b02:1;
  uint32_t b03:1;
  uint32_t b04:1;
  uint32_t b05:1;
  uint32_t b06:1;
  uint32_t b07:1;
  uint32_t b08:1;
  uint32_t b09:1;
  uint32_t b10:1;
  uint32_t b11:1;
  uint32_t b12:1;
  uint32_t b13:1;
  uint32_t b14:1;
  uint32_t b15:1;
  uint32_t b16:1;
  uint32_t b17:1;
  uint32_t b18:1;
  uint32_t b19:1;
  uint32_t b20:1;
  uint32_t b21:1;
  uint32_t b22:1;
  uint32_t b23:1;
  uint32_t b24:1;
  uint32_t b25:1;
  uint32_t b26:1;
  uint32_t b27:1;
  uint32_t b28:1;
  uint32_t b29:1;
  uint32_t b30:1;
  uint32_t b31:1;
} bits32_t;

// long long型数据位定义 (64位)
typedef struct {
  uint64_t b00:1;
  uint64_t b01:1;
  uint64_t b02:1;
  uint64_t b03:1;
  uint64_t b04:1;
  uint64_t b05:1;
  uint64_t b06:1;
  uint64_t b07:1;
  uint64_t b08:1;
  uint64_t b09:1;
  uint64_t b10:1;
  uint64_t b11:1;
  uint64_t b12:1;
  uint64_t b13:1;
  uint64_t b14:1;
  uint64_t b15:1;
  uint64_t b16:1;
  uint64_t b17:1;
  uint64_t b18:1;
  uint64_t b19:1;
  uint64_t b20:1;
  uint64_t b21:1;
  uint64_t b22:1;
  uint64_t b23:1;
  uint64_t b24:1;
  uint64_t b25:1;
  uint64_t b26:1;
  uint64_t b27:1;
  uint64_t b28:1;
  uint64_t b29:1;
  uint64_t b30:1;
  uint64_t b31:1;
  uint64_t b32:1;
  uint64_t b33:1;
  uint64_t b34:1;
  uint64_t b35:1;
  uint64_t b36:1;
  uint64_t b37:1;
  uint64_t b38:1;
  uint64_t b39:1;
  uint64_t b40:1;
  uint64_t b41:1;
  uint64_t b42:1;
  uint64_t b43:1;
  uint64_t b44:1;
  uint64_t b45:1;
  uint64_t b46:1;
  uint64_t b47:1;
  uint64_t b48:1;
  uint64_t b49:1;
  uint64_t b50:1;
  uint64_t b51:1;
  uint64_t b52:1;
  uint64_t b53:1;
  uint64_t b54:1;
  uint64_t b55:1;
  uint64_t b56:1;
  uint64_t b57:1;
  uint64_t b58:1;
  uint64_t b59:1;
  uint64_t b60:1;
  uint64_t b61:1;
  uint64_t b62:1;
  uint64_t b63:1;
} bits64_t;

// unsigned char 数据重定义
typedef union {
  uint8_t byte;  // 字节
  int8_t s_byte; // 字节
  bits8_t bit;   // 比特
} uext8_t;

// unsigned short 数据重定义
typedef union {
  uint16_t word;    // 字
  int16_t s_word;   // 字
  uint8_t byte[2];  // 字节
  int8_t s_byte[2]; // 字节
  bits16_t bit;     // 比特
} uext16_t;

// unsigned long 数据重定义
typedef union {
  float binary32; // 单精度浮点数
  uint32_t dword; // 双字
  int32_t s_dword;
  uint16_t word[2];  // 字
  int16_t s_word[2]; // 字
  uint8_t byte[4];   // 字节
  int8_t s_byte[4];  // 字节
  bits32_t bit;      // 比特
} uext32_t;

// unsigned long long 数据重定义 (64位)
typedef union {
  uint64_t qword;    // 四字
  uint32_t dword[2]; // 双字
  uint16_t word[4];  // 字
  uint8_t byte[8];   // 字节
  bits64_t bit;      // 比特
} uext64_t;

#pragma pack()

typedef struct {
  size_t size;
  uint8_t *data;
} pu_data_t, *pu_data_p;

#define pu_data_t_init_zero {0, NULL}

#endif // PU_TYPE_H_
