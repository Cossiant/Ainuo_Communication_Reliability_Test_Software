// GPIBWork.cpp
// GPIB Worker：NI-VISA 操作实现
// 精确延时：1ms QTimer轮询 + QElapsedTimer + 微秒忙等 + EMA补偿
// ★ 新增：代际标记防止信号串扰

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
    // ★ 阶段二：预投递命令的截止等待定时器
    m_holdTimer = new QTimer(this);
    m_holdTimer->setTimerType(Qt::PreciseTimer);
    m_holdTimer->setInterval(1);
    m_holdTimer->setSingleShot(false);
    connect(m_holdTimer, &QTimer::timeout,
            this, &GPIBWork::onHoldTick);

    // ★ 发送时间戳时间轴（测量期望发送 / 实际 write / 两次 write 间隔）
    m_txClock.start();
    m_txClockWallAnchorMs = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "GPIBWork: 初始化完成"
             << "(线程:" << QThread::currentThreadId() << ")"
             << "| 精确延时: 1ms轮询+忙等自旋+EMA补偿";
}

GPIBWork::~GPIBWork()
{
    m_holdTimer->stop();
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

// ★ 新增：重置误差补偿（每次批量发送开始时调用）
void GPIBWork::resetTimingCompensation()
{
    m_timingCompensationMs = 0;
    m_prevTxAnchorElapsedMs = -1;   // ★ 新批次重新锚定第一条命令的发送时刻
    qDebug() << "GPIBWork: 误差补偿已重置";
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
    m_prevTxAnchorElapsedMs = -1;   // 新会话重置绝对发送锚点
    m_lastWriteElapsedMs = -1;      // 新会话重新锚定“两次 write 间隔”日志

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
    cancelPendingCommands();   // ★ 丢弃预投递命令并停止截止定时器

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
//  generation：代际标记，到期时原样传回供 GPIBExcel 校验
// ═══════════════════════════════════════════════════════════════
void GPIBWork::sendStringWithDelay(const QString &text, bool hexMode,
                                    const QByteArray &expectedResponse,
                                    int delayMs,
                                    bool forceRead,
                                    int responseTimeoutMs,
                                    int generation)
{
    if (!isOpen() || text.isEmpty() || !m_instrument)
        return;

    // ★ 阶段二：预投递 + 按住到“上一条 viWrite + 本条延时”的绝对截止时刻。
    //    viRead 在 write 之后同步执行；若 viRead 快于延时，下一条仍精确命中截止时刻；
    //    若 viRead 本身超过延时，则按“回复完成后立即发下一条”的原有语义执行。
    m_pendingSend.text       = text;
    m_pendingSend.hexMode    = hexMode;
    m_pendingSend.expected   = expectedResponse;
    m_pendingSend.delayMs    = delayMs;
    m_pendingSend.forceRead  = forceRead;
    m_pendingSend.readTimeoutMs = responseTimeoutMs;
    m_pendingSend.generation = generation;
    m_hasPendingSend = true;

    qint64 arrivalElapsedMs = m_txClock.elapsed();
    m_holdTargetMs = (m_prevTxAnchorElapsedMs >= 0)
            ? m_prevTxAnchorElapsedMs + delayMs
            : arrivalElapsedMs;
    m_expectedWriteElapsedMs = m_holdTargetMs;

    if (m_holdTargetMs <= arrivalElapsedMs) {
        writePendingAtDeadline();
    } else {
        m_holdTimer->start();
    }
}

void GPIBWork::onHoldTick()
{
    if (!m_hasPendingSend) {
        m_holdTimer->stop();
        return;
    }

    qint64 elapsedMs = m_txClock.elapsed();
    if (elapsedMs < m_holdTargetMs - 1)
        return;

    while (m_txClock.elapsed() < m_holdTargetMs) {
        // 最后 1ms 忙等自旋，精准命中截止时刻
    }

    writePendingAtDeadline();
}

void GPIBWork::writePendingAtDeadline()
{
    if (!m_hasPendingSend || !isOpen() || !m_instrument)
        return;

    PendingSend s = m_pendingSend;
    m_hasPendingSend = false;
    m_holdTimer->stop();

    m_expectedResponse = s.expected;
    m_currentGeneration = s.generation;

    QByteArray data = buildSendData(s.text, s.hexMode);

    if (!doVISAWrite(data)) {
        emit commandWritten(s.generation);   // 让 GUI 走超时/下一步流程
        return;
    }
    qint64 actualWriteElapsedMs = m_txClock.elapsed();

    qint64 intervalMs = (m_lastWriteElapsedMs >= 0)
            ? actualWriteElapsedMs - m_lastWriteElapsedMs
            : -1;
    qint64 writeErrorMs = actualWriteElapsedMs - m_expectedWriteElapsedMs;
    m_lastWriteElapsedMs = actualWriteElapsedMs;
    m_prevTxAnchorElapsedMs = actualWriteElapsedMs;

    qDebug() << "[TxTiming][GPIB]"
             << "gen=" << s.generation
             << "delay=" << s.delayMs << "ms"
             << "expectedSend="
             << QDateTime::fromMSecsSinceEpoch(m_txClockWallAnchorMs + m_expectedWriteElapsedMs)
                    .toString("HH:mm:ss.zzz")
             << "actualWrite="
             << QDateTime::fromMSecsSinceEpoch(m_txClockWallAnchorMs + actualWriteElapsedMs)
                    .toString("HH:mm:ss.zzz")
             << "writeError=" << writeErrorMs << "ms"
             << "interval=" << intervalMs << "ms";

    // ★ 先通知 GUI“本条已实际写入”，让响应超时从 viWrite 后开始计时
    emit commandWritten(s.generation);

    bool shouldRead = s.forceRead || !m_expectedResponse.isEmpty();
    if (shouldRead) {
        // ★ 让 viRead 遵守 Excel 行级的全局响应超时，并以上层仪器超时封顶
        int readTimeoutMs = qMin(s.readTimeoutMs, m_timeoutMs);
        if (readTimeoutMs < 1) readTimeoutMs = 1;
        QByteArray response = doVISARead(readTimeoutMs);
        if (!response.isEmpty()) {
            emitData(response);
        }
    } else {
        emit responseReceived(QByteArray());
    }
}

void GPIBWork::cancelPendingCommands()
{
    m_hasPendingSend = false;
    m_holdTimer->stop();
    m_expectedResponse.clear();
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
