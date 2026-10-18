#include "Theme.h"

#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QStandardPaths>

namespace
{
QColor hsl(float h, float s, float l)
{
    return QColor::fromHslF(h < 0 ? 0 : h, qBound(0.0f, s, 1.0f), qBound(0.0f, l, 1.0f));
}

QString iconDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.isEmpty())
        dir = QDir::tempPath() + QStringLiteral("/expense-tracker");
    dir += QStringLiteral("/icons");
    QDir().mkpath(dir);
    return dir;
}

// Chevron icon written once per color; returns the path usable in url(...).
QString chevron(const QString &dir, const QString &kind, const QColor &color)
{
    const QString path = QStringLiteral("%1/chevron-%2-%3.png").arg(dir, kind, color.name().mid(1));
    if (!QFile::exists(path)) {
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(color, 3.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPolygonF line;
        if (kind == QLatin1String("down"))
            line << QPointF(8, 12) << QPointF(16, 20) << QPointF(24, 12);
        else if (kind == QLatin1String("up"))
            line << QPointF(8, 20) << QPointF(16, 12) << QPointF(24, 20);
        else if (kind == QLatin1String("left"))
            line << QPointF(20, 8) << QPointF(12, 16) << QPointF(20, 24);
        else
            line << QPointF(12, 8) << QPointF(20, 16) << QPointF(12, 24);
        p.drawPolyline(line);
        p.end();
        pixmap.save(path, "PNG");
    }
    return QDir::fromNativeSeparators(path);
}
}

QColor effectiveAccent(const QColor &accent, bool dark)
{
    float h = 0, s = 0, l = 0;
    accent.getHslF(&h, &s, &l);
    l = dark ? qBound(0.58f, l, 0.78f) : qBound(0.26f, l, 0.60f);
    return hsl(h, s, l);
}

QColor readableOn(const QColor &c)
{
    const double luminance = 0.299 * c.redF() + 0.587 * c.greenF() + 0.114 * c.blueF();
    return luminance > 0.62 ? QColor(QStringLiteral("#1b1f2a")) : QColor(Qt::white);
}

