

Based on the code map provided, this is a Qt-based desktop application that reads Excel data and sends SCPI commands, likely for test and measurement equipment control. Let me create the README based on the structure.


# Ainuo Excel SCPI Sender

## 项目简介

Ainuo Excel SCPI Sender 是一个基于 Qt 框架开发的桌面应用程序，主要用于将 Excel 文件中的测试参数转换为 SCPI（Standard Commands for Programmable Instruments）命令，并发送给测试测量设备。

## 功能特性

- **Excel 数据读取**：支持从 Excel 文件加载测试参数数据并显示在表格中
- **SCPI 命令发送**：自动将表格中的参数转换为 SCPI 命令并发送至仪器设备
- **可视化界面**：提供直观的图形用户界面，方便操作和监控
- **服务器功能**：内置内容服务器，支持远程连接和数据传输

## 技术栈

- Qt 框架（C++）
- Qt Dialog 模块

## 构建说明

### 环境要求

- Qt 5.x 或更高版本
- 支持 C++11 的编译器

### 构建步骤

1. 使用 Qt Creator 打开项目文件 `ExcelSCPISender.pro`
2. 配置构建套件（Kit）
3. 点击构建按钮进行编译

或者使用命令行：

```bash
qmake
make
```

## 项目结构

```
ExcelSCPISender/
├── GUI.cpp              # 主界面实现
├── GUI.h                # 主界面头文件
├── main.cpp             # 程序入口
├── readExcelData.cpp   # Excel 数据读取实现
├── readExcelData.h     # Excel 数据读取头文件
└── ExcelSCPISender.pro # Qt 项目文件
```

## 使用说明

1. 运行程序
2. 点击"打开 Excel"按钮选择 Excel 文件
3. 查看表格中加载的数据
4. 配置服务器连接参数
5. 点击发送按钮将 SCPI 命令发送至设备

## 许可证

本项目仅供学习参考使用。