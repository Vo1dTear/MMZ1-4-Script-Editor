#include "scripteditor.h"
#include <QListWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QListWidgetItem>
#include <QFileInfo>
#include <QTextCursor>
#include <QKeySequence>
#include <QFont>
#include <QColor>
#include <QVariantMap>
#include <QDebug>
#include <QTextBlock>
#include <QLineEdit>
#include <QDockWidget>
#include <QShortcut>
#include <QApplication>
#include <QStyleHints>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QCheckBox>
#include <QFontDatabase>
#include <QPlainTextDocumentLayout>

// ---------------- Constructor ----------------
ScriptEditor::ScriptEditor(QWidget *parent)
: QMainWindow(parent),
currentIndex(-1),
loadingTpl(false),
loadingScript(false)
{
    setAcceptDrops(true);
    
    setWindowTitle("Mega Man Zero 1-4 Script Editor (Qt6 C++)");
    resize(1000, 600);
    setMinimumSize(800, 400);

    setupUi();
    applyThemeColors();

    // Undo / Redo
    connect(textEditor, &QPlainTextEdit::textChanged,
            this, &ScriptEditor::updateUndoRedoButtons);
}

// ---------------- UI ----------------
void ScriptEditor::setupUi()
{
    QMenuBar* menuBar = this->menuBar();
    QMenu* fileMenu = menuBar->addMenu("File");
    
    QAction* openAction = new QAction("Open TPL", this);
    openAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(openAction, &QAction::triggered,
            this, &ScriptEditor::openTpl);
    fileMenu->addAction(openAction);
    
    QAction* saveCurrentAction = new QAction("Save Current Script", this);
    saveCurrentAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(saveCurrentAction, &QAction::triggered,
            this, &ScriptEditor::saveCurrentFile);
    fileMenu->addAction(saveCurrentAction);
    
    QAction* saveAllAction = new QAction("Save All", this);
    connect(saveAllAction, &QAction::triggered,
            this, &ScriptEditor::saveTpl);
    fileMenu->addAction(saveAllAction);
    
    QWidget* central = new QWidget(this);
    
    QHBoxLayout* rootLayout = new QHBoxLayout();
    
    rootLayout->setContentsMargins(0,0,0,0);
    
    central->setLayout(rootLayout);
    
    QAction* searchAction = new QAction("Search", this);
    
    searchAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    
    connect(searchAction, &QAction::triggered, [this]()
    {
        bool visible = searchDock->isVisible();
        
        searchDock->setVisible(!visible);
        
        if(!visible){
            searchDock->raise();
            searchBox->setFocus();
        }
    });
    
    fileMenu->addAction(searchAction);
    
    // EXIT
    QAction* exitAction = new QAction("Exit", this);
    
    exitAction->setShortcut(QKeySequence("Alt+F4"));
    
    connect(
        exitAction,
        &QAction::triggered,
        this,
        &QWidget::close
    );
    
    fileMenu->addSeparator();
    
    fileMenu->addAction(exitAction);
    
    // ========================================
    // LEFT PANEL
    // ========================================
    
    QVBoxLayout* leftLayout = new QVBoxLayout();
    
    scriptList = new QListWidget(this);
    leftLayout->addWidget(scriptList);
    
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setLayout(leftLayout);
    
    leftPanel->setFixedWidth(220);
    
    rootLayout->addWidget(leftPanel);
    
    // ========================================
    // RIGHT PANEL
    // ========================================
    
    QVBoxLayout* rightLayout = new QVBoxLayout();
    
    fileLabel = new QLabel("File: (none)", this);
    
    scriptLabel = new QLabel("", this);
    
    QFont boldFont;
    boldFont.setBold(true);
    
    fileLabel->setFont(boldFont);
    scriptLabel->setFont(boldFont);
    
    rightLayout->addWidget(fileLabel);
    rightLayout->addWidget(scriptLabel);
    
    textEditor = new ScriptTextEditor(this);
    
    QFont mono;
    
    mono.setFamilies({
        "DejaVu Sans Mono",
        "Liberation Mono",
        "Noto Sans Mono",
        "Monospace"
    });
    
    mono.setFixedPitch(true);
    
    mono.setKerning(false);
    
    mono.setStyleStrategy(
        QFont::PreferMatch
    );
    
    mono.setHintingPreference(
        QFont::PreferFullHinting
    );
    
    mono.setPointSize(11);
    
    textEditor->document()->setDefaultFont(mono);
    
    textEditor->setFont(mono);
    
    connect(textEditor, &QPlainTextEdit::textChanged,
            this, &ScriptEditor::updateCharCount);
    
    connect(textEditor, &QPlainTextEdit::cursorPositionChanged,
            this, &ScriptEditor::updateCharCount);
    
    rightLayout->addWidget(textEditor, 1);
    
    // ========================================
    // BUTTONS
    // ========================================
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    saveButton = new QPushButton("Save Current Script", this);
    connect(saveButton, &QPushButton::clicked,
            this, &ScriptEditor::saveCurrentFile);
    btnLayout->addWidget(saveButton);
    
    undoButton = new QPushButton("Undo", this);
    undoButton->setEnabled(false);
    
    connect(undoButton, &QPushButton::clicked,
            this, &ScriptEditor::undoChange);
    
    btnLayout->addWidget(undoButton);
    
    redoButton = new QPushButton("Redo", this);
    redoButton->setEnabled(false);
    
    connect(redoButton, &QPushButton::clicked,
            this, &ScriptEditor::redoChange);
    
    btnLayout->addWidget(redoButton);
    
    // ========================================
    // SEARCH BUTTON
    // ========================================
    
    QPushButton* searchButton = new QPushButton("Search", this);
    
    connect(searchButton, &QPushButton::clicked, [this]()
    {
        
        bool visible = searchDock->isVisible();
        
        searchDock->setVisible(!visible);
        
        if(!visible){
            searchDock->raise();
            searchBox->setFocus();
        }
    });
    
    btnLayout->addWidget(searchButton);    
    
    charCountLabel = new QLabel("Characters: 0", this);
    btnLayout->addWidget(charCountLabel, 0, Qt::AlignRight);
    
    rightLayout->addLayout(btnLayout);
    
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setLayout(rightLayout);
    
    rootLayout->addWidget(rightPanel, 1);
    
    setCentralWidget(central);
    
    // SEARCH DOCK
    // ========================================
    
    searchDock = new QDockWidget("Search", this);
    
    searchDock->setStyleSheet(R"(
        QDockWidget {
            border: none;
        }

        QDockWidget::title {
            text-align: center;
            padding: 6px;
            background: transparent;
            border: none;
            font-weight: bold;
        }
    )");
    
    searchDock->setObjectName("SearchDock");
    
    searchDock->setAllowedAreas(
        Qt::RightDockWidgetArea
    );
    
    searchDock->setFeatures(
        QDockWidget::NoDockWidgetFeatures
    );
    
    searchDock->setFloating(false);
    
    setCorner(
        Qt::TopRightCorner,
        Qt::RightDockWidgetArea
    );
    
    setDockOptions(
        QMainWindow::AnimatedDocks |
        QMainWindow::ForceTabbedDocks
    );
    
    QWidget* searchWidget = new QWidget(this);
    
    QVBoxLayout* searchLayout =
    new QVBoxLayout(searchWidget);
    
    searchBox = new QLineEdit(this);
    
    searchBox->setPlaceholderText(
        "Search across all scripts..."
    );
    
    regexCheckBox = new QCheckBox(
        "Use Regex",
        this
    );
    
    searchResults = new QListWidget(this);
    
    searchResults->setUniformItemSizes(true);
    
    searchLayout->addWidget(searchBox);
    searchLayout->addWidget(regexCheckBox);
    searchLayout->addWidget(searchResults);
    
    searchWidget->setLayout(searchLayout);
    
    searchDock->setWidget(searchWidget);
    
    searchDock->setMinimumWidth(300);
    searchDock->setMaximumWidth(300);
    
    // ADD TO THE RIGHT SIDE
    addDockWidget(
        Qt::RightDockWidgetArea,
        searchDock
    );
    
    searchDock->hide();
    
    // ========================================
    // CONNECTIONS
    // ========================================
    
    connect(scriptList,
            &QListWidget::currentRowChanged,
            this,
            &ScriptEditor::onSelectScript);
    
    connect(searchBox,
            &QLineEdit::textChanged,
            this,
            &ScriptEditor::performGlobalSearch);
    
    connect(regexCheckBox,
            &QCheckBox::toggled,
            this,
            [this]()
            {
                performGlobalSearch(searchBox->text());
            });
    
    connect(searchResults,
            &QListWidget::itemDoubleClicked,
            this,
            &ScriptEditor::openSearchResult);
}

