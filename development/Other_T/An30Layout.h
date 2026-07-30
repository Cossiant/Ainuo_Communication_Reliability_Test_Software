// An30Layout.h
// AN3.0 命令字段布局注册表 — 支持 RGL / EVT / EVH 三产品系列

#pragma once

#include <QString>
#include <QVector>
#include <cstdint>
#include <map>

struct FieldDef {
    QString name;
    int     byteLen;
    double  divisor;
};

struct CmdLayout {
    QString           cmdName;
    uint8_t           cmdType;
    uint8_t           cmdWord;
    QVector<FieldDef> fields;
    bool hasSignSkip    = false;
    int  signSkipOffset = 0;
};

// ★ 产品系列
enum class An30Product {
    RGL = 0,   // ANRGL 系列可回馈交流源载一体机
    EVT = 1,   // ★ 原 EVH 产品（已更名为 EVT）
    EVH = 2    // ★ ANEVH 系列双向可编程直流电源（根据用户手册 v1.5）
};

class An30Layout {
public:
    static An30Layout& instance();

    /// ★ 设置当前产品系列（调用后 find/extractAll 使用对应产品命令表）
    void setProduct(An30Product product);

    /// ★ 获取当前产品系列
    An30Product product() const;

    const CmdLayout* find(uint8_t cmdType, uint8_t cmdWord) const;

    QVector<double> extractAll(const CmdLayout& layout,
                                const uint8_t* payload,
                                int payloadLen) const;

private:
    An30Layout();
    void initRGL();
    void initEVT();
    void initEVH();

    An30Product m_product = An30Product::RGL;

    // RGL 命令表
    std::map<std::pair<uint8_t, uint8_t>, CmdLayout> m_rglMap;
    // EVT 命令表（原 EVH）
    std::map<std::pair<uint8_t, uint8_t>, CmdLayout> m_evtMap;
    // EVH 命令表（根据用户手册 v1.5）
    std::map<std::pair<uint8_t, uint8_t>, CmdLayout> m_evhMap;
};
