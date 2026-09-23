#!/usr/bin/env bash
# QEMU aarch64 启动脚本
# 用法:
#   ./run-qemu.sh                                    # 使用默认文件名 (Image + rootfs.ext4)
#   ./run-qemu.sh -i Image -r rootfs.ext4            # 指定内核与 ext4 根文件系统
#   ./run-qemu.sh -i Image -n initramfs.cpio.gz      # 使用 initramfs 启动
#   ./run-qemu.sh -d                                 # 启动 gdb 调试端口 (1234)，配合 vmlinux
#   ./run-qemu.sh -D data.img                        # 指定数据盘（不存在则自动创建+格式化）
#
# 数据同步:
#   数据盘自动作为 /dev/vdb 附加，rootfs 内 /etc/fstab 已配置开机自动挂载到 /mnt/data。
#   宿主机读写 data.img 内容:  Linux/WSL:  sudo mount -o loop data.img /mnt/data
#                              Windows:    用 qemu-nbd 或 7-Zip 直接打开 ext4 镜像
# 依赖: qemu-system-aarch64（自动创建数据盘时需要 mkfs.ext4，缺失时仅建裸盘）
set -euo pipefail

KERNEL=""
ROOTFS=""
INITRD=""
DATA=""
GDB=0
SMP=2
MEM=512M
DATA_SIZE=1G

usage() { grep '^#' "$0" | tail -n +2; exit 0; }

while getopts "i:r:n:D:mds:h" opt; do
  case $opt in
    i) KERNEL=$OPTARG ;;
    r) ROOTFS=$OPTARG ;;
    n) INITRD=$OPTARG ;;
    D) DATA=$OPTARG ;;
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
# 数据盘默认名
[ -z "$DATA" ] && DATA=data.img

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
  # 数据盘：不存在则创建（稀疏文件），有 mkfs.ext4 就顺手格式化
  if [ ! -f "$DATA" ]; then
    echo ">>> 创建数据盘 $DATA ($DATA_SIZE, sparse)"
    dd if=/dev/zero of="$DATA" bs=1 seek="$DATA_SIZE" count=0 2>/dev/null
    if command -v mkfs.ext4 >/dev/null 2>&1; then
      mkfs.ext4 -q -F "$DATA"
    else
      echo ">>> 警告: 未找到 mkfs.ext4，数据盘未格式化，需在 guest 内执行 mkfs.ext4 /dev/vdb"
    fi
  fi
  echo ">>> ext4 根文件系统模式: kernel=$KERNEL rootfs=$ROOTFS data=$DATA"
  # 设备顺序（实测为准）：data.img 在前，rootfs 在后，root=/dev/vda 引导正常
  CMD+=(
    -drive "file=$DATA,format=raw,if=none,id=hd1"
    -device virtio-blk-device,drive=hd1
    -drive "file=$ROOTFS,format=raw,if=none,id=hd0"
    -device virtio-blk-device,drive=hd0
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
