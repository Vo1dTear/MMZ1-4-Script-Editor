#include "scripteditor.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <algorithm>

QVariantList ScriptEditor::scripts() const
{
    QVariantList result;
    for (const auto &s : m_scripts)
        result.append(QVariantMap{{"label", s.label}, {"modified", s.history[s.position] != s.savedText},
            {"saved", s.saved}});
    return result;
}
QString ScriptEditor::text() const { return m_index < 0 ? QString() : m_scripts[m_index].history[m_scripts[m_index].position]; }
QString ScriptEditor::fileName() const { return QFileInfo(m_path).fileName(); }
bool ScriptEditor::modified() const
{
    return std::any_of(m_scripts.cbegin(), m_scripts.cend(), [](const Script &s) { return s.history[s.position] != s.savedText; });
}
bool ScriptEditor::canUndo() const { return m_index >= 0 && m_scripts[m_index].position > 0; }
bool ScriptEditor::canRedo() const { return m_index >= 0 && m_scripts[m_index].position + 1 < m_scripts[m_index].history.size(); }

bool ScriptEditor::openFile(const QUrl &url)
{
    if (!url.isLocalFile()) { emit error(tr("Choose a local TPL file.")); return false; }
    QFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) { emit error(file.errorString()); return false; }
    const QByteArray disk = file.readAll();
    if (file.error() != QFile::NoError) { emit error(file.errorString()); return false; }
    QString source = QString::fromUtf8(disk);
    if (source.toUtf8() != disk) { emit error(tr("The file is not valid UTF-8.")); return false; }
    const QString newline = source.contains("\r\n") ? QStringLiteral("\r\n") : source.contains('\r') ? QStringLiteral("\r") : QStringLiteral("\n");
    source.replace("\r\n", "\n").replace('\r', '\n');
    // Keep all text outside script bodies, including headers, spacing and final newline.
    static const QRegularExpression header(QStringLiteral("^[\\t ]*script[\\t ]+(\\d+)[\\t ]+(\\S+)[\\t ]*\\{[\\t ]*$"));
    QVector<Script> parsed;
    int offset = 0;
    int active = -1;
    for (const auto &line : source.split('\n')) {
        const auto match = header.match(line);
        if (match.hasMatch()) {
            if (active >= 0) { emit error(tr("A script is missing its closing brace.")); return false; }
            Script s;
            s.label = QStringLiteral("script %1 %2").arg(match.captured(1), match.captured(2));
            s.start = offset + line.size() + 1;
            parsed.append(s);
            active = parsed.size() - 1;
        } else if (active >= 0 && line.trimmed() == "}") {
            auto &s = parsed[active];
            s.end = offset;
            s.savedText = source.mid(s.start, s.end - s.start);
            if (s.savedText.endsWith('\n')) s.savedText.chop(1);
            s.history.append(s.savedText);
            active = -1;
        }
        offset += line.size() + 1;
    }
    if (active >= 0 || parsed.isEmpty()) { emit error(tr("No complete TPL scripts found, or a closing brace is missing.")); return false; }
    m_scripts = parsed;
    m_source = source;
    m_disk = disk;
    m_newline = newline;
    m_path = url.toLocalFile();
    m_index = 0;
    emit selectionChanged(); emit textChanged(); emit stateChanged();
    return true;
}
void ScriptEditor::selectScript(int index)
{
    if (index < 0 || index >= m_scripts.size() || index == m_index) return;
    m_index = index;
    emit selectionChanged(); emit textChanged(); emit stateChanged();
}
void ScriptEditor::editText(const QString &value)
{
    if (m_index < 0 || value == text()) return;
    auto &s = m_scripts[m_index];
    s.history = s.history.mid(0, s.position + 1);
    s.history.append(value);
    ++s.position;
    emit stateChanged();
}
void ScriptEditor::undo()
{
    if (!canUndo()) return;
    --m_scripts[m_index].position;
    emit textChanged(); emit stateChanged();
}
void ScriptEditor::redo()
{
    if (!canRedo()) return;
    ++m_scripts[m_index].position;
    emit textChanged(); emit stateChanged();
}
bool ScriptEditor::save(bool all)
{
    if (m_index < 0) return false;
    QFile existing(m_path);
    if (!existing.open(QIODevice::ReadOnly)) { emit error(existing.errorString()); return false; }
    if (existing.readAll() != m_disk) { emit error(tr("The file changed on disk. Reopen it before saving to avoid overwriting external changes.")); return false; }
    existing.close();
    QString output = m_source;
    QVector<Script> updated = m_scripts;
    int shift = 0;
    for (int i = 0; i < m_scripts.size(); ++i) {
        const auto &s = m_scripts[i];
        auto &next = updated[i];
        next.start += shift;
        next.end += shift;
        if ((!all && i != m_index)) continue;
        if (all && s.history[s.position] == s.savedText) continue;
        next.saved = true;
        if (s.history[s.position] == s.savedText) continue;
        const QString value = s.history[s.position];
        for (const auto &line : value.split('\n')) {
            if (line.trimmed() == "}") { emit error(tr("A script body cannot contain a standalone closing brace.")); return false; }
        }
        const QString body = value.isEmpty() ? QString() : value + '\n';
        output.replace(next.start, s.end - s.start, body);
        const int delta = body.size() - (s.end - s.start);
        next.end += delta;
        shift += delta;
        next.savedText = value;
    }
    QString encoded = output;
    encoded.replace("\n", m_newline);
    const QByteArray bytes = encoded.toUtf8();
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        emit error(file.errorString()); return false;
    }
    m_source = output; m_disk = bytes; m_scripts = updated;
    emit stateChanged(); emit saved(all ? tr("All scripts saved.") : tr("Current script saved."));
    return true;
}
QVariantList ScriptEditor::search(const QString &query, bool regex)
{
    QVariantList results;
    if (query.isEmpty()) return results;
    QRegularExpression expression(regex ? query : QRegularExpression::escape(query), QRegularExpression::CaseInsensitiveOption);
    if (!expression.isValid()) { emit error(tr("Invalid regular expression: %1").arg(expression.errorString())); return results; }
    for (int i = 0; i < m_scripts.size(); ++i) {
        const auto &s = m_scripts[i];
        const QString value = s.history[s.position];
        auto matches = expression.globalMatch(value);
        while (matches.hasNext()) {
            const auto match = matches.next();
            const int line = value.left(match.capturedStart()).count('\n') + 1;
            results.append(QVariantMap{{"scriptIndex", i}, {"position", match.capturedStart()}, {"length", match.capturedLength()},
                {"label", tr("%1 · line %2: %3").arg(s.label).arg(line).arg(value.section('\n', line - 1, line - 1).trimmed())}});
        }
    }
    return results;
}