// ---------------- THEMES ----------------
QColor ScriptEditor::getThemeTextColor() const
{
    bool dark = QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    return dark ? QColor("white") : QColor("black");
}

bool ScriptEditor::isDarkTheme() const
{
    return QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

void ScriptEditor::applyThemeColors()
{
    bool dark =
    QApplication::styleHints()->colorScheme()
    == Qt::ColorScheme::Dark;
    
    if(dark){
        
        textEditor->setStyleSheet(R"(
            QPlainTextEdit {
                background-color: #1e1e1e;
                color: white;
                selection-background-color: #3b6ea5;
                selection-color: white;
            }
                )");
        
    } else {
        
        textEditor->setStyleSheet("");
    }
    
    for(int i = 0; i < scriptList->count(); ++i)
        refreshItemColor(i);
}

// ---------------- FILES ----------------
void ScriptEditor::openTpl()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open TPL", "", "TPL files (*.tpl)");
    if(filePath.isEmpty()) return;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox::critical(this, "Error", "Could not open the file");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    loadingTpl = true;
    
    // IMPORTANT:
    // Detach the current document from QTextEdit
    // before deleting stored documents
    QTextDocument* emptyDoc = new QTextDocument(this);
    
    emptyDoc->setDocumentLayout(
        new QPlainTextDocumentLayout(emptyDoc)
    );
    
    emptyDoc->setDefaultFont(
        textEditor->font()
    );
    
    textEditor->setDocument(emptyDoc);
    
    scriptList->clear();
    
    fileLabel->setText(
        "File: " + QFileInfo(filePath).fileName()
    );
    
    currentFile = filePath;
    
    editedScripts.clear();
    savedScripts.clear();
    headerLines.clear();
    scripts.clear();
    
    // Delete old documents safely
    qDeleteAll(documents);
    documents.clear();
    
    parseScripts(content, headerLines, scripts);

    for(auto &s : scripts){
        QListWidgetItem* item = new QListWidgetItem(
            QString("script %1 %2").arg(s["number"].toInt()).arg(s["type"].toString())
        );
        item->setForeground(getThemeTextColor());
        scriptList->addItem(item);
    }

    if(!scripts.isEmpty()){
        currentIndex = 0;
        scriptList->setCurrentRow(0);
        loadScript(0);
    }

    for(int i=0; i<scriptList->count(); ++i)
        refreshItemColor(i);

    loadingTpl = false;
}

void ScriptEditor::parseScripts(const QString &content, QStringList &header, QList<QVariantMap> &scriptsOut)
{
    QStringList lines = content.split(QRegularExpression("\r\n|\r|\n"));
    header.clear();
    scriptsOut.clear();
    QVariantMap current;

    for(int i=0;i<lines.size();++i){
        QString line = lines[i];
        if(line.startsWith("script")){
            if(!current.isEmpty()) scriptsOut.append(current);
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            current.clear();
            current["number"] = parts[1].toInt();
            current["type"] = parts[2];
            current["lines"] = QStringList();
            if(scriptsOut.isEmpty()) header = lines.mid(0,i);
        } else if(!current.isEmpty()){
            if(line.trimmed() != "}") {
                QStringList temp = current["lines"].toStringList();
                temp.append(line);
                current["lines"] = temp;
            }
        }
    }
    if(!current.isEmpty()) scriptsOut.append(current);
}

// ---------------- SELECTION ----------------
void ScriptEditor::onSelectScript(int index)
{
    if(loadingTpl || index < 0 || index >= scripts.size()) return;

    if(currentIndex != -1 && currentIndex != index)
        saveCurrentScript(false);

    currentIndex = index;
    loadScript(index);

    for(int i = 0; i < scriptList->count(); ++i){
        refreshItemColor(i);
    }
}

void ScriptEditor::loadScript(int index)
{
    if(index < 0 || index >= scripts.size())
        return;
    
    QVariantMap script = scripts[index];
    
    loadingScript = true;
    
    // Disconnect previous document signals
    disconnect(
        textEditor->document(),
               nullptr,
               this,
               nullptr
    );
    
    // Create document if it does not exist
    if(!documents.contains(index)) {
        
        QTextDocument* doc = new QTextDocument();
        
        doc->setDocumentLayout(
            new QPlainTextDocumentLayout(doc)
        );
        
        doc->setDefaultFont(
            textEditor->font()
        );
        
        doc->setPlainText(
            script["lines"].toStringList().join("\n")
        );
        
        documents[index] = doc;
    }
    
    // Switch document
    textEditor->setDocument(documents[index]);
    
    // Reconnect signals
    connectDocumentSignals();
    
    loadingScript = false;
    
    scriptLabel->setText(
        QString("script %1 %2")
        .arg(script["number"].toInt())
        .arg(script["type"].toString())
    );
    
    scriptLabel->setPalette(QApplication::palette());
    
    updateCharCount();
    updateUndoRedoButtons();
}

// ---------------- CHARACTER COUNTER ----------------
void ScriptEditor::updateCharCount()
{
    QTextCursor cursor = textEditor->textCursor();
    if(cursor.hasSelection()){
        QString selected = cursor.selectedText().replace(QChar::ParagraphSeparator,"\n");
        charCountLabel->setText(QString("Selected: %1").arg(selected.replace("\t","").length()));
    } else {
        QString line = cursor.block().text();
        int pos = cursor.positionInBlock();
        if(line.startsWith("\t")) { line = line.mid(1); pos = qMax(0,pos-1); }
        QString visible = line.left(pos).replace("\t","");
        charCountLabel->setText(QString("Characters: %1").arg(visible.length()));
    }
}

// ---------------- SAVE ----------------
void ScriptEditor::saveCurrentScript(bool updateList, bool forceSave)
{
    if(currentIndex == -1) return;

    QVariantMap &script = scripts[currentIndex];
    QString currentText = textEditor->toPlainText();
    QStringList newLines = currentText.split(QRegularExpression("\r\n|\r|\n"));

    if(forceSave || script["lines"].toStringList() != newLines){
        script["lines"] = newLines;
    }

    if(updateList) refreshItemColor(currentIndex);

    lastText = currentText;
}

void ScriptEditor::saveCurrentFile()
{
    if(currentIndex == -1 || currentFile.isEmpty()) return;

    saveCurrentScript(false);

    QVariantMap &script = scripts[currentIndex];

    // Read the original file
    QFile file(currentFile);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox::critical(this,"Error","Could not open the file for reading");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QStringList lines = in.readAll().split(QRegularExpression("\r\n|\r|\n"));
    file.close();

    // Search for the script in the file
    QString header = QString("script %1 %2").arg(script["number"].toInt()).arg(script["type"].toString());
    int startIdx = -1, endIdx = -1;
    for(int i=0; i<lines.size(); ++i){
        if(lines[i].trimmed().startsWith(header)) startIdx = i;
        if(startIdx != -1 && lines[i].trimmed() == "}") { endIdx = i; break; }
    }

    if(startIdx == -1 || endIdx == -1){
        QMessageBox::warning(this,"Save","Could not find the script in the file");
        return;
    }

    QStringList newLinesFile;

    // Add lines before the script
    for(int i=0; i<=startIdx; ++i) newLinesFile.append(lines[i]);

    // Add script content
    QStringList scriptLines = script["lines"].toStringList();

    // --- MAIN FIX ---
    // Remove empty lines at the end of the script to avoid an extra line before closing
    while(!scriptLines.isEmpty() && scriptLines.last().trimmed().isEmpty())
        scriptLines.removeLast();

    newLinesFile.append(scriptLines);

    // Add closing brace
    newLinesFile.append("}");

    // Add lines after the script, if any
    for(int i=endIdx+1; i<lines.size(); ++i) newLinesFile.append(lines[i]);

    // Write to the file using CRLF, without an extra empty line at the end
    QFile outFile(currentFile);
    if(!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        QMessageBox::critical(this,"Error","Could not open the file for writing");
        return;
    }

    QTextStream out(&outFile);
    out.setEncoding(QStringConverter::Utf8);

    for(int i=0; i<newLinesFile.size(); ++i){
        out << newLinesFile[i];
        if(i < newLinesFile.size() - 1) out << "\r\n"; // Only add CRLF if this is not the last line
    }

    outFile.close();

    // Update state
    editedScripts.remove(currentIndex);
    savedScripts.insert(currentIndex);
    refreshItemColor(currentIndex);

    QMessageBox::information(this,"Save",QString("Script %1 saved successfully").arg(script["number"].toInt()));
}

void ScriptEditor::saveTpl()
{
    if(currentFile.isEmpty()) return;
    if(currentIndex != -1) saveCurrentScript(false);

    QFile file(currentFile);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox::critical(this,"Error","Could not open the file for reading");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QStringList lines = in.readAll().split(QRegularExpression("\r\n|\r|\n"), Qt::KeepEmptyParts);
    file.close();

    QStringList newLines;
    int lineIndex = 0;

    // Add header
    if(!headerLines.isEmpty()){
        for(int i=0; i<headerLines.size(); ++i){
            newLines.append(headerLines[i]);
        }
        lineIndex = headerLines.size();
    }

    // Iterate through scripts
    for(const QVariantMap &s : scripts){
        QString header = QString("script %1 %2 {").arg(s["number"].toInt()).arg(s["type"].toString());

        // Find the original script position in lines
        int startIdx = -1, endIdx = -1;
        for(int i=lineIndex; i<lines.size(); ++i){
            if(lines[i].trimmed().startsWith(QString("script %1 %2").arg(s["number"].toInt()).arg(s["type"].toString())))
                startIdx = i;
            if(startIdx != -1 && lines[i].trimmed() == "}") { endIdx = i; break; }
        }

        if(startIdx == -1 || endIdx == -1){
            // If not found, add script as new
            newLines.append(header);
            QStringList scriptLines = s["lines"].toStringList();
            if(!scriptLines.isEmpty() && scriptLines.last().isEmpty())
                scriptLines.removeLast();
            newLines.append(scriptLines);
            newLines.append("}");
            continue;
        }

        // Add lines before the script
        for(int i=lineIndex; i<startIdx; ++i)
            newLines.append(lines[i]);

        // Add original script header
        newLines.append(lines[startIdx]);

        // Add script content, removing final empty line if it exists
        QStringList scriptLines = s["lines"].toStringList();
        if(!scriptLines.isEmpty() && scriptLines.last().isEmpty())
            scriptLines.removeLast();
        newLines.append(scriptLines);

        // Add script closing
        newLines.append("}");

        // Continue from the line after the closing brace
        lineIndex = endIdx + 1;
    }

    // Add the rest of the file (if there was anything after the last script)
    for(int i=lineIndex; i<lines.size(); ++i)
        newLines.append(lines[i]);

    // Write to the file using CRLF, preserving the original last line
    QFile outFile(currentFile);
    if(!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        QMessageBox::critical(this,"Error","Could not open the file for writing");
        return;
    }

    QTextStream out(&outFile);
    out.setEncoding(QStringConverter::Utf8);

    for(int i=0; i<newLines.size(); ++i){
        out << newLines[i];
        if(i < newLines.size() - 1) out << "\r\n";
    }

    outFile.close();

    editedScripts.clear();
    savedScripts.clear();
    for(int i=0; i<scripts.size(); ++i) savedScripts.insert(i);
    for(int i=0; i<scriptList->count(); ++i) refreshItemColor(i);

    QMessageBox::information(this,"Save All","All scripts saved successfully");
}

