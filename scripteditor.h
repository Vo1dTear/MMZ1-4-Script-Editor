#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QVector>

class ScriptEditor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList scripts READ scripts NOTIFY stateChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY selectionChanged)
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY stateChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY stateChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY stateChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY stateChanged)
public:
    explicit ScriptEditor(QObject *parent = nullptr) : QObject(parent) {}
    QVariantList scripts() const;
    int currentIndex() const { return m_index; }
    QString text() const;
    QString fileName() const;
    bool modified() const;
    bool canUndo() const;
    bool canRedo() const;
    Q_INVOKABLE bool openFile(const QUrl &url);
    Q_INVOKABLE void selectScript(int index);
    Q_INVOKABLE void editText(const QString &text);
    Q_INVOKABLE void updateCursor(const QString &text, int cursor, int anchor);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE bool save(bool all = false);
    Q_INVOKABLE QVariantList search(const QString &query, bool regex);
signals:
    void stateChanged();
    void selectionChanged();
    void textChanged();
    void cursorRestored(int cursor, int anchor);
    void error(const QString &message);
    void saved(const QString &message);
private:
    struct Script {
        struct Cursor { int cursor = 0; int anchor = 0; };
        QString label;
        QString savedText;
        QStringList history;
        QVector<Cursor> cursors{{}};
        int position = 0;
        int start = 0;
        int end = 0;
        bool saved = false;
    };
    QVector<Script> m_scripts;
    int m_index = -1;
    QString m_path;
    QString m_source;
    QByteArray m_disk;
    QString m_newline = QStringLiteral("\n");
};
