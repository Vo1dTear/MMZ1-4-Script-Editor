#ifndef SCRIPTTEXTEDITOR_H
#define SCRIPTTEXTEDITOR_H

#include <QPlainTextEdit>
#include <QWidget>

class ScriptTextEditor;

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(ScriptTextEditor *editor);
    
    QSize sizeHint() const override;
    
protected:
    void paintEvent(QPaintEvent *event) override;
    
private:
    ScriptTextEditor *codeEditor;
};

class ScriptTextEditor : public QPlainTextEdit
{
    Q_OBJECT
    
public:
    explicit ScriptTextEditor(QWidget *parent = nullptr);
    
    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    
protected:
    void resizeEvent(QResizeEvent *event) override;
    
private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();
    
private:
    QWidget *lineNumberArea;
};

#endif