QString buildStyleSheet(const QColor &chosen, bool dark)
{
    const QColor accent = effectiveAccent(chosen, dark);
    float h = 0, sat = 0, l = 0;
    accent.getHslF(&h, &sat, &l);
    const QColor accentHover = dark ? accent.lighter(112) : accent.darker(112);

    QString bg, text, sidebar, navText, navHover, card, border, muted, alt, selection, selectionText,
        input, inputBorder, header, secondaryText, secondaryHover;
    auto set = [](QString &target, const QColor &c) { target = c.name(); };
    if (dark) {
        set(bg, hsl(h, sat * 0.35f, 0.085f));        set(text, hsl(h, sat * 0.25f, 0.92f));
        set(sidebar, hsl(h, sat * 0.40f, 0.055f));   set(navText, hsl(h, sat * 0.20f, 0.72f));
        set(navHover, hsl(h, sat * 0.35f, 0.14f));   set(card, hsl(h, sat * 0.30f, 0.125f));
        set(border, hsl(h, sat * 0.25f, 0.19f));     set(muted, hsl(h, sat * 0.15f, 0.62f));
        set(alt, hsl(h, sat * 0.30f, 0.145f));       set(selection, hsl(h, sat * 0.50f, 0.30f));
        selectionText = QStringLiteral("#ffffff");   set(input, hsl(h, sat * 0.30f, 0.10f));
        set(inputBorder, hsl(h, sat * 0.20f, 0.26f)); set(header, hsl(h, sat * 0.30f, 0.105f));
        set(secondaryText, hsl(h, sat * 0.20f, 0.90f)); set(secondaryHover, hsl(h, sat * 0.30f, 0.17f));
    } else {
        set(bg, hsl(h, sat * 0.45f, 0.97f));         set(text, hsl(h, sat * 0.35f, 0.13f));
        set(sidebar, hsl(h, sat * 0.45f, 0.13f));    set(navText, hsl(h, sat * 0.20f, 0.80f));
        set(navHover, hsl(h, sat * 0.40f, 0.21f));   card = QStringLiteral("#ffffff");
        set(border, hsl(h, sat * 0.30f, 0.91f));     set(muted, hsl(h, sat * 0.12f, 0.50f));
        set(alt, hsl(h, sat * 0.40f, 0.985f));       set(selection, hsl(h, sat * 0.90f, 0.92f));
        set(selectionText, hsl(h, sat * 0.60f, 0.20f)); input = QStringLiteral("#ffffff");
        set(inputBorder, hsl(h, sat * 0.20f, 0.85f)); set(header, hsl(h, sat * 0.40f, 0.975f));
        set(secondaryText, hsl(h, sat * 0.30f, 0.22f)); set(secondaryHover, hsl(h, sat * 0.80f, 0.95f));
    }

    const QString dir = iconDir();
    const QColor arrow = QColor(muted);
    QString css = QStringLiteral(R"(
        QMainWindow, QWidget { background: {{bg}}; color: {{text}}; font-size: 10pt; }
        QLabel { background: transparent; }
        QScrollArea { border: none; }
        QWidget#transparentBox { background: transparent; }
        QToolTip { background: {{card}}; color: {{text}}; border: 1px solid {{border}}; border-radius: 6px; padding: 6px 8px; }

        QFrame#sidebar { background: {{sidebar}}; border: none; }
        QLabel#brand { color: #ffffff; font-size: 12pt; font-weight: 700; padding: 12px 4px 16px 4px; }
        QLabel#sidebarCaption { color: {{navText}}; font-size: 8pt; font-weight: 700; padding: 4px 2px 0 2px; }
        QLabel#sidebarFooter { color: {{navText}}; padding: 8px; }
        QFrame#sidebar QComboBox { background: {{navHover}}; color: #ffffff; border: 1px solid {{navHover}}; }
        QFrame#sidebar QComboBox:hover { border-color: {{accent}}; }
        QPushButton#navButton { color: {{navText}}; background: transparent; text-align: left; border: 0; border-radius: 9px; padding: 12px 14px; font-weight: 600; }
        QPushButton#navButton:hover { background: {{navHover}}; color: #ffffff; }
        QPushButton#navButton:checked { background: {{accent}}; color: {{accentText}}; }

        QPushButton#primaryButton { background: {{accent}}; color: {{accentText}}; border: none; border-radius: 9px; padding: 10px 18px; font-weight: 600; }
        QPushButton#primaryButton:hover { background: {{accentHover}}; }
        QPushButton#secondaryButton { background: {{card}}; color: {{secondaryText}}; border: 1px solid {{inputBorder}}; border-radius: 9px; padding: 9px 16px; font-weight: 600; }
        QPushButton#secondaryButton:hover { background: {{secondaryHover}}; border-color: {{accent}}; }
        QPushButton#dangerButton { background: #d94d64; color: #ffffff; border: none; border-radius: 9px; padding: 10px 18px; font-weight: 600; }
        QPushButton#dangerButton:hover { background: #c23f56; }
        QPushButton#segment { background: {{card}}; color: {{secondaryText}}; border: 1px solid {{inputBorder}}; border-radius: 9px; padding: 10px 22px; font-weight: 600; }
        QPushButton#segment:hover { border-color: {{accent}}; }
        QPushButton#segment:checked { background: {{accent}}; color: {{accentText}}; border-color: {{accent}}; }
        QPushButton:focus { outline: none; }

        QLabel#pageTitle { font-size: 23pt; font-weight: 700; color: {{text}}; }
        QLabel#subTitle, QLabel#summaryLine { color: {{muted}}; }
        QLabel#panelHeading { font-size: 13pt; font-weight: 700; }
        QLabel#emptyState { color: {{muted}}; font-size: 11pt; padding: 30px; }
        QLabel#warning, QLabel#popupError { color: #d94d64; font-weight: 600; }
        QLabel#fieldLabel { color: {{muted}}; font-size: 9pt; font-weight: 600; }
        QLabel#popupTitle { font-size: 15pt; font-weight: 700; }
        QLabel#popupText { color: {{text}}; }
        QLabel#toast { background: {{text}}; color: {{bg}}; border-radius: 10px; padding: 11px 18px; font-weight: 600; }
        QWidget#popupScrim { background: rgba(6, 8, 18, 150); }
        QFrame#popupCard { background: {{card}}; border: 1px solid {{border}}; border-radius: 16px; }
        QFrame#metricCard, QFrame#panel { background: {{card}}; border: 1px solid {{border}}; border-radius: 13px; }
        QLabel#metricCaption { color: {{muted}}; }
        QLabel#metricValue { color: {{text}}; font-size: 19pt; font-weight: 700; }

        QTableWidget { background: {{card}}; alternate-background-color: {{alt}}; border: 1px solid {{border}}; border-radius: 10px; padding: 5px; selection-background-color: {{accent}}; selection-color: {{accentText}}; outline: 0; }
        QTableWidget::item:selected, QTableWidget::item:selected:!active { background: {{accent}}; color: {{accentText}}; }
        QTableWidget::item:focus { outline: none; border: none; }
        QHeaderView::section { background: {{header}}; color: {{muted}}; border: none; border-bottom: 1px solid {{border}}; padding: 11px 8px; font-weight: 600; }

        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox { background: {{input}}; color: {{text}}; border: 1px solid {{inputBorder}}; border-radius: 9px; padding: 0 12px; min-height: 38px; selection-background-color: {{accent}}; selection-color: {{accentText}}; placeholder-text-color: {{muted}}; }
        QLineEdit:hover, QComboBox:hover, QDateEdit:hover, QDoubleSpinBox:hover { border-color: {{muted}}; }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QDoubleSpinBox:focus { border: 1px solid {{accent}}; }
        QComboBox::drop-down, QDateEdit::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 30px; border: none; background: transparent; }
        QComboBox::down-arrow, QDateEdit::down-arrow { image: url("{{iconDown}}"); width: 12px; height: 12px; }
        QDoubleSpinBox { padding-right: 30px; }
        QDoubleSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 24px; border: none; background: transparent; margin: 4px 4px 0 0; border-radius: 6px; }
        QDoubleSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 24px; border: none; background: transparent; margin: 0 4px 4px 0; border-radius: 6px; }
        QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover { background: {{selection}}; }
        QDoubleSpinBox::up-arrow { image: url("{{iconUp}}"); width: 10px; height: 10px; }
        QDoubleSpinBox::down-arrow { image: url("{{iconDown}}"); width: 10px; height: 10px; }
        QComboBox QAbstractItemView { background: {{card}}; color: {{text}}; border: 1px solid {{border}}; border-radius: 8px; padding: 4px; selection-background-color: {{selection}}; selection-color: {{selectionText}}; outline: 0; }

        QCalendarWidget QWidget#qt_calendar_navigationbar { background: {{card}}; border-bottom: 1px solid {{border}}; }
        QCalendarWidget QToolButton { color: {{text}}; background: transparent; border: none; border-radius: 8px; padding: 6px 10px; font-weight: 600; icon-size: 14px; }
        QCalendarWidget QToolButton:hover { background: {{selection}}; }
        QCalendarWidget QToolButton#qt_calendar_prevmonth { qproperty-icon: url("{{iconLeft}}"); }
        QCalendarWidget QToolButton#qt_calendar_nextmonth { qproperty-icon: url("{{iconRight}}"); }
        QCalendarWidget QToolButton::menu-indicator { image: none; }
        QCalendarWidget QMenu { background: {{card}}; color: {{text}}; border: 1px solid {{border}}; }
        QCalendarWidget QSpinBox { background: {{input}}; color: {{text}}; border: 1px solid {{inputBorder}}; border-radius: 6px; min-height: 26px; padding: 0 6px; }
        QCalendarWidget QAbstractItemView:enabled { background: {{card}}; color: {{text}}; selection-background-color: {{accent}}; selection-color: {{accentText}}; outline: 0; }
        QCalendarWidget QAbstractItemView:disabled { color: {{muted}}; }

        QScrollBar:vertical { background: transparent; width: 12px; margin: 2px; }
        QScrollBar:horizontal { background: transparent; height: 12px; margin: 2px; }
        QScrollBar::handle:vertical { background: {{inputBorder}}; border-radius: 4px; min-height: 36px; margin: 0 2px; }
        QScrollBar::handle:horizontal { background: {{inputBorder}}; border-radius: 4px; min-width: 36px; margin: 2px 0; }
        QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover { background: {{accent}}; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; background: none; border: none; }
        QScrollBar::add-page, QScrollBar::sub-page { background: none; }
    )");
    const QList<QPair<QString, QString>> tokens = {
        {"bg", bg}, {"text", text}, {"sidebar", sidebar}, {"navText", navText}, {"navHover", navHover},
        {"accent", accent.name()}, {"accentHover", accentHover.name()},
        {"accentText", readableOn(accent).name()}, {"card", card}, {"border", border},
        {"muted", muted}, {"alt", alt}, {"selection", selection}, {"selectionText", selectionText},
        {"input", input}, {"inputBorder", inputBorder}, {"header", header},
        {"secondaryText", secondaryText}, {"secondaryHover", secondaryHover},
        {"iconDown", chevron(dir, QStringLiteral("down"), arrow)},
        {"iconUp", chevron(dir, QStringLiteral("up"), arrow)},
        {"iconLeft", chevron(dir, QStringLiteral("left"), arrow)},
        {"iconRight", chevron(dir, QStringLiteral("right"), arrow)}};
    for (const auto &token : tokens)
        css.replace(QStringLiteral("{{%1}}").arg(token.first), token.second);
    return css;
}
