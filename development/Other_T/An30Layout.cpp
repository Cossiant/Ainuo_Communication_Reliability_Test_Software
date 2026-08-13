// An30Layout.cpp

#include "An30Layout.h"
#include <QDebug>

An30Layout& An30Layout::instance() {
    static An30Layout s;
    return s;
}

An30Layout::An30Layout() {
    initRGL();
    initEVT();
    initEVH();
}

void An30Layout::setProduct(An30Product product) {
    m_product = product;
    const char* name = "Unknown";
    switch (product) {
        case An30Product::RGL: name = "RGL系列"; break;
        case An30Product::EVT: name = "EVT系列"; break;
        case An30Product::EVH: name = "EVH系列"; break;
    }
    qDebug() << "An30Layout: 切换到" << name;
}

An30Product An30Layout::product() const {
    return m_product;
}

const CmdLayout* An30Layout::find(uint8_t cmdType, uint8_t cmdWord) const {
    const std::map<std::pair<uint8_t, uint8_t>, CmdLayout>* map = nullptr;
    switch (m_product) {
        case An30Product::RGL: map = &m_rglMap; break;
        case An30Product::EVT: map = &m_evtMap; break;
        case An30Product::EVH: map = &m_evhMap; break;
    }
    if (!map) return nullptr;
    auto it = map->find({cmdType, cmdWord});
    return (it != map->end()) ? &it->second : nullptr;
}

QVector<double> An30Layout::extractAll(const CmdLayout& layout,
                                        const uint8_t* payload,
                                        int payloadLen) const {
    QVector<double> values;
    int offset = 0;

    for (int i = 0; i < layout.fields.size(); ++i) {
        const FieldDef& f = layout.fields[i];

        if (layout.hasSignSkip && offset == layout.signSkipOffset) {
            offset += 1;
        }

        if (offset + f.byteLen > payloadLen) {
            values.append(0.0);
            break;
        }

        uint64_t raw = 0;
        for (int b = 0; b < f.byteLen; ++b)
            raw = (raw << 8) | payload[offset + b];

        values.append(static_cast<double>(raw) / f.divisor);
        offset += f.byteLen;
    }
    return values;
}

