# Ainuo Excel SCPI Sender

## Project Introduction

Ainuo Excel SCPI Sender is a desktop application developed using the Qt framework, primarily designed to convert test parameters from Excel files into SCPI (Standard Commands for Programmable Instruments) commands and send them to test and measurement equipment.

## Features

- **Excel Data Reading**: Supports loading test parameter data from Excel files and displaying it in a table
- **SCPI Command Sending**: Automatically converts table parameters into SCPI commands and transmits them to connected instruments
- **Visual Interface**: Provides an intuitive graphical user interface for easy operation and monitoring
- **Server Functionality**: Includes an embedded content server supporting remote connections and data transmission

## Technology Stack

- Qt Framework (C++)
- Qt Dialog Module

## Build Instructions

### System Requirements

- Qt 5.x or higher
- A C++11-compatible compiler

### Build Steps

1. Open the project file `ExcelSCPISender.pro` using Qt Creator
2. Configure the build kit
3. Click the build button to compile

Alternatively, use the command line:

```bash
qmake
make
```

## Project Structure

```
ExcelSCPISender/
├── GUI.cpp              # Main interface implementation
├── GUI.h                # Main interface header file
├── main.cpp             # Program entry point
├── readExcelData.cpp    # Excel data reading implementation
├── readExcelData.h      # Excel data reading header file
└── ExcelSCPISender.pro  # Qt project file
```

## Usage Instructions

1. Run the application
2. Click the "Open Excel" button to select an Excel file
3. Review the loaded data in the table
4. Configure server connection parameters
5. Click the send button to transmit SCPI commands to the device

## License

This project is provided for learning and reference purposes only.