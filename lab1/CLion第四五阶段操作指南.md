# 实验 1 第四、五阶段 · macOS + CLion 版操作指南

> 这是给「不用 VS Code、改用 CLion」的同学写的补充说明。
> 前三阶段（工具链、编译、Presets）已在命令行全部完成并通过验收。
> 本实验不需要插开发板，第四阶段做到「能编译、能跳转」即可，第五阶段只需理解寄存器、无需真板。

---

## 先确认环境（已经全部就绪）

| 组件 | 状态 |
|---|---|
| Arm GNU Toolchain 15.3.Rel2 | ✅ 已装，含 gcc/gdb/size/newlib |
| CMake 4.4.5 / Ninja 1.13.2 / OpenOCD 0.12.0 | ✅ 已装 |
| clangd 官方版 23.1.4 | ✅ 已装（`/opt/homebrew/opt/llvm/bin/clangd`）|
| CLion 2023.3.6 | ✅ 已装 |
| build/Debug/lab1.elf + .map + compile_commands.json | ✅ 已生成 |

---

## 第四阶段：用 CLion 打开工程并验证跳转

### 1. 打开工程
CLion → **Open**，选择目录 `~/Desktop/嵌入式/lab1`（选包含 `CMakeLists.txt` 的那层，别选上层 `嵌入式` 文件夹）。

CLion 会自动读取 `CMakeLists.txt` 和 `CMakePresets.json`，右下角可能弹出「Load CMake project」——点 **Load**。

### 2. 选择工具链
CLion 默认用系统编译器，必须切到 Arm 交叉工具链：
1. 打开 **Settings**（`Cmd + ,`）→ **Build, Execution, Deployment → Toolchains**
2. 新建一个工具链，把 C Compiler / C++ Compiler 指向：
   ```
   /Applications/ArmGNUToolchain/15.3.rel2/arm-none-eabi/bin/arm-none-eabi-gcc
   /Applications/ArmGNUToolchain/15.3.rel2/arm-none-eabi/bin/arm-none-eabi-g++
   ```
3. Debugger 指向同目录的 `arm-none-eabi-gdb`

> 提示：其实这个工程已经用 `CMakePresets.json` + `gcc-arm-none-eabi.cmake` 工具链文件锁定了交叉编译器，CLion 的 Toolchains 设置主要是为了让代码分析、跳转、调试器一致。核心编译仍然走 presets。

### 3. 选 CMake Profile
CLion 会识别 `CMakePresets.json` 里的 `Debug` / `Release` 两个 profile。在顶部的 **构建配置下拉框** 选 **Debug**。

### 4. 验证 clangd 跳转（对应实验的 F12 跳转）
1. 打开 `Core/Src/main.c`
2. 光标停在 `HAL_Init()` 上，按 `Cmd + B`（或按住 `Cmd` 点击）
3. 应跳到 `Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c` 里的 `HAL_Init` 定义
4. 输入 `HAL_GPIO_` 应立刻出现补全列表

> 如果跳转失灵，确认 `build/Debug/compile_commands.json` 存在（已存在），并在 CLion 里执行 **Tools → Compilation Database → Reload**。
> 工程根的 `.clangd` 已配置 `CompilationDatabase: build/Debug`，CLion 的 clangd 内核会读它。

### 5. 构建（对应实验的 Ctrl+Shift+B）
CLion 里点工具栏的 **绿色锤子 Build**（或 `Cmd + F9`），会走 Debug profile，产物更新到 `build/Debug/lab1.elf`。

---

## 第五阶段：理解寄存器（无需真板）

第五阶段的「外设视图」是 VS Code 专属扩展（Peripheral Viewer），**CLion 没有等价图形面板**。但本实验不插板子，验收只要求你能解释寄存器变化。下面直接讲透。

### LED1 亮灭背后的寄存器（PB1）

STM32F429 上 LED1 接 **PB1**，低电平点亮。主循环：
```c
HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);  // 亮
HAL_Delay(500);
HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);    // 灭
HAL_Delay(500);
```

涉及三个寄存器，都在 GPIOB 外设（基地址 `0x40020400`）：

| 寄存器 | 偏移 | 地址 | 字段 | 亮（RESET 后）| 灭（SET 后）|
|---|---|---|---|---|---|
| MODER | 0x00 | 0x40020400 | MODER1 | `01`（输出）| `01`（不变）|
| ODR | 0x14 | 0x40020414 | ODR1 | `0` | `1` |
| （RCC）AHB1ENR | — | 0x40023830 | GPIOBEN(bit1) | `1` | `1` |

关键点：
- **MODER1 = 01**：PB1 被配成通用输出模式（由 `GPIO_Init()` 里 `GPIO_MODE_OUTPUT_PP` 设置）
- **ODR1 = 0 时灯亮，= 1 时灯灭**：因为 LED 是低电平点亮（active-low），`GPIO_PIN_RESET` 把 ODR1 写 0，`GPIO_PIN_SET` 写 1
- **GPIOBEN = 1**：`__HAL_RCC_GPIOB_CLK_ENABLE()` 打开了 GPIOB 的时钟，没这一步寄存器根本读不到值

### 用 GDB 亲自验证（不插板子也能跑）

可以不连硬件，直接对编译出的 ELF 用 GDB 反汇编/查符号，理解地址：

```bash
cd ~/Desktop/嵌入式/lab1
arm-none-eabi-gdb build/Debug/lab1.elf
```

在 `(gdb)` 提示符下：
```
(gdb) info address HAL_GPIO_WritePin     # 看这个函数的地址
(gdb) disassemble main                    # 看 main 的反汇编
(gdb) x/4xw 0x40020400                    # 模拟看 GPIOB 寄存器区（不连板子读不到真值，但地址正确）
(gdb) quit
```

> 说明：`x/4xw 0x40020400` 在没连目标板时读不到真实值（会显示无法访问内存），这一步的真正目的是**验证地址和字段位置**，跟 SVD 文件里的描述对上。真正读值需要 ST-Link 连板子。

### SVD 文件本身
工程里的 `svd/STM32F429.svd` 就是描述这些寄存器地址和字段名的 XML。CLion 虽然不能像 VS Code 那样可视化展开，但你可以直接打开这个 XML 文件，搜 `0x40020400` 能看到 MODER/ODR 的定义，验证上面的表。

---

## 一句话总结

- **第四阶段**：CLion 能替代 VS Code 完成「编译 + clangd 跳转」，F5 图形调试需要板子（本实验跳过）
- **第五阶段**：寄存器用文字 + GDB 反汇编理解即可，不插板子
- **CLion 唯一做不了**的是 Peripheral Viewer 可视化外设视图，但那不是本实验（不插板）的验收点

---

## 附：如果之后老师要求 VS Code 版

VS Code 下载：https://code.visualstudio.com/download （选 Apple Silicon 版）
装完在扩展市场装：`clangd`（llvm-vs-code-extensions）、`Cortex-Debug`（marus25）、`Peripheral Viewer`（mcu-debug）
工程的 `.vscode/` 配置已就绪，打开 `lab1` 目录即可用 F5 调试（需板子）。
