# 实验 1 · 第三阶段：CMake Presets 构建

先完成[第二阶段的完整命令行流程](LAB1_STAGE_02_CLI_BUILD_FLASH_DEBUG.md)。本阶段仍在终端操作：阅读工程已有的 `CMakePresets.json`，用短命令复现第二阶段的配置与编译，并分别构建 Debug、Release。所有命令在 `lab1` 工程根目录运行。

## 1. 读懂现有预设

工程根目录的 `CMakePresets.json` 使用版本 3，定义了隐藏的 `default` 配置、`Debug` 和 `Release` 配置，以及同名构建预设。`Debug` 和 `Release` 从 `default` 继承 Ninja、工具链文件、构建目录模板和编译数据库选项。

| 第二阶段显式参数 | `CMakePresets.json` 中的对应项 |
| --- | --- |
| `-G Ninja` | `"generator": "Ninja"` |
| `-B build/Debug` | `"binaryDir": "${sourceDir}/build/${presetName}"`，Debug 预设名展开为 `Debug` |
| `-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake` | `"toolchainFile": "${sourceDir}/cmake/gcc-arm-none-eabi.cmake"` |
| `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` | `"cacheVariables": { "CMAKE_EXPORT_COMPILE_COMMANDS": true }` |
| `-DCMAKE_BUILD_TYPE=Debug` | Debug 配置中的 `"CMAKE_BUILD_TYPE": "Debug"` |

`default` 是隐藏预设，只供其他预设继承。`configurePresets` 管理配置参数；`buildPresets` 通过 `configurePreset` 指向对应的配置。`Release` 输出到 `build/Release`，工程工具链为其设置 `-Os -g0`；Debug 使用 `-O0 -g3`。两个目录的 CMake 缓存和目标文件互不混用。

## 2. 构建 Debug

```text
cmake --list-presets
cmake --preset Debug
cmake --build --preset Debug
arm-none-eabi-size build/Debug/lab1.elf
```

`cmake --preset Debug` 对应第二阶段的完整配置命令；`cmake --build --preset Debug` 对应 `cmake --build build/Debug`。配置阶段应生成 `build/Debug/compile_commands.json`，构建阶段应更新 `build/Debug/lab1.elf` 和 `.map` 文件。

若要再次烧录 Debug 固件，仍使用第二阶段的 OpenOCD 命令：

```text
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/Debug/lab1.elf verify reset exit"
```

## 3. 构建并比较 Release

```text
cmake --preset Release
cmake --build --preset Release
arm-none-eabi-size build/Debug/lab1.elf
arm-none-eabi-size build/Release/lab1.elf
```

对比两行输出的 `text`、`data`、`bss`；具体大小取决于工具链版本，不必与示例数字完全一致。Release 的 `-Os` 侧重体积优化，`-g0` 不保留适合源码级调试的符号；下一阶段 F5 调试选 Debug。

如果 CMake 报 `No such preset`，检查当前目录是否含 `CMakePresets.json`，并注意 `Debug`、`Release` 的大小写。如果构建目录先前使用了不同生成器或工具链，先检查相应目录的 `CMakeCache.txt`；必要时仅清理该构建目录后重新配置。

## 阶段验收

- [ ] 能指出预设中生成器、工具链、构建目录、编译数据库和构建类型的字段。
- [ ] `cmake --preset Debug` 与 `cmake --build --preset Debug` 均成功。
- [ ] Release 固件输出在 `build/Release`，且可与 Debug 固件比较段大小。

下一步：[第四阶段：VS Code、clangd 与图形界面调试](LAB1_STAGE_04_VSCODE_CLANGD.md)。
