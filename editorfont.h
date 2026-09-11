#pragma once

#include <QFontDatabase>
#include <QStringList>

inline QString editorFontFamily()
{
    const QStringList installed = QFontDatabase::families();
    // Offscreen can return the generic "monospace" alias even on Windows,
    // where that alias may resolve to a proportional font.
    const QStringList preferred = {
        QFontDatabase::systemFont(QFontDatabase::FixedFont).family(),
        QStringLiteral("Consolas"),
        QStringLiteral("Cascadia Mono"),
        QStringLiteral("Courier New"),
        QStringLiteral("DejaVu Sans Mono"),
        QStringLiteral("Liberation Mono")
    };
    for (const auto &family : preferred) {
        if (installed.contains(family, Qt::CaseInsensitive)
            && QFontDatabase::isFixedPitch(family))
            return family;
    }
    for (const auto &family : installed) {
        if (QFontDatabase::isFixedPitch(family))
            return family;
    }
    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}
