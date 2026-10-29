# CMake 入门：从 Lab1 的 LED 工程看懂构建过程

本文面向第一次接触 CMake 的同学。阅读后应能回答三个问题：`CMakeLists.txt` 写了什么、为什么 STM32 工程需要工具链文件、怎样用完整命令或 Presets 构建 Debug/Release。请在本仓库的 `lab1` 工程根目录操作；先按[第一阶段](LAB1_STAGE_01_CLI_ENVIRONMENT.md)安装 Arm GCC、CMake 和 Ninja。本教程只涉及**构建**，板上烧录和调试见[第二阶段](LAB1_STAGE_02_CLI_BUILD_FLASH_DEBUG.md)。

## 1. 先分清四种工具

```text
C/汇编源码 + CMakeLists.txt + 工具链文件
                 ↓ CMake 配置并生成
             build/Debug/build.ninja
                 ↓ Ninja 按规则调用
          arm-none-eabi-gcc 编译、链接
                 ↓
        build/Debug/lab1.elf
```

| 名称 | 在 Lab1 中做什么 |
| --- | --- |
| **CMake** | 读取工程描述，确定编译器、源码、头文件路径和编译选项，生成 Ninja 的构建规则 |
| **Ninja** | 根据依赖关系执行需要的编译和链接命令 |
| **Arm GCC** | 真正把源码变成 Cortex-M4F 指令，并用链接脚本生成 ELF |
| **OpenOCD** | 把 ELF 烧录到 STM32；它不参与 CMake 构建 |

