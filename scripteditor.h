#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

#include <QMainWindow>
#include <QColor>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QList>
#include <QSet>
#include <QMap>
#include <QTimer>
#include <QTextDocument>
#include "scripttexteditor.h"

class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class QLineEdit;
class QDockWidget;
class QDragEnterEvent;
class QDropEvent;
class QCheckBox;

class ScriptEditor : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit ScriptEditor(QWidget *parent = nullptr);
    ~ScriptEditor();
    
    void applyThemeColors();
    
private slots:
    // Files
    void openTpl();
    void onSelectScript(int index);
    void saveCurrentFile();
    void saveTpl();
    void saveCurrentScript(bool updateList = true, bool forceSave = false);
    
    // Undo / Redo
    void onUndoAvailable(bool available);
    void onRedoAvailable(bool available);
    void onModificationChanged(bool modified);
    void updateUndoRedoButtons();
    void connectDocumentSignals();
    void onDocumentContentsChanged();
    
    // Text
    void undoChange();
    void redoChange();
    void selectAll();
    void updateCharCount();
    
    // Search
    void performGlobalSearch(const QString &text);
    void openSearchResult(QListWidgetItem* item);
    
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    
private:
    void setupUi();
    QColor getThemeTextColor() const;
    bool isDarkTheme() const;
    void checkThemeChange();
    void refreshItemColor(int index);
    void loadScript(int index);
    void parseScripts(const QString &content,
                      QStringList &header,
                      QList<QVariantMap> &scriptsOut);
    
    // UI
    QListWidget* scriptList;
    
    ScriptTextEditor* textEditor;
    
    QLineEdit* searchBox;
    QListWidget* searchResults;
    QDockWidget* searchDock;
    QCheckBox* regexCheckBox;
    
    QLabel* fileLabel;
    QLabel* scriptLabel;
    QLabel* charCountLabel;
    
    QPushButton* saveButton;
    QPushButton* undoButton;
    QPushButton* redoButton;
    
    // Internal state
    QString currentFile;
    int currentIndex;
    
    bool loadingTpl;
    bool loadingScript = false;
    
    QString lastText;
    
    QStringList headerLines;
    
    QList<QVariantMap> scripts;
    
    QSet<int> editedScripts;
    QSet<int> savedScripts;
    
    QMap<int, QTextDocument*> documents;
    
    int lastLightness;
    
    QTimer* themeTimer;
};

#endif // SCRIPTEDITOR_H
