# Ainuo Excel SCPI Sender

## Introduction

Ainuo Excel SCPI Sender is a Qt-based GUI tool for batch sending SCPI commands to power supplies (or any SCPI-compatible instrument) over TCP.  
It reads command lists from Microsoft Excel files (first column: commands to send, second column: expected return values), supports loop sending, configurable command intervals, send count limits, and displays sent/received data as well as send statistics in real time.

## Key Features

- **Excel File Reading**: Supports `.xlsx` and `.xls` formats, automatically detects the used range of the worksheet, reads commands and expected return values, and displays them in a table.
- **TCP Communication**: Connects to a specified IP address and port via TCP Socket to send SCPI commands and receive device responses.
- **Multi-threaded Sending**: The command sending logic runs in a separate thread to avoid blocking the GUI; supports start/stop sending.
- **Send Control**:
  - Configurable delay between commands (milliseconds).
  - Configurable total send count limit (0 means infinite loop).
  - Manual stop support.
- **Real-time Display**:
  - Sent commands and received data are shown in two lists, with a maximum display limit (default 300 items).
  - Dynamically displays the total number of sent commands.
- **Thread-safe Stop**: The sending thread uses an atomic flag to implement interruptible busy waiting, ensuring quick response to stop requests.

## Build and Runtime Environment

- **Framework**: Qt 5.x or Qt 6.x (built with Qt Creator or qmake)
- **Compiler**: C++11 compatible (MSVC, MinGW, GCC, etc.)
- **Required Qt Modules**:
  - `core`, `gui`, `widgets`
  - `network` (for TCP Socket)
  - `axcontainer` (for Excel access; requires Windows and Excel/WPS installation)
- **Operating System**: Windows (due to the use of `QAxObject`)

## Build Steps

1. Open the project file `ExcelSCPISender.pro` in Qt Creator.
2. Configure the build kit (ensure the `axcontainer` module is installed).
3. Build and run.

Or use the command line:

```bash
qmake ExcelSCPISender.pro
make   # or mingw32-make / nmake
```

```markdown
## Usage Instructions

### 1. Connect to the Power Supply

Enter the power supply's IP address and port number in the interface (default example: 127.0.0.1:20108).

Click the "Connect to Power" button to establish the TCP connection. Upon successful connection, the button states will switch.

### 2. Load Excel Command File

Click the "Open Excel and Read" button and select an Excel file containing commands.

The file must have at least two columns:

- **Column A**: SCPI commands to send (strings)
- **Column B**: Expected correct return values (optional, currently only displayed, no automatic verification)

After successful loading, the table will show all commands and their corresponding return values.

### 3. Configure Sending Parameters

- **Delay per command (ms)**: Waiting time between two adjacent commands (milliseconds).
- **Send count (0 for infinite loop)**: Total number of commands to send (e.g., set to 100 to stop after 100 commands); 0 means infinite loop until manually stopped.

### 4. Start Sending

Click the "Send Excel Commands to Power" button. The program will send the commands in Column A sequentially.

During sending, the "Total commands sent" counter will update in real time.

Sent commands are displayed in the right-hand "Sent Data" list; received device responses are displayed in the middle "Received Data" list.

### 5. Stop Sending

Click the "Stop Sending Excel Commands to Power" button. The sending thread will stop as soon as possible after finishing the current command.

After sending stops, button states are restored, and you can modify parameters and send again.

### 6. Disconnect

Click the "Disconnect" button to close the TCP connection and release related resources.

## Project Structure

| File | Description |
|------|-------------|
| `main.cpp` | Application entry point, launches the GUI |
| `gui.h` / `gui.cpp` | Main window class, handles layout, user interaction, and thread management |
| `connectnetwork.h` / `connectnetwork.cpp` | TCP network communication class, runs in a separate thread, handles data transmission and reception |
| `readexceldata.h` / `readexceldata.cpp` | Uses QAxObject to read Excel files and populate QTableWidget |
| `excelsendworker.h` / `excelsendworker.cpp` | Sending worker class, runs in a separate thread, implements the send strategy (delay, loop, stop) |
| `threadsendexcel.h` / `threadsendexcel.cpp` | (Unused, reserved or deprecated) |

## Key Design Notes

### Multi-threaded Architecture

- **GUI Thread**: Handles UI events and user actions.
- **Network Thread**: Runs the `connectNetwork` object, manages TCP connection, reading/writing, preventing network I/O from blocking the UI.
- **Send Thread**: Runs the `ExcelSendWorker` object, controls send timing (delay, loop, stop flag).  
  Uses `std::atomic<bool>` as the stop flag and `QThread::yieldCurrentThread()` for microsecond‑level interruptible waiting.

### Signal-Slot Connections

- The sending thread emits the `sendCommand` signal to pass commands to the network thread (queued connection, auto‑cross‑thread).
- The stop signal uses **`Qt::DirectConnection`** to ensure `stopWork()` is called directly in the GUI thread, immediately setting the atomic flag and avoiding blocking due to the sending thread.

### Excel Reading Compatibility

- Attempts to read cell properties `Value`, `Text`, and `Value2` to be compatible with different Excel content formats.
- Uses `UsedRange` to obtain the actual row count of the data area, avoiding empty rows.

## Important Notes

1. **Windows Platform + Excel/WPS**: Because `QAxObject` is used, this software runs only on Windows and requires Microsoft Excel or WPS (with COM automation support).
2. **Thread Safety**: `deleteLater()` and proper thread exit waiting are used for the network object to avoid resource leaks.
3. **UI Responsiveness**: The UI does not freeze during sending; however, a stop request will take effect at most after the current command finishes (i.e., can be stopped immediately during the delay period).
4. **Display Limit**: The sent and received lists are limited to 300 items by default; older items are automatically deleted to prevent excessive memory usage.

## License

This project does not specify an open‑source license. It is intended for internal testing only. If you wish to modify or redistribute, please retain the original copyright notice.

## Contact

For questions or suggestions, please contact the project developer.
```