// GPIBWork.cpp
// GPIB Worker：NI-VISA 操作实现
// 精确延时：1ms QTimer轮询 + QElapsedTimer + 微秒忙等 + EMA补偿

#include "GPIBWork.h"
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <QCoreApplication>
#include <visa.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// ═══════════════════════════════════════════════════════════════
//  构造 / 析构
// ═══════════════════════════════════════════════════════════════
GPIBWork::GPIBWork(QObject *parent)
    : QObject(parent)
{
    // ★ 1ms 轮询定时器 — 配合 QElapsedTimer 实现高精度
    m_interCmdTimer = new QTimer(this);
    m_interCmdTimer->setTimerType(Qt::PreciseTimer);
    m_interCmdTimer->setInterval(1);          // 每1ms触发
    m_interCmdTimer->setSingleShot(false);     // 持续触发直到手动停止
    connect(m_interCmdTimer, &QTimer::timeout,
            this, &GPIBWork::onInterCmdDelay);

    qDebug() << "GPIBWork: 初始化完成"
             << "(线程:" << QThread::currentThreadId() << ")"
             << "| 精确延时: 1ms轮询+忙等自旋+EMA补偿";
}

GPIBWork::~GPIBWork()
{
    m_interCmdTimer->stop();
    closeGPIBPort();
    qDebug() << "GPIBWork: 已销毁";
}

// ═══════════════════════════════════════════════════════════════
//  查询 / 设置 接口
// ═══════════════════════════════════════════════════════════════
bool GPIBWork::isOpen() const
{
    return m_opened.loadRelaxed() != 0;
}

int GPIBWork::totalRecvCount() const
{
    return m_totalRecv;
}

void GPIBWork::resetRecvCount()
{
    m_totalRecv = 0;
    emit recvCountChanged(0);
}

void GPIBWork::setExpectedResponse(const QByteArray &expected)
{
    m_expectedResponse = expected;
}

QByteArray GPIBWork::expectedResponse() const
{
    return m_expectedResponse;
}

void GPIBWork::setHexDisplayMode(bool hexMode)
{
    m_hexDisplay = hexMode;
}

void GPIBWork::setSuffixMode(int mode)
{
    m_suffixMode = static_cast<GPIBSuffix>(mode);
    qDebug() << "GPIBWork: 后缀模式 =" << mode
             << (mode == 0 ? "None" : mode == 1 ? "CR" : mode == 2 ? "LF" : "CRLF");
}

// ═══════════════════════════════════════════════════════════════
//  构建发送数据：unescape → 去尾 → 加后缀
// ═══════════════════════════════════════════════════════════════
QByteArray GPIBWork::buildSendData(const QString &text, bool hexMode) const
{
    if (hexMode) {
        QString hex = text;
        hex.remove(' ');
        return QByteArray::fromHex(hex.toLatin1());
    }

    // ★ Step 1: 将用户输入的 \r \n 转义还原为真实控制字符
    QString unescaped = text;
    unescaped.replace(QLatin1String("\\r"), QLatin1String("\r"));
    unescaped.replace(QLatin1String("\\n"), QLatin1String("\n"));

    QByteArray data = unescaped.toUtf8();

    // ★ Step 2: 去掉末尾已有的 \r \n（避免与后缀重复）
    while (!data.isEmpty()) {
        char last = data.at(data.size() - 1);
        if (last == '\r' || last == '\n')
            data.chop(1);
        else
            break;
    }

    // ★ Step 3: 追加用户选择的后缀
    switch (m_suffixMode) {
        case GPIBSuffix::CR:   data.append('\r');          break;
        case GPIBSuffix::LF:   data.append('\n');          break;
        case GPIBSuffix::CRLF: data.append("\r\n");        break;
        case GPIBSuffix::None:                             break;
    }

    return data;
}

// ═══════════════════════════════════════════════════════════════
//  打开 GPIB 设备
// ═══════════════════════════════════════════════════════════════
void GPIBWork::openGPIBPort(int boardIndex,
                             int primaryAddress,
                             int secondaryAddress,
                             int timeoutMs,
                             bool termCharEnabled,
                             char termChar,
                             bool sendEndEnabled)
{
    if (m_opened.loadRelaxed() != 0) {
        closeGPIBPort();
    }

    m_timeoutMs       = timeoutMs;
    m_termChar        = termChar;
    m_termCharEnabled = termCharEnabled;
    m_sendEndEnabled  = sendEndEnabled;

    // ── 步骤1: 打开 VISA 资源管理器 ──
    ViStatus status = viOpenDefaultRM(&m_resourceManager);
    if (!checkVISAStatus(status, QStringLiteral("viOpenDefaultRM"))) {
        emit gpibClosed();
        return;
    }

    // ── 步骤2: 构建资源名称 ──
    QString resourceName;
    if (secondaryAddress > 0) {
        resourceName = QStringLiteral("GPIB%1::%2::%3::INSTR")
                           .arg(boardIndex)
                           .arg(primaryAddress)
                           .arg(secondaryAddress);
    } else {
        resourceName = QStringLiteral("GPIB%1::%2::INSTR")
                           .arg(boardIndex)
                           .arg(primaryAddress);
    }

    // ── 步骤3: 打开仪器会话 ──
    status = viOpen(m_resourceManager,
                    resourceName.toLatin1().constData(),
                    VI_NULL, VI_NULL,
                    &m_instrument);
    if (!checkVISAStatus(status, QStringLiteral("viOpen(%1)").arg(resourceName))) {
        viClose(m_resourceManager);
        m_resourceManager = 0;
        emit gpibClosed();
        return;
    }

    // ── 步骤4: 配置仪器属性 ──
    viSetAttribute(m_instrument, VI_ATTR_TMO_VALUE,
                   static_cast<ViAttrState>(timeoutMs));

    viSetAttribute(m_instrument, VI_ATTR_TERMCHAR_EN,
                   termCharEnabled ? VI_TRUE : VI_FALSE);

    viSetAttribute(m_instrument, VI_ATTR_TERMCHAR,
                   static_cast<ViAttrState>(static_cast<unsigned char>(termChar)));

    viSetAttribute(m_instrument, VI_ATTR_SEND_END_EN,
                   sendEndEnabled ? VI_TRUE : VI_FALSE);

    // ★ 新连接重置误差补偿
    m_timingCompensationMs = 0;

    m_opened.storeRelaxed(1);
    emit gpibOpened();

    qDebug() << "GPIBWork: GPIB 已打开" << resourceName
             << "超时:" << timeoutMs << "ms"
             << "(线程:" << QThread::currentThreadId() << ")";
}

// ═══════════════════════════════════════════════════════════════
//  关闭 GPIB 设备
// ═══════════════════════════════════════════════════════════════
void GPIBWork::closeGPIBPort()
{
    m_interCmdTimer->stop();   // ★ 停止命令间隔定时器

    if (m_instrument) {
        viClose(m_instrument);
        m_instrument = 0;
    }
    if (m_resourceManager) {
        viClose(m_resourceManager);
        m_resourceManager = 0;
    }

    m_opened.storeRelaxed(0);
    emit gpibClosed();

    qDebug() << "GPIBWork: GPIB 已关闭"
             << "(线程:" << QThread::currentThreadId() << ")";
}

// ═══════════════════════════════════════════════════════════════
//  发送字符串（单条发送用，写后读取）
// ═══════════════════════════════════════════════════════════════
void GPIBWork::sendString(const QString &text, bool hexMode)
{
    if (!isOpen() || text.isEmpty())
        return;

    QByteArray data = buildSendData(text, hexMode);   // 使用统一构建方法

    if (!doVISAWrite(data))
        return;

    // ★ GPIB 必须显式 viRead 才能获取仪器响应
    QByteArray response = doVISARead(m_timeoutMs);
    if (!response.isEmpty()) {
        emitData(response);
    }
}

// ═══════════════════════════════════════════════════════════════
//  ★★★ 核心：发送 + 条件读取 + 1ms轮询精确延时 ★★★
//  forceRead: 捕获模式强制读取；否则仅在期望非空时读取
// ═══════════════════════════════════════════════════════════════
void GPIBWork::sendStringWithDelay(const QString &text, bool hexMode,
                                    const QByteArray &expectedResponse,
                                    int delayMs,
                                    bool forceRead)
{
    if (!isOpen() || text.isEmpty() || !m_instrument)
        return;

    m_expectedResponse = expectedResponse;

    // ── 构建数据 ──
    QByteArray data = buildSendData(text, hexMode);

    // ── 步骤1: viWrite 发送命令 ──
    // ★ 在 viRead 之前启动计时，让延时包含 viRead 的阻塞时间
    //   这样 TX→TX 间隔 = max(viRead, delayMs)，而不是 viRead + delayMs
    if (delayMs > 0) {
        m_originalDelayMs = delayMs;
        m_preciseDelayTimer.start();
    }

    // ── 步骤1: viWrite 发送命令 ──
    if (!doVISAWrite(data)) {
        // 发送失败但照样启动延时（保持与 Serial/Network 行为一致）
        if (delayMs > 0) {
            int compensatedMs = delayMs + m_timingCompensationMs;
            if (compensatedMs < 0) compensatedMs = 0;

            const int MAX_COMPENSATION = 100;
            m_timingCompensationMs = qBound(-MAX_COMPENSATION,
                                             m_timingCompensationMs,
                                             MAX_COMPENSATION);

            m_targetDelayMs   = compensatedMs;
            m_interCmdTimer->start();
        } else {
            emit interCmdDelayFinished();
        }
        return;
    }

    // ── 步骤2: viRead 读取响应 ──
    // ★ 关键优化：
    //    forceRead=true（捕获模式）→ 总是读取仪器响应
    //    forceRead=false 且期望非空 → 读取响应用于校验
    //    forceRead=false 且期望为空 → 跳过 viRead（设置命令无需等待）
    bool shouldRead = forceRead || !m_expectedResponse.isEmpty();

    if (shouldRead) {
        QByteArray response = doVISARead(m_timeoutMs);
        if (!response.isEmpty()) {
            emitData(response);
        }
    } else {
        // ★ 无期望响应也不强制读取 → 发出空响应信号让流程继续
        emit responseReceived(QByteArray());
    }

    // ═══════════════════════════════════════════════════════════
    //  步骤3: 精确延时（计时起点在 viRead 之前）
    // ═══════════════════════════════════════════════════════════
    if (delayMs > 0) {
        qint64 alreadyElapsed = m_preciseDelayTimer.elapsed();

        if (alreadyElapsed >= delayMs) {
            // viRead 已经消耗了所有延时，立即通知主线程
            emit interCmdDelayFinished();
        } else {
            // 计算剩余需要等待的时间
            int remainingMs = static_cast<int>(delayMs - alreadyElapsed);

            // EMA 补偿
            int compensatedMs = remainingMs + m_timingCompensationMs;
            if (compensatedMs < 0) compensatedMs = 0;

            const int MAX_COMPENSATION = 100;
            m_timingCompensationMs = qBound(-MAX_COMPENSATION,
                                             m_timingCompensationMs,
                                             MAX_COMPENSATION);

            // m_targetDelayMs = 从计时起点算起的绝对目标时间
            m_targetDelayMs = static_cast<int>(alreadyElapsed + compensatedMs);
            m_interCmdTimer->start();  // 每1ms触发 onInterCmdDelay()
        }
    } else {
        emit interCmdDelayFinished();
    }
}


