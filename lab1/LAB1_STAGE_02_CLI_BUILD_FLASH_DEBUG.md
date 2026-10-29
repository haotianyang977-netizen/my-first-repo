# 实验 1 · 第二阶段：完整命令行编译、烧录与调试

先完成[第一阶段的工具安装与检查](LAB1_STAGE_01_CLI_ENVIRONMENT.md)。本阶段全程在终端操作，显式写出 CMake 配置参数，再用 OpenOCD 和 GDB 完成板上验证。命令均从 `lab1` 工程根目录执行；Windows PowerShell 与 Linux 终端都可直接复制这里的单行命令。

## 1. 认识现有工程

| 文件或目录 | 作用 |
| --- | --- |
| `CMakeLists.txt` | 列出应用源码、HAL 源码、头文件目录和宏定义 |
| `cmake/gcc-arm-none-eabi.cmake` | 指定交叉编译器、Cortex-M4F 参数、Debug/Release 优化选项 |
| `STM32F429XX_FLASH.ld` | 定义 Flash、RAM、CCMRAM 和各段的链接位置 |
| `startup_stm32f429xx.s` | 提供中断向量表和复位入口 |
| `Core/Src/main.c`、`Core/Src/gpio.c` | 设置系统时钟、PB1 输出与 LED1 闪烁 |
| `Drivers/` | CMSIS 与 STM32F4 HAL 驱动 |

程序用 25 MHz 外部晶振配置 180 MHz 系统时钟。LED1 接在 PB1，低电平点亮；主循环点亮 500 ms、熄灭 500 ms。请确认使用的是本课程对应的开发板和晶振配置。

## 2. 显式参数配置与编译

### 2.1 配置 Debug 构建目录

```text
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

| 参数 | 作用 |
| --- | --- |
| `-S .` | 使用当前目录的 `CMakeLists.txt` |
| `-B build/Debug` | 把 Debug 缓存与产物放在独立目录 |
| `-G Ninja` | 生成 Ninja 构建文件 |
| `-DCMAKE_BUILD_TYPE=Debug` | 使用工程定义的 `-O0 -g3`，便于源码调试 |
| `-DCMAKE_TOOLCHAIN_FILE=...` | 使用 Arm 交叉工具链与 STM32 链接选项 |
| `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` | 生成供 clangd 使用的编译数据库 |

这里手动输入所有参数，不使用第三阶段的预设。Release 构建也可按同样方式配置到 `build/Release`，同时将 `CMAKE_BUILD_TYPE` 改为 `Release`；本阶段的烧录与 GDB 使用 Debug 产物。

### 2.2 编译并检查产物

```text
cmake --build build/Debug
arm-none-eabi-size build/Debug/lab1.elf
```

构建成功后应有以下文件：

| 路径 | 用途 |
| --- | --- |
| `build/Debug/lab1.elf` | 含程序段和调试符号的固件，用于烧录与 GDB |
| `build/Debug/lab1.map` | 链接映射与符号地址 |
| `build/Debug/compile_commands.json` | 每个源码文件的实际编译命令，供第四阶段 clangd 使用 |

`arm-none-eabi-size` 输出的 `text` 主要是代码与只读数据，`data` 是有初值的全局/静态数据，`bss` 是未初始化的全局/静态数据。程序启动时把 `data` 初值从 Flash 复制到 RAM，并清零 `bss`。Flash 大致占用 `text + data`，RAM 静态占用 `data + bss`；堆与栈另计。具体字节数随工具链版本变化。

## 3. 连接开发板并烧录

给开发板供电，将 ST-Link 的 SWDIO 接 PA13、SWCLK 接 PA14、GND 接 GND。若 ST-Link 接口需要目标电压参考，按开发板和调试器标识连接 3V3/Vref；先核对供电方式，避免两个供电输出直接相连。

在工程根目录执行：

```text
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/Debug/lab1.elf verify reset exit"
```

`interface/stlink.cfg` 选择 ST-Link，`target/stm32f4x.cfg` 选择 STM32F4 目标。`program` 写入 ELF，`verify` 回读校验，`reset` 让程序运行，`exit` 释放调试器。日志中应出现校验成功信息。观察 LED1：点亮约 500 ms、熄灭约 500 ms，持续交替。

## 4. 双终端 GDB 在线调试

GDB 客户端经本机 TCP 3333 端口连接 OpenOCD；OpenOCD 经 USB/ST-Link 和 SWD 访问 STM32。烧录命令执行完毕后会退出，不会占用 ST-Link。

### 4.1 终端 1：启动 OpenOCD

```text
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

保持该终端运行，确认日志出现 `Listening on port 3333 for gdb connections`。

### 4.2 终端 2：启动 GDB

打开第二个终端并进入同一工程根目录。Ubuntu / Debian 使用：

```text
gdb-multiarch build/Debug/lab1.elf
```

若安装的是 Arm 工具链自带的 GDB（Windows 通常如此），使用：

```text
arm-none-eabi-gdb build/Debug/lab1.elf
```

出现 `(gdb)` 提示符后，逐行输入下列命令；提示符本身不需要输入。

```gdb
target remote :3333
load
monitor reset halt
break main
continue
```

`target remote` 建立连接，`load` 把当前 ELF 写入 Flash，`monitor reset halt` 复位并停机，`break main` 设置断点，`continue` 运行到 `main()`。成功时 GDB 会显示 `Core/Src/main.c` 的当前源码行。

在 `main()` 附近继续练习：

```gdb
next
step
info registers
print /x $pc
print LED1_Pin
x/4xw 0x40020400
stepi
```

`next` 执行当前源码行但不进入函数，`step` 进入函数；请在 `GPIO_Init()` 调用处执行 `step`，观察 `Core/Src/gpio.c`。`info registers` 查看 Cortex-M4 寄存器，`$pc` 是当前指令地址，`x/4xw` 以 16 进制查看 GPIOB 基地址 `0x40020400` 起的四个 32 位字，`stepi` 执行一条机器指令。继续用 `next` 走到主循环，执行写 PB1 的两行代码，观察 LED1 状态变化。单步停机时 LED 不会持续闪烁。

结束时在 GDB 输入 `quit`，再在终端 1 按 `Ctrl+C` 停止 OpenOCD。

## 5. 常见问题

| 现象 | 排查 |
| --- | --- |
| CMake 找不到 Arm GCC 或 Ninja | 回到第一阶段检查 `PATH`，打开新终端重试 |
| OpenOCD 找不到 ST-Link | 检查 USB 数据线、驱动、开发板供电和 SWD 接线 |
| Linux 出现 `LIBUSB_ERROR_ACCESS` | 检查发行版 udev 规则、当前用户的 USB 访问权限，重新登录后拔插 ST-Link |
| `Target not examined yet` | 检查目标板供电、SWDIO/SWCLK/GND 和目标电压参考 |
| GDB 提示 `Connection refused` | 确认终端 1 的 OpenOCD 正在监听 3333 端口 |
| 烧录成功但 LED 不闪 | 检查供电、BOOT0、25 MHz 晶振和 LED1 是否确实连接 PB1 |

## 阶段验收

- [ ] 显式执行 CMake 配置和构建，得到 Debug ELF、MAP 和编译数据库。
- [ ] 能解释 `text`、`data`、`bss` 的含义。
- [ ] OpenOCD 校验烧录成功，LED1 每隔约 500 ms 改变亮灭状态。
- [ ] GDB 能连接、停在 `main`、单步并查看寄存器和 GPIOB 地址。

下一步：[第三阶段：CMake Presets](LAB1_STAGE_03_CMAKE_PRESETS.md)。
