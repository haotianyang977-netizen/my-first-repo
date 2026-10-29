# 实验 1：STM32F429 开发环境、构建与调试

本实验针对已有的 LED1（PB1）闪烁工程，依次完成命令行工具安装、显式命令构建与板上调试、CMake 预设构建、VS Code 图形界面开发，以及用 SVD 查看外设寄存器。工程已包含启动文件、链接脚本、CMSIS/HAL 驱动和应用代码，无需从零编写固件。

## 五阶段学习路线

| 顺序 | 文档 | 完成标志 |
| --- | --- | --- |
| 1 | [纯命令行环境搭建](LAB1_STAGE_01_CLI_ENVIRONMENT.md) | Arm GCC、GDB、CMake、Ninja、OpenOCD 均能在终端运行 |
| 2 | [完整命令行编译、烧录与调试](LAB1_STAGE_02_CLI_BUILD_FLASH_DEBUG.md) | 生成 ELF；OpenOCD 烧录成功；GDB 停在 `main` 并单步 |
| 3 | [CMake Presets](LAB1_STAGE_03_CMAKE_PRESETS.md) | 使用 Debug/Release 预设分别构建并比较固件大小 |
| 4 | [VS Code 与 clangd 图形界面配置](LAB1_STAGE_04_VSCODE_CLANGD.md) | clangd 可跳转与补全；VS Code 可构建并用 F5 调试 |
| 5 | [用 SVD 查看外设寄存器](LAB1_STAGE_05_SVD_REGISTERS.md) | 在外设视图中观察 GPIOB 输出位随 LED1 亮灭变化 |

按上表从第一阶段开始，依次完成环境检查、板上调试、预设构建、图形界面配置和寄存器观察。

第一次接触构建系统时，可先读 [CMake 入门：从 Lab1 看懂构建过程](CMAKE_BEGINNER_GUIDE.md)，再进入第二、三阶段操作。

## 硬件与预期现象

- MCU：STM32F429IGT6，程序按 25 MHz 外部晶振配置系统时钟。
- 调试器：ST-Link，经 SWDIO（PA13）、SWCLK（PA14）、GND 连接开发板。
- LED1：PB1，低电平点亮。运行时点亮约 500 ms、熄灭约 500 ms，循环交替。

## 工程结构

```text
lab1/
├── CMakeLists.txt                 # 源码、头文件和目标定义
├── CMakePresets.json              # Debug / Release 配置与构建预设
├── .clangd                        # 使用 Debug 编译数据库
├── .vscode/                      # VS Code 构建、调试和扩展推荐配置
├── svd/STM32F429.svd              # STM32F429 外设寄存器描述文件
├── cmake/gcc-arm-none-eabi.cmake  # Arm 交叉工具链和编译链接选项
├── STM32F429XX_FLASH.ld           # Flash / RAM 链接脚本
├── startup_stm32f429xx.s          # 启动文件
├── Core/                           # 应用与系统源码
└── Drivers/                        # CMSIS 和 STM32F4 HAL
```

第二阶段和第三阶段都将 Debug 产物写到 `build/Debug/`，Release 产物写到 `build/Release/`。第四、五阶段的调试与寄存器观察使用 Debug 目录。