CMake 的“配置/生成”和 Ninja 的“编译/链接”是两步。`cmake -S ... -B ...` 完成前一步；`cmake --build ...` 完成后一步。生成的 ELF 面向 STM32，不能在电脑上直接运行。[CMake 官方入门教程](https://cmake.org/cmake/help/latest/guide/tutorial/Getting%20Started%20with%20CMake.html)也按先配置、后构建的顺序讲解。

## 2. 认识源码目录与构建目录

| 目录或文件 | 内容 |
| --- | --- |
| 工程根目录 | `CMakeLists.txt`、源码、驱动、链接脚本、工具链文件和预设 |
| `build/Debug/` | Debug 的 CMake 缓存、Ninja 规则、目标文件、ELF、MAP 和编译数据库 |
| `build/Release/` | Release 的独立缓存和构建产物 |

`-S .` 指定当前目录为源码目录；`-B build/Debug` 指定 Debug 构建目录。**同一个构建目录会保存上次配置**，包括生成器和编译器选择，记录在 `CMakeCache.txt` 中。初学时把 Debug 与 Release 分开，检查问题也更直观。

下面的完整命令在工程根目录执行；Windows PowerShell 与 Linux 终端均可使用此单行形式：

```text
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/Debug
```

| 参数 | 在本工程中的含义 |
| --- | --- |
| `-S .` | 读取当前目录的 `CMakeLists.txt` |
| `-B build/Debug` | 把生成文件和产物放进 `build/Debug` |
| `-G Ninja` | 选择 Ninja 生成器 |
| `-D...=...` | 在配置时设置 CMake 缓存变量 |
| `CMAKE_BUILD_TYPE=Debug` | 为 Ninja 这类单配置生成器选择 Debug |
| `CMAKE_TOOLCHAIN_FILE=...` | 在首次配置该构建目录时载入 Arm 交叉编译工具链 |
| `CMAKE_EXPORT_COMPILE_COMMANDS=ON` | 生成 `compile_commands.json`，供 clangd 读取真实编译参数 |

第二条命令会在需要时调用 Ninja。你也可运行 `cmake --build build/Debug --verbose` 查看实际传给 Arm GCC 的命令。首次配置应明确指定工具链；如需改用另一个编译器或生成器，请用新的构建目录重新配置，因为旧目录保留了前一次的编译器选择。[工具链变量说明](https://cmake.org/cmake/help/latest/variable/CMAKE_TOOLCHAIN_FILE.html)。

## 3. 按顺序读 `CMakeLists.txt`

### 3.1 声明版本、语言和目标

工程开头的 `cmake_minimum_required(VERSION 3.22)` 表示至少需要 CMake 3.22。`CMAKE_C_STANDARD 11` 要求 C11，`CMAKE_C_STANDARD_REQUIRED ON` 不允许悄悄降级，`CMAKE_C_EXTENSIONS ON` 允许编译器扩展；当前 GCC 命令使用 `gnu11`。

`project(lab1 LANGUAGES C ASM)` 声明项目名以及 C、汇编两种语言。`add_executable(${PROJECT_NAME})` 创建名为 `lab1` 的**目标**；在 CMake 中，目标是保存源码、头文件路径、宏、链接依赖等属性的对象。最后生成的文件因工具链设置而以 `.elf` 结尾。[CMake 官方教程中的目标概念](https://cmake.org/cmake/help/latest/guide/tutorial/Getting%20Started%20with%20CMake.html)。

### 3.2 给目标添加源码

`target_sources(${PROJECT_NAME} PRIVATE ...)` 列出了启动汇编 `startup_stm32f429xx.s`、应用源码 `Core/Src/main.c`、`Core/Src/gpio.c`，以及 STM32 HAL 的多个 `.c` 文件。CMake 只会编译被列入目标的源码；**新建 `.c` 文件后，要把它加入 `target_sources`**。头文件通常通过 `#include` 参与编译，不需要当作独立的 C 编译单元。

这里的 `PRIVATE` 表示这些源码只属于这个目标。Lab1 当前只有一个可执行目标；以后拆出库时，再学习 `PUBLIC` 和 `INTERFACE` 如何把使用要求传给其他目标。

### 3.3 告诉编译器去哪里找头文件、定义哪些宏

`target_include_directories(${PROJECT_NAME} PRIVATE ...)` 包含 `Core/Inc`、HAL 头文件目录和 CMSIS 目录。它们会变成 Arm GCC 命令中的 `-I...`。例如 `main.c` 中包含 `main.h`，编译器可在 `Core/Inc` 找到它。

`target_compile_definitions(${PROJECT_NAME} PRIVATE USE_HAL_DRIVER STM32F429xx $<$<CONFIG:Debug>:DEBUG>)` 添加预处理宏：

| 宏 | 用途 |
| --- | --- |
| `USE_HAL_DRIVER` | 选择 HAL 驱动相关声明 |
| `STM32F429xx` | 选择 STM32F429 的 CMSIS 设备定义 |
| `$<$<CONFIG:Debug>:DEBUG>` | 仅在 Debug 配置中定义 `DEBUG` |

最后一项叫**生成器表达式**，在生成构建规则时按配置求值。这里的 `PRIVATE` 同样表示宏只用于当前目标。[`target_compile_definitions` 官方说明](https://cmake.org/cmake/help/latest/command/target_compile_definitions.html)。

### 3.4 链接和清理

`target_link_libraries(${PROJECT_NAME} PRIVATE m)` 将数学库 `m` 加入链接依赖。`ADDITIONAL_CLEAN_FILES` 将生成的 `.map` 文件加入清理列表；MAP 的生成方式在工具链文件里定义。

## 4. 为什么要有交叉编译工具链文件

电脑的普通 `gcc` 生成电脑平台程序；本实验的芯片需要 `arm-none-eabi-gcc`。工程的 [`cmake/gcc-arm-none-eabi.cmake`](cmake/gcc-arm-none-eabi.cmake) 在 CMake 识别项目语言之前提供目标平台和编译器信息：

| 工具链设置 | 作用 |
| --- | --- |
| `CMAKE_SYSTEM_NAME Generic` | 目标是无操作系统的裸机平台 |
| `CMAKE_SYSTEM_PROCESSOR arm` | 指明目标处理器架构 |
| `CMAKE_C_COMPILER arm-none-eabi-gcc` | 选择 Arm C 编译器；它必须在 `PATH` 中 |
| `CMAKE_ASM_COMPILER` | 让启动汇编也由 Arm 工具链处理 |
| `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY` | CMake 做编译器检查时避免要求生成可运行的裸机程序 |

文件中的 `TARGET_FLAGS` 包含 `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`。这些选项应与 STM32F429 的 CPU/FPU 和所用运行库一致。链接选项使用 `-T STM32F429XX_FLASH.ld` 指定 Flash/RAM 布局，使用 `--specs=nano.specs`，并通过 `-Wl,-Map=lab1.map` 生成 MAP 文件；`--gc-sections` 配合按函数/数据分段的编译选项移除未使用的段。

这些配置说明了**构建目标是谁**，而 `CMakeLists.txt` 说明了**要构建什么**。更换芯片时，设备宏、启动文件、链接脚本和 CPU/FPU 选项需要一起核对。

## 5. Debug、Release 和 Presets

在 Ninja 构建下，`CMAKE_BUILD_TYPE` 选择一套配置。当前工具链文件定义：

| 配置 | C 编译选项 | 典型用途 | 输出目录 |
| --- | --- | --- | --- |
| Debug | `-O0 -g3` | 源码断点和单步 | `build/Debug/` |
| Release | `-Os -g0` | 优化固件体积 | `build/Release/` |

工程的 `CMakePresets.json` 把完整命令的参数保存起来。文件中的 `"version": 3` 是 **Presets 文件格式版本**，不是 CMake 程序版本。隐藏的 `default` 预设统一设置 Ninja、工具链文件、`build/${presetName}` 和编译数据库；`Debug`、`Release` 继承这些设置并指定各自的 `CMAKE_BUILD_TYPE`。`buildPresets` 再把构建动作绑定到对应配置。[CMake Presets 官方说明](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)。

在工程根目录运行：

```text
cmake --list-presets
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
```

`cmake --preset Debug` 与第 2 节的显式 Debug 配置命令作用相同；`cmake --build --preset Debug` 与 `cmake --build build/Debug` 作用相同。预设**不会**自动烧录芯片；烧录仍需 OpenOCD。更多对照见[第三阶段](LAB1_STAGE_03_CMAKE_PRESETS.md)。

## 6. 观察真实构建结果

按顺序运行以下命令，不需要修改源码，也不需要连接开发板：

```text
cmake --preset Debug
cmake --build --preset Debug --verbose
arm-none-eabi-size build/Debug/lab1.elf
cmake --preset Release
cmake --build --preset Release
arm-none-eabi-size build/Release/lab1.elf
```

第一次构建时，观察 `-I` 头文件路径、`-DSTM32F429xx` 宏、`-mcpu=cortex-m4` 和链接脚本选项。若之前已经构建且源码未改变，Ninja 可能显示 `no work to do`；此时打开 `build/Debug/compile_commands.json` 也能看到每个 C 文件的编译命令。比较两种配置的 `text`、`data`、`bss`；字节数随编译器版本变化。

完成后尝试口头解释：为什么 `.elf` 出现在 `build/Debug`，为什么编译命令里有 `-DSTM32F429xx`，为什么要在第一次配置时指定工具链文件。

## 7. 常见问题

| 现象 | 先检查什么 |
| --- | --- |
| `CMakeLists.txt` 找不到 | 当前目录应是 `lab1`，或给 `-S` 指定正确路径 |
| CMake 找不到 Ninja 或 Arm GCC | 运行 `ninja --version`、`arm-none-eabi-gcc --version`，检查新终端的 `PATH` |
| 修改工具链文件后仍用旧编译器 | 当前构建目录已有缓存；改用新的构建目录重新配置 |
| `No such preset` | 当前目录是否有 `CMakePresets.json`；预设名区分大小写 |
| `build/Debug` 与 `build/Release` 混淆 | 配置与构建命令使用同一个预设或同一个构建目录 |
| 生成 ELF 但没有 LED 现象 | CMake 只负责构建；按[第二阶段](LAB1_STAGE_02_CLI_BUILD_FLASH_DEBUG.md)连接开发板并烧录 |

继续学习可从 [CMake 官方教程](https://cmake.org/cmake/help/latest/guide/tutorial/index.html) 的目标和缓存章节开始；回到[实验入口](README.md)可按五阶段完成整个 Lab1。
