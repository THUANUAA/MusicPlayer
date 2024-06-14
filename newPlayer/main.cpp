#include "widget.h"
#include <QApplication>
#include <QTextCodec>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置全局文本编码器为UTF-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    Widget w;
    w.show();

    return a.exec();
}
