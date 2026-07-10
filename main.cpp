#include <QApplication>
#include <QStyleFactory>
#include <QStyleHints>
#include "scripteditor.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    #ifdef Q_OS_WIN
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    #endif
    
    ScriptEditor w;
    
    QObject::connect(
        app.styleHints(),
                     &QStyleHints::colorSchemeChanged,
                     &w,
                     [&w]() {
                         w.applyThemeColors();
                     }
    );
    
    w.applyThemeColors();
    w.show();
    
    return app.exec();
}
