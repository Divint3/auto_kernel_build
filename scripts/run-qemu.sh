#!/usr/bin/env bash
# QEMU aarch64 启动脚本
# 用法:
#   ./run-qemu.sh                                    # 使用默认文件名 (Image + rootfs.ext4)
#   ./run-qemu.sh -i Image -r rootfs.ext4            # 指定内核与 ext4 根文件系统
#   ./run-qemu.sh -i Image -n initramfs.cpio.gz      # 使用 initramfs 启动
#   ./run-qemu.sh -d                                 # 启动 gdb 调试端口 (1234)，配合 vmlinux
#
# 依赖: qemu-system-aarch64
set -euo pipefail

KERNEL=""
ROOTFS=""
INITRD=""
GDB=0
SMP=2
MEM=512M

usage() { grep '^#' "$0" | tail -n +2; exit 0; }

while getopts "i:r:n:mds:h" opt; do
  case $opt in
    i) KERNEL=$OPTARG ;;
    r) ROOTFS=$OPTARG ;;
    n) INITRD=$OPTARG ;;
    m) GDB=1 ;;
    s) SMP=$OPTARG ;;
    h) usage ;;
    *) usage ;;
  esac
done

# 自动探测默认文件
[ -z "$KERNEL" ] && [ -f boot/Image ] && KERNEL=boot/Image
[ -z "$KERNEL" ] && [ -f Image ] && KERNEL=Image
if [ -z "$ROOTFS" ] && [ -z "$INITRD" ]; then
  [ -f rootfs.ext4 ] && ROOTFS=rootfs.ext4
  [ -f initramfs.cpio.gz ] && INITRD=initramfs.cpio.gz
fi

[ -z "$KERNEL" ] && { echo "错误: 未找到内核镜像，用 -i 指定"; exit 1; }

CMD=(qemu-system-aarch64
  -M virt -cpu cortex-a57
  -nographic
  -smp "$SMP" -m "$MEM"
  -kernel "$KERNEL"
)

if [ -n "$INITRD" ]; then
  echo ">>> initramfs 模式: kernel=$KERNEL initrd=$INITRD"
  CMD+=(-initrd "$INITRD" -append "console=ttyAMA0")
else
  [ -z "$ROOTFS" ] && { echo "错误: 未找到根文件系统，用 -r 或 -n 指定"; exit 1; }
  echo ">>> ext4 根文件系统模式: kernel=$KERNEL rootfs=$ROOTFS"
  CMD+=(
    -drive "file=$ROOTFS,format=raw,if=virtio"
    -append "console=ttyAMA0 root=/dev/vda rw"
  )
fi

if [ "$GDB" -eq 1 ]; then
  echo ">>> gdb 调试端口已开启: localhost:1234 (用 aarch64 gdb 执行 target remote :1234)"
  CMD+=(-S -gdb tcp::1234)
fi

echo ">>> 启动命令: ${CMD[*]}"
echo ">>> 退出: Ctrl+A 然后按 X"
exec "${CMD[@]}"
