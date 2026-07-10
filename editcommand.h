#ifndef EDITCOMMAND_H
#define EDITCOMMAND_H

#include <QUndoCommand>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class EditCommand : public QUndoCommand
{
public:
    EditCommand(QVariantMap* script, const QString& oldText, const QString& newText)
        : script(script), oldText(oldText), newText(newText) {}

    void undo() override {
        (*script)["lines"] = oldText.split('\n');
    }

    void redo() override {
        (*script)["lines"] = newText.split('\n');
    }

private:
    QVariantMap* script;
    QString oldText;
    QString newText;
};

#endif // EDITCOMMAND_H
