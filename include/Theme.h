#pragma once

#include <QColor>
#include <QString>

// Accent shifted to stay readable on the light or dark surface.
QColor effectiveAccent(const QColor &accent, bool dark);

// Black or white, whichever reads better on top of `background`.
QColor readableOn(const QColor &background);

// Complete application style sheet derived from one accent color. Arrow icons
// (combo boxes, spin boxes, calendar) are generated on disk in the theme's colors.
QString buildStyleSheet(const QColor &accent, bool dark);
