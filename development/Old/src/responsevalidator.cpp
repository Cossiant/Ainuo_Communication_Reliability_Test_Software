//
// Created by Cossiant on 2026/6/2.
//

#include "../include/responsevalidator.h"

ResponseValidator::ResponseValidator(QObject *parent)
    : QObject(parent)
{
}

void ResponseValidator::onSetTestPacketLossMode(bool enabled,bool onlySendDataMode)
{
    m_testPacketLossMode = enabled;
    m_onlySendDataMode = onlySendDataMode;
}

void ResponseValidator::onCommandSent(QByteArray expectedResponse)
{
    // 直接入队，注意线程只在本对象所在线程执行
    m_expectedQueue.enqueue(expectedResponse);
}

void ResponseValidator::onDataReceived(QByteArray data)
{
    if(m_onlySendDataMode){
        return;
    }
    if (m_testPacketLossMode) {
        // 丢包测试模式：严格顺序对应，取出队首
        //如果已经取值到最后一个，他还在发送，那么就是出现故障了，始终以最后一个值为正确值
        if(!m_expectedQueue.isEmpty()) expected = m_expectedQueue.dequeue();
    } else {
        // 正常模式：丢弃所有前面的期望，只保留最后一个（最新发送的命令）
        // 首先，先判断队列是否是空，如果是空，证明在发送之后，读取到了2个以上的数据
        if(!m_expectedQueue.isEmpty()){
            while (m_expectedQueue.size()>1) {
                m_expectedQueue.dequeue();
            }
            expected = m_expectedQueue.dequeue();
        }
    }
    qDebug()<<"expected is:"<<expected;
    // 比对（忽略首尾空白）
    if (data.trimmed() != expected.trimmed()) {
        emit errorDetected(expected, data);
    }
}

void ResponseValidator::onReset()
{
    m_expectedQueue.clear();
    // 不清除模式标志，保持复选框状态
}
