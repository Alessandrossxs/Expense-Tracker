#include "ChartWidgets.h"

#include <QFontMetrics>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace
{
QColor textColor(bool dark) { return QColor(dark ? "#e6e8f2" : "#172033"); }
QColor mutedColor(bool dark) { return QColor(dark ? "#8b92ab" : "#778097"); }
QColor gridColor(bool dark) { return QColor(dark ? "#2a2e45" : "#e7eaf1"); }

const QColor kIncomeColor(QStringLiteral("#16845b"));
const QColor kExpenseColor(QStringLiteral("#d94d64"));

QColor sliceColor(int index, bool other, const QColor &accent, bool dark)
{
    qreal hue = accent.hslHueF();
    if (hue < 0)
        hue = 0;
    const qreal saturation = other ? 0.22 : 0.78;
    const qreal lightness = other ? (dark ? 0.68 : 0.50) : (dark ? 0.68 : 0.48);
    if (!other)
        hue = std::fmod(hue + index * 72.0 / 360.0, 1.0);
    return QColor::fromHslF(hue, saturation, lightness);
}

void drawCentered(QPainter &p, const QRect &r, const QString &text, const QColor &color)
{
    p.setPen(color);
    p.drawText(r, Qt::AlignCenter, text);
}
}

DonutChart::DonutChart(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("transparentBox"));
}

void DonutChart::setData(const QList<DonutSlice> &slices, const QString &centerValue,
                         const QString &centerCaption)
{
    m_slices = slices;
    m_centerValue = centerValue;
    m_centerCaption = centerCaption;
    update();
}

void DonutChart::setEmptyText(const QString &text) { m_emptyText = text; update(); }
void DonutChart::setDarkMode(bool dark) { m_dark = dark; update(); }
void DonutChart::setAccent(const QColor &accent) { m_accent = accent; update(); }
QSize DonutChart::sizeHint() const { return {420, 220}; }
QSize DonutChart::minimumSizeHint() const { return {300, 190}; }

void DonutChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    qint64 total = 0;
    for (const DonutSlice &s : m_slices) total += s.value;
    if (m_slices.isEmpty() || total <= 0) {
        p.setPen(mutedColor(m_dark));
        p.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, m_emptyText);
        return;
    }

    const int side = qMax(110, qMin(height() - 10, static_cast<int>(width() * 0.42)));
    const QRect ringBox(4, (height() - side) / 2, side, side);
    const int ringWidth = qMax(14, side / 7);
    const QRect arc = ringBox.adjusted(ringWidth / 2, ringWidth / 2, -ringWidth / 2, -ringWidth / 2);

    int start = 90 * 16;
    qint64 accumulated = 0;
    for (int i = 0; i < m_slices.size(); ++i) {
        accumulated += m_slices.at(i).value;
        const int end = 90 * 16 - qRound(360.0 * 16.0 * double(accumulated) / double(total));
        QPen pen(sliceColor(i, m_slices.at(i).other, m_accent, m_dark), ringWidth);
        pen.setCapStyle(Qt::FlatCap);
        p.setPen(pen);
        p.drawArc(arc, start, end - start);
        start = end;
    }

    QFont big = font();
    big.setBold(true);
    big.setPointSizeF(qBound(9.0, side / 14.0, 16.0));
    const QRect inner = ringBox.adjusted(ringWidth + 6, ringWidth + 6, -ringWidth - 6, -ringWidth - 6);
    p.setFont(big);
    drawCentered(p, inner.adjusted(0, 0, 0, -inner.height() / 5), m_centerValue, textColor(m_dark));
    QFont small = font();
    small.setPointSizeF(qMax(7.5, side / 18.0));
    p.setFont(small);
    drawCentered(p, inner.adjusted(0, inner.height() / 4, 0, 0), m_centerCaption, mutedColor(m_dark));

    // Legend
    p.setFont(font());
    const QFontMetrics fm(font());
    const int x0 = ringBox.right() + 28;
    const int rowH = 26;
    const int count = m_slices.size();
    int y = (height() - count * rowH) / 2;
    const int right = width() - 8;
    for (int i = 0; i < count; ++i, y += rowH) {
        const DonutSlice &s = m_slices.at(i);
        p.setPen(Qt::NoPen);
        p.setBrush(sliceColor(i, s.other, m_accent, m_dark));
        p.drawRoundedRect(QRect(x0, y + rowH / 2 - 5, 10, 10), 3, 3);
        const int valueW = fm.horizontalAdvance(s.valueText);
        const QRect valueRect(right - valueW, y, valueW, rowH);
        const int labelW = qMax(20, valueRect.left() - (x0 + 18) - 8);
        p.setPen(textColor(m_dark));
        p.drawText(QRect(x0 + 18, y, labelW, rowH), Qt::AlignVCenter | Qt::AlignLeft,
                   fm.elidedText(s.label, Qt::ElideRight, labelW));
        p.setPen(mutedColor(m_dark));
        p.drawText(valueRect, Qt::AlignVCenter | Qt::AlignRight, s.valueText);
    }
}