// ═══════════════════════════════════════════════════════════════
//  1ms 轮询回调：检测是否到期 → 微秒忙等 → 误差补偿 → 发射信号
// ═══════════════════════════════════════════════════════════════
void GPIBWork::onInterCmdDelay()
{
    qint64 elapsedMs = m_preciseDelayTimer.elapsed();

    // ★ 还没到目标时间，继续等（定时器下次再触发）
    if (elapsedMs < m_targetDelayMs - 1) {
        return;
    }

    // ★ 距离目标 ≤1ms：进入忙等自旋，精准命中
    while (m_preciseDelayTimer.elapsed() < m_targetDelayMs) {
        // 自旋等待
    }

    // ★ 停止轮询
    m_interCmdTimer->stop();

    // ★ 测量实际耗时，计算误差
    qint64 actualMs = m_preciseDelayTimer.elapsed();
    int    errorMs  = static_cast<int>(actualMs - m_targetDelayMs);

    // ★ EMA 平滑更新补偿值 (alpha = 0.5)
    const int MAX_COMPENSATION = 100;
    m_timingCompensationMs -= errorMs / 2;
    m_timingCompensationMs  = qBound(-MAX_COMPENSATION,
                                      m_timingCompensationMs,
                                      MAX_COMPENSATION);

    // ★ 诊断日志
    if (qAbs(errorMs) >= 1) {
        qDebug() << "GPIBWork:[精确延时]"
                 << "请求" << m_originalDelayMs << "ms"
                 << "→补偿后" << m_targetDelayMs << "ms"
                 << "→实际" << actualMs << "ms"
                 << "|误差" << errorMs << "ms"
                 << "|累积补偿" << m_timingCompensationMs << "ms";
    }

    // ★ 通知主线程
    emit interCmdDelayFinished();
}

// ═══════════════════════════════════════════════════════════════
//  VISA 写入
// ═══════════════════════════════════════════════════════════════
bool GPIBWork::doVISAWrite(const QByteArray &data)
{
    if (!m_instrument || data.isEmpty())
        return false;

    ViUInt32 retCount = 0;
    ViStatus status = viWrite(m_instrument,
                              reinterpret_cast<ViBuf>(const_cast<char*>(data.constData())),
                              static_cast<ViUInt32>(data.size()),
                              &retCount);

    if (!checkVISAStatus(status, QStringLiteral("viWrite")))
        return false;

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit sendLogLine(QString("[%1] TX → %2").arg(timeStr, display));

    return true;
}

// ═══════════════════════════════════════════════════════════════
//  VISA 读取（阻塞，直到数据到达或超时）
// ═══════════════════════════════════════════════════════════════
QByteArray GPIBWork::doVISARead(int timeoutMs)
{
    if (!m_instrument)
        return QByteArray();

    // 临时设置读取超时
    viSetAttribute(m_instrument, VI_ATTR_TMO_VALUE,
                   static_cast<ViAttrState>(timeoutMs));

    const int bufferSize = 4096;
    QByteArray buffer(bufferSize, '\0');
    ViUInt32 retCount = 0;

    ViStatus status = viRead(m_instrument,
                             reinterpret_cast<ViBuf>(buffer.data()),
                             static_cast<ViUInt32>(bufferSize),
                             &retCount);

    // 恢复原来的超时值
    viSetAttribute(m_instrument, VI_ATTR_TMO_VALUE,
                   static_cast<ViAttrState>(m_timeoutMs));

    // VI_SUCCESS_TERM_CHAR 和 VI_SUCCESS_MAX_CNT 也算成功（status >= 0）
    if (status >= 0 && retCount > 0) {
        buffer.resize(static_cast<int>(retCount));
        return buffer;
    }

    if (status < 0 && status != -1073807339) {  // 忽略超时错误 (VI_ERROR_TMO)
        checkVISAStatus(status, QStringLiteral("viRead"));
    }

    return QByteArray();
}

// ═══════════════════════════════════════════════════════════════
//  VISA 错误处理
// ═══════════════════════════════════════════════════════════════
bool GPIBWork::checkVISAStatus(ViStatus status, const QString &operation)
{
    if (status >= 0) {
        return true; // VISA 成功状态码为非负值
    }

    // 打开资源失败时，优先给出"面向用户"的原因提示
    if (operation.startsWith(QStringLiteral("viOpen"))) {
        QString message = tr("%1失败: %2\n错误码(%3)")
                              .arg(operation)
                              .arg(viOpenFailureReason(status))
                              .arg(visaStatusHex(status));
        emit errorOccurred(message);
        return false;
    }

    QString message = tr("%1失败，错误码(%2)")
                          .arg(operation)
                          .arg(visaStatusHex(status));

    emit errorOccurred(message);
    return false;
}

