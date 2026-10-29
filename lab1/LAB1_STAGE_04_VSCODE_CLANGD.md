# 实验 1 · 第四阶段：VS Code 与 clangd 图形界面配置

先完成[第三阶段的预设构建](LAB1_STAGE_03_CMAKE_PRESETS.md)，确认 `build/Debug/compile_commands.json` 与 `build/Debug/lab1.elf` 已生成。本阶段通过 VS Code 的扩展、构建任务和“运行和调试”界面操作同一套 CMake、OpenOCD、GDB 工具。

> [!IMPORTANT]
> **打开目录说明**：请务必通过 VS Code 的 **“文件 → 打开文件夹”（File → Open Folder）** 单独打开 **`lab1` 工程根目录**（即包含 `CMakePresets.json`、`CMakeLists.txt` 与 `.vscode/` 的目录），而不是上层大仓库目录。
> 这样才能保证 `${workspaceFolder}` 正确指向工程根目录，且 `${workspaceFolderBasename}` 自动解析为 `lab1`（与 `CMakeLists.txt` 中声明的 `project(lab1)` 输出产物 `lab1.elf` 匹配）。

---

## 1. 安装编辑与调试工具

1. 安装 [VS Code](https://code.visualstudio.com/download) 和 [clangd](https://clangd.llvm.org/installation)。
   - **Ubuntu / Debian**：在终端执行：
     ```bash
     sudo apt update && sudo apt install clangd gdb-multiarch
     ```
   - **Windows**：根据 [clangd 官方安装文档](https://clangd.llvm.org/installation)，Windows 平台提供以下两种常见安装方式（推荐方式一）：
     - **方式一（推荐，独立绿色包 Standalone .zip Releases）**：
       访问 [clangd 最新 Releases (GitHub)](https://github.com/clangd/clangd/releases/latest)，下载 Windows 版本的压缩包（如 `clangd-windows-*.zip`）。解压到本地固定目录（例如 `C:\Develop\clangd`），将其包含 `clangd.exe` 的 `bin` 目录（例如 `C:\Develop\clangd\bin`）添加至 Windows 系统的环境变量 `Path` 中。
     - **方式二（完整 LLVM 安装包 LLVM Installer）**：
       访问 [LLVM Releases (GitHub)](https://github.com/llvm/llvm-project/releases) 或 [LLVM 官网下载页](https://releases.llvm.org/download.html)，下载适用于 Windows 的预编译安装程序（如 `LLVM-xx.x.x-win64.exe`）。运行安装向导时，务必勾选 **“Add LLVM to the system PATH”**（为当前用户或所有用户添加至系统 PATH），安装完成后其自带的 `clangd.exe` 即全局可用。
     > *（备选方式）*：若未预先安装，在 VS Code 中安装完 `clangd` 插件后首次打开 C 文件时，扩展通常会检测 PATH 并弹出提示：“Clangd language server is not installed. Download clangd?”，点击 Install 即可由插件自动下载独立二进制并管理。
2. 在新终端运行 `clangd --version`，确认命令可用。若 VS Code 启动于安装前，请重新启动 VS Code。
3. 打开 `lab1` 目录后，VS Code 右下角通常会提示安装推荐扩展，点击 **Install All** 即可；或在 VS Code 的“扩展”界面安装：
   - **clangd**（`llvm-vs-code-extensions.vscode-clangd`）：提供高精度的 C 语言补全、宏展开与语法跳转。
   - **Cortex-Debug**（`marus25.cortex-debug`）：提供图形化 ARM Cortex-M 芯片调试环境。
   - **Peripheral Viewer**（`mcu-debug.peripheral-viewer`）：第五阶段查看外设寄存器必需。
   （工程的 `.vscode/extensions.json` 已预置这三款推荐扩展）。
4. 打开 VS Code 内置终端（快捷键 `` Ctrl+` ``），运行 `cmake --preset Debug`。若找不到命令，检查第一阶段的 `Path` 设置并重新启动 VS Code。

> [!TIP]
> 若同时安装了 Microsoft 官方的 **C/C++** 扩展（`ms-vscode.cpptools`），VS Code 可能会提示两者提供重叠的语言服务。建议在当前工作区禁用 C/C++ 扩展的 IntelliSense（可在设置中搜索 `intelliSenseEngine` 设为 `disabled`），以免同一份 C 代码出现两份红波浪线；代码调试功能仍由 Cortex-Debug 负责。

---

## 2. 让 clangd 读取 Debug 编译数据库

工程根目录已有 `.clangd` 文件，内容如下：

```yaml
CompileFlags:
  CompilationDatabase: build/Debug
```

clangd 会自动读取 `build/Debug/compile_commands.json` 中的真实宏定义（如 `-DSTM32F429xx`）、头文件包含路径（如 HAL 库目录）和编译参数。需要按 Release 的参数分析代码时，可以临时将路径改成 `build/Release`，并先在终端运行 `cmake --preset Release`。不要把路径只写成 `build/`：本实验的编译数据库位于其 `Debug` 或 `Release` 子目录。

为确保 clangd 能正确识别 Arm 交叉编译器自带的系统头文件（如 `<stdint.h>`、`<stdbool.h>`），工程根目录的 [`.vscode/settings.json`](.vscode/settings.json) 已预置参数：

```json
{
  "clangd.arguments": [
    "--query-driver=**/*arm-none-eabi*"
  ]
}
```

- `--query-driver` 告诉 clangd 允许调用匹配 `*arm-none-eabi*` 的交叉编译器，以提取其内置的头文件搜索路径和预定义宏。
- 配置保存后，按 `Ctrl+Shift+P` 打开命令面板，输入并执行 `clangd: Restart language server` 重启语言服务器。
- 若 clangd 扩展仍提示找不到可执行文件，可在扩展设置中指定本机 `clangd` 的完整可执行文件路径。

**语法跳转验证**：
打开 `Core/Src/main.c`，按住 `Ctrl` 点击 `HAL_Init()`（或直接按 `F12`），应能精准跳转至 `stm32f4xx_hal.c` 中的函数实现；在编辑器中输入 `HAL_GPIO_`，应能即时看到代码补全建议列表。

---

## 3. 配置图形界面构建任务

工程已提供 [`.vscode/tasks.json`](.vscode/tasks.json)。在资源管理器中打开并核对以下内容。每个 Build 任务通过 `dependsOn` 先运行对应的配置任务，确保在首次拉取代码或更换电脑后，也能直接从图形界面完成配置与编译：

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Configure Debug",
      "type": "process",
      "command": "cmake",
      "args": ["--preset", "Debug"],
      "problemMatcher": []
    },
    {
      "label": "Build Debug",
      "type": "process",
      "command": "cmake",
      "args": ["--build", "--preset", "Debug"],
      "dependsOn": "Configure Debug",
      "problemMatcher": [],
      "group": { "kind": "build", "isDefault": true }
    },
    {
      "label": "Configure Release",
      "type": "process",
      "command": "cmake",
      "args": ["--preset", "Release"],
      "problemMatcher": []
    },
    {
      "label": "Build Release",
      "type": "process",
      "command": "cmake",
      "args": ["--build", "--preset", "Release"],
      "dependsOn": "Configure Release",
      "problemMatcher": []
    }
  ]
}
```

**任务运行方式**：
- **构建 Debug（默认任务）**：按快捷键 `Ctrl+Shift+B`，VS Code 会在下方终端自动执行配置和构建，产物生成在 `build/Debug/lab1.elf`。
- **构建 Release**：按 `Ctrl+Shift+P`，输入 `Tasks: Run Task` 并回车，选择 `Build Release`，产物生成在 `build/Release/lab1.elf`。

---

## 4. 配置 F5 板上调试

工程已提供 [`.vscode/launch.json`](.vscode/launch.json)。文件预置了通用的 `STM32 Debug (OpenOCD)` 调试配置，支持 `${workspaceFolderBasename}.elf` 动态解析固件路径，并通过 `linux` 平台覆盖块实现跨平台兼容：

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "STM32 Debug (OpenOCD)",
      "type": "cortex-debug",
      "request": "launch",
      "servertype": "openocd",
      "cwd": "${workspaceFolder}",
      "device": "STM32F429IG",
      "executable": "${workspaceFolder}/build/Debug/${workspaceFolderBasename}.elf",
      "svdFile": "${workspaceFolder}/svd/STM32F429.svd",
      "configFiles": ["interface/stlink.cfg", "target/stm32f4x.cfg"],
      "gdbPath": "arm-none-eabi-gdb",
      "linux": {
        "gdbPath": "gdb-multiarch"
      },
      "runToEntryPoint": "main",
      "preLaunchTask": "Build Debug"
    }
  ]
}
```

**关键配置说明**：
- **`${workspaceFolderBasename}.elf`**：打开 `lab1` 目录时，`${workspaceFolderBasename}` 自动替换为 `lab1`，即 `build/Debug/lab1.elf`，与 `CMakeLists.txt` 中声明的工程输出目标对齐，无需针对每个工程手动改名。
- **跨平台 GDB 路径适配**：
  | 操作系统 | 默认 GDB 命令 | 配置字段 | 说明 |
  | --- | --- | --- | --- |
  | **Linux** | `gdb-multiarch` | `linux.gdbPath` | Ubuntu/Debian 系统推荐通过 `apt` 安装 `gdb-multiarch` |
  | **Windows / 其他** | `arm-none-eabi-gdb` | `gdbPath` | Windows 环境下直接调用 Arm GNU 工具链自带的 GDB |
  若本地 GDB 命令不同或存放在非标准路径下，只需修改对应配置字段即可。
- **`preLaunchTask: "Build Debug"`**：每次按 `F5` 启动调试前，VS Code 会先自动调用任务编译最新代码，保证烧录的固件始终是最新的。

**启动单步调试流程**：
1. 用 USB 线连接开发板与电脑并供电，确保 ST-Link 驱动正常。
2. **关键提醒**：如果第二阶段曾在终端手动启动了 `openocd`，必须先按下 `Ctrl+C` 退出，释放 3333 调试端口。
3. 点击 VS Code 左侧侧边栏的 **“运行和调试”** 图标（`Ctrl+Shift+D`），在顶部下拉列表中选择 **`STM32 Debug (OpenOCD)`**。
4. 按 **`F5`** 启动调试：
   - VS Code 会自动调用 CMake 编译 Debug 目标；
   - 自动在后台启动 OpenOCD，连接 ST-Link 并将 `lab1.elf` 烧录进芯片 Flash；
   - 启动 GDB 挂接，芯片复位后停在 `main()` 函数第一行代码处。
5. 在 `Core/Src/main.c` 行号左侧点击红点设置断点；使用顶部悬浮调试控制栏的 **继续（F5）**、**单步跳过（F10）**、**单步单入（F11）** 观察代码运行，在左侧侧边栏查看“变量”与“调用堆栈”。
6. 调试完成后，点击红色方块 **停止（Shift+F5）** 结束调试会话。

> [!NOTE]
> 源码级调试必须使用 Debug 固件。Release 固件使用了 `-g0` 且开启了代码优化（`-Os`），不包含完整调试符号，不适用于单步断点调试流程。

---

## 5. 常见问题排查与验收

| 现象 / 报错 | 根本原因 | 排查与解决方法 |
| --- | --- | --- |
| clangd 提示找不到 `<stdint.h>` 或 HAL 头文件 | 缺少编译数据库或未配置 query-driver | 1. 确认已运行过 `cmake --preset Debug`，且 `build/Debug/compile_commands.json` 存在；<br>2. 检查工程根目录 `.clangd` 与 `.vscode/settings.json` 参数无误；<br>3. `Ctrl+Shift+P` 执行 `clangd: Restart language server` |
| VS Code 任务提示 `No such preset` 或找不到 CMakePresets.json | 打开的工作区目录不正确 | 检查 VS Code 左侧资源管理器顶层目录名是否为 `lab1`。不要打开上层大目录，重新“文件 → 打开文件夹”进入 `lab1` |
| F5 报错提示找不到固件 `lab1.elf` | 尚未生成目标文件或工程名不匹配 | 1. 检查 `CMakeLists.txt` 中是否为 `project(lab1 LANGUAGES C ASM)`；<br>2. 按 `Ctrl+Shift+B` 确保构建成功 |
| F5 报错找不到 GDB（如 `gdb-multiarch: not found`） | 系统缺少 GDB 或环境变量未就绪 | Linux 下执行 `sudo apt install gdb-multiarch`；Windows 下确保 `arm-none-eabi-gdb` 在 `Path` 中 |
| F5 提示 3333 端口被占用（Address already in use） | 后台有残留的 OpenOCD 进程 | 关闭之前终端中手动运行的 OpenOCD，或在终端执行 `killall openocd`（Linux） |
| F5 提示无法连接板卡（Error: open failed） | 物理连接断开或 Linux 下 USB 权限不足 | 检查 ST-Link 连线；Linux 下确认已安装 OpenOCD udev 规则（`/etc/udev/rules.d/60-openocd.rules`）并重插 USB |
| VS Code 打开 launch.json 提示“属性不允许” | 扩展尚未激活或粘贴错文件 | 确认未将调试配置写进 `settings.json`；确认当前窗口已启用 `marus25.cortex-debug` 扩展，执行 `Developer: Reload Window` |

### 阶段验收清单

- [ ] 在 VS Code 中打开 `Core/Src/main.c`，clangd 能正常跳转至 HAL 库定义（`F12`）且无虚假头文件报错。
- [ ] 按 `Ctrl+Shift+B` 能自动完成 Debug 配置与构建，在终端无报错并生成 `build/Debug/lab1.elf`。
- [ ] 按 `F5` 能自动构建、烧录开发板、停在 `main()`，并在图形化界面中单步执行、命中设置的断点。

下一步：[第五阶段：用 SVD 查看外设寄存器](LAB1_STAGE_05_SVD_REGISTERS.md)。