// ---------------- COLORS ----------------
void ScriptEditor::refreshItemColor(int index)
{
    QListWidgetItem* item = scriptList->item(index);
    if (!item) return;

    if (editedScripts.contains(index)) {
        item->setForeground(QColor("#ffaa00"));
    }
    else if (savedScripts.contains(index)) {
        item->setForeground(QColor("#00cc00"));
    }
    else {
        item->setForeground(getThemeTextColor());
    }
}

void ScriptEditor::connectDocumentSignals()
{
    connect(textEditor->document(), &QTextDocument::undoAvailable,
            this, &ScriptEditor::onUndoAvailable);
    
    connect(textEditor->document(), &QTextDocument::redoAvailable,
            this, &ScriptEditor::onRedoAvailable);
    
    connect(textEditor->document(), &QTextDocument::modificationChanged,
            this, &ScriptEditor::onModificationChanged);
    
    connect(textEditor->document(), &QTextDocument::contentsChanged,
            this, &ScriptEditor::onDocumentContentsChanged);
}

void ScriptEditor::onDocumentContentsChanged()
{
    if(currentIndex == -1 || loadingScript)
        return;
    
    editedScripts.insert(currentIndex);
    savedScripts.remove(currentIndex);
    
    refreshItemColor(currentIndex);
}

// ---------------- UNDO / REDO ----------------
void ScriptEditor::onUndoAvailable(bool available) { undoButton->setEnabled(available); updateUndoRedoButtons(); }
void ScriptEditor::onRedoAvailable(bool available) { redoButton->setEnabled(available); updateUndoRedoButtons(); }
void ScriptEditor::onModificationChanged(bool modified)
{
    if(currentIndex == -1 || loadingScript)
        return;
    
    if(modified){
        editedScripts.insert(currentIndex);
        savedScripts.remove(currentIndex);
    } else {
        editedScripts.remove(currentIndex);
    }
    
    refreshItemColor(currentIndex);
    updateUndoRedoButtons();
}

void ScriptEditor::updateUndoRedoButtons()
{
    QTextDocument* doc = textEditor->document();
    
    undoButton->setEnabled(doc->isUndoAvailable());
    redoButton->setEnabled(doc->isRedoAvailable());
    
    undoButton->setText("Undo");
    redoButton->setText("Redo");
}

void ScriptEditor::performGlobalSearch(const QString &text)
{
    searchResults->clear();
    
    if(text.trimmed().isEmpty())
        return;
    
    QRegularExpression regex;
    
    if(regexCheckBox->isChecked()){
        
        regex = QRegularExpression(
            text,
            QRegularExpression::CaseInsensitiveOption
        );
        
        if(!regex.isValid()){
            
            searchResults->addItem(
                "Invalid regex pattern"
            );
            
            return;
        }
    }
    
    for(int i = 0; i < scripts.size(); ++i){
        
        QVariantMap script = scripts[i];
        
        QString content;
        
        if(documents.contains(i)){
            content = documents[i]->toPlainText();
        } else {
            content = script["lines"].toStringList().join("\n");
        }
        
        QStringList lines = content.split('\n');
        
        for(int lineIndex = 0; lineIndex < lines.size(); ++lineIndex){
            
            QString line = lines[lineIndex];
            
            bool match = false;
            
            if(regexCheckBox->isChecked()){
                
                match = regex.match(line).hasMatch();
                
            } else {
                
                match = line.contains(
                    text,
                    Qt::CaseInsensitive
                );
            }
            
            if(match){
                
                QString preview = line.trimmed();
                
                if(preview.length() > 80)
                    preview = preview.left(80) + "...";
                
                QString label = QString(
                    "Script %1 %2 [Line %3]\n%4"
                )
                .arg(script["number"].toInt())
                .arg(script["type"].toString())
                .arg(lineIndex + 1)
                .arg(preview);
                
                QListWidgetItem* item =
                new QListWidgetItem(label);
                
                // Script index
                item->setData(Qt::UserRole, i);
                
                // Line number
                item->setData(Qt::UserRole + 1, lineIndex);
                
                searchResults->addItem(item);
            }
        }
    }
}