// ═══════════════════════════════════════════════════════════════
//  VISA 状态码 → 十六进制字符串
// ═══════════════════════════════════════════════════════════════
QString GPIBWork::visaStatusHex(ViStatus status)
{
    return QStringLiteral("0x%1")
        .arg(static_cast<quint32>(status), 8, 16, QChar('0'))
        .toUpper();
}

// ═══════════════════════════════════════════════════════════════
//  viOpen 失败的详细原因（面向用户）
// ═══════════════════════════════════════════════════════════════
QString GPIBWork::viOpenFailureReason(ViStatus status)
{
    if (status == VI_ERROR_INTF_NUM_NCONFIG) {
        return tr("资源名称无效。请检查板卡号配置是否正确，或设备连线是否稳定。");
    }
    if (status == VI_ERROR_RSRC_NFOUND) {
        return tr("未找到目标资源。请确认 GPIB 板卡号、仪器主地址、设备上电状态，"
                  "以及 NI-MAX 中资源可见。");
    }
    if (status == VI_ERROR_RSRC_BUSY) {
        return tr("目标资源正忙。设备可能正在被 NI-MAX 或其他程序占用，请先释放后重试。");
    }
    if (status == VI_ERROR_RSRC_LOCKED) {
        return tr("目标资源被锁定。请关闭占用该资源的进程后重试。");
    }
    if (status == VI_ERROR_INV_RSRC_NAME) {
        return tr("资源名称无效。请检查板卡号配置是否正确，或设备连线是否稳定。");
    }
    if (status == VI_ERROR_INV_ACC_MODE) {
        return tr("访问模式无效。请检查 VISA 打开参数与驱动环境。");
    }
    if (status == VI_ERROR_ALLOC) {
        return tr("系统资源分配失败。请关闭部分程序后重试。");
    }
    if (status == VI_ERROR_TMO) {
        return tr("打开资源超时。请检查设备连线、地址与仪器响应状态。");
    }
    if (status == VI_ERROR_LIBRARY_NFOUND) {
        return tr("未找到 VISA 运行库。请确认 NI-VISA 已正确安装。");
    }
    if (status == VI_ERROR_SYSTEM_ERROR) {
        return tr("系统层错误。建议重启 NI 相关服务或重启系统后重试。");
    }

    return tr("VISA 打开失败（未匹配到明确原因）。建议在 NI-MAX 中执行通信测试定位问题或重启本软件。");
}

// ═══════════════════════════════════════════════════════════════
//  内部：统一的数据输出入口
// ═══════════════════════════════════════════════════════════════
void GPIBWork::emitData(const QByteArray &data)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString display = formatByteArray(data);
    emit recvLogLine(QString("[%1] RX ← %2").arg(timeStr, display));

    m_totalRecv++;
    emit recvCountChanged(m_totalRecv);

    emit dataReceived(data);
    emit responseReceived(data);
}

// ═══════════════════════════════════════════════════════════════
//  内部：格式化字节数组
// ═══════════════════════════════════════════════════════════════
QString GPIBWork::formatByteArray(const QByteArray &data) const
{
    if (m_hexDisplay) {
        return data.toHex(' ').toUpper();
    } else {
        QString text = QString::fromUtf8(data);
        if (!text.isEmpty()) {
            // ★ 将控制字符转义为可见字符串，避免被 QListWidget 解释为换行
            text.replace(QLatin1Char('\r'), QLatin1String("\\r"));
            text.replace(QLatin1Char('\n'), QLatin1String("\\n"));
            return text;
        } else {
            return data.toHex(' ').toUpper();
        }
    }
}
