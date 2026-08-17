#include "CommIOModule/ZLGCanLoader.h"

#include <QLibrary>

namespace ZLGCanDriver {

namespace {

QLibrary *g_library = nullptr;

using FnOpen = DEVICE_HANDLE (FUNC_CALL *)(UINT, UINT, UINT);
using FnClose = UINT (FUNC_CALL *)(DEVICE_HANDLE);
using FnInit = CHANNEL_HANDLE (FUNC_CALL *)(DEVICE_HANDLE, UINT, ZCAN_CHANNEL_INIT_CONFIG *);
using FnChannelUint = UINT (FUNC_CALL *)(CHANNEL_HANDLE);
using FnErr = UINT (FUNC_CALL *)(CHANNEL_HANDLE, ZCAN_CHANNEL_ERR_INFO *);
using FnTx = UINT (FUNC_CALL *)(CHANNEL_HANDLE, ZCAN_Transmit_Data *, UINT);
using FnRx = UINT (FUNC_CALL *)(CHANNEL_HANDLE, ZCAN_Receive_Data *, UINT, int);
using FnTxFD = UINT (FUNC_CALL *)(CHANNEL_HANDLE, ZCAN_TransmitFD_Data *, UINT);
using FnRxFD = UINT (FUNC_CALL *)(CHANNEL_HANDLE, ZCAN_ReceiveFD_Data *, UINT, int);
using FnRxData = UINT (FUNC_CALL *)(DEVICE_HANDLE, ZCANDataObj *, UINT, int);
using FnSetValue = UINT (FUNC_CALL *)(DEVICE_HANDLE, const char *, const void *);

FnOpen p_openDevice = nullptr;
FnClose p_closeDevice = nullptr;
FnInit p_initCAN = nullptr;
FnChannelUint p_startCAN = nullptr;
FnChannelUint p_resetCAN = nullptr;
FnChannelUint p_clearBuffer = nullptr;
FnErr p_readErr = nullptr;
FnTx p_transmit = nullptr;
FnRx p_receive = nullptr;
FnTxFD p_transmitFD = nullptr;
FnRxFD p_receiveFD = nullptr;
FnRxData p_receiveData = nullptr;
FnSetValue p_setValue = nullptr;

template <typename T>
bool resolve(const char *name, T &target)
{
    target = reinterpret_cast<T>(g_library->resolve(name));
    return target != nullptr;
}

} // namespace

bool load()
{
    if (g_library) {
        return isLoaded();
    }

    g_library = new QLibrary(QStringLiteral("zlgcan.dll"));
    if (!g_library->load()) {
        return false;
    }

    if (!resolve("ZCAN_OpenDevice", p_openDevice)
        || !resolve("ZCAN_CloseDevice", p_closeDevice)
        || !resolve("ZCAN_InitCAN", p_initCAN)
        || !resolve("ZCAN_StartCAN", p_startCAN)
        || !resolve("ZCAN_ResetCAN", p_resetCAN)
        || !resolve("ZCAN_ClearBuffer", p_clearBuffer)
        || !resolve("ZCAN_ReadChannelErrInfo", p_readErr)
        || !resolve("ZCAN_Transmit", p_transmit)
        || !resolve("ZCAN_Receive", p_receive)
        || !resolve("ZCAN_TransmitFD", p_transmitFD)
        || !resolve("ZCAN_ReceiveFD", p_receiveFD)
        || !resolve("ZCAN_ReceiveData", p_receiveData)
        || !resolve("ZCAN_SetValue", p_setValue)) {
        unload();
        return false;
    }

    return true;
}

void unload()
{
    p_openDevice = nullptr;
    p_closeDevice = nullptr;
    p_initCAN = nullptr;
    p_startCAN = nullptr;
    p_resetCAN = nullptr;
    p_clearBuffer = nullptr;
    p_readErr = nullptr;
    p_transmit = nullptr;
    p_receive = nullptr;
    p_transmitFD = nullptr;
    p_receiveFD = nullptr;
    p_receiveData = nullptr;
    p_setValue = nullptr;

    if (g_library) {
        delete g_library;
        g_library = nullptr;
    }
}

bool isLoaded()
{
    return g_library
           && g_library->isLoaded()
           && p_openDevice
           && p_initCAN
           && p_transmit
           && p_transmitFD
           && p_receiveData;
}

DEVICE_HANDLE openDevice(UINT deviceType, UINT deviceIndex, UINT reserved)
{
    return p_openDevice ? p_openDevice(deviceType, deviceIndex, reserved) : 0;
}

UINT closeDevice(DEVICE_HANDLE deviceHandle)
{
    return p_closeDevice ? p_closeDevice(deviceHandle) : 0;
}

CHANNEL_HANDLE initCAN(DEVICE_HANDLE deviceHandle,
                       UINT canIndex,
                       ZCAN_CHANNEL_INIT_CONFIG *initConfig)
{
    return p_initCAN ? p_initCAN(deviceHandle, canIndex, initConfig) : 0;
}

UINT startCAN(CHANNEL_HANDLE channelHandle)
{
    return p_startCAN ? p_startCAN(channelHandle) : 0;
}

UINT resetCAN(CHANNEL_HANDLE channelHandle)
{
    return p_resetCAN ? p_resetCAN(channelHandle) : 0;
}

UINT clearBuffer(CHANNEL_HANDLE channelHandle)
{
    return p_clearBuffer ? p_clearBuffer(channelHandle) : 0;
}

UINT readChannelErrInfo(CHANNEL_HANDLE channelHandle, ZCAN_CHANNEL_ERR_INFO *errInfo)
{
    return p_readErr ? p_readErr(channelHandle, errInfo) : 0;
}

UINT transmit(CHANNEL_HANDLE channelHandle, ZCAN_Transmit_Data *transmit, UINT len)
{
    return p_transmit ? p_transmit(channelHandle, transmit, len) : 0;
}

UINT receive(CHANNEL_HANDLE channelHandle, ZCAN_Receive_Data *receive, UINT len, int waitTime)
{
    return p_receive ? p_receive(channelHandle, receive, len, waitTime) : 0;
}

UINT transmitFD(CHANNEL_HANDLE channelHandle, ZCAN_TransmitFD_Data *transmit, UINT len)
{
    return p_transmitFD ? p_transmitFD(channelHandle, transmit, len) : 0;
}

UINT receiveFD(CHANNEL_HANDLE channelHandle, ZCAN_ReceiveFD_Data *receive, UINT len, int waitTime)
{
    return p_receiveFD ? p_receiveFD(channelHandle, receive, len, waitTime) : 0;
}

UINT receiveData(DEVICE_HANDLE deviceHandle, ZCANDataObj *receive, UINT len, int waitTime)
{
    return p_receiveData ? p_receiveData(deviceHandle, receive, len, waitTime) : 0;
}

UINT setValue(DEVICE_HANDLE deviceHandle, const char *path, const void *value)
{
    return p_setValue ? p_setValue(deviceHandle, path, value) : 0;
}

} // namespace ZLGCanDriver