// ═══════════════════════════════════════════════════════════════
//  RGL 命令表（原有，不变）
// ═══════════════════════════════════════════════════════════════
void An30Layout::initRGL() {
    // ═══════════════════════════════════════════════════════════════
    //  查询类 (0xF0)：读取实时测量值 / 状态
    // ═══════════════════════════════════════════════════════════════

    // ── 0xF0 0xA4: 查询输出测量值 (单相55字节/三相158字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询输出测量值";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0xA4;
        lay.hasSignSkip   = true;
        lay.signSkipOffset = 6;  // 混合电压3B + 混合电流3B = 6字节后跳过1字节符号位
        lay.fields = {
            {"混合电压_V",   3, 100},
            {"混合电流_A",   3, 100},
            // ★ 此处自动跳过1字节有功符号位
            {"有功功率_W",   4, 100},
            {"视在功率_VA",  4, 100},
            {"功率因数_%",   2, 100},
            {"频率_Hz",      3, 1000},
            {"交流电压_V",   3, 100},
            {"交流电流_A",   3, 100},
            {"无功功率_Var", 4, 100},
            {"直流电压_V",   3, 100},
            {"直流电流_A",   3, 100},
            {"峰值因数_%",   2, 100},
            {"峰值电压_V",   3, 100},
            {"峰值电流_A",   3, 100},
            {"浪涌电流_A",   3, 100},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xF0 0xEB: 查询状态/报警码 (11字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询状态/报警码";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0xEB;
        lay.fields = {
            {"状态码",  1, 1},     // 0=非法 1=急停 2=报警 3=运行 ...
            {"报警码",  2, 1},     // 如 12 → E012
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xF0 0xED: 查询仪表型号 (24字节，ASCII) ──
    // 特殊处理：不含 FieldDef，直接读16字节ASCII

    // ═══════════════════════════════════════════════════════════════
    //  查询设置类 (0xA5)：回读已设定的参数
    // ═══════════════════════════════════════════════════════════════

    // ── 0xA5 0x41: 查询常规参数（发送 0xA5 0xAA → 应答变 0x41）(17字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询常规参数(设定电压/频率)";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x41;
        lay.fields = {
            {"设定交流电压_V", 3, 100},
            {"设定直流电压_V", 3, 100},
            {"设定频率_Hz",    3, 1000},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0x40: 查询更多设置(转换率/波形) (29字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询更多设置";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x40;
        lay.fields = {
            {"起始角度_°",        2, 10},
            {"停止角度_°",        2, 10},
            {"交流转换率_VpMs",   3, 100},
            {"直流转换率_VpMs",   3, 100},
            {"频率转换率_HzpMs",  3, 1000},
            {"终止直流转换率",    3, 100},
            {"波形选择",          1, 1},
            {"模式选择",          1, 1},
            {"百分比_%",          2, 100},
            {"波形组号",          1, 1},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xAE: 查询序列参数 (38字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询序列参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xAE;
        lay.fields = {
            {"步号",              1, 1},
            {"交流起始电压_V",     3, 100},
            {"交流终止电压_V",     3, 100},
            {"直流起始电压_V",     3, 100},
            {"直流终止电压_V",     3, 100},
            {"频率起始_Hz",        3, 1000},
            {"频率终止_Hz",        3, 1000},
            {"波形选择",          1, 1},
            {"波形组号",          1, 1},
            {"步阶相位_°",        2, 10},
            {"运行长度_ms",       4, 1},
            {"停止角度_°",        2, 10},
            {"停止角度使能",      1, 1},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0x80: 查询输出限值参数 (单相20字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询输出限值参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x80;
        lay.fields = {
            {"交流电压限值_V",   3, 100},
            {"直流电压正限_V",   3, 100},
            {"直流电压负限_V",   3, 100},
            {"频率限值_Hz",      3, 1000},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0x81: 查询输出保护参数 (源模式单相21字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询输出保护参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x81;
        lay.fields = {
            {"过流限值_A",       3, 1},      // 单位A，无除数
            {"过流延时_S",       3, 1},      // 单位S
            {"过载限值_VA",      4, 1},      // 单位VA
            {"过载延时_S",       3, 1000},   // 单位0.001S
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0x82: 查询输出波形参数 (单相18字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询输出波形参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x82;
        lay.fields = {
            {"A波形选择",        1, 1},
            {"B波形选择",        1, 1},
            {"A模式",            1, 1},
            {"B模式",            1, 1},
            {"A百分比_%",        2, 100},
            {"B百分比_%",        2, 100},
            {"A波形组号",        1, 1},
            {"B波形组号",        1, 1},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0x83: 查询输出其他参数 (20字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询输出其他参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x83;
        lay.fields = {
            {"输出继电器",        1, 1},     // 0关1开
            {"输出抑制",          1, 1},
            {"远程控制",          1, 1},
            {"浪涌持续时间_ms",   2, 1},
            {"远端测量",          1, 1},
            {"浪涌开始时间_ms",   2, 1},
            {"外部控制",          1, 1},
            {"外部控制方式",      1, 1},     // 0放大1线性
            {"平均时间",          2, 1},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0x90: 查询系统状态参数 (14字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询系统状态参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x90;
        lay.fields = {
            {"输出模式",          1, 1},     // 0单相1三相
            {"正负相序",          1, 1},     // 0正1负
            {"三相关系",          1, 1},     // 0独立1同频率2平衡
            {"相位重定位",        1, 1},
            {"电压设置",          1, 1},     // 0相电压1线电压
            {"源载模式",          1, 1},     // 0源1载
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xEF: 查询第二组系统状态 (13字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询第二组系统状态";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xEF;
        lay.fields = {
            {"电压直停",          1, 1},     // 0关1开
            {"电压摆率",          1, 1},     // 0.0-5.0
            {"环路速度",          1, 1},     // 0低1中2高3定制
            {"电流摆率",          2, 1},     // 0.000-5.000
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0x20: 查询输出模式 (9字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询输出模式";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x20;
        lay.fields = {
            {"输出模式",          1, 1},     // 0AC+DC 1AC 2DC
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xA3: 查询恒流参数 (单相16字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询恒流参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xA3;
        lay.fields = {
            {"电流值_A",         3, 100},
            {"CF值",             2, 1000},    // 1.414-3.000，单位0.001
            {"PF值",             2, 100},     // 0.00-1.00，单位0.01
            {"超前滞后",         1, 1},       // 0滞后1超前
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xB0: 查询恒流更多参数 (单相19字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询恒流更多参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xB0;
        lay.fields = {
            {"CF_PF优先级",      1, 1},       // 0仅PF 1仅CF 2PF优先 3CF优先
            {"CC波形",           1, 1},       // 2SINE 8HARM
            {"HARM组号",         1, 1},
            {"CSIN模式",         1, 1},
            {"CSIN百分比_%",     2, 100},
            {"起始角_°",         2, 10},
            {"变化率_ApMs",      3, 1000},    // 0.000-3000.000
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xA6: 查询恒有功参数 (单相17字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询恒有功参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xA6;
        lay.fields = {
            {"有功功率_W",       4, 100},
            {"CF值",             2, 1000},
            {"PF值",             2, 100},
            {"超前滞后",         1, 1},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xB2: 查询恒有功更多参数 (单相14字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询恒有功更多参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xB2;
        lay.fields = {
            {"CF_PF优先级",      1, 1},
            {"起始角_°",         2, 10},
            {"变化率_WpMs",      3, 1000},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xA5: 查询恒视在参数 (单相17字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询恒视在参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xA5;
        lay.fields = {
            {"视在功率_VA",      4, 100},
            {"CF值",             2, 1000},
            {"PF值",             2, 100},
            {"超前滞后",         1, 1},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xB1: 查询恒视在更多参数 (单相14字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询恒视在更多参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xB1;
        lay.fields = {
            {"CF_PF优先级",      1, 1},
            {"起始角_°",         2, 10},
            {"变化率_VApMs",     3, 1000},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xA4: 查询恒阻参数 (单相11字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询恒阻参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xA4;
        lay.fields = {
            {"电阻值_Ω",         3, 1000},    // 0.001-300.000Ω，单位0.001Ω
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xBF: 查询恒阻更多参数 (单相10字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询恒阻更多参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xBF;
        lay.fields = {
            {"起始角_°",         2, 10},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0xA7: 查询RLC参数 (单相25字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询RLC参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0xA7;
        lay.fields = {
            {"R值_Ω",            3, 1000},
            {"RL值_Ω",           3, 1000},
            {"L值_mH",           3, 100},     // 0.1-5000.000mH，单位0.01mH → 实际上说0.01...
            {"RC值_Ω",           3, 1000},
            {"C值_uF",           4, 1000},    // 0.001-5000.000μF
            {"拓扑结构",         1, 1},       // 1-7
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0x32: 查询间谐波参数 (单相20字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询间谐波参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x32;
        lay.fields = {
            {"开始频率_Hz",      3, 1000},
            {"结束频率_Hz",      3, 1000},
            {"百分比_%",         2, 100},
            {"扫描时间_ms",      3, 1},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }

    // ── 0xA5 0x62: 查询谐波更多参数 (单相19字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询谐波更多参数";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x62;
        lay.fields = {
            {"基波电压_V",       3, 100},
            {"基波频率_Hz",      3, 1000},
            {"直流分量_V",       3, 100},
            {"起始角度_°",       2, 10},
        };
        m_rglMap[{lay.cmdType, lay.cmdWord}] = lay;
    }
}

// ═══════════════════════════════════════════════════════════════
//  ★ EVT 命令表（原 EVH，保持不变）
// ═══════════════════════════════════════════════════════════════
void An30Layout::initEVT() {
    // ── 0xF0 0x00: 查询输出状态 (9字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询输出状态";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x00;
        lay.fields = {
            {"输出状态", 1, 1},  // 1=停止, 3=CV, 4=CC, 5=CP, 6=CR
        };
        m_evtMap[{0xF0, 0x00}] = lay;
    }

    // ── 0xF0 0x80: 查询电压电流功率 (18字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询电压电流功率";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x80;
        lay.fields = {
            {"电压_V",   4, 100},   // ≤500V: ×100, >500V: ×10
            {"电流_A",   4, 100},   // 有符号
            {"功率_kW",  4, 1000},  // 有符号
        };
        m_evtMap[{0xF0, 0x80}] = lay;
    }

    // ── 0xF0 0x81: 查询电压电流功率电阻 (24字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询电压电流功率电阻";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x81;
        lay.fields = {
            {"电压_V",   4, 100},
            {"电流_A",   4, 100},
            {"功率_kW",  4, 1000},
            {"电阻_Ω",   4, 1000},
        };
        m_evtMap[{0xF0, 0x81}] = lay;
    }

    // ── 0xF0 0xEB: 查询当前状态 (9字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询当前状态";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0xEB;
        lay.fields = {
            {"状态", 1, 1},  // 1=待机, 2=启动, 3=报警
        };
        m_evtMap[{0xF0, 0xEB}] = lay;
    }

    // ── 0xF0 0x24: 查询报警代码 (10字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询报警代码";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x24;
        lay.fields = {
            {"报警码", 2, 1},
        };
        m_evtMap[{0xF0, 0x24}] = lay;
    }

    // ── 0xA5 0x00: 查询设定电压 (11字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定电压";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x00;
        lay.fields = {
            {"设定电压_V", 3, 100},
        };
        m_evtMap[{0xA5, 0x00}] = lay;
    }

    // ── 0xA5 0x01: 查询设定正向电流 (11字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定正向电流";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x01;
        lay.fields = {
            {"设定正向电流_A", 3, 100},
        };
        m_evtMap[{0xA5, 0x01}] = lay;
    }

    // ── 0xA5 0x02: 查询设定正向功率 (11字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定正向功率";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x02;
        lay.fields = {
            {"设定正向功率_kW", 3, 1000},
        };
        m_evtMap[{0xA5, 0x02}] = lay;
    }

    // ── 0xA5 0x03: 查询OVP (11字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询OVP";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x03;
        lay.fields = {
            {"OVP_V", 3, 100},
        };
        m_evtMap[{0xA5, 0x03}] = lay;
    }

    // ── 0xA5 0x04: 查询设定反向电流 (11字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定反向电流";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x04;
        lay.fields = {
            {"设定反向电流_A", 3, 100},
        };
        m_evtMap[{0xA5, 0x04}] = lay;
    }

    // ── 0xA5 0x05: 查询设定反向功率 (11字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定反向功率";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x05;
        lay.fields = {
            {"设定反向功率_kW", 3, 1000},
        };
        m_evtMap[{0xA5, 0x05}] = lay;
    }

    // ── 0xA5 0x06: 查询设定电阻 (11字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定电阻";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x06;
        lay.fields = {
            {"设定电阻_Ω", 3, 1000},
        };
        m_evtMap[{0xA5, 0x06}] = lay;
    }

    // ── 0xA5 0x1F: 查询全部设定值 (28字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询全部设定值";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x1F;
        lay.fields = {
            {"设定电压_V",       3, 100},
            {"设定正向电流_A",   3, 100},
            {"设定正向功率_kW",  3, 1000},
            {"电压斜率_VpMs",    2, 100},
            {"设定反向电流_A",   3, 100},
            {"设定反向功率_kW",  3, 1000},
            {"设定电阻_Ω",       3, 1000},
        };
        m_evtMap[{0xA5, 0x1F}] = lay;
    }

    // ── 0xA5 0x60: 查询电压上下限报警 (22字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询电压上下限报警";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x60;
        lay.fields = {
            {"电压上限报警_V",   3, 100},
            {"电压下限报警_V",   3, 100},
            {"上限延时_s",       3, 1000},
            {"下限延时_s",       3, 1000},
            {"上限报警使能",     1, 1},
            {"下限报警使能",     1, 1},
        };
        m_evtMap[{0xA5, 0x60}] = lay;
    }

    // ── 0xA5 0x63: 查询限值 (23字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询限值";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x63;
        lay.fields = {
            {"电压上限_V",    3, 100},
            {"电压下限_V",    3, 100},
            {"电流上限_A",    3, 100},
            {"电流下限_A",    3, 100},
            {"功率上限_kW",   3, 1000},
        };
        m_evtMap[{0xA5, 0x63}] = lay;
    }

    // ── 0xA5 0x68: 查询全部限值 (32字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询全部限值";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x68;
        lay.fields = {
            {"电压上限_V",    3, 100},
            {"电压下限_V",    3, 100},
            {"电流上限_A",    3, 100},
            {"电流下限_A",    3, 100},
            {"功率上限_kW",   3, 1000},
            {"功率下限_kW",   3, 1000},
            {"电阻上限_Ω",    3, 1000},
            {"电阻下限_Ω",    3, 1000},
        };
        m_evtMap[{0xA5, 0x68}] = lay;
    }

    // ── 0xA6 0x01: 查询电子负载参数 (21字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询电子负载参数";
        lay.cmdType = 0xA6;
        lay.cmdWord = 0x01;
        lay.fields = {
            {"模式",    1, 1},    // 0=CV,1=CC,2=CP,3=CR,4=CVCC,5=CVCR,6=CRCC,7=Auto
            {"电压_V",  3, 100},
            {"电流_A",  3, 100},
            {"功率_kW", 3, 1000},
            {"电阻_Ω",  3, 1000},
        };
        m_evtMap[{0xA6, 0x01}] = lay;
    }

    qDebug() << "An30Layout: EVT 命令表已注册（原 EVH）";
}

// ═══════════════════════════════════════════════════════════════
//  ★ EVH 命令表（根据 ANEVH 用户手册 v1.5 — ainuo3.0 协议）
// ═══════════════════════════════════════════════════════════════
void An30Layout::initEVH() {
    // ═══════════════════════════════════════════════════════════════
    //  A3.2 查询类命令 (0xF0)：读取实时测量值 / 状态
    // ═══════════════════════════════════════════════════════════════

    // ── 0xF0 0x00: 查询电源输出状态 (返回9字节) ──
    // 参数: 1字节 — 1=非启动(OFF), 3=CV, 4=CC, 5=CP, 6=CR
    {
        CmdLayout lay;
        lay.cmdName = "查询输出状态";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x00;
        lay.fields = {
            {"输出状态", 1, 1},
        };
        m_evhMap[{0xF0, 0x00}] = lay;
    }

    // ── 0xF0 0x10: 查询电压输出值 (返回10字节) ──
    // 参数: 2字节 — ≤500V: ×100, >500V: ×10
    {
        CmdLayout lay;
        lay.cmdName = "查询电压输出值";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x10;
        lay.fields = {
            {"电压_V", 2, 100},   // ≤500V: ÷100, >500V: 上层按÷10处理
        };
        m_evhMap[{0xF0, 0x10}] = lay;
    }

    // ── 0xF0 0x11: 查询电流输出值 (返回12字节) ──
    // 参数: 4字节有符号 — ×100，单位A
    {
        CmdLayout lay;
        lay.cmdName = "查询电流输出值";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x11;
        lay.fields = {
            {"电流_A", 4, 100},   // 有符号32bit, ×100
        };
        m_evhMap[{0xF0, 0x11}] = lay;
    }

    // ── 0xF0 0x12: 查询功率输出值 (返回12字节) ──
    // 参数: 4字节有符号 — ×1000，单位kW
    {
        CmdLayout lay;
        lay.cmdName = "查询功率输出值";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x12;
        lay.fields = {
            {"功率_kW", 4, 1000},  // 有符号32bit, ×1000
        };
        m_evhMap[{0xF0, 0x12}] = lay;
    }

    // ── 0xF0 0x20: 查询电压电流斜率值 (返回12字节) ──
    // 参数: 2字节电压斜率 + 2字节电流斜率 — 均为 ×100
    {
        CmdLayout lay;
        lay.cmdName = "查询电压电流斜率";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x20;
        lay.fields = {
            {"电压斜率_VpMs",  2, 100},
            {"电流斜率_ApMs",  2, 100},
        };
        m_evhMap[{0xF0, 0x20}] = lay;
    }

    // ── 0xF0 0x80: 查询电压、电流、功率输出值 (返回18字节) ──
    // 参数: 2字节电压(≤500V:×100, >500V:×10) + 4字节电流有符号(×100) + 4字节功率有符号(×1000)
    {
        CmdLayout lay;
        lay.cmdName = "查询电压电流功率";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x80;
        lay.fields = {
            {"电压_V",   2, 100},   // ≤500V: ÷100, >500V: 上层按÷10处理
            {"电流_A",   4, 100},   // 有符号32bit, ×100
            {"功率_kW",  4, 1000},  // 有符号32bit, ×1000
        };
        m_evhMap[{0xF0, 0x80}] = lay;
    }

    // ── 0xF0 0x81: 查询电压、电流、功率、电阻输出值 (返回24字节) ──
    // 参数: 4字节电压(×100) + 4字节电流有符号(×100) + 4字节功率有符号(×1000) + 4字节电阻(×1000)
    // 注意: 0x81 命令下电压统一为4字节×100, 不再区分电压等级
    {
        CmdLayout lay;
        lay.cmdName = "查询电压电流功率电阻";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x81;
        lay.fields = {
            {"电压_V",   4, 100},
            {"电流_A",   4, 100},   // 有符号32bit
            {"功率_kW",  4, 1000},  // 有符号32bit
            {"电阻_Ω",   4, 1000},
        };
        m_evhMap[{0xF0, 0x81}] = lay;
    }

    // ── 0xF0 0xEB: 查询当前状态 (返回9字节) ──
    // 参数: 1字节 — 1=待机, 2=启动, 3=报警
    {
        CmdLayout lay;
        lay.cmdName = "查询当前状态";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0xEB;
        lay.fields = {
            {"状态", 1, 1},  // 1=待机, 2=启动, 3=报警
        };
        m_evhMap[{0xF0, 0xEB}] = lay;
    }

    // ── 0xF0 0x24: 查询报警代码 (返回10字节) ──
    // 参数: 2字节 — 报警代码(16进制)
    {
        CmdLayout lay;
        lay.cmdName = "查询报警代码";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x24;
        lay.fields = {
            {"报警码", 2, 1},
        };
        m_evhMap[{0xF0, 0x24}] = lay;
    }

    // ── 0xF0 0xED: 查询机器型号 (返回14字节) ──
    // 参数: 2字节电压等级 + 2字节电流等级 + 2字节功率等级
    {
        CmdLayout lay;
        lay.cmdName = "查询机器型号";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0xED;
        lay.fields = {
            {"电压等级_V",  2, 1},   // 例: 0x03E8=1000 → EVH1000
            {"电流等级_A",  2, 1},   // 例: 0x008C=140
            {"功率等级_kW", 2, 1},   // 例: 0x001E=30
        };
        m_evhMap[{0xF0, 0xED}] = lay;
    }

    // ═══════════════════════════════════════════════════════════════
    //  A3.4 设置查询类命令 (0xA5)：回读已设定的参数
    // ═══════════════════════════════════════════════════════════════

    // ── 0xA5 0x00: 查询设定的电压值 (返回11字节, 3字节×100) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定电压";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x00;
        lay.fields = {
            {"设定电压_V", 3, 100},
        };
        m_evhMap[{0xA5, 0x00}] = lay;
    }

    // ── 0xA5 0x01: 查询设定的正向电流值 (返回11字节, 3字节×100) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定正向电流";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x01;
        lay.fields = {
            {"设定正向电流_A", 3, 100},
        };
        m_evhMap[{0xA5, 0x01}] = lay;
    }

    // ── 0xA5 0x02: 查询设定的正向功率值 (返回11字节, 3字节×1000) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定正向功率";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x02;
        lay.fields = {
            {"设定正向功率_kW", 3, 1000},
        };
        m_evhMap[{0xA5, 0x02}] = lay;
    }

    // ── 0xA5 0x03: 查询设定的OVP值 (返回11字节, 3字节×100) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询OVP";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x03;
        lay.fields = {
            {"OVP_V", 3, 100},
        };
        m_evhMap[{0xA5, 0x03}] = lay;
    }

    // ── 0xA5 0x04: 查询设定的反向电流值 (返回11字节, 3字节×100) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定反向电流";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x04;
        lay.fields = {
            {"设定反向电流_A", 3, 100},
        };
        m_evhMap[{0xA5, 0x04}] = lay;
    }

    // ── 0xA5 0x05: 查询设定的反向功率值 (返回11字节, 3字节×1000) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定反向功率";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x05;
        lay.fields = {
            {"设定反向功率_kW", 3, 1000},
        };
        m_evhMap[{0xA5, 0x05}] = lay;
    }

    // ── 0xA5 0x06: 查询设定的电阻值 (返回11字节, 3字节×1000) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询设定电阻";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x06;
        lay.fields = {
            {"设定电阻_Ω", 3, 1000},
        };
        m_evhMap[{0xA5, 0x06}] = lay;
    }

    // ── 0xA5 0x1F: 查询全部设定值 (返回28字节) ──
    // 参数: 电压3B(×100) + 正向电流3B(×100) + 正向功率3B(×1000)
    //       + 电压斜率2B(×100) + 反向电流3B(×100) + 反向功率3B(×1000) + 电阻3B(×1000)
    {
        CmdLayout lay;
        lay.cmdName = "查询全部设定值";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x1F;
        lay.fields = {
            {"设定电压_V",       3, 100},
            {"设定正向电流_A",   3, 100},
            {"设定正向功率_kW",  3, 1000},
            {"电压斜率_VpMs",    2, 100},
            {"设定反向电流_A",   3, 100},
            {"设定反向功率_kW",  3, 1000},
            {"设定电阻_Ω",       3, 1000},
        };
        m_evhMap[{0xA5, 0x1F}] = lay;
    }

    // ── 0xA5 0x58: 查询正向功率上限报警值和延时 (返回14字节) ──
    // 参数: 3字节报警值(×1000, 0=不使能) + 3字节延时(ms)
    {
        CmdLayout lay;
        lay.cmdName = "查询正向功率上限报警";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x58;
        lay.fields = {
            {"正向功率报警值_kW", 3, 1000},
            {"报警延时_ms",       3, 1},
        };
        m_evhMap[{0xA5, 0x58}] = lay;
    }

    // ── 0xA5 0x59: 查询反向功率上限报警值和延时 (返回14字节) ──
    // 参数: 3字节报警值(×1000, 0=不使能) + 3字节延时(ms)
    // 注意: 正向功率延时与反向功率延时共用同一值
    {
        CmdLayout lay;
        lay.cmdName = "查询反向功率上限报警";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x59;
        lay.fields = {
            {"反向功率报警值_kW", 3, 1000},
            {"报警延时_ms",       3, 1},
        };
        m_evhMap[{0xA5, 0x59}] = lay;
    }

    // ── 0xA5 0x60: 查询电压上下限报警 (返回22字节) ──
    // 参数: 电压上限3B(×100) + 电压下限3B(×100)
    //       + 上限延时3B(ms) + 下限延时3B(ms) + 上限使能1B + 下限使能1B
    // 使能: 0=不报警, 1=提示, 2=报警
    {
        CmdLayout lay;
        lay.cmdName = "查询电压上下限报警";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x60;
        lay.fields = {
            {"电压上限报警_V",   3, 100},
            {"电压下限报警_V",   3, 100},
            {"上限延时_ms",      3, 1},
            {"下限延时_ms",      3, 1},
            {"上限报警使能",     1, 1},     // 0=不报警, 1=提示, 2=报警
            {"下限报警使能",     1, 1},
        };
        m_evhMap[{0xA5, 0x60}] = lay;
    }

    // ── 0xA5 0x61: 查询正向电流上下限报警 (返回22字节) ──
    // 参数: 电流上限3B(×100) + 电流下限3B(×100)
    //       + 上限延时3B(ms) + 下限延时3B(ms) + 上限使能1B + 下限使能1B
    {
        CmdLayout lay;
        lay.cmdName = "查询正向电流上下限报警";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x61;
        lay.fields = {
            {"电流上限报警_A",   3, 100},
            {"电流下限报警_A",   3, 100},
            {"上限延时_ms",      3, 1},
            {"下限延时_ms",      3, 1},
            {"上限报警使能",     1, 1},
            {"下限报警使能",     1, 1},
        };
        m_evhMap[{0xA5, 0x61}] = lay;
    }

    // ── 0xA5 0x62: 查询反向电流上下限报警 (返回15字节) ──
    // 参数: 反向电流上限3B(×100) + 上限延时3B(ms) + 使能1B
    // 注意: 正向电流上限延时/使能与反向共用同一值
    {
        CmdLayout lay;
        lay.cmdName = "查询反向电流上下限报警";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x62;
        lay.fields = {
            {"反向电流上限报警_A", 3, 100},
            {"上限延时_ms",        3, 1},
            {"报警使能",           1, 1},     // 0=不报警, 1=提示, 2=报警
        };
        m_evhMap[{0xA5, 0x62}] = lay;
    }

    // ── 0xA5 0x63: 查询设定的限值 (返回23字节) ──
    // 参数: 电压上限3B(×100) + 电压下限3B(×100) + 电流上限3B(×100)
    //       + 电流下限3B(×100) + 功率上限3B(×1000)
    {
        CmdLayout lay;
        lay.cmdName = "查询限值";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x63;
        lay.fields = {
            {"电压上限_V",    3, 100},
            {"电压下限_V",    3, 100},
            {"电流上限_A",    3, 100},
            {"电流下限_A",    3, 100},
            {"功率上限_kW",   3, 1000},
        };
        m_evhMap[{0xA5, 0x63}] = lay;
    }

    // ── 0xA5 0x68: 查询全部限值 (返回32字节) ──
    // 参数: 电压上限3B(×100) + 电压下限3B(×100) + 电流上限3B(×100) + 电流下限3B(×100)
    //       + 功率上限3B(×1000) + 功率下限3B(×1000) + 电阻上限3B(×1000) + 电阻下限3B(×1000)
    {
        CmdLayout lay;
        lay.cmdName = "查询全部限值";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x68;
        lay.fields = {
            {"电压上限_V",    3, 100},
            {"电压下限_V",    3, 100},
            {"电流上限_A",    3, 100},
            {"电流下限_A",    3, 100},
            {"功率上限_kW",   3, 1000},
            {"功率下限_kW",   3, 1000},
            {"电阻上限_Ω",    3, 1000},
            {"电阻下限_Ω",    3, 1000},
        };
        m_evhMap[{0xA5, 0x68}] = lay;
    }

    // ── 0xA5 0x67: 查询从机/声音/背光等系统配置 (返回11字节) ──
    // 参数: 从机数量1B + 状态位1B(bit0:自动启动,bit1:报警声,bit2:按键声,bit3:自动背光,bit4:旋钮方式)
    //       + 屏幕亮度1B(%)
    {
        CmdLayout lay;
        lay.cmdName = "查询从机系统配置";
        lay.cmdType = 0xA5;
        lay.cmdWord = 0x67;
        lay.fields = {
            {"从机数量",      1, 1},
            {"状态配置",      1, 1},     // bit0~bit4 各功能开关
            {"屏幕亮度_%",    1, 1},
        };
        m_evhMap[{0xA5, 0x67}] = lay;
    }

    // ═══════════════════════════════════════════════════════════════
    //  A3.7 电子负载查询类 (0xA6)
    // ═══════════════════════════════════════════════════════════════

    // ── 0xA6 0x01: 查询电子负载模式和参数 (返回21字节) ──
    // 参数: 模式1B + 电压3B(×100) + 电流3B(×100) + 功率3B(×1000) + 电阻3B(×1000)
    // 模式: 0=CV, 1=CC, 2=CP, 3=CR, 4=CVCC, 5=CVCR, 6=CRCC, 7=Auto
    {
        CmdLayout lay;
        lay.cmdName = "查询电子负载参数";
        lay.cmdType = 0xA6;
        lay.cmdWord = 0x01;
        lay.fields = {
            {"模式",    1, 1},     // 0=CV,1=CC,2=CP,3=CR,4=CVCC,5=CVCR,6=CRCC,7=Auto
            {"电压_V",  3, 100},
            {"电流_A",  3, 100},
            {"功率_kW", 3, 1000},
            {"电阻_Ω",  3, 1000},
        };
        m_evhMap[{0xA6, 0x01}] = lay;
    }

    qDebug() << "An30Layout: EVH 命令表已注册（根据用户手册 v1.5）";
}
