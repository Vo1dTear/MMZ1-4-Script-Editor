#include <QGuiApplication>
#include <cstdio>
#ifdef Q_OS_WIN
#include <QFileDialog>
#else
#include <KFileCustomDialog>
#include <KFileFilter>
#include <KFileWidget>
#endif
#include <QDialog>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <QImage>
#include "scripteditor.h"

class KdeFileDialogController : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    Q_INVOKABLE void open()
    {
#ifdef Q_OS_WIN
        const QUrl url = QFileDialog::getOpenFileUrl(
            nullptr,
            tr("Open TPL"),
            QUrl(),
            tr("TPL files (*.tpl);;All files (*)"));
        if (url.isValid())
            emit fileSelected(url);
#else
        KFileCustomDialog dialog;
        dialog.setWindowTitle(tr("Open TPL"));
        dialog.setOperationMode(KFileWidget::Opening);
        dialog.fileWidget()->setFilters({
            KFileFilter(tr("TPL files (*.tpl)"), {QStringLiteral("*.tpl")}, {}),
            KFileFilter(tr("All files (*)"), {QStringLiteral("*")}, {})
        });
        connect(dialog.fileWidget(), &KFileWidget::accepted, &dialog, &QDialog::accept);
        dialog.resize(900, 600);
        if (dialog.exec() == QDialog::Accepted)
            emit fileSelected(dialog.fileWidget()->selectedUrl());
    #endif
    }

signals:
    void fileSelected(const QUrl &url);
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("MMZScriptEditor"));
    app.setOrganizationName(QStringLiteral("MMZScriptEditor"));
#ifdef Q_OS_WIN
    // The native Vista-based styles keep a light palette on Windows 10.
    // Configure both layers: QApplication also supplies the palette used by
    // Kirigami. With Qt >= 6.5, Fusion follows the system color scheme.
    QApplication::setStyle(QStringLiteral("Fusion"));
    QQuickStyle::setStyle(QStringLiteral("Fusion"));
#else
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    QQuickStyle::setFallbackStyle(QStringLiteral("Fusion"));
#endif
    ScriptEditor editor;
    KdeFileDialogController fileDialogController;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("editorBackend"), &editor);
    engine.rootContext()->setContextProperty(QStringLiteral("fileDialogController"), &fileDialogController);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    QObject::connect(&engine, &QQmlEngine::warnings, &app, [](const QList<QQmlError> &warnings) {
        for (const auto &warning : warnings) fprintf(stderr, "%s\n", qPrintable(warning.toString()));
    });
    engine.loadFromModule(QStringLiteral("MMZ.Editor"), QStringLiteral("Main"));
    if (app.arguments().contains(QStringLiteral("--smoke-test"))) {
        if (engine.rootObjects().isEmpty()) return 1;
        for (const auto *name : {"document-open", "document-save", "document-save-all", "edit-find"}) {
            const QString path = QStringLiteral(":/icons/%1.svg").arg(QString::fromLatin1(name));
            if (QImage(path).isNull()) {
                fprintf(stderr, "Cannot load action icon: %s\n", qPrintable(path));
                return 1;
            }
        }
        // Exercise initial QML creation and the event loop in packaging CI.
        QTimer::singleShot(1000, &app, &QCoreApplication::quit);
    }
    return app.exec();
}

#include "main.moc"
