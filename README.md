# aarch64 内核构建项目（QEMU 可引导）

使用 GitHub Actions 交叉编译 aarch64（ARM64）Linux 内核，产出 **QEMU 可直接引导的完整包**：vmlinux + 根文件系统，并支持树外内核模块编译。

## 功能

- **QEMU 可引导产物**：vmlinux（ELF，带符号可 gdb 调试）、Image、`rootfs.ext4`（busybox 根文件系统 + 全部内核模块）、`initramfs.cpio.gz`（快速验证用）
- **自动下载最新内核**：`kernel_version` 留空时自动从 kernel.org 获取最新 stable 版本
- **指定版本工具链**：`toolchain_version`（默认 `13.2.rel1`，Arm GNU aarch64-none-linux-gnu）
- **内核模块支持**：内核模块安装进 rootfs.ext4；附树外模块示例（`kernel-module/`）
- **CI 内 QEMU 自测**：构建完成后自动用 `qemu-system-aarch64 -M virt` 引导 initramfs，检测到 `QEMU-BOOT-OK` 标志即通过（可关闭）
- **本地一键启动**：附 `run-qemu.sh`

## 使用方法

1. 推送 GitHub → **Actions → Build aarch64 Kernel (QEMU bootable) → Run workflow**
2. 下载 artifact **`qemu-boot-aarch64-<版本>`**，解压到同一目录
3. 本地启动（需安装 `qemu-system-aarch64`）：

```bash
# ext4 根文件系统方式（默认，模块已装在 /lib/modules）
./run-qemu.sh -i boot/Image -r rootfs.ext4

# initramfs 快速启动
./run-qemu.sh -i boot/Image -n initramfs.cpio.gz

# gdb 调试模式（端口 1234，配合 vmlinux + aarch64-none-linux-gnu-gdb）
./run-qemu.sh -i boot/Image -r rootfs.ext4 -m
# 另一终端: target remote :1234; file vmlinux; b start_kernel; c

# 数据同步盘（host <-> guest）
./run-qemu.sh -i boot/Image -r rootfs.ext4        # data.img 不存在时自动创建+格式化
# guest 内自动挂载到 /mnt/data（fstab nofail）；宿主机读写:
sudo mount -o loop data.img /mnt/data   # Linux/WSL；Windows 可用 7-Zip 打开 ext4 镜像
```

## 可配输入项

| 输入 | 说明 | 默认 |
|---|---|---|
| `kernel_version` | 如 `6.9.2`，留空 = 最新 stable | 空 |
| `toolchain_version` | Arm GNU 工具链版本 | `13.2.rel1` |
| `build_module` | 是否编译树外模块示例 | true |
| `run_qemu_test` | CI 内 QEMU 启动自测 | true |
| `minimal_config` | 快速编译：allnoconfig 最小配置（补齐 QEMU 引导所需符号，约 3~5 分钟）；关闭 = defconfig 全量（20~40 分钟） | true |

> 快速模式说明：allnoconfig 会关掉所有 `default y` 的选项，workflow 已逐项补回 TTY/PRINTK/BINFMT_ELF/BLK_DEV_INITRD/RD_GZIP/VIRTIO/EXT4 等引导必需符号，并在 `olddefconfig` 后回读 .config 校验（依赖不满足时 kconfig 会静默丢弃符号）。

## 目录结构

```
.
├── .github/workflows/build-kernel.yml   # CI 工作流（内核+rootfs+QEMU自测）
├── kernel-module/                        # 树外模块示例
│   ├── Makefile
│   └── hello_main.c
├── scripts/
│   └── run-qemu.sh                       # 本地 QEMU 启动脚本
└── configs/                              # 可选：自定义 defconfig
```

## 自定义根文件系统

当前 rootfs 为 busybox 最小系统（init 脚本在工作流第 7/8 步内联生成）。如需自定义，可将 `inittab`、`rcS` 等放到仓库目录并在 workflow 中复制进去；或后续接 Buildroot/Debrootfs 方案。
