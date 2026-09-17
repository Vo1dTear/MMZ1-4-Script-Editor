#include "scripteditor.h"
#include <QFile>
#include "editorfont.h"
#include <QFontMetricsF>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickItem>
#include <QQuickWindow>

class EditorTest : public QObject {
    Q_OBJECT
private slots:
    void saveAllOnlyMarksModifiedScripts() {
        QTemporaryDir dir;
        QFile file(dir.filePath("save-all.tpl"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        const QByteArray original = "script 1 mmz1 {\nhello\n}\nscript 2 mmz1 {\nworld\n}\n";
        file.write(original); file.close();
        ScriptEditor editor;
        QVERIFY(editor.openFile(QUrl::fromLocalFile(file.fileName())));
        QVERIFY(editor.save(true));
        for (const auto &script : editor.scripts())
            QVERIFY(!script.toMap().value("saved").toBool());
        editor.editText("edited longer text");
        editor.selectScript(1);
        editor.editText("temporary");
        editor.undo();
        QVERIFY(editor.save(true));
        QVERIFY(!editor.modified());
        QVERIFY(editor.scripts()[0].toMap().value("saved").toBool());
        QVERIFY(!editor.scripts()[1].toMap().value("saved").toBool());
        QVERIFY(file.open(QIODevice::ReadOnly));
        auto expected = original;
        expected.replace("hello", "edited longer text");
        QCOMPARE(file.readAll(), expected);
    }
    void roundTripAndHistory() {
        QTemporaryDir dir;
        const QString path = dir.filePath("test.tpl");
        const QByteArray original = "// header\r\nscript 1 mmz1 {\r\n\thola á\r\n}\r\n\r\nscript 10 mmz1 {\r\n\tworld\r\n}\r\n// end\r\n";
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write(original); file.close();
        ScriptEditor editor;
        QVERIFY(editor.openFile(QUrl::fromLocalFile(path)));
        QCOMPARE(editor.scripts().size(), 2);
        QVERIFY(editor.save(true));
        QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), original); file.close();
        editor.editText("\tchanged");
        editor.selectScript(1); editor.editText("\tsecond");
        QVERIFY(editor.save(false)); QVERIFY(editor.modified());
        QVERIFY(file.open(QIODevice::ReadOnly));
        auto expected = original; expected.replace("\tworld", "\tsecond");
        QCOMPARE(file.readAll(), expected); file.close();
        editor.selectScript(0); QCOMPARE(editor.text(), QString("\tchanged"));
        editor.undo(); QCOMPARE(editor.text(), QString::fromUtf8("\thola á"));
        editor.redo(); QCOMPARE(editor.text(), QString("\tchanged"));
        QCOMPARE(editor.search("SECOND", false).size(), 1);
        QCOMPARE(editor.search("ch.*", true).size(), 1);
        QVERIFY(editor.save(true)); QVERIFY(!editor.modified());
        expected.replace("\thola á", "\tchanged");
        QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(), expected); file.close();
        ScriptEditor reopened; QVERIFY(reopened.openFile(QUrl::fromLocalFile(path)));
        QCOMPARE(reopened.text(), editor.text());
        QVERIFY(file.open(QIODevice::Append)); file.write("external"); file.close();
        editor.editText("new"); QVERIFY(!editor.save(true)); QVERIFY(editor.modified());
    }
    void qmlEditing() {
        QTest::failOnWarning(QRegularExpression(QStringLiteral("QTextCursor::setPosition:.*out of range")));
        QQuickStyle::setStyle("Fusion");
        ScriptEditor editor;
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("editorFontFamily",
            editorFontFamily());
        engine.rootContext()->setContextProperty("editorBackend", &editor);
        engine.load(QUrl::fromLocalFile(QStringLiteral(MMZ_SOURCE_DIR "/Main.qml")));
        QVERIFY(!engine.rootObjects().isEmpty());
        QObject *area = engine.rootObjects().first()->findChild<QObject *>("scriptTextArea");
        QVERIFY(area);
        const QFont editorFont = area->property("font").value<QFont>();
        const QFontMetricsF editorMetrics(editorFont);
        const qreal narrowWidth = editorMetrics.horizontalAdvance("iiii");
        const qreal wideWidth = editorMetrics.horizontalAdvance("WWWW");
        QVERIFY2(qAbs(narrowWidth - wideWidth) < 0.01,
            qPrintable(QString("Editor font '%1' on platform '%2': iiii=%3, WWWW=%4")
                .arg(editorFont.family(), QGuiApplication::platformName())
                .arg(narrowWidth).arg(wideWidth)));
        QTemporaryDir dir;
        QFile file(dir.filePath("ui.tpl"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("script 1 mmz1 {\nhello\n}\nscript 2 mmz1 {\nworld\n}\n"); file.close();
        QVERIFY(editor.openFile(QUrl::fromLocalFile(file.fileName())));
        QCOMPARE(area->property("text").toString(), QString("hello"));
        QVERIFY(area->setProperty("text", "edited"));
        QCOMPARE(editor.text(), QString("edited"));
        editor.selectScript(1);
        QCOMPARE(area->property("text").toString(), QString("world"));
        editor.selectScript(0); editor.undo();
        QCOMPARE(area->property("text").toString(), QString("hello"));
        editor.redo();
        QCOMPARE(area->property("text").toString(), QString("edited"));
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        auto *list = window->findChild<QQuickItem *>("scriptsList");
        QVERIFY(list);
        list->forceActiveFocus();
        QTRY_VERIFY(list->hasActiveFocus());
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(editor.currentIndex(), 1);
        QCOMPARE(area->property("text").toString(), QString("world"));
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(editor.currentIndex(), 1);
        QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(editor.currentIndex(), 0);
        QCOMPARE(area->property("text").toString(), QString("edited"));
        QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(editor.currentIndex(), 0);
        qobject_cast<QQuickItem *>(area)->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_Down);
        QCOMPARE(editor.currentIndex(), 0);
        QTest::qWait(50);
        const auto findVisualItem = [](auto &&self, QQuickItem *parent, const QString &name) -> QQuickItem * {
            if (parent->objectName() == name) return parent;
            for (auto *child : parent->childItems())
                if (auto *found = self(self, child, name)) return found;
            return nullptr;
        };
        auto rowCenter = [list, &findVisualItem](int index) {
            auto *row = findVisualItem(findVisualItem, list, QString("scriptRow%1").arg(index));
            return row ? row->mapToScene(QPointF(row->width() / 2, row->height() / 2)).toPoint() : QPoint(-1, -1);
        };
        const QPoint first = rowCenter(0);
        const QPoint second = rowCenter(1);
        QVERIFY(first.x() >= 0);
        QVERIFY(second.x() >= 0);
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, second);
        QCOMPARE(editor.currentIndex(), 1);
        QTest::mouseMove(window, first);
        QCOMPARE(editor.currentIndex(), 0);
        QCOMPARE(area->property("text").toString(), QString("edited"));
        QTest::mouseMove(window, second);
        QCOMPARE(editor.currentIndex(), 1);
        QCOMPARE(area->property("text").toString(), QString("world"));
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, second);
        QTest::mouseMove(window, first);
        QCOMPARE(editor.currentIndex(), 1);
        QTest::keyClick(window, Qt::Key_Up);
        QCOMPARE(editor.currentIndex(), 0);
        QTest::qWait(250);
        const auto verifyLineNumbers = [&] {
            const QString text = area->property("text").toString();
            int position = 0;
            const auto lines = text.split('\n');
            for (int i = 0; i < lines.size(); ++i) {
                auto *number = findVisualItem(findVisualItem, window->contentItem(),
                    QString("lineNumber%1").arg(i));
                QVERIFY(number);
                QVERIFY(area->setProperty("cursorPosition", position));
                const QRectF cursor = area->property("cursorRectangle").toRectF();
                QTRY_VERIFY(qAbs(number->y() - cursor.y()) < 0.01);
                QTRY_VERIFY(qAbs(number->height() - cursor.height()) < 0.01);
                QCOMPARE(number->property("text").toString(), QString::number(i + 1));
                position += lines[i].size() + 1;
            }
            QVERIFY(!findVisualItem(findVisualItem, window->contentItem(),
                QString("lineNumber%1").arg(lines.size())));
        };
        editor.editText(QString("hello\n\n\táéíóú\n").repeated(40));
        verifyLineNumbers();
        QFont largerFont = editorFont;
        largerFont.setPointSizeF(13.5);
        QVERIFY(area->setProperty("font", largerFont));
        QVERIFY(area->setProperty("topPadding", 17));
        verifyLineNumbers();
        editor.editText("short\ntext\n");
        verifyLineNumbers();
        editor.undo();
        verifyLineNumbers();
        editor.redo();
        verifyLineNumbers();
        QVERIFY(area->setProperty("text", ""));
        verifyLineNumbers();
        editor.undo();
        verifyLineNumbers();
        editor.selectScript(1);
        verifyLineNumbers();

        QFile manyScripts(dir.filePath("many.tpl"));
        QVERIFY(manyScripts.open(QIODevice::WriteOnly));
        for (int i = 1; i <= 100; ++i)
            manyScripts.write(QString("script %1 mmz1 {\nhello\n}\n").arg(i).toUtf8());
        manyScripts.close();
        QVERIFY(editor.openFile(QUrl::fromLocalFile(manyScripts.fileName())));
        QTest::qWait(100);
        editor.selectScript(49);
        qobject_cast<QQuickItem *>(area)->forceActiveFocus();
        QTest::qWait(100);
        const qreal scrollPosition = list->property("contentY").toReal();
        QVERIFY(scrollPosition > 0);
        const auto verifyScroll = [&] {
            QTest::qWait(50);
            QCOMPARE(editor.currentIndex(), 49);
            QCOMPARE(list->property("contentY").toReal(), scrollPosition);
            auto *row = findVisualItem(findVisualItem, list, "scriptRow49");
            QVERIFY(row);
            QCOMPARE(row->property("text").toString().startsWith("* "), editor.modified());
        };
        QTest::keyClick(window, Qt::Key_E);
        verifyScroll();
        editor.undo();
        verifyScroll();
        editor.redo();
        verifyScroll();
        QVERIFY(editor.save(false));
        verifyScroll();
    }
    void malformedFileKeepsState() {
        QTemporaryDir dir; QFile file(dir.filePath("bad.tpl"));
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("script\n"); file.close();
        ScriptEditor editor; QSignalSpy errors(&editor, &ScriptEditor::error);
        QVERIFY(!editor.openFile(QUrl::fromLocalFile(file.fileName())));
        QCOMPARE(editor.currentIndex(), -1); QCOMPARE(errors.size(), 1);
    }
};
QTEST_MAIN(EditorTest)
#include "scripteditor_test.moc"
