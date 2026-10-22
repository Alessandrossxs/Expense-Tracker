#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>

// App logo drawn with QPainter in the given accent color: a rounded badge with
// rising bars and a coin. No image files needed, and it follows the theme color.
QPixmap logoPixmap(int logicalSize, const QColor &accent, qreal devicePixelRatio = 2.0);

// Multi-size icon for the window, taskbar and dialogs.
QIcon appIcon(const QColor &accent);
