# ============================================================
# 内核模块 Makefile（kbuild 主路径）
#
# 构建（树外模块，必须指定内核源码树）:
#   make -C <内核源码目录> M=$(pwd) modules
#   例: make -C ~/linux-7.2.7 M=$(pwd) modules
#
# 产物: hello.ko
#   = pu_* 工具库 + rn8209 计量芯片驱动 + meter_uart 串口传输层(/dev/ttyAMA1)
#   加载: insmod hello.ko                  # 默认打开 /dev/ttyAMA1
#         insmod hello.ko uart_dev=/dev/ttyAMA0
#   卸载: rmmod hello.ko
#
# 可选: make test —— 同一份源码的用户态自测程序（显式选择才编译）
# ============================================================

ifneq ($(KERNELRELEASE),)

# ---------- 内核态 ----------
obj-m += hello.o

hello-objs := hello_main.o \
              meter_uart.o \
              rn8209_driver.o \
              nor_flash_simulator.o \
              pu_bitmap.o \
              pu_doublecheck_bitmap.o \
              pu_mem_pool.o \
              pu_mem_tracker.o \
              pu_ndb.o \
              pu_queue.o \
              pu_ringbuffer.o \
              pu_util.o

# rn8209_driver.c 含 float 运算（pow/fabs 已由 pu_port.h 提供内核降级实现）。
# arm64 内核全局使用 -mgeneral-regs-only 禁用浮点指令，单独对本文件放开；
# 代价: 调用其浮点 API 前必须用 PU_FP_BEGIN()/PU_FP_END() 包住
# （kernel_neon_begin 保存/恢复用户态 FP 状态），且区间内禁止睡眠。
CFLAGS_REMOVE_rn8209_driver.o += -mgeneral-regs-only

else

# ---------- 裸 make: 不再默认编译用户程序，只给构建提示 ----------
.DEFAULT_GOAL := help
help:
	@echo "内核模块 Makefile —— 请使用 kbuild 构建:"
	@echo "    make -C <内核源码目录> M=\$$(pwd) modules"
	@echo "    例: make -C ~/linux-7.2.7 M=\$$(pwd) modules"
	@echo ""
	@echo "产物 hello.ko = pu 库 + rn8209 驱动 + meter_uart(/dev/ttyAMA1)"
	@echo "可选用户态自测: make test"

# ---- 用户态自测（可选，显式触发；与内核态共用同一份源码）----
CC      ?= gcc
CFLAGS  ?= -g -O2 -Wall -std=c11
LDLIBS  ?= -lm   # rn8209_driver.c 使用 pow/fabs

LIB_OBJS := pu_bitmap.o \
            pu_doublecheck_bitmap.o \
            pu_mem_pool.o \
            pu_mem_tracker.o \
            pu_ndb.o \
            pu_queue.o \
            pu_ringbuffer.o \
            pu_util.o \
            nor_flash_simulator.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: hello_test

hello_test: hello_main.c $(LIB_OBJS) rn8209_driver.o
	$(CC) $(CFLAGS) hello_main.c $(LIB_OBJS) rn8209_driver.o $(LDLIBS) -o $@

clean:
	rm -f *.o hello_test

.PHONY: help test clean

endif
