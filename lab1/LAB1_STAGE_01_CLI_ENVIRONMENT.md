# 实验 1 · 第一阶段：纯命令行开发环境搭建

本阶段只准备和检查命令行工具。完成后应能在新终端中运行 Arm GCC、GDB、CMake、Ninja 和 OpenOCD；编译、烧录与调试从[第二阶段](LAB1_STAGE_02_CLI_BUILD_FLASH_DEBUG.md)开始。本实验已有 STM32F429IGT6 的启动文件、链接脚本、HAL 库与 LED1（PB1）闪烁程序，无需先编写代码。

## 1. 认识工具链

| 工具 | 本实验中的用途 |
| --- | --- |
| `arm-none-eabi-gcc` | 将 C 和汇编代码编译并链接为 Cortex-M4F 的 ELF 固件 |
| `arm-none-eabi-size` | 查看固件的 `text`、`data`、`bss` 大小 |
| `gdb-multiarch` 或 `arm-none-eabi-gdb` | 从终端连接目标板并设置断点、单步调试 |
| CMake、Ninja | 配置工程并执行构建 |
| OpenOCD | 通过 ST-Link 烧录固件，并向 GDB 提供调试服务 |

本阶段无需安装 VS Code。第四阶段再安装 VS Code 与 clangd。

## 2. 安装命令行工具

只执行自己所用系统的小节。安装后关闭并重新打开终端，让新的 `PATH` 生效。

### Windows 10 / 11

1. 从 [Arm GNU Toolchain](https://developer.arm.com/tools-and-software/gnu-toolchain#Downloads) 下载 Windows 平台的 `arm-none-eabi` 工具链，安装后将其 `bin` 目录加入用户 `Path`。安装包通常包含 `arm-none-eabi-gdb`。
2. 安装 [CMake](https://cmake.org/download/)（本工程至少需要 3.22），安装时将 CMake 加入 `Path`。
3. 从 [Ninja Releases](https://github.com/ninja-build/ninja/releases) 下载 `ninja.exe`，将它所在目录加入 `Path`。
4. 按 [xPack OpenOCD 安装说明](https://xpack-dev-tools.github.io/openocd-xpack/docs/install/)安装 Windows 版，将 OpenOCD 的 `bin` 目录加入 `Path`。
5. 使用 ST-Link/V2 或 V2-1 时，安装 [ST-LINK USB 驱动](https://www.st.com/en/development-tools/stsw-link009.html)。

`Path` 可在“编辑系统环境变量”→“环境变量”→用户变量 `Path` 中编辑。添加目录，不要添加 `.exe` 文件本身。安装过程可能用到安装程序的图形界面；本阶段的工程操作和验收全部在终端完成。

### Ubuntu / Debian

```sh
sudo apt update
sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi gdb-multiarch cmake ninja-build openocd
```

如果包管理器找不到软件包，先检查发行版软件源；Ubuntu 可检查 `universe` 仓库。其他 Linux 发行版使用本系统包管理器安装同类工具，包名以发行版为准。

Linux 上若 OpenOCD 后续提示 `LIBUSB_ERROR_ACCESS`，先按发行版的 OpenOCD/ST-Link udev 规则配置 USB 权限，并重新插拔 ST-Link。Ubuntu / Debian 可检查用户是否属于 `plugdev` 组；需要加入时执行 `sudo usermod -aG plugdev "$USER"`，然后注销并重新登录。具体错误排查见[第二阶段](LAB1_STAGE_02_CLI_BUILD_FLASH_DEBUG.md#5-常见问题)。

## 3. 在终端检查安装

以下命令可逐行在 PowerShell 或 Linux 终端执行。CMake 版本应不低于 3.22；每条命令都应显示版本信息。

```text
arm-none-eabi-gcc --version
arm-none-eabi-size --version
cmake --version
ninja --version
openocd --version
```

Windows 上通常执行 `arm-none-eabi-gdb --version`；上述 Ubuntu / Debian 安装方式执行 `gdb-multiarch --version`。只需有其中一种 GDB 命令可用。若提示找不到命令，检查安装目录的 `bin` 是否在 `Path` 中，再打开新终端重试。

## 4. 确认工程根目录

后续命令都在包含 `CMakeLists.txt`、`CMakePresets.json`、`Core/`、`Drivers/` 的 `lab1` 目录下执行。先在终端切换到该目录，运行 `cmake --version` 和 `arm-none-eabi-gcc --version`，确认当前终端可以调用工具。工程的 `cmake/gcc-arm-none-eabi.cmake` 将在下一阶段指定 Arm GCC、Cortex-M4F 编译参数和链接脚本 `STM32F429XX_FLASH.ld`。

## 阶段验收

- [ ] Arm GCC、`arm-none-eabi-size`、GDB、CMake、Ninja、OpenOCD 均可从新终端运行。
- [ ] CMake 版本满足工程要求，已找到 `lab1` 工程根目录。
- [ ] 知道 Linux 与 Windows 上本机可用的 GDB 命令名。

下一步：[第二阶段：完整命令行编译、烧录与调试](LAB1_STAGE_02_CLI_BUILD_FLASH_DEBUG.md)。
