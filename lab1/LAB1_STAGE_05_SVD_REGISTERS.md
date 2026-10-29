# 实验 1 · 第五阶段：用 SVD 查看外设寄存器

先完成[第四阶段的 VS Code 板上调试](LAB1_STAGE_04_VSCODE_CLANGD.md)，确认按 `F5` 能停在 `main()`。本阶段将工程自带的 `svd/STM32F429.svd` 接入调试配置，在图形界面中查看 RCC 与 GPIOB 的寄存器和字段，解释 LED1（PB1）亮灭时的寄存器变化。

## 1. 认识 SVD 文件

SVD（System View Description）是描述芯片外设、寄存器地址和位字段的 XML 文件。工程中的 [`STM32F429.svd`](svd/STM32F429.svd) 来自 STMicroelectronics 的 STM32CubeCLT，设备名为 `STM32F429`、SVD 版本为 `1.9`；文件头标注 Apache-2.0，许可证副本在 [`svd/LICENSE`](svd/LICENSE)。[ST 的 STM32CubeCLT 说明](https://www.st.com/resource/en/user_manual/um3088-stm32cube-commandline-toolset-quick-start-guide-stmicroelectronics.pdf)列出 SVD 文件作为工具包内容。

SVD 只提供寄存器的名称和地址。调试时，外设视图通过调试器读取目标芯片当前值；仅打开 SVD 文件不会显示实时寄存器值，也不会修改固件。

## 2. 接入 VS Code 调试配置

在 VS Code“扩展”中安装并启用 **Peripheral Viewer**（`mcu-debug.peripheral-viewer`）。工程的 `.vscode/extensions.json` 只是安装建议，不会自动安装扩展。当前扩展注册的视图名称是 **`XPeripherals`**；只有扩展读到外设数据后，这个视图才会在“运行和调试”侧栏显示。[扩展视图定义](https://github.com/mcu-debug/peripheral-viewer/blob/master/package.json)、[Cortex-Debug 配置项](https://github.com/Marus/cortex-debug/blob/master/debug_attributes.md)。

打开工程自带的 [`.vscode/launch.json`](.vscode/launch.json)，确认配置中包含以下字段：

```text
"svdFile": "${workspaceFolder}/svd/STM32F429.svd"
```

`svdFile` 使用工程内的路径，因此 Windows 与 Linux 可以共用同一份文件。[MCU Peripheral Viewer 的配置说明](https://github.com/mcu-debug/peripheral-viewer#specifying-svd-files)确认：与 Cortex-Debug 配合时可通过 `svdFile` 提供 SVD 路径。

若你手头的配置缺少 `svdFile`，请以工程根目录的 `.vscode/launch.json` 为准。`launch.json` 的“属性不允许”提示需要先核对文件位置，以及当前窗口是否已安装并启用 Cortex-Debug；这些字段不能直接放进 `settings.json`。

## 3. 停机观察 GPIOB

1. 给开发板供电，确认没有单独运行的 OpenOCD 进程。在 VS Code 中选择 `STM32 Debug (OpenOCD)` 配置，按 `F5` 启动调试，程序应停在 `main()`。
2. **F5 调试已启动并停在 `main()` 后**，在“运行和调试”侧栏找到 `XPeripherals`，展开 `RCC` 和 `GPIOB`。若没有看到该面板，先检查扩展是否安装并启用、是否使用了这份 `launch.json`，再检查 SVD 路径。
3. 在 `Core/Src/main.c` 的两条 `HAL_Delay(500);` 上分别设置断点：第一条紧跟 `GPIO_PIN_RESET` 写入，第二条紧跟 `GPIO_PIN_SET` 写入。
4. 点“继续”到第一条延时语句；此时 LED1 应点亮。查看 `RCC → AHB1ENR → GPIOBEN`、`GPIOB → MODER → MODER1` 和 `GPIOB → ODR → ODR1`。
5. 再点“继续”到第二条延时语句；此时 LED1 应熄灭。比较 `ODR1` 的值。要再次比较，继续运行到下一轮循环的第一条延时语句。

| 字段 | 预期观察 | 与程序的关系 |
| --- | --- | --- |
| `RCC.AHB1ENR.GPIOBEN` | `1` | `GPIO_Init()` 已使能 GPIOB 时钟 |
| `GPIOB.MODER.MODER1` | `01`（输出模式） | PB1 已配置为通用输出 |
| `GPIOB.ODR.ODR1` | 第一条延时处为 `0`；第二条延时处为 `1` | LED1 低电平亮、高电平灭 |

SVD 中 GPIOB 基地址是 `0x40020400`；`MODER` 偏移 `0x00`，`ODR` 偏移 `0x14`，因此 `GPIOB.ODR` 地址是 `0x40020414`。`RCC.AHB1ENR` 位于 `0x40023830`，`GPIOBEN` 是 bit 1。可用第二阶段学过的 GDB 内存查看命令核对：`x/1xw 0x40020414`。`GPIOB.BSRR` 是写入置位/复位寄存器，不适合作为亮灭状态的读取依据；请比较 `ODR1`。

外设视图主要在目标**暂停**时更新。程序持续运行期间看到的数值可能是上一次停机时的快照；每次在断点停住后再比较。

## 4. 常见问题与验收

| 现象 | 排查 |
| --- | --- |
| `XPeripherals` 未出现 | 先确认扩展列表中有 `mcu-debug.peripheral-viewer` 和 `marus25.cortex-debug`；F5 成功停机后再看“运行和调试”侧栏。扩展输出可在“查看 → 输出”中选择 `Peripheral Viewer` |
| `launch.json` 提示“属性不允许” | 确认文件位于当前打开的工程根目录下的 `.vscode/launch.json`，并重新加载已启用 Cortex-Debug 的窗口 |
| 显示 `No SVD File Loaded` | 确认打开的是工程根目录，且 `svd/STM32F429.svd` 存在、`svdFile` 拼写正确 |
| 寄存器值不可读或未更新 | 确认目标已在断点暂停、ST-Link 已连接；不要用只写的 `BSRR` 验证状态 |
| `GPIOBEN` 或 `MODER1` 与预期不符 | 确认已执行完 `GPIO_Init()`，断点应设在主循环的延时语句 |

- [ ] 能说明 SVD 文件提供地址和字段名称，实际数值来自目标芯片。
- [ ] `XPeripherals` 能展开 RCC、GPIOB 寄存器和字段。
- [ ] 能在两条延时语句的断点处观察 `GPIOB.ODR.ODR1` 从 `0` 变为 `1`，并解释 LED1 状态。

完成后可回到[实验入口](README.md)复核五个阶段的验收目标。