void ScriptEditor::openSearchResult(QListWidgetItem* item)
{
    if(!item)
        return;
    
    int scriptIndex =
    item->data(Qt::UserRole).toInt();
    
    int lineIndex =
    item->data(Qt::UserRole + 1).toInt();
    
    scriptList->setCurrentRow(scriptIndex);
    
    QTextDocument* doc = textEditor->document();
    
    QTextBlock block =
    doc->findBlockByLineNumber(lineIndex);
    
    if(block.isValid()){
        
        QTextCursor cursor(block);
        
        textEditor->setTextCursor(cursor);
        
        textEditor->ensureCursorVisible();
    }
    
    textEditor->setFocus();
}

// ---------------- DRAG & DROP ----------------

void ScriptEditor::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls()){
        
        QList<QUrl> urls = event->mimeData()->urls();
        
        if(!urls.isEmpty()){
            
            QString filePath = urls.first().toLocalFile();
            
            if(filePath.endsWith(".tpl", Qt::CaseInsensitive)){
                event->acceptProposedAction();
                return;
            }
        }
    }
    
    event->ignore();
}

void ScriptEditor::dropEvent(QDropEvent* event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    
    if(urls.isEmpty())
        return;
    
    QString filePath = urls.first().toLocalFile();
    
    if(!filePath.endsWith(".tpl", Qt::CaseInsensitive))
        return;
    
    QFile file(filePath);
    
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox::critical(
            this,
            "Error",
            "Could not open the file"
        );
        return;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    
    QString content = in.readAll();
    
    file.close();
    
    loadingTpl = true;
    
    QTextDocument* emptyDoc = new QTextDocument(this);
    
    emptyDoc->setDocumentLayout(
        new QPlainTextDocumentLayout(emptyDoc)
    );
    
    emptyDoc->setDefaultFont(
        textEditor->font()
    );
    
    textEditor->setDocument(emptyDoc);
    
    scriptList->clear();
    
    fileLabel->setText(
        "File: " + QFileInfo(filePath).fileName()
    );
    
    currentFile = filePath;
    
    editedScripts.clear();
    savedScripts.clear();
    headerLines.clear();
    scripts.clear();
    
    qDeleteAll(documents);
    documents.clear();
    
    parseScripts(content, headerLines, scripts);
    
    for(auto &s : scripts){
        
        QListWidgetItem* item =
        new QListWidgetItem(
            QString("script %1 %2")
            .arg(s["number"].toInt())
            .arg(s["type"].toString())
        );
        
        item->setForeground(getThemeTextColor());
        
        scriptList->addItem(item);
    }
    
    if(!scripts.isEmpty()){
        
        currentIndex = 0;
        
        scriptList->setCurrentRow(0);
        
        loadScript(0);
    }
    
    for(int i = 0; i < scriptList->count(); ++i)
        refreshItemColor(i);
    
    loadingTpl = false;
    
    event->acceptProposedAction();
}

// ---------------- EQUIVALENT PYTHON SLOTS ----------------
void ScriptEditor::undoChange() { textEditor->undo(); }
void ScriptEditor::redoChange() { textEditor->redo(); }
void ScriptEditor::selectAll() { textEditor->selectAll(); }

ScriptEditor::~ScriptEditor()
{
    qDeleteAll(documents);
    documents.clear();
}
