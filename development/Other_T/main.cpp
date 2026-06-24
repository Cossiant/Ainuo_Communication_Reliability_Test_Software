#include <QApplication>
#include <QGuiApplication>
#include "ElaApplication.h"
#include "ElaWidgetToolsDemo.h"

int main(int argc, char *argv[]) {
    //启用高 DPI 图标。在高分屏（4K、Retina）下，如果程序里用了 QPixmap，这条语句会让 Qt 自动使用 @2x 的高分辨率版本，避免图标模糊。
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    //强制缩放倍率 1.5 倍。所有控件和文字都会被放大 150%。这是手动覆盖系统的 DPI 设置。
    qputenv("QT_SCALE_FACTOR","1");
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon("../ico/icon.ico"));
    //eApp 是一个全局指针，eApp->init() 负责初始化 ElaWidgetTools 的主题、样式、配置等。必须在使用任何 Ela 控件之前调用
    ElaApplication::getInstance()->init();
    ElaWidgetToolsDemo w;
    w.show();
    return a.exec();
}