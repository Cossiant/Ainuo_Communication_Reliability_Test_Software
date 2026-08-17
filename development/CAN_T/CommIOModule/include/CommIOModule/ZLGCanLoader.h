#pragma once

#include <zlgcan.h>

// ZLGCAN x64 动态加载封装。
// 运行时通过 QLibrary 加载 zlgcan.dll，便于部署时替换驱动。
namespace ZLGCanDriver {

bool load();
void unload();
bool isLoaded();

DEVICE_HANDLE openDevice(UINT deviceType, UINT deviceIndex, UINT reserved);
UINT closeDevice(DEVICE_HANDLE deviceHandle);
CHANNEL_HANDLE initCAN(DEVICE_HANDLE deviceHandle,
                       UINT canIndex,
                       ZCAN_CHANNEL_INIT_CONFIG *initConfig);
UINT startCAN(CHANNEL_HANDLE channelHandle);
UINT resetCAN(CHANNEL_HANDLE channelHandle);
UINT clearBuffer(CHANNEL_HANDLE channelHandle);
UINT readChannelErrInfo(CHANNEL_HANDLE channelHandle, ZCAN_CHANNEL_ERR_INFO *errInfo);

UINT transmit(CHANNEL_HANDLE channelHandle, ZCAN_Transmit_Data *transmit, UINT len);
UINT receive(CHANNEL_HANDLE channelHandle, ZCAN_Receive_Data *receive, UINT len, int waitTime);
UINT transmitFD(CHANNEL_HANDLE channelHandle, ZCAN_TransmitFD_Data *transmit, UINT len);
UINT receiveFD(CHANNEL_HANDLE channelHandle, ZCAN_ReceiveFD_Data *receive, UINT len, int waitTime);
UINT receiveData(DEVICE_HANDLE deviceHandle, ZCANDataObj *receive, UINT len, int waitTime);
UINT setValue(DEVICE_HANDLE deviceHandle, const char *path, const void *value);

} // namespace ZLGCanDriver
