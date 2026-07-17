// An30Layout.cpp

#include "An30Layout.h"
#include <QDebug>

An30Layout& An30Layout::instance() {
    static An30Layout s;
    return s;
}

An30Layout::An30Layout() {
    initRGL();
    initEVH();
}

void An30Layout::setProduct(An30Product product) {
    m_product = product;
    qDebug() << "An30Layout: 切换到" << (product == An30Product::RGL ? "RGL系列" : "EVH系列");
}

An30Product An30Layout::product() const {
    return m_product;
}

const CmdLayout* An30Layout::find(uint8_t cmdType, uint8_t cmdWord) const {
    const auto& map = (m_product == An30Product::RGL) ? m_rglMap : m_evhMap;
    auto it = map.find({cmdType, cmdWord});
    return (it != map.end()) ? &it->second : nullptr;
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
    // ... 原有全部 RGL 命令保持不变 ...
    // （此处省略，使用你现有的 initRGL 内容）
}

// ═══════════════════════════════════════════════════════════════
//  ★ EVH 命令表
// ═══════════════════════════════════════════════════════════════
void An30Layout::initEVH() {
    // ── 0xF0 0x00: 查询输出状态 (9字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询输出状态";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x00;
        lay.fields = {
            {"输出状态", 1, 1},  // 1=停止, 3=CV, 4=CC, 5=CP, 6=CR
        };
        m_evhMap[{0xF0, 0x00}] = lay;
    }

    // ── 0xF0 0x80: 查询电压电流功率 (18字节) ──
    {
        CmdLayout lay;
        lay.cmdName = "查询电压电流功率";
        lay.cmdType = 0xF0;
        lay.cmdWord = 0x80;
        lay.fields = {
            {"电压_V",   2, 100},   // ≤500V: ×100, >500V: ×10
            {"电流_A",   4, 100},   // 有符号
            {"功率_kW",  4, 1000},  // 有符号
        };
        m_evhMap[{0xF0, 0x80}] = lay;
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
        m_evhMap[{0xF0, 0x81}] = lay;
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
        m_evhMap[{0xF0, 0xEB}] = lay;
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
        m_evhMap[{0xF0, 0x24}] = lay;
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
        m_evhMap[{0xA5, 0x00}] = lay;
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
        m_evhMap[{0xA5, 0x01}] = lay;
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
        m_evhMap[{0xA5, 0x02}] = lay;
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
        m_evhMap[{0xA5, 0x03}] = lay;
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
        m_evhMap[{0xA5, 0x04}] = lay;
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
        m_evhMap[{0xA5, 0x05}] = lay;
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
        m_evhMap[{0xA5, 0x06}] = lay;
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
        m_evhMap[{0xA5, 0x1F}] = lay;
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
        m_evhMap[{0xA5, 0x60}] = lay;
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
        m_evhMap[{0xA5, 0x63}] = lay;
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
        m_evhMap[{0xA5, 0x68}] = lay;
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
        m_evhMap[{0xA6, 0x01}] = lay;
    }

    qDebug() << "An30Layout: EVH 命令表已注册";
}
