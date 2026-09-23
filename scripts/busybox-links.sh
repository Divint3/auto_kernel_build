#!/usr/bin/env bash
# 将 busybox 的全部 applet 导出为符号链接（busybox --list + ln -sf）
#
# 用法:
#   ./busybox-links.sh <busybox二进制> <目标rootfs目录> [bin目录，默认 /bin]
#
# 示例:
#   # 宿主机为 rootfs 目录补全所有命令链接
#   ./busybox-links.sh ~/busybox-1.36.1/busybox ~/ext4-root
#   # guest 内自补全（init/rcS 中调用）
#   /bin/busybox-links.sh /bin/busybox / 
#
# 说明:
#   busybox 为 aarch64 交叉编译产物，宿主机无法直接执行 --list，
#   脚本会自动尝试 qemu-aarch64(-static) 用户态模拟执行；
#   找不到 qemu-user 时退化为从二进制 strings 中提取 applet 名（次优）。
set -euo pipefail

[ $# -lt 2 ] && { grep '^#' "$0" | tail -n +2; exit 1; }

BB_BIN=$1
ROOTFS=$2
BINDIR=${3:-/bin}

[ -x "$BB_BIN" ] || { echo "错误: busybox 二进制不可执行: $BB_BIN"; exit 1; }
mkdir -p "$ROOTFS$BINDIR"

# ---- 获取 applet 列表 ----
APPETS=""
if "$BB_BIN" --list >/dev/null 2>&1; then
  # 本机可直接执行（guest 内或架构匹配）
  APPETS=$("$BB_BIN" --list)
else
  # 交叉编译产物：尝试 qemu 用户态模拟
  RUNNER=""
  for q in qemu-aarch64-static qemu-aarch64; do
    if command -v "$q" >/dev/null 2>&1; then RUNNER=$q; break; fi
  done
  if [ -n "$RUNNER" ]; then
    echo ">>> 通过 $RUNNER 执行 busybox --list"
    APPETS=$("$RUNNER" "$BB_BIN" --list)
  else
    # 兜底：从编译产物 busybox.links 或 strings 提取（可能混入杂质，做白名单过滤）
    echo ">>> 警告: 未找到 qemu-user，从二进制提取 applet 名（建议 apt install qemu-user-static）"
    APPETS=$(strings "$BB_BIN" | grep -E '^[a-z][a-z0-9_.-]{0,20}$' | sort -u)
  fi
fi

[ -n "$APPETS" ] || { echo "错误: 未获取到 applet 列表"; exit 1; }

# busybox 自身已是真实文件，其余全部软链
count=0
while IFS= read -r cmd; do
  [ -z "$cmd" ] && continue
  [ "$cmd" = "busybox" ] && continue
  ln -sf "$BINDIR/busybox" "$ROOTFS$BINDIR/$cmd"
  count=$((count + 1))
done <<< "$APPETS"

echo ">>> 已导出 $count 个 busybox 命令到 $ROOTFS$BINDIR/"
