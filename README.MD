# Ainuo 通用通讯可靠性测试软件

[![Version](https://img.shields.io/badge/version-v3.4.11-blue)](https://github.com)
[![Qt](https://img.shields.io/badge/Qt-5.15-green)](https://www.qt.io/)
[![License](https://img.shields.io/badge/license-MIT-orange)](LICENSE)

一款基于 Qt 的**多协议仪器通讯自动化测试工具**，支持通过 Excel 表格批量发送 SCPI 指令，自动校验设备返回值，统计通讯可靠性。

**V3.4.11 全面启用 AN3.0 协议 HEX 区间判断**，支持 **RGL 系列交流源载一体机**和 **EVH 系列双向直流电源**双产品系列，覆盖 39 条查询命令自动解析。

---

## ✨ 功能特性

| 模块 | 说明 |
|------|------|
| 🔌 **串口通讯** | RS-232 / RS-485，波特率/数据位/停止位/校验位可配置，支持 HEX/ASCII 模式、发送后缀(无/CR/LF/CRLF) |
| 🌐 **网口通讯** | TCP 客户端，支持 Nagle 算法禁用（低延迟），静默超时断开，发送后缀(无/CR/LF/CRLF) |
| 🔬 **GPIB 通讯** | 基于 NI-VISA，支持板卡号/主副地址配置、发送后缀(无/CR/LF/CRLF)、EOI 信号、结束字符控制、连接超时保护 |
| 🚗 **CAN 通讯** | 预留接口，后续版本完善 |
| 📊 **Excel 批量发送** | 读取 .xlsx 命令表，自动逐条发送、校验响应、标记超时/内容错误 |
| 📝 **单条命令调试** | 独立发送页，实时查看收发日志（毫秒级时间戳） |
| 📈 **错误统计** | 6 列详细错误列表（序号/时间/类型/命令/期望值/实际值），颜色区分超时(橙)与内容错误(红) |
| ⏱️ **精确延时** | 1ms 精度命令间隔控制，EMA 误差补偿算法，计时起点在 I/O 前（TX→TX 间隔 = max(I/O耗时, 设定延时)） |
| 🧵 **多线程架构** | I/O 操作独立工作线程，UI 完全无阻塞 |
| 🎯 **ASCII 区间判断** | 支持 ASCII 返回值区间容差校验，自动解析数字并判定是否在容差范围内 |
| 🎯 **AN3.0 HEX 区间判断** | ★ V3.4.11：自动识别命令码、同构解析二进制帧、逐字段 ±tolerance 对比，支持 RGL/EVH 双产品系列 |
| 🎛️ **产品系列切换** | 设置页新增下拉框，一键切换 RGL / EVH 命令表 |
| 🧩 **模块化架构** | Page / PageUI / PageSignals / ErrorHandler 四层分离，公共工具库（RangeComparer / StickySplitter / An30Layout） |
| 🎨 **现代 UI** | 基于 ElaWidgetTools 的 Fluent Design 风格，支持明暗主题切换 |

---

## 🛠️ 技术栈

- **语言:** C++11
- **框架:** Qt 5.15.2
- **编译器:** MinGW 8.1 (64-bit)
- **构建系统:** CMake 3.10+
- **UI 组件:** [ElaWidgetTools](https://github.com/Liniyous/ElaWidgetTools)（Fluent Design 风格）
- **Excel 引擎:** [QXlsx](https://github.com/QtExcel/QXlsx)（跨平台 .xlsx 读写）
- **GPIB 驱动:** NI-VISA（NI-488.2 或独立运行时）

---

## 📁 项目结构

```
ExcelSCPISender/
├── CMakeLists.txt
├── README.md
├── development/
│   ├── Serial_T/                    # 串口通讯模块
│   │   ├── SerialPage.h/.cpp        #   页面协调器
│   │   ├── SerialPageUI.h/.cpp      #   UI 构建（5 个页面）
│   │   ├── SerialPageSignals.h/.cpp #   线程管理 + 信号/槽连接
│   │   ├── SerialErrorHandler.h/.cpp#   错误记录与统计
│   │   ├── SerialWork.h/.cpp        #   串口 I/O 工作线程
│   │   └── SerialExcel.h/.cpp       #   Excel 批量发送
│   ├── Network_T/                   # 网口通讯模块
│   │   ├── NetworkPage.h/.cpp
│   │   ├── NetworkPageUI.h/.cpp
│   │   ├── NetworkPageSignals.h/.cpp
│   │   ├── NetworkErrorHandler.h/.cpp
│   │   ├── NetworkWork.h/.cpp
│   │   └── NetworkExcel.h/.cpp
│   ├── GPIB_T/                      # GPIB 通讯模块
│   │   ├── GPIBPage.h/.cpp
│   │   ├── GPIBPageUI.h/.cpp
│   │   ├── GPIBPageSignals.h/.cpp
│   │   ├── GPIBErrorHandler.h/.cpp
│   │   ├── GPIBWork.h/.cpp
│   │   └── GPIBExcel.h/.cpp
│   ├── CAN_T/                       # CAN 通讯模块（预留）
│   ├── USER_T/
│   ├── Other_T/                     # 核心 / 公共组件
│   │   ├── MainPage.h/.cpp          #   首页 / 帮助 / 关于页面
│   │   ├── ElaWidgetToolsDemo.h/.cpp#   主窗口入口
│   │   ├── LED.h/.cpp               #   状态指示灯组件
│   │   ├── StatCard.h/.cpp          #   统计卡片组件
│   │   ├── RangeComparer.h/.cpp     # ★ 公共区间比较（ASCII + HEX）
│   │   ├── StickySplitter.h/.cpp    # ★ 公共粘包分割
│   │   ├── An30Layout.h/.cpp        # ★ AN3.0 命令码自动匹配布局（V3.4.11）
│   │   └── main.cpp
│   ├── Old/
│   ├── ico/
│   └── ExcelSCPISender.qrc
├── QXlsx/
└── ElaWidgetTools/
```

---

## 🚀 快速开始

### 环境要求

| 依赖 | 版本 | 说明 |
|------|------|------|
| Windows | 10 / 11 (64-bit) | 开发与运行平台 |
| Qt | 5.15.2 | 需包含 `SerialPort`, `Network`, `AxContainer` 模块 |
| CMake | ≥ 3.10 | 构建系统 |
| MinGW | 8.1 (64-bit) | 编译器 |
| NI-VISA | 最新版 | **仅 GPIB 模块需要**，[免费下载](https://www.ni.com/en/support/downloads/drivers/download.ni-visa.html) |

### 克隆项目

```bash
git clone --recurse-submodules https://github.com/your-username/ExcelSCPISender.git
cd ExcelSCPISender
```

> 项目包含 QXlsx 和 ElaWidgetTools 子模块，务必使用 `--recurse-submodules`。

### 构建

**方式一：CLion（推荐）**

1. 用 CLion 打开项目根目录
2. 在 `CMakeLists.txt` 中确认 Qt 和 NI-VISA 路径正确：
   ```cmake
   set(CMAKE_PREFIX_PATH "D:/Apps/Work/Qt/5.15.2/mingw81_64")
   set(VISA_INCLUDE_DIR "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/include")
   set(VISA_LIB_DIR "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/lib_x64/msc")
   ```
3. 选择 `Release-MinGW` 工具链 → 构建

**方式二：命令行**

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" \
  -DCMAKE_PREFIX_PATH="D:/Apps/Work/Qt/5.15.2/mingw81_64"
cmake --build . --target untitled -j 14
```

### 运行

构建成功后，在构建目录下运行 `untitled.exe`。首次运行会自动执行 `windeployqt` 部署 Qt 运行时 DLL。

---

## 📖 使用指南

### 1. 连接设备

- **串口:** 设置 → 选择端口号 / 波特率 → 打开串口
- **网口:** 设置 → 输入 IP / 端口 → 连接网络
- **GPIB:** 设置 → 输入板卡号 / 仪器地址 → 选择发送后缀 → 打开 GPIB

> 连接成功后对应 LED 指示灯变为**绿色**。

### 2. 单条命令发送

在「单条发送」页面直接输入 SCPI 命令（如 `*IDN?`），点击发送，实时查看返回结果。

### 3. Excel 批量发送

1. 点击 **「下载示例模板」** 获取标准 Excel 文件
2. 按模板填写命令表：

   | 发送的命令 (A) | 正确的返回值 (B) | 到下一条命令的时间ms (C) |
      |---------------|-----------------|------------------------|
   | `*IDN?` | 期望的返回值 | 50 |
   | `MEAS:VOLT:DC?` | （留空=不校验） | 200 |

3. 点击 **「打开 Excel 并读取」** 加载文件
4. 设置 **发送次数**（0 = 无限循环）和 **超时时间**
5. 点击 **「开始发送」** 启动批量测试
6. 在 **「发送日志」** 和 **「错误统计」** 页面实时监控

> 💡 **设置命令（无回复）:** 对于 `*RST`、`CONF:VOLT:DC 10` 等设备不回复的设置命令，只需将 B 列留空，软件会自动识别并在 Work 层 emit 空响应，仅按 C 列延时后立即发送下一条。查询命令（如 `*IDN?`）在 B 列填写期望值即可自动校验。

### 4. 读取返回值模式

点击 **「读取返回值」**，软件仅发送一轮命令，并将仪器的实际返回值自动填入 Excel B 列（用于快速建立期望值模板）。

### 5. 发送后缀说明

**串口、网口、GPIB** 三个模块的设置页中均提供「发送后缀」下拉框，支持以下选项：

| 后缀选项 | 实际追加字节 | 适用场景 |
|---------|-------------|---------|
| 无 (None) | 不追加任何字节 | GPIB 仪器仅靠 EOI 硬件信号识别命令结束；部分串口/网口设备无需终止符 |
| CR (`\r`) | 0x0D | 少数仪器使用回车符作为命令终止符 |
| LF (`\n`) | 0x0A | **大多数 SCPI 仪器**使用换行符终止命令 |
| CRLF (`\r\n`) | 0x0D 0x0A | 部分仪器要求回车+换行双字符终止（如某些 PLC 设备） |

> **注意：** 后缀追加的是真实的 **二进制控制字符**（即 `0x0D` / `0x0A`），而非字符串 `\r` `\n`。如果 Excel 命令列中已包含 `\n`，软件会自动去除末尾的 `\r`/`\n` 后再追加所选后缀，避免重复。建议 Excel 中不写 `\n`，统一通过后缀选项控制。

### 6. 后缀处理流程

软件内部通过统一的 `buildSendData()` 方法处理所有发送数据，流程如下：

```
用户输入命令 (如 "*IDN?")
    │
    ▼
Step 1: unescape — 将 \r \n 字面量还原为真实控制字符 0x0D 0x0A
    │
    ▼
Step 2: 去尾 — 去除末尾已有的 \r \n（避免与后缀重复）
    │
    ▼
Step 3: 加后缀 — 根据下拉框选择追加 None / 0x0D / 0x0A / 0x0D+0x0A
    │
    ▼
发送到设备
```

> 此方法同时应用于**单条发送**和 **Excel 批量发送**，确保两条路径的后缀行为完全一致。

### 7. ASCII 区间判断

当设备返回带数字的字符串（如 `3.05\r\n`）时，可启用区间判断功能：

1. 在**设置页面**勾选 **「启用ASCII区间判断」**
2. 在 **「ASCII区间值」** 输入框中填入容差值（如 `0.5`）
3. 在 Excel B 列填入**标准期望值**（如 `3.00\r\n`）
4. 软件会将实际返回值去除 `\r` `\n` 后解析为数字，判定 `|实际值 - 期望值| ≤ 区间值` 即为通过

> **示例：** 期望值 `3.00`，区间值 `0.5` → 实际返回 `2.50` ~ `3.50` 均判定正确，超出范围则记录内容错误。

### 8. AN3.0 HEX 区间判断（V3.4.11 新增）

适用于 **RGL 系列交流源载一体机**和 **EVH 系列双向直流电源**的 AN3.0 二进制协议。

1. 在设置页选择**产品系列**：`RGL系列 (交流源载)` 或 `EVH系列 (直流电源)`
2. 勾选 **「以HEX格式发送（AN3.0）」**，HEX 区间判断控件自动解禁
3. 勾选 **「启用HEX区间判断（AN3.0自动解析）」**，输入容差值（如 `0.5`）
4. 先用**捕获模式**获取参考帧，自动填入 Excel B 列（完整的 HEX 帧，含 `7B...7D`）
5. 切换到**发送模式**，软件自动根据命令码匹配字段布局，同构解析后逐字段对比

**支持的查询命令：**

| 产品系列 | 命令数 | 典型命令 |
|---------|--------|---------|
| RGL系列 | 22 条 | `F0 A4` 查询输出测量值(15字段) / `A5 41` 查询常规参数 / `F0 EB` 查询状态 |
| EVH系列 | 17 条 | `F0 80` 查询电压电流功率 / `A5 1F` 查询全部设定值(7字段) / `A6 01` 查询电子负载 |

**原理：** 软件内置 `An30Layout` 命令注册表，收到 HEX 帧后根据 `data[4]` 和 `data[5]` 自动查找字段名、字节长度、除数，将参考帧和实时帧同构解析为物理值，逐字段判定 `|实际 - 参考| ≤ 容差`。

> **不勾选 HEX 区间判断时：** HEX 模式仍使用精确逐字节比对，兼容旧行为。

---

## 🏗️ 架构设计

```
┌─────────────────────────────────────────────────────┐
│  主线程 (GUI Thread)                                 │
│  ┌──────────┐  ┌────────────┐  ┌─────────────────┐  │
│  │ UI 响应   │  │ XxxExcel   │  │ 校验 + 错误记录  │  │
│  │ PageUI   │  │ 流程控制    │  │ ErrorHandler   │  │
│  └──────────┘  └────────────┘  └─────────────────┘  │
│       ↑              ↑↓ QueuedConnection    ↑        │
│       │              │         │            │        │
├───────┼──────────────┼─────────┼────────────┼────────┤
│  工作线程 (Work Thread)  │         │            │        │
│  ┌────────────────────┴─────────┴────────────┴─────┐ │
│  │  buildSendData() → 发送 → 接收 → 精确延时 → emit │ │
│  │  (阻塞 I/O，不阻塞 UI)                           │ │
│  └─────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

### 模块分层

每个通讯模块（Serial / Network / GPIB）均采用统一的四层分离架构：

| 子模块 | 职责 |
|--------|------|
| **XxxPage** | 页面协调器：持有各子模块指针，串联初始化流程 |
| **XxxPageUI** | 纯 UI 构建：5 个 createXxxPage() 方法 |
| **XxxPageSignals** | 线程创建 + 全部 connect() 信号/槽连接 + 超时定时器 |
| **XxxErrorHandler** | 错误计数器维护 + 错误表格写入 + 颜色标记 + 自动滚动 |
| **XxxWork** | 独立工作线程中的阻塞 I/O 操作 |
| **XxxExcel** | 主线程批量发送流程控制 + 校验逻辑（含区间判断） |

### 公共工具库

| 工具 | 位置 | 职责 | 共享方 |
|------|------|------|--------|
| **RangeComparer** | `Other_T/RangeComparer.h/.cpp` | ASCII 区间比较 + AN3.0 HEX 帧区间比较 | 串口 / 网口 / GPIB |
| **StickySplitter** | `Other_T/StickySplitter.h/.cpp` | 粘包分割：按分隔符拆分字节流 | 串口 / 网口 |
| **An30Layout** | `Other_T/An30Layout.h/.cpp` | ★ AN3.0 命令码→字段布局自动匹配，支持 RGL/EVH 双系列 | 串口 / 网口 / GPIB |

- **线程通讯:** 所有跨线程调用使用 `QMetaObject::invokeMethod(Qt::QueuedConnection)`，Work 对象通过 `moveToThread()` 移入独立线程

---

## 📦 依赖与致谢

| 库 | 说明 | 链接 |
|---|------|------|
| **ElaWidgetTools** | 现代化 Fluent Design UI 组件库 | [GitHub](https://github.com/Liniyous/ElaWidgetTools) |
| **QXlsx** | 高性能跨平台 Excel 读写库 | [GitHub](https://github.com/QtExcel/QXlsx) |
| **NI-VISA** | GPIB / 仪器控制驱动 | [NI 官网](https://www.ni.com/en/support/downloads/drivers/download.ni-visa.html) |

---

## 📝 更新日志

### V3.4.11 (2026-07)

- ✨ **AN3.0 HEX 区间判断全面启用：** 串口/网口/GPIB「HEX区间判断」从预留变为可用，支持 RGL / EVH 双产品系列
- ✨ **An30Layout 命令码自动匹配：** 新增公共解析库，注册 RGL 系列 22 条 + EVH 系列 17 条 AN3.0 查询命令，根据应答帧命令码自动查找字段名、字节长度、除数
- ✨ **产品系列切换：** 设置页新增下拉框「产品系列」（RGL / EVH），一键切换命令表，联动 HEX 区间判断解析逻辑
- ✨ **同构解析 + 逐字段区间对比：** `compareHexFrame()` 对参考帧和实时帧两侧均用同一布局解析，逐字段判定 `|实际 - 参考| ≤ tolerance`
- ✨ **零配置使用流程：** Excel B 列直接填入捕获到的 HEX 帧，无需手动填写偏移/字节数/除数
- 🔧 **HEX 区间判断三级联动：** HEX 发送→解锁区间判断勾选→解锁偏差值输入，取消 HEX 发送自动关闭后两级
- 🔧 **RangeComparer 公共工具扩展：** 新增 `compareHexFrame()` 方法，三模块共享，支持 RGL/EVH 双产品系列自动识别
- 🐛 **修复 HEX 区间模式重复记录错误：** 移除 `tryRangeCompare` 内部 `addContentError` 调用，由顶层 `onResponseReceived` 统一处理
- 🐛 **修复 HEX 区间判断控件灰色无法操作：** 去掉"预留"标记和硬编码 `setEnabled(false)`，改为联动控制
- 📝 README 升级至 V3.4.11，新增 AN3.0 HEX 区间判断使用指南、双产品系列说明、架构图
- 📝 帮助文档新增「AN3.0 HEX 区间判断」专题章节（含 RGL 22 条 + EVH 17 条命令速查表）
- 📝 关于软件页面更新至 V3.4.11

### V3.4.10 (2026-07)

- ✨ **全模块新增 ASCII 区间判断功能：** 串口/网口/GPIB 设置页新增「启用ASCII区间判断」勾选框和「ASCII区间值」输入框
- ✨ **HEX 区间判断控件预留：** 设置页同步添加 HEX 区间判断勾选框和输入框（暂时禁用）
- 🔧 区间判断逻辑：自动去除 `\r` `\n` 空格后解析为 `double`
- 🧩 **代码架构大重构 —— 四层分离：** 每个通讯模块拆分为 Page / PageUI / PageSignals / ErrorHandler
- 🧩 **公共工具库提取：** RangeComparer（区间比较）/ StickySplitter（粘包分割），三模块共享

### V3.4.9 (2026-07)

- ✨ **串口/网口新增「发送后缀」功能：** 对齐 GPIB 模块，设置页新增后缀下拉框
- ✨ **全模块 `buildSendData()` 统一数据构建：** 三模块均采用统一方法
- 🔧 **串口/网口 `sendString` 重构：** 改用 `buildSendData()` 构建发送数据
- 📝 README 新增「发送后缀说明」和「后缀处理流程」章节

### V3.4.8 (2026-07)

- 🔧 **串口/网口 `sendStringWithDelay` 架构对齐 GPIB：** 新增 `forceRead` 参数
- ⚡ **精确延时计时起点前移：** 计时器在 `write` 之前启动
- 🔇 **网口连接超时/错误弹窗静默化**
- 📐 **三模块 `sendStringWithDelay` 函数签名统一**

### V3.4.7 (2026-07)

- 🐛 **修复串口/网口 Excel 批量发送中设置命令卡死问题**
- ✨ 设置命令不再启动全局超时计时器
- 📝 补充设置命令与查询命令的使用说明

### V3.4.6 (2026-07)

- ✨ GPIB 设置页新增「发送后缀」选择
- ✨ 发送数据自动 unescape：`\r` `\n` 字面量自动转换为真实控制字符
- ✨ 智能去重：发送前自动去除命令末尾已有 `\r` `\n`
- 🐛 修复 GPIB 命令中 `\n` 作为字面量字符发送而非换行符的 Bug

### V3.4.5 (2026-07)

- 🐛 修复 GPIB 精确延时偏差 10ms 的 Bug
- 🐛 修复串口/网口 `waitForBytesWritten(10)` 引入 10ms 额外延迟
- ✨ 全模块日志/表格支持控制字符转义显示
- ✨ 捕获模式：`\r` `\n` 在 Excel B 列中存储为 `\\r` `\\n`

### V3.4.4 (2026-07)

- 🐛 修复 GPIB 单条发送无回传数据显示
- 🐛 修复 GPIB 表格发送中设置命令导致阻塞超时
- 🐛 修复串口表格发送延时 0ms 被强制覆盖为 100ms
- ⚡ GPIB 设置命令间隔从 ~1.5s 降至 ~2ms

### V3.4.1 (2026-07)

- ✅ GPIB 模块完整实现
- ✅ 精确延时 EMA 误差补偿算法
- ✅ 界面优化：GroupBox 分组布局、卡片样式

---

## 📄 许可证

MIT License. 详见 [LICENSE](LICENSE) 文件。

---

## 👤 作者

**Cossiant**

- 2026 年 7 月
- Qt 5.15 + MinGW

---

<p align="center">
  <sub>Made with ❤️ for instrument automation testing</sub>
</p>
```
