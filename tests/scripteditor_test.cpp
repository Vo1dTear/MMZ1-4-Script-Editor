#include "scripteditor.h"
#include <QFile>
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
        QQuickStyle::setStyle("Fusion");
        ScriptEditor editor;
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("editorBackend", &editor);
        engine.load(QUrl::fromLocalFile(QStringLiteral(MMZ_SOURCE_DIR "/Main.qml")));
        QVERIFY(!engine.rootObjects().isEmpty());
        QObject *area = engine.rootObjects().first()->findChild<QObject *>("scriptTextArea");
        QVERIFY(area);
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