MonthlyBarChart::MonthlyBarChart(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("transparentBox"));
}

void MonthlyBarChart::setData(const QList<BarGroup> &groups, const QString &maxLabel)
{
    m_groups = groups;
    m_maxLabel = maxLabel;
    update();
}

void MonthlyBarChart::setEmptyText(const QString &text) { m_emptyText = text; update(); }
void MonthlyBarChart::setDarkMode(bool dark) { m_dark = dark; update(); }
QSize MonthlyBarChart::sizeHint() const { return {420, 220}; }
QSize MonthlyBarChart::minimumSizeHint() const { return {300, 190}; }

void MonthlyBarChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    qint64 maxValue = 0;
    for (const BarGroup &g : m_groups) maxValue = std::max({maxValue, g.income, g.expenses});
    if (m_groups.isEmpty() || maxValue <= 0) {
        p.setPen(mutedColor(m_dark));
        p.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, m_emptyText);
        return;
    }

    const QFontMetrics fm(font());
    const QRect plot(6, 30, width() - 12, height() - 30 - 24);

    // Legend (top right)
    int lx = width() - 8;
    for (const auto &entry : {std::pair<QString, QColor>{QObject::tr("Uscite"), kExpenseColor},
                              std::pair<QString, QColor>{QObject::tr("Entrate"), kIncomeColor}}) {
        const int w = fm.horizontalAdvance(entry.first);
        p.setPen(mutedColor(m_dark));
        p.drawText(QRect(lx - w, 2, w, 18), Qt::AlignVCenter | Qt::AlignRight, entry.first);
        p.setPen(Qt::NoPen);
        p.setBrush(entry.second);
        p.drawRoundedRect(QRect(lx - w - 16, 7, 10, 10), 3, 3);
        lx -= w + 30;
    }

    // Grid
    p.setPen(QPen(gridColor(m_dark), 1));
    for (int i = 0; i <= 2; ++i) {
        const int y = plot.bottom() - plot.height() * i / 2;
        p.drawLine(plot.left(), y, plot.right(), y);
    }
    p.setPen(mutedColor(m_dark));
    p.drawText(QRect(plot.left(), 2, qMax(60, lx - plot.left()), 18), Qt::AlignVCenter | Qt::AlignLeft, m_maxLabel);

    const int n = m_groups.size();
    const double groupW = double(plot.width()) / n;
    const int barW = qBound(8, static_cast<int>(groupW * 0.28), 30);
    auto barHeight = [&](qint64 v) {
        return v <= 0 ? 0 : qMax(2, static_cast<int>(plot.height() * double(v) / double(maxValue)));
    };
    for (int i = 0; i < n; ++i) {
        const BarGroup &g = m_groups.at(i);
        const int cx = plot.left() + static_cast<int>(groupW * (i + 0.5));
        p.setPen(Qt::NoPen);
        const int hi = barHeight(g.income);
        const int he = barHeight(g.expenses);
        p.setBrush(kIncomeColor);
        if (hi) p.drawRoundedRect(QRect(cx - barW - 2, plot.bottom() - hi, barW, hi), 3, 3);
        p.setBrush(kExpenseColor);
        if (he) p.drawRoundedRect(QRect(cx + 2, plot.bottom() - he, barW, he), 3, 3);
        p.setPen(mutedColor(m_dark));
        p.drawText(QRect(cx - static_cast<int>(groupW / 2), plot.bottom() + 5, static_cast<int>(groupW), 18),
                   Qt::AlignCenter, g.label);
    }
}
