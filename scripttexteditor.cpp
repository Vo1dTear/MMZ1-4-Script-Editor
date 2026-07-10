#include "scripttexteditor.h"

#include <QPainter>
#include <QTextBlock>

LineNumberArea::LineNumberArea(ScriptTextEditor *editor)
: QWidget(editor),
codeEditor(editor)
{
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(codeEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    codeEditor->lineNumberAreaPaintEvent(event);
}

ScriptTextEditor::ScriptTextEditor(QWidget *parent)
: QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);
    
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &ScriptTextEditor::updateLineNumberAreaWidth);
    
    connect(this, &QPlainTextEdit::updateRequest,
            this, &ScriptTextEditor::updateLineNumberArea);
    
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &ScriptTextEditor::highlightCurrentLine);
    
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int ScriptTextEditor::lineNumberAreaWidth()
{
    const int fixedDigits = 4;
    
    int space =
    12 +
    fontMetrics().horizontalAdvance(
        QLatin1Char('9')
    ) * fixedDigits;
    
    return space;
}

void ScriptTextEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void ScriptTextEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if(dy){
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(
            0,
            rect.y(),
                               lineNumberArea->width(),
                               rect.height()
        );
    }
    
    if(rect.contains(viewport()->rect())){
        updateLineNumberAreaWidth(0);
    }
}

void ScriptTextEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    
    QRect cr = contentsRect();
    
    lineNumberArea->setGeometry(
        QRect(
            cr.left(),
              cr.top(),
              lineNumberAreaWidth(),
              cr.height()
        )
    );
}

void ScriptTextEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    QTextEdit::ExtraSelection selection;
    
    QColor lineColor(120, 120, 120, 50);
    
    selection.format.setBackground(lineColor);
    
    selection.format.setProperty(
        QTextFormat::FullWidthSelection,
        true
    );
    
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    
    extraSelections.append(selection);
    
    setExtraSelections(extraSelections);
}

void ScriptTextEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    
    painter.fillRect(
        event->rect(),
                     palette().alternateBase()
    );
    
    QTextBlock block = firstVisibleBlock();
    
    int blockNumber = block.blockNumber();
    
    int top = qRound(
        blockBoundingGeometry(block)
        .translated(contentOffset())
        .top()
    );
    
    int bottom = top + qRound(blockBoundingRect(block).height());
    
    while(block.isValid() && top <= event->rect().bottom()){
        
        if(block.isVisible() && bottom >= event->rect().top()){
            
            QString number = QString::number(blockNumber + 1);
            
            painter.setPen(
                palette().text().color()
            );
            
            painter.drawText(
                0,
                top,
                lineNumberArea->width() - 8,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             number
            );
        }
        
        block = block.next();
        
        top = bottom;
        
        bottom = top + qRound(blockBoundingRect(block).height());
        
        ++blockNumber;
    }
}
