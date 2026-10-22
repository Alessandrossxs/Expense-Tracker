#include "Logo.h"

#include "Theme.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

QPixmap logoPixmap(int logicalSize, const QColor &accent, qreal devicePixelRatio)
{
    const int px = qMax(1, qRound(logicalSize * devicePixelRatio));
    QPixmap pixmap(px, px);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(devicePixelRatio);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(logicalSize / 100.0, logicalSize / 100.0); // draw on a 100x100 canvas

    // Badge
    QLinearGradient gradient(8, 4, 92, 96);
    gradient.setColorAt(0.0, accent.lighter(125));
    gradient.setColorAt(1.0, accent.darker(130));
    p.setPen(Qt::NoPen);
    p.setBrush(gradient);
    p.drawRoundedRect(QRectF(4, 4, 92, 92), 24, 24);

    // Glyph in a color that stays readable on the accent
    const QColor glyph = readableOn(accent);
    p.setBrush(glyph);
    const qreal baseline = 76;
    const qreal barWidth = 13;
    const qreal heights[] = {20, 34, 50};
    for (int i = 0; i < 3; ++i) {
        const qreal x = 24 + i * 20;
        QPainterPath bar;
        bar.addRoundedRect(QRectF(x, baseline - heights[i], barWidth, heights[i]), 4, 4);
        p.drawPath(bar);
    }
    // Coin
    p.setBrush(QColor(glyph.red(), glyph.green(), glyph.blue(), 215));
    p.drawEllipse(QPointF(70, 27), 9, 9);
    return pixmap;
}

QIcon appIcon(const QColor &accent)
{
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        icon.addPixmap(logoPixmap(size, accent, 1.0));
    return icon;
}
