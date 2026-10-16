#include "MainWindow.h"

#include "CategoryTranslations.h"
#include "ComboPopupDelegate.h"
#include "TransactionDialog.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFile>\n#include <QFileDialog>
#include <QFormLayout>
#include <QFont>
#include <QFontDatabase>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QKeySequence>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPalette>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QProxyStyle>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QShortcut>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QDir>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTimer>
#include <QHash>
#include <QDebug>
#include <QFileInfo>
#include <limits>

#include <algorithm>
#include <cmath>
#include <memory>

namespace
{
class ChevronStyle final : public QProxyStyle
{
public:
    explicit ChevronStyle(QStyle *baseStyle)
        : QProxyStyle(baseStyle)
    {
    }

    void drawPrimitive(
        PrimitiveElement element,
        const QStyleOption *option,
        QPainter *painter,
        const QWidget *widget = nullptr) const override
    {
        Qt::ArrowType direction;
        switch (element) {
        case PE_IndicatorArrowDown:
        case PE_IndicatorSpinDown:
            direction = Qt::DownArrow;
            break;
        case PE_IndicatorArrowUp:
        case PE_IndicatorSpinUp:
            direction = Qt::UpArrow;
            break;
        case PE_IndicatorArrowLeft:
            direction = Qt::LeftArrow;
            break;
        case PE_IndicatorArrowRight:
            direction = Qt::RightArrow;
            break;
        default:
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }

        painter->save();
        QColor color = option->palette.color(QPalette::ButtonText);
        if (!(option->state & State_Enabled))
            color = option->palette.color(QPalette::Disabled, QPalette::ButtonText);
        QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        const QRectF bounds = option->rect;
        const QPointF center = bounds.center();
        const qreal extent = qMin<qreal>(5.0, qMin(bounds.width(), bounds.height()) / 3.0);
        QPainterPath chevron;
        switch (direction) {
        case Qt::DownArrow:
            chevron.moveTo(center.x() - extent, center.y() - extent / 2.0);
            chevron.lineTo(center.x(), center.y() + extent / 2.0);
            chevron.lineTo(center.x() + extent, center.y() - extent / 2.0);
            break;
        case Qt::UpArrow:
            chevron.moveTo(center.x() - extent, center.y() + extent / 2.0);
            chevron.lineTo(center.x(), center.y() - extent / 2.0);
            chevron.lineTo(center.x() + extent, center.y() + extent / 2.0);
            break;
        case Qt::LeftArrow:
            chevron.moveTo(center.x() + extent / 2.0, center.y() - extent);
            chevron.lineTo(center.x() - extent / 2.0, center.y());
            chevron.lineTo(center.x() + extent / 2.0, center.y() + extent);
            break;
        case Qt::RightArrow:
            chevron.moveTo(center.x() - extent / 2.0, center.y() - extent);
            chevron.lineTo(center.x() + extent / 2.0, center.y());
            chevron.lineTo(center.x() - extent / 2.0, center.y() + extent);
            break;
        default:
            break;
        }
        painter->drawPath(chevron);
        painter->restore();
    }
};

class TutorialGraphic final : public QWidget
{
public:
    explicit TutorialGraphic(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(220);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void setStep(int step)
    {
        m_step = step;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(width() / 560.0, height() / 220.0);

        const QColor background = palette().color(QPalette::Window);
        const QColor panel = palette().color(QPalette::Base);
        const QColor text = palette().color(QPalette::WindowText);
        const QColor muted = palette().color(QPalette::Mid);
        const QColor accent = palette().color(QPalette::Highlight);
        const QColor border = palette().color(QPalette::Midlight);
        auto box = [&painter, &border](const QRectF &rect,
                                      const QColor &fill,
                                      qreal radius = 7.0) {
            painter.setPen(QPen(border, 1));
            painter.setBrush(fill);
            painter.drawRoundedRect(rect, radius, radius);
        };
        auto line = [&painter, &muted](qreal x, qreal y, qreal w,
                                       qreal thickness = 5.0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(muted);
            painter.drawRoundedRect(QRectF(x, y, w, thickness), 2, 2);
        };

        painter.fillRect(QRectF(0, 0, 560, 220), background);
        box(QRectF(8, 8, 544, 204), panel, 12);
        painter.setPen(Qt::NoPen);
        painter.setBrush(background);
        painter.drawRoundedRect(QRectF(18, 18, 106, 184), 8, 8);
        for (int index = 0; index < 5; ++index) {
            const QRectF item(26, 50 + index * 27, 90, 20);
            if (index == m_step)
                box(item, accent, 5);
            line(item.x() + 8, item.y() + 7, index == m_step ? 46 : 54, 5);
        }
        line(28, 29, 71, 7);

        if (m_step == 0) {
            for (int index = 0; index < 3; ++index) {
                const QRectF card(140 + index * 126, 26, 112, 42);
                box(card, index == 0 ? accent : background);
                line(card.x() + 10, card.y() + 10, 40);
                line(card.x() + 10, card.y() + 25, 68, 7);
            }
            box(QRectF(140, 80, 235, 105), background);
            box(QRectF(386, 80, 148, 105), background);
            painter.setPen(QPen(accent, 3, Qt::SolidLine, Qt::RoundCap));
            painter.drawPolyline(QPolygonF{
                QPointF(156, 161), QPointF(194, 140), QPointF(232, 151),
                QPointF(270, 116), QPointF(310, 129), QPointF(357, 99)});
            for (int index = 0; index < 3; ++index)
                line(400, 101 + index * 25, 90);
        } else if (m_step == 1) {
            box(QRectF(140, 26, 394, 36), accent);
            line(154, 39, 130);
            for (int index = 0; index < 4; ++index) {
                const QRectF row(140, 72 + index * 29, 394, 23);
                box(row, index == 1 ? accent : background, 5);
                line(154, row.y() + 9, 120);
                line(402, row.y() + 9, 88);
            }
        } else if (m_step == 2) {
            box(QRectF(140, 26, 178, 154), background);
            for (int index = 0; index < 4; ++index) {
                box(QRectF(154, 42 + index * 29, 150, 19),
                    index == 3 ? accent : panel, 4);
            }
            box(QRectF(330, 26, 204, 154), background);
            for (int index = 0; index < 4; ++index) {
                const QRectF row(344, 42 + index * 31, 176, 23);
                box(row, index == 2 ? accent : panel, 4);
                line(352, row.y() + 9, 84);
                line(462, row.y() + 9, 45);
            }
        } else if (m_step == 3) {
            box(QRectF(140, 26, 394, 154), background);
            painter.setPen(QPen(accent, 3, Qt::SolidLine, Qt::RoundCap));
            painter.drawPolyline(QPolygonF{
                QPointF(160, 145), QPointF(205, 122), QPointF(248, 137),
                QPointF(292, 86), QPointF(337, 105), QPointF(380, 68),
                QPointF(425, 82), QPointF(510, 48)});
            for (int index = 0; index < 4; ++index) {
                const QRectF bar(160, 158 - index * 24, 116 + index * 29, 8);
                painter.setPen(Qt::NoPen);
                painter.setBrush(index == 0 ? accent : muted);
                painter.drawRoundedRect(bar, 3, 3);
            }
        } else {
            box(QRectF(140, 26, 394, 154), background);
            for (int index = 0; index < 3; ++index) {
                const QRectF card(156 + index * 121, 48, 105, 103);
                box(card, index == 1 ? accent : panel, 9);
                painter.setPen(Qt::NoPen);
                painter.setBrush(index == 1 ? panel : accent);
                painter.drawEllipse(QRectF(card.x() + 35, card.y() + 17, 34, 34));
                line(card.x() + 18, card.y() + 67, 69);
                line(card.x() + 18, card.y() + 82, 48);
            }
        }
    }

private:
    int m_step = 0;
};

QList<QColor> chartColors(const QString &palette, bool lightMode = false)
{
    static const QList<QColor> ocean = {
        QColor(QStringLiteral("#22d3ee")),
        QColor(QStringLiteral("#34d399")),
        QColor(QStringLiteral("#fb7185")),
        QColor(QStringLiteral("#fbbf24")),
        QColor(QStringLiteral("#a78bfa")),
        QColor(QStringLiteral("#60a5fa")),
        QColor(QStringLiteral("#f97316")),
        QColor(QStringLiteral("#e879f9")),
        QColor(QStringLiteral("#a3e635")),
        QColor(QStringLiteral("#e11d48"))
    };
    QList<QColor> colors = ocean;
    if (palette == QStringLiteral("emerald")) {
        colors = {
            QColor(QStringLiteral("#34d399")), QColor(QStringLiteral("#22d3ee")),
            QColor(QStringLiteral("#a3e635")), QColor(QStringLiteral("#60a5fa")),
            QColor(QStringLiteral("#fbbf24")), QColor(QStringLiteral("#f97316")),
            QColor(QStringLiteral("#a78bfa")), QColor(QStringLiteral("#e879f9")),
            QColor(QStringLiteral("#fb7185")), QColor(QStringLiteral("#e11d48"))};
    }
    if (palette == QStringLiteral("violet")) {
        colors = {
            QColor(QStringLiteral("#a78bfa")), QColor(QStringLiteral("#f97316")),
            QColor(QStringLiteral("#22d3ee")), QColor(QStringLiteral("#f472b6")),
            QColor(QStringLiteral("#34d399")), QColor(QStringLiteral("#fbbf24")),
            QColor(QStringLiteral("#e879f9")), QColor(QStringLiteral("#60a5fa")),
            QColor(QStringLiteral("#a3e635")), QColor(QStringLiteral("#e11d48"))};
    }
    if (palette == QStringLiteral("coral")) {
        colors = {
            QColor(QStringLiteral("#fb7185")), QColor(QStringLiteral("#f97316")),
            QColor(QStringLiteral("#a78bfa")), QColor(QStringLiteral("#22d3ee")),
            QColor(QStringLiteral("#f472b6")), QColor(QStringLiteral("#34d399")),
            QColor(QStringLiteral("#60a5fa")), QColor(QStringLiteral("#fbbf24")),
            QColor(QStringLiteral("#a3e635")), QColor(QStringLiteral("#e11d48"))};
    }
    if (palette != QStringLiteral("emerald") &&
        palette != QStringLiteral("violet") &&
        palette != QStringLiteral("coral")) {
        colors = ocean;
    }
    if (lightMode) {
        for (QColor &color : colors)
            color = color.darker(200);
    }
    return colors;
}

QIcon paletteIcon(const QString &palette, bool lightMode)
{
    QPixmap swatch(60, 16);
    swatch.fill(Qt::transparent);
    QPainter painter(&swatch);
    const QList<QColor> colors = chartColors(palette, lightMode);
    const int segmentWidth = swatch.width() / 4;
    const QColor separator(lightMode ? QStringLiteral("#ffffff")
                                     : QStringLiteral("#111827"));
    for (int index = 0; index < 4; ++index) {
        const QRect segment(index * segmentWidth, 0, segmentWidth, swatch.height());
        painter.fillRect(segment, colors.at(index));
        painter.setPen(separator);
        painter.drawRect(segment.adjusted(0, 0, -1, -1));
    }
    return QIcon(swatch);
}

QIcon expenseTrackerLogo(int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal unit = size / 64.0;
    painter.scale(unit, unit);

    QPainterPath tile;
    tile.addRoundedRect(QRectF(2, 2, 60, 60), 16, 16);
    QLinearGradient background(4, 4, 60, 60);
    background.setColorAt(0.0, QColor(QStringLiteral("#102b3c")));
    background.setColorAt(1.0, QColor(QStringLiteral("#111827")));
    painter.fillPath(tile, background);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(QStringLiteral("#22d3ee")), 3.2,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawEllipse(QRectF(13, 13, 38, 38));

    QPainterPath trend;
    trend.moveTo(21, 39);
    trend.lineTo(29, 31);
    trend.lineTo(35, 36);
    trend.lineTo(44, 25);
    painter.drawPath(trend);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#34d399")));
    painter.drawEllipse(QRectF(40, 21, 7, 7));
    return QIcon(pixmap);
}

QLabel *pageTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("pageTitle"));
    label->setTextInteractionFlags(Qt::NoTextInteraction);
    return label;
}

QFrame *metricCard(
    const QString &title,
    const QString &objectName,
    QLabel **valueLabel,
    QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("metricCard"));
    card->setProperty("metricStyle", objectName);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(8);
    auto *caption = new QLabel(title, card);
    caption->setObjectName(QStringLiteral("metricCaption"));
    *valueLabel = new QLabel(QStringLiteral("0,00 €"), card);
    (*valueLabel)->setObjectName(QStringLiteral("metricValue"));
    layout->addWidget(caption);
    layout->addWidget(*valueLabel);
    caption->setTextInteractionFlags(Qt::NoTextInteraction);
    (*valueLabel)->setTextInteractionFlags(Qt::NoTextInteraction);
    return card;
}

QTableWidget *makeTable(const QStringList &headers, QWidget *parent)
{
    auto *table = new QTableWidget(parent);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setShowGrid(false);
    table->setMinimumHeight(220);
    return table;
}

QTableWidgetItem *cell(
    const QString &text,
    Qt::Alignment alignment = Qt::AlignVCenter | Qt::AlignLeft)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(alignment);
    return item;
}

QString databasePath()
{
    const QString fileName = QStringLiteral("expense_tracker.db");
    const QString currentDatabase = QDir::current().filePath(fileName);
    const QString applicationDataDirectory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (applicationDataDirectory.isEmpty()) {
        qWarning() << "Unable to determine the application data directory";
        return currentDatabase;
    }
    if (!QDir().mkpath(applicationDataDirectory)) {
        qWarning() << "Unable to create application data directory:"
                   << applicationDataDirectory;
        return currentDatabase;
    }

    const QString stableDatabase = QDir(applicationDataDirectory).filePath(fileName);
    if (QFileInfo::exists(stableDatabase))
        return stableDatabase;

    // One-time migration from legacy working-directory locations. Copy rather than
    // move so the original remains available as a backup if anything goes wrong.
    const QDir applicationDirectory(QCoreApplication::applicationDirPath());
    const QString workspaceDatabase =
        applicationDirectory.filePath(QStringLiteral("../") + fileName);
    const QStringList legacyCandidates = {workspaceDatabase, currentDatabase};
    for (const QString &legacyPath : legacyCandidates) {
        if (!QFileInfo::exists(legacyPath))
            continue;

        const QString canonicalLegacyPath = QFileInfo(legacyPath).canonicalFilePath();
        if (canonicalLegacyPath.isEmpty())
            continue;
        if (canonicalLegacyPath == QFileInfo(stableDatabase).absoluteFilePath())
            return stableDatabase;

        if (QFile::copy(canonicalLegacyPath, stableDatabase)) {
            QFile::setPermissions(
                stableDatabase,
                QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            qInfo() << "Migrated Expense Tracker database to:" << stableDatabase;
            return stableDatabase;
        }

        // Do not silently create an empty database if migration fails; keep using
        // the existing file and report the issue in the application log.
        qWarning() << "Could not migrate legacy database from" << canonicalLegacyPath
                   << "to" << stableDatabase << "- continuing with legacy path";
        return canonicalLegacyPath;
    }

    return stableDatabase;
}

QString accountTypeLabel(const QString &type)
{
    if (type == QStringLiteral("debit_card"))
        return QCoreApplication::translate("MainWindow", "Carta di debito");
    if (type == QStringLiteral("credit_card"))
        return QCoreApplication::translate("MainWindow", "Carta di credito");
    if (type == QStringLiteral("savings"))
        return QCoreApplication::translate("MainWindow", "Risparmio");
    if (type == QStringLiteral("cash"))
        return QCoreApplication::translate("MainWindow", "Contanti");
    if (type == QStringLiteral("other"))
        return QCoreApplication::translate("MainWindow", "Altro");
    return QCoreApplication::translate("MainWindow", "Conto corrente");
}

QColor accountTypeColor(const QString &type)
{
    if (type == QStringLiteral("debit_card"))
        return QColor(QStringLiteral("#059669"));
    if (type == QStringLiteral("credit_card"))
        return QColor(QStringLiteral("#7c3aed"));
    if (type == QStringLiteral("savings"))
        return QColor(QStringLiteral("#d97706"));
    if (type == QStringLiteral("cash"))
        return QColor(QStringLiteral("#0891b2"));
    if (type == QStringLiteral("other"))
        return QColor(QStringLiteral("#64748b"));
    return QColor(QStringLiteral("#2563eb"));
}

QIcon accountTypeIcon(const QString &type)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(accountTypeColor(type));
    painter.drawEllipse(QRectF(2, 2, 12, 12));
    return QIcon(pixmap);
}

QTableWidgetItem *transactionTypeCell(bool income, bool lightMode)
{
    auto *item = cell(
        income ? QObject::tr("Entrata") : QObject::tr("Uscita"),
        Qt::AlignCenter);
    item->setForeground(QColor(income
                                   ? (lightMode ? QStringLiteral("#166534")
                                                : QStringLiteral("#6ee7b7"))
                                   : (lightMode ? QStringLiteral("#b91c1c")
                                                : QStringLiteral("#fda4af"))));
    item->setBackground(QColor(income
                                   ? (lightMode ? QStringLiteral("#dcfce7")
                                                : QStringLiteral("#12352f"))
                                   : (lightMode ? QStringLiteral("#fee2e2")
                                                : QStringLiteral("#3a202b"))));
    return item;
}

QColor incomeTextColor(bool lightMode)
{
    return QColor(lightMode ? QStringLiteral("#166534")
                            : QStringLiteral("#34d399"));
}

QColor expenseTextColor(bool lightMode)
{
    return QColor(lightMode ? QStringLiteral("#b91c1c")
                            : QStringLiteral("#fb7185"));
}
}

class ExpenseDonutChart : public QWidget
{
    Q_OBJECT

public:
    explicit ExpenseDonutChart(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(260, 300);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setAttribute(Qt::WA_OpaquePaintEvent, false);
    }

    void setData(const QList<QPair<QString, qint64>> &segments, qint64 total)
    {
        m_segments = segments;
        m_total = total;
        update();
    }

    void setAppearance(const QList<QColor> &colors, const QColor &panel, const QColor &muted,
                       const QColor &text, const QColor &track)
    {
        m_colors = colors;
        m_panelColor = panel;
        m_mutedColor = muted;
        m_textColor = text;
        m_trackColor = track;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const int diameter = qMax(160, qMin(width() - 40, height() - 34));
        const QRectF chartRect(
            (width() - diameter) / 2.0,
            (height() - diameter) / 2.0,
            diameter,
            diameter);

        painter.setPen(Qt::NoPen);
        painter.setBrush(m_trackColor);
        painter.drawEllipse(chartRect);

        if (m_total > 0) {
            qint64 accumulated = 0;
            for (int index = 0; index < m_segments.size(); ++index) {
                const qint64 value = m_segments.at(index).second;
                const int startAngle = static_cast<int>(
                    (static_cast<long double>(accumulated) / m_total) * 5760.0L);
                accumulated += value;
                const int endAngle = index == m_segments.size() - 1
                    ? 5760
                    : static_cast<int>(
                          (static_cast<long double>(accumulated) / m_total) * 5760.0L);
                const int span = endAngle - startAngle;
                if (span > 0) {
                    painter.setBrush(m_colors.at(index % m_colors.size()));
                    painter.setPen(QPen(m_panelColor, 3.0));
                    painter.drawPie(
                        chartRect,
                        90 * 16 - startAngle,
                        -span);
                }
            }

            const qreal holeInset = diameter * 0.235;
            const QRectF holeRect = chartRect.adjusted(
                holeInset, holeInset, -holeInset, -holeInset);
            painter.setBrush(m_panelColor);
            painter.drawEllipse(holeRect);

            painter.setPen(m_mutedColor);
            QFont captionFont = painter.font();
            captionFont.setPointSize(9);
            painter.setFont(captionFont);
            painter.drawText(
                holeRect.adjusted(5, holeRect.height() * 0.17, -5, 0),
                Qt::AlignHCenter | Qt::AlignTop,
                tr("TOTALE USCITE"));
            painter.setPen(m_textColor);
            QFont totalFont = painter.font();
            totalFont.setPointSize(15);
            totalFont.setWeight(QFont::Bold);
            painter.setFont(totalFont);
            painter.drawText(
                holeRect.adjusted(4, 0, -4, -holeRect.height() * 0.12),
                Qt::AlignCenter,
                QLocale().toCurrencyString(
                    static_cast<double>(m_total) / 100.0,
                    QStringLiteral("€")));
            return;
        }

        painter.setPen(m_mutedColor);
        painter.drawText(
            chartRect.adjusted(18, 18, -18, -18),
            Qt::AlignCenter | Qt::TextWordWrap,
            tr("Aggiungi una spesa per visualizzare la ripartizione."));
    }

private:
    QList<QPair<QString, qint64>> m_segments;
    QList<QColor> m_colors = chartColors(QStringLiteral("ocean"));
    QColor m_panelColor = QColor(QStringLiteral("#111c2c"));
    QColor m_mutedColor = QColor(QStringLiteral("#c0ccda"));
    QColor m_textColor = QColor(QStringLiteral("#f8fafc"));
    QColor m_trackColor = QColor(QStringLiteral("#243247"));
    qint64 m_total = 0;
};

class ExpenseTrendChart : public QWidget
{
    Q_OBJECT

public:
    explicit ExpenseTrendChart(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(180);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setData(const QStringList &months, const QList<qint64> &income,
                 const QList<qint64> &expenses, const QColor &incomeColor,
                 const QColor &expenseColor, const QColor &textColor,
                 const QColor &mutedColor, const QColor &gridColor)
    {
        m_months = months;
        m_income = income;
        m_expenses = expenses;
        m_incomeColor = incomeColor;
        m_expenseColor = expenseColor;
        m_textColor = textColor;
        m_mutedColor = mutedColor;
        m_gridColor = gridColor;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF chart = QRectF(rect()).adjusted(52, 14, -18, -36);
        const int count = qMin(m_months.size(), qMin(m_income.size(), m_expenses.size()));
        if (count == 0) {
            painter.setPen(m_mutedColor);
            painter.drawText(rect(), Qt::AlignCenter, tr("Nessun dato mensile disponibile."));
            return;
        }

        qint64 maximum = 0;
        for (int index = 0; index < count; ++index)
            maximum = qMax(maximum, qMax(m_income.at(index), m_expenses.at(index)));
        if (maximum <= 0)
            maximum = 1;

        painter.setFont(QFont(font().family(), 8));
        for (int line = 0; line <= 3; ++line) {
            const qreal y = chart.bottom() -
                chart.height() * static_cast<qreal>(line) / 3.0;
            painter.setPen(QPen(m_gridColor, 1, Qt::DashLine));
            painter.drawLine(QPointF(chart.left(), y), QPointF(chart.right(), y));
            const qint64 value = static_cast<qint64>(
                static_cast<long double>(maximum) * line / 3.0L);
            painter.setPen(m_mutedColor);
            painter.drawText(
                QRectF(0, y - 8, chart.left() - 8, 16),
                Qt::AlignRight | Qt::AlignVCenter,
                QLocale().toCurrencyString(
                    static_cast<double>(value) / 100.0, QStringLiteral("€")));
        }

        const qreal groupWidth = chart.width() / count;
        const qreal barWidth = qMin<qreal>(22, groupWidth * 0.26);
        for (int index = 0; index < count; ++index) {
            const qreal center = chart.left() + groupWidth * (index + 0.5);
            const qreal incomeHeight =
                chart.height() * static_cast<qreal>(m_income.at(index)) / maximum;
            const qreal expenseHeight =
                chart.height() * static_cast<qreal>(m_expenses.at(index)) / maximum;
            painter.setPen(Qt::NoPen);
            if (m_income.at(index) > 0) {
                painter.setBrush(m_incomeColor);
                painter.drawRoundedRect(
                    QRectF(center - barWidth - 2, chart.bottom() - incomeHeight,
                           barWidth, qMax<qreal>(4, incomeHeight)), 4, 4);
            }
            if (m_expenses.at(index) > 0) {
                painter.setBrush(m_expenseColor);
                painter.drawRoundedRect(
                    QRectF(center + 2, chart.bottom() - expenseHeight,
                           barWidth, qMax<qreal>(4, expenseHeight)), 4, 4);
            }
            painter.setPen(m_textColor);
            painter.drawText(
                QRectF(chart.left() + groupWidth * index,
                       chart.bottom() + 7, groupWidth, 20),
                Qt::AlignHCenter | Qt::AlignTop, m_months.at(index));
        }

        const qreal legendY = height() - 8;
        const qreal legendX = qMax<qreal>(chart.left(), width() / 2.0 - 95);
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_incomeColor);
        painter.drawRoundedRect(QRectF(legendX, legendY - 9, 10, 10), 3, 3);
        painter.setPen(m_textColor);
        painter.drawText(QRectF(legendX + 15, legendY - 13, 72, 18),
                         Qt::AlignLeft | Qt::AlignVCenter, tr("Entrate"));
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_expenseColor);
        painter.drawRoundedRect(QRectF(legendX + 92, legendY - 9, 10, 10), 3, 3);
        painter.setPen(m_textColor);
        painter.drawText(QRectF(legendX + 107, legendY - 13, 90, 18),
                         Qt::AlignLeft | Qt::AlignVCenter, tr("Uscite"));
    }

private:
    QStringList m_months;
    QList<qint64> m_income;
    QList<qint64> m_expenses;
    QColor m_incomeColor = QColor(QStringLiteral("#15803d"));
    QColor m_expenseColor = QColor(QStringLiteral("#b91c1c"));
    QColor m_textColor = QColor(QStringLiteral("#172033"));
    QColor m_mutedColor = QColor(QStringLiteral("#475569"));
    QColor m_gridColor = QColor(QStringLiteral("#cbd5e1"));
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_manager(databasePath())
{
    QStyle *baseStyle = QStyleFactory::create(QStringLiteral("Fusion"));
    if (!baseStyle) {
        qWarning() << "Unable to create Fusion style for custom control arrows";
    } else {
        QApplication::setStyle(new ChevronStyle(baseStyle));
    }

    QSettings settings;
    m_languageKey = settings.value(
        QStringLiteral("appearance/language"), QStringLiteral("it")).toString();
    if (m_languageKey != QStringLiteral("it") &&
        m_languageKey != QStringLiteral("en")) {
        m_languageKey = QStringLiteral("it");
    }

    const QString translationFile = m_languageKey == QStringLiteral("en")
        ? QStringLiteral("ExpenseTracker_en.qm")
        : QStringLiteral("ExpenseTracker_it.qm");
    const QString translationPath = QDir(
        QCoreApplication::applicationDirPath()).filePath(
            QStringLiteral("translations/") + translationFile);
    const bool translatorLoaded =
        m_translator.load(translationPath) &&
        QCoreApplication::installTranslator(&m_translator);
    if (!translatorLoaded) {
        qWarning().noquote() << "Unable to load translation catalog:" << translationPath;
        if (m_languageKey == QStringLiteral("en")) {
            m_languageKey = QStringLiteral("it");
            const QString italianPath = QDir(
                QCoreApplication::applicationDirPath()).filePath(
                    QStringLiteral("translations/ExpenseTracker_it.qm"));
            if (!m_translator.load(italianPath) ||
                !QCoreApplication::installTranslator(&m_translator)) {
                qWarning().noquote()
                    << "Unable to load Italian fallback catalog:" << italianPath;
            }
            settings.setValue(QStringLiteral("appearance/language"), m_languageKey);
            settings.sync();
        }
    }

    QLocale::setDefault(m_languageKey == QStringLiteral("en")
                            ? QLocale(QLocale::English, QLocale::UnitedStates)
                            : QLocale(QLocale::Italian, QLocale::Italy));

    QFontDatabase fontDatabase;
    const QStringList installedFonts = fontDatabase.families();
    m_fontFamily = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    const QStringList preferredFonts = {
        QStringLiteral("Segoe UI Variable Text"),
        QStringLiteral("Segoe UI Variable"),
        QStringLiteral("Segoe UI")
    };
    for (const QString &preferredFont : preferredFonts) {
        if (installedFonts.contains(preferredFont, Qt::CaseInsensitive)) {
            m_fontFamily = preferredFont;
            break;
        }
    }
    QFont applicationFont(m_fontFamily);
    applicationFont.setPointSize(10);
    applicationFont.setHintingPreference(QFont::PreferFullHinting);
    setFont(applicationFont);

    m_themeModeKey = settings.value(
        QStringLiteral("appearance/mode"), QStringLiteral("dark")).toString();
    m_themePaletteKey = settings.value(
        QStringLiteral("appearance/palette"), QStringLiteral("ocean")).toString();
    if (m_themeModeKey != QStringLiteral("light") &&
        m_themeModeKey != QStringLiteral("dark")) {
        m_themeModeKey = QStringLiteral("dark");
    }
    if (m_themePaletteKey != QStringLiteral("ocean") &&
        m_themePaletteKey != QStringLiteral("emerald") &&
        m_themePaletteKey != QStringLiteral("violet") &&
        m_themePaletteKey != QStringLiteral("coral")) {
        m_themePaletteKey = QStringLiteral("ocean");
    }

    setWindowTitle(tr("Expense Tracker"));
    setWindowIcon(expenseTrackerLogo(64));
    resize(1440, 900);
    setMinimumSize(900, 620);
    setWindowState(windowState() | Qt::WindowMaximized);
    m_baseStyleSheet = QStringLiteral(R"(
        QMainWindow, QWidget {
            background: @background@;
            color: @foreground@;
            font-family: "%1";
            font-size: 10pt;
        }
        QLabel {
            background: transparent;
            color: @foreground@;
            border: none;
        }
        QFrame#sidebar QLabel { background: transparent; }
        QFrame#sidebar { background: @sidebarBackground@; border: none; }
        QLabel#brand {
            color: @sidebarBrand@;
            font-size: 14pt;
            font-weight: 800;
            padding: 0;
        }
        QPushButton#navButton {
            color: @sidebarText@;
            background: @navBackground@;
            text-align: left;
            border: none;
            border-radius: 9px;
            padding: 14px 16px;
            min-height: 24px;
            font-weight: 650;
        }
        QPushButton#navButton:hover {
            background: @navHoverBackground@;
            color: @navHoverText@;
        }
        QPushButton#navButton:checked {
            background: @navSelectedBackground@;
            color: @navSelectedText@;
            border: none;
        }
        QPushButton#primaryButton {
            background: @accent@;
            color: @accentForeground@;
            border: 1px solid @accentBright@;
            border-radius: 8px;
            padding: 10px 16px;
            min-height: 20px;
            font-weight: 700;
        }
        QPushButton#primaryButton:hover { background: @accentHover@; }
        QPushButton#secondaryButton {
            background: @buttonBackground@;
            color: @foreground@;
            border: 1px solid @border@;
            border-radius: 8px;
            padding: 9px 14px;
            min-height: 20px;
            font-weight: 650;
        }
        QPushButton#secondaryButton:hover {
            background: @buttonHoverBackground@;
            border-color: @accentBright@;
            color: @foreground@;
        }
        QPushButton#paletteOption {
            background: @panelBackground@;
            color: @foreground@;
            border: 1px solid @border@;
            border-radius: 9px;
            padding: 12px 14px;
            text-align: left;
            font-weight: 650;
        }
        QPushButton#paletteOption:hover { border-color: @accentBright@; }
        QPushButton#paletteOption:checked {
            background: @navSelectedBackground@;
            color: @navSelectedText@;
            border: 1px solid @border@;
        }
        QLabel#appearanceLegendMain, QLabel#appearanceLegendMuted,
        QLabel#appearanceLegendIncome, QLabel#appearanceLegendExpense,
        QLabel#appearanceLegendAccent {
            border-radius: 6px;
            padding: 4px 8px;
            font-weight: 800;
        }
        QLabel#appearanceLegendMain {
            color: @legendMainText@;
            background: @legendMainBackground@;
        }
        QLabel#appearanceLegendMuted {
            color: @legendMutedText@;
            background: @legendMutedBackground@;
        }
        QLabel#appearanceLegendIncome {
            color: @legendIncomeText@;
            background: @legendIncomeBackground@;
        }
        QLabel#appearanceLegendExpense {
            color: @legendExpenseText@;
            background: @legendExpenseBackground@;
        }
        QLabel#appearanceLegendAccent {
            color: @navSelectedText@;
            background: @navSelectedBackground@;
            border-color: @navSelectedBorder@;
        }
        QPushButton:disabled { color: @disabledForeground@; background: @disabledBackground@; border-color: @disabledBorder@; }
        QPushButton:focus { outline: none; }
        QDateEdit:focus, QDoubleSpinBox:focus, QLineEdit:focus, QComboBox:focus {
            border: 1px solid @border@;
            outline: none;
        }
        QAbstractItemView::item:focus {
            border: none;
            outline: none;
        }
        QComboBox QAbstractItemView::item:focus {
            border: none;
            outline: none;
        }
        QCheckBox { color: @foreground@; spacing: 7px; font-weight: 650; }
        QCheckBox::indicator {
            width: 17px; height: 17px;
            border: 1px solid @border@;
            border-radius: 4px;
            background: @inputBackground@;
        }
        QCheckBox::indicator:checked {
            background: @accent@;
            border-color: @accentBright@;
        }
        QLabel#pageTitle { font-size: 25pt; font-weight: 800; color: @foreground@; }
        QLabel#subTitle { color: @muted@; font-size: 10pt; }
        QFrame#metricCard, QFrame#panel {
            background: @panelBackground@;
            border: 1px solid @border@;
            border-radius: 12px;
        }
        QFrame#metricCard { border-left: 4px solid #22d3ee; }
        QFrame#metricCard[metricStyle="balance"] { border-left-color: #22d3ee; }
        QFrame#metricCard[metricStyle="income"] { border-left-color: #34d399; }
        QFrame#metricCard[metricStyle="expenses"] { border-left-color: #fb7185; }
        QFrame#metricCard[metricStyle="month"] { border-left-color: #fbbf24; }
        QFrame#transactionSummaryCard {
            background: @panelBackground@;
            border: 1px solid @border@;
            border-radius: 9px;
        }
        QLabel#transactionSummaryCaption { color: @muted@; font-size: 9pt; font-weight: 600; }
        QLabel#transactionSummaryValue { color: @foreground@; font-size: 12pt; font-weight: 750; }
        QLabel#metricCaption { color: @muted@; font-size: 10pt; font-weight: 600; }
        QLabel#metricValue { color: @foreground@; font-size: 20pt; font-weight: 800; }
        QTableWidget {
            background: @panelBackground@;
            alternate-background-color: @alternateBackground@;
            color: @foreground@;
            border: 1px solid @border@;
            border-radius: 10px;
            padding: 5px;
            selection-background-color: @selectionBackground@;
            selection-color: @selectionForeground@;
            gridline-color: @border@;
        }
        QTableWidget::item { padding: 9px 8px; border: none; }
        QTableWidget::item:hover { background: @itemHover@; }
        QTableWidget#categoriesTable::item:hover { background: transparent; }
        QTableWidget#analysisTable::item:hover { background: transparent; }
        QHeaderView::section {
            background: @headerBackground@;
            color: @foreground@;
            border: none;
            border-bottom: 2px solid @border@;
            padding: 12px 8px;
            font-weight: 700;
        }
        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox {
            background: @inputBackground@;
            color: @foreground@;
            border: 1px solid @border@;
            border-radius: 7px;
            padding: 8px;
            selection-background-color: @selectionBackground@;
            selection-color: @selectionForeground@;
        }
        QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: right;
            width: 30px;
            height: 18px;
            border: none;
            background: transparent;
        }
        QDoubleSpinBox::up-button {
            subcontrol-position: top right;
            border-top-right-radius: 6px;
        }
        QDoubleSpinBox::down-button {
            subcontrol-position: bottom right;
            border-bottom-right-radius: 6px;
        }
        QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
            background: @itemHover@;
        }
        QDoubleSpinBox { padding-right: 38px; }
        QComboBox { padding-right: 34px; }
        QComboBox QAbstractItemView {
            background: @panelBackground@;
            color: @foreground@;
            selection-background-color: @selectionBackground@;
            selection-color: @selectionForeground@;
            outline: none;
            border: none;
            border-radius: 8px;
            padding: 6px;
            show-decoration-selected: 1;
        }
        QComboBox QAbstractItemView::item {
            min-height: 32px;
            padding: 8px 12px;
            border: none;
            border-radius: 6px;
        }
        QComboBox QAbstractItemView::item:selected {
            background: @selectionBackground@;
            color: @selectionForeground@;
            border: none;
        }
        QComboBox QAbstractItemView::item:hover {
            background: @itemHover@;
            color: @foreground@;
        }
        QComboBox::drop-down {
            width: 32px;
            border: none;
            background: transparent;
            border-top-right-radius: 7px;
            border-bottom-right-radius: 7px;
        }
        QDateEdit::drop-down, QDateTimeEdit::drop-down {
            width: 32px;
            border: none;
            background: transparent;
            border-top-right-radius: 7px;
            border-bottom-right-radius: 7px;
        }
        QCalendarWidget QWidget { background: @panelBackground@; color: @foreground@; }
        QCalendarWidget QToolButton { color: @foreground@; background: @buttonBackground@; }
        QCalendarWidget QAbstractItemView:enabled {
            color: @foreground@;
            selection-background-color: @selectionBackground@;
            selection-color: @selectionForeground@;
        }
        QLabel#sidebarFooter { color: @sidebarMuted@; padding: 8px; }
        QStatusBar {
            background: @statusBackground@;
            color: @statusForeground@;
            border-top: 1px solid @border@;
        }
        QStatusBar::item { border: none; }
        QToolTip { background: @tooltipBackground@; color: @tooltipForeground@; border: 1px solid @border@; padding: 5px; }
        QScrollBar:vertical { background: @background@; width: 12px; margin: 2px; }
        QScrollBar::handle:vertical { background: @scrollHandle@; min-height: 24px; border-radius: 5px; }
        QScrollBar::handle:vertical:hover { background: @accentBright@; }
    )").arg(m_fontFamily);
    applyAppearance();

    if (!initializeData()) {
        QTimer::singleShot(0, this, [this] { close(); });
        return;
    }
    buildUi();
    applyAppearance();
    refreshAll();
}

MainWindow::~MainWindow()
{
    QCoreApplication::removeTranslator(&m_translator);
}

void MainWindow::applyAppearance()
{
    QString background = QStringLiteral("#0b1220");
    QString sidebar = QStringLiteral("#080e19");
    QString sidebarBrand = QStringLiteral("#ffffff");
    QString sidebarText = QStringLiteral("#cbd5e1");
    QString sidebarMuted = QStringLiteral("#b7c4d4");
    QString navBackground = QStringLiteral("transparent");
    QString navHoverBackground = QStringLiteral("#172438");
    QString navHoverText = QStringLiteral("#ffffff");
    QString foreground = QStringLiteral("#f1f5f9");
    QString muted = QStringLiteral("#c0ccda");
    QString border = QStringLiteral("#35445a");
    QString panel = QStringLiteral("#111c2c");
    QString alternate = QStringLiteral("#152235");
    QString header = QStringLiteral("#1b2a3e");
    QString input = QStringLiteral("#0d1726");
    QString button = QStringLiteral("#172438");
    QString buttonHover = QStringLiteral("#22334b");
    QString disabledForeground = QStringLiteral("#8795a8");
    QString disabledBackground = QStringLiteral("#182333");
    QString disabledBorder = QStringLiteral("#344256");
    QString track = QStringLiteral("#243247");
    QString statusBackground = QStringLiteral("#080e19");
    QString statusForeground = QStringLiteral("#cbd5e1");
    QString legendMainBackground = QStringLiteral("#172438");
    QString legendMutedBackground = QStringLiteral("#172438");
    QString legendMainText = QStringLiteral("#f8fafc");
    QString legendMutedText = QStringLiteral("#c0ccda");
    QString legendIncomeText = QStringLiteral("#6ee7b7");
    QString legendIncomeBackground = QStringLiteral("#12352f");
    QString legendExpenseText = QStringLiteral("#fda4af");
    QString legendExpenseBackground = QStringLiteral("#3a202b");

    if (m_themeModeKey == QStringLiteral("light")) {
        background = QStringLiteral("#ffffff");
        sidebar = QStringLiteral("#f8fafc");
        sidebarBrand = QStringLiteral("#0f172a");
        sidebarText = QStringLiteral("#334155");
        sidebarMuted = QStringLiteral("#475569");
        navHoverBackground = QStringLiteral("#e2e8f0");
        navHoverText = QStringLiteral("#0f172a");
        foreground = QStringLiteral("#172033");
        muted = QStringLiteral("#475569");
        border = QStringLiteral("#cbd5e1");
        panel = QStringLiteral("#ffffff");
        alternate = QStringLiteral("#f1f5f9");
        header = QStringLiteral("#e2e8f0");
        input = QStringLiteral("#ffffff");
        button = QStringLiteral("#f1f5f9");
        buttonHover = QStringLiteral("#e2e8f0");
        disabledForeground = QStringLiteral("#526176");
        disabledBackground = QStringLiteral("#f1f5f9");
        disabledBorder = QStringLiteral("#cbd5e1");
        track = QStringLiteral("#e2e8f0");
        statusBackground = QStringLiteral("#f8fafc");
        statusForeground = QStringLiteral("#334155");
        legendMainBackground = QStringLiteral("#ffffff");
        legendMutedBackground = QStringLiteral("#f8fafc");
        legendMainText = QStringLiteral("#172033");
        legendMutedText = QStringLiteral("#475569");
        legendIncomeText = QStringLiteral("#166534");
        legendIncomeBackground = QStringLiteral("#dcfce7");
        legendExpenseText = QStringLiteral("#b91c1c");
        legendExpenseBackground = QStringLiteral("#fee2e2");
    }

    QString accent = QStringLiteral("#0891b2");
    QString accentBright = QStringLiteral("#22d3ee");
    QString accentHover = QStringLiteral("#0e7490");
    QString accentSelected = QStringLiteral("#155e75");
    QString accentText = QStringLiteral("#67e8f9");
    if (m_themePaletteKey == QStringLiteral("emerald")) {
        accent = QStringLiteral("#059669");
        accentBright = QStringLiteral("#6ee7b7");
        accentHover = QStringLiteral("#047857");
        accentSelected = QStringLiteral("#065f46");
        accentText = m_themeModeKey == QStringLiteral("light")
            ? QStringLiteral("#047857")
            : QStringLiteral("#6ee7b7");
    } else if (m_themePaletteKey == QStringLiteral("violet")) {
        accent = QStringLiteral("#7c3aed");
        accentBright = QStringLiteral("#c4b5fd");
        accentHover = QStringLiteral("#6d28d9");
        accentSelected = QStringLiteral("#5b21b6");
        accentText = m_themeModeKey == QStringLiteral("light")
            ? QStringLiteral("#6d28d9")
            : QStringLiteral("#c4b5fd");
    } else if (m_themePaletteKey == QStringLiteral("coral")) {
        accent = QStringLiteral("#e11d48");
        accentBright = QStringLiteral("#fda4af");
        accentHover = QStringLiteral("#be123c");
        accentSelected = QStringLiteral("#9f1239");
        accentText = m_themeModeKey == QStringLiteral("light")
            ? QStringLiteral("#be123c")
            : QStringLiteral("#fda4af");
    }
    if (m_themePaletteKey == QStringLiteral("ocean") &&
        m_themeModeKey == QStringLiteral("light")) {
        accentText = QStringLiteral("#0e7490");
    }
    if (m_themeModeKey == QStringLiteral("light")) {
        if (m_themePaletteKey == QStringLiteral("emerald")) {
            accentBright = QStringLiteral("#047857");
        } else if (m_themePaletteKey == QStringLiteral("violet")) {
            accentBright = QStringLiteral("#6d28d9");
        } else if (m_themePaletteKey == QStringLiteral("coral")) {
            accentBright = QStringLiteral("#be123c");
        } else {
            accentBright = QStringLiteral("#0e7490");
        }
    }
    QString selectionBackground = accentSelected;
    QString selectionForeground = QStringLiteral("#ffffff");
    QString navSelectedBackground = accentSelected;
    QString navSelectedText = accentBright;
    QString navSelectedBorder = accentBright;
    if (m_themeModeKey == QStringLiteral("light")) {
        selectionForeground = QStringLiteral("#172033");
        navSelectedText = accentHover;
        navSelectedBorder = accentHover;
        if (m_themePaletteKey == QStringLiteral("emerald")) {
            selectionBackground = QStringLiteral("#d1fae5");
        } else if (m_themePaletteKey == QStringLiteral("violet")) {
            selectionBackground = QStringLiteral("#ede9fe");
        } else if (m_themePaletteKey == QStringLiteral("coral")) {
            selectionBackground = QStringLiteral("#ffe4e6");
        } else {
            selectionBackground = QStringLiteral("#cffafe");
        }
        navSelectedBackground = selectionBackground;
    }

    QString style = m_baseStyleSheet;
    const QList<QPair<QString, QString>> replacements = {
        {QStringLiteral("#34d399"), m_themeModeKey == QStringLiteral("light")
                                       ? QStringLiteral("#15803d")
                                       : QStringLiteral("#34d399")},
        {QStringLiteral("#fb7185"), m_themeModeKey == QStringLiteral("light")
                                       ? QStringLiteral("#b91c1c")
                                       : QStringLiteral("#fb7185")},
        {QStringLiteral("#fbbf24"), m_themeModeKey == QStringLiteral("light")
                                       ? QStringLiteral("#a16207")
                                       : QStringLiteral("#fbbf24")},
        {QStringLiteral("#0891b2"), accent},
        {QStringLiteral("#22d3ee"), accentBright},
        {QStringLiteral("#0e7490"), accentHover},
        {QStringLiteral("#155e75"), accentSelected},
        {QStringLiteral("#67e8f9"), accentText},
        {QStringLiteral("#102b3c"), m_themeModeKey == QStringLiteral("light")
                                         ? QStringLiteral("#e0f2fe")
                                         : accentSelected}
    };
    for (const auto &replacement : replacements)
        style.replace(replacement.first, replacement.second);
    const auto linearChannel = [](double channel) {
        return channel <= 0.04045
            ? channel / 12.92
            : std::pow((channel + 0.055) / 1.055, 2.4);
    };
    const QColor accentColor(accent);
    const double accentLuminance =
        0.2126 * linearChannel(accentColor.redF()) +
        0.7152 * linearChannel(accentColor.greenF()) +
        0.0722 * linearChannel(accentColor.blueF());
    const QString accentForeground = accentLuminance > 0.179
        ? QStringLiteral("#0b1220")
        : QStringLiteral("#ffffff");
    style.replace(QStringLiteral("@accentForeground@"), accentForeground);
    style.replace(QStringLiteral("@tooltipBackground@"),
                  m_themeModeKey == QStringLiteral("light")
                      ? QStringLiteral("#ffffff")
                      : QStringLiteral("#f8fafc"));
    style.replace(QStringLiteral("@tooltipForeground@"),
                  m_themeModeKey == QStringLiteral("light")
                      ? QStringLiteral("#172033")
                      : QStringLiteral("#0b1220"));
    const QList<QPair<QString, QString>> themeTokens = {
        {QStringLiteral("@background@"), background},
        {QStringLiteral("@foreground@"), foreground},
        {QStringLiteral("@muted@"), muted},
        {QStringLiteral("@panelBackground@"), panel},
        {QStringLiteral("@alternateBackground@"), alternate},
        {QStringLiteral("@headerBackground@"), header},
        {QStringLiteral("@inputBackground@"), input},
        {QStringLiteral("@buttonBackground@"), button},
        {QStringLiteral("@buttonHoverBackground@"), buttonHover},
        {QStringLiteral("@disabledForeground@"), disabledForeground},
        {QStringLiteral("@disabledBackground@"), disabledBackground},
        {QStringLiteral("@disabledBorder@"), disabledBorder},
        {QStringLiteral("@scrollHandle@"),
         m_themeModeKey == QStringLiteral("light")
             ? QStringLiteral("#94a3b8")
             : QStringLiteral("#50617a")},
        {QStringLiteral("@accent@"), accent},
        {QStringLiteral("@sidebarBackground@"), sidebar},
        {QStringLiteral("@sidebarBrand@"), sidebarBrand},
        {QStringLiteral("@sidebarText@"), sidebarText},
        {QStringLiteral("@sidebarMuted@"), sidebarMuted},
        {QStringLiteral("@navBackground@"), navBackground},
        {QStringLiteral("@navHoverBackground@"), navHoverBackground},
        {QStringLiteral("@navHoverText@"), navHoverText},
        {QStringLiteral("@navSelectedBackground@"), navSelectedBackground},
        {QStringLiteral("@navSelectedText@"), navSelectedText},
        {QStringLiteral("@navSelectedBorder@"), navSelectedBorder},
        {QStringLiteral("@statusBackground@"), statusBackground},
        {QStringLiteral("@statusForeground@"), statusForeground},
        {QStringLiteral("@legendMainBackground@"), legendMainBackground},
        {QStringLiteral("@legendMutedBackground@"), legendMutedBackground},
        {QStringLiteral("@legendMainText@"), legendMainText},
        {QStringLiteral("@legendMutedText@"), legendMutedText},
        {QStringLiteral("@legendIncomeText@"), legendIncomeText},
        {QStringLiteral("@legendIncomeBackground@"), legendIncomeBackground},
        {QStringLiteral("@legendExpenseText@"), legendExpenseText},
        {QStringLiteral("@legendExpenseBackground@"), legendExpenseBackground},
        {QStringLiteral("@itemHover@"),
         m_themeModeKey == QStringLiteral("light")
             ? QStringLiteral("#e2e8f0")
             : QStringLiteral("#1e344a")},
        {QStringLiteral("@border@"), border},
        {QStringLiteral("@foreground@"), foreground},
        {QStringLiteral("@accentBright@"), accentBright},
        {QStringLiteral("@accentHover@"), accentHover},
        {QStringLiteral("@selectionBackground@"), selectionBackground},
        {QStringLiteral("@selectionForeground@"), selectionForeground}
    };
    for (const auto &token : themeTokens)
        style.replace(token.first, token.second);
    setStyleSheet(style);

    for (QFrame *card : m_transactionSummaryCards) {
        const QString accentProperty = m_themeModeKey == QStringLiteral("light")
            ? QStringLiteral("summaryAccentLight")
            : QStringLiteral("summaryAccentDark");
        const QString summaryAccent =
            card->property(accentProperty.toUtf8().constData()).toString();
        card->setStyleSheet(QStringLiteral(
            "QFrame#transactionSummaryCard { background:%1; border:1px solid %2; "
            "border-left:3px solid %3; border-radius:9px; }")
            .arg(panel, border, summaryAccent));
    }

    for (QPushButton *option : m_paletteOptions) {
        const QString paletteKey = option->property("paletteKey").toString();
        option->setChecked(paletteKey == m_themePaletteKey);
        option->setIcon(
            paletteIcon(paletteKey, m_themeModeKey == QStringLiteral("light")));
    }
    if (m_appearancePreview) {
        m_appearancePreview->setStyleSheet(QStringLiteral(
            "background:%1; border:none; border-radius:8px; padding:14px; "
            "color:%2; font-weight:700;")
            .arg(accent, accentForeground));
    }
    if (m_expenseDonut) {
        m_expenseDonut->setAppearance(
            chartColors(m_themePaletteKey, m_themeModeKey == QStringLiteral("light")),
            QColor(panel),
            QColor(muted),
            QColor(m_themeModeKey == QStringLiteral("light")
                       ? QStringLiteral("#172033")
                       : QStringLiteral("#f8fafc")),
            QColor(track));
    }
    if (m_recentTable)
        refreshDashboard();
    if (m_transactionsTable)
        refreshTransactions();
    if (m_categoriesTable)
        refreshCategories();
    if (m_analysisTable)
        refreshAnalysis();
}

void MainWindow::saveAppearance()
{
    QSettings settings;
    settings.setValue(QStringLiteral("appearance/mode"), m_themeModeKey);
    settings.setValue(QStringLiteral("appearance/palette"), m_themePaletteKey);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        showError(tr("Impossibile salvare le preferenze di aspetto."));
        return;
    }
    statusBar()->showMessage(tr("Aspetto salvato"), 2500);
}

bool MainWindow::initializeData()
{
    if (!m_manager.open()) {
        showError(tr("Impossibile aprire il database: %1").arg(m_manager.lastError()));
        return false;
    }

    QList<QVariantMap> accounts;
    if (!m_manager.getAccounts(accounts)) {
        showError(tr("Impossibile leggere gli account: %1").arg(m_manager.lastError()));
        return false;
    }
    if (accounts.isEmpty()) {
        const QString hash = QString::fromLatin1(
            QCryptographicHash::hash(QByteArrayLiteral("expense-tracker-local-account"),
                                     QCryptographicHash::Sha256).toHex());
        int firstAccountId = 0;
        if (!m_manager.addAccount(tr("Personale"), hash,
                                  QDate::currentDate().toString(Qt::ISODate), 0,
                                  &firstAccountId)) {
            showError(tr("Impossibile creare l'account iniziale: %1").arg(m_manager.lastError()));
            return false;
        }
        accounts.append({
            {QStringLiteral("ID_Account"), firstAccountId},
            {QStringLiteral("Name"), tr("Personale")},
            {QStringLiteral("Type"), QStringLiteral("checking")},
            {QStringLiteral("Balance"), 0}
        });
    }
    QSettings settings;
    const int savedAccountId = settings.value(
        QStringLiteral("accounts/activeId"), 0).toInt();
    const auto savedAccount = std::find_if(
        accounts.cbegin(), accounts.cend(), [savedAccountId](const QVariantMap &account) {
            return account.value(QStringLiteral("ID_Account")).toInt() == savedAccountId;
        });
    m_accountId = savedAccount == accounts.cend()
        ? accounts.first().value(QStringLiteral("ID_Account")).toInt()
        : savedAccountId;

    if (!m_manager.getCategories(m_categories)) {
        showError(tr("Impossibile caricare le categorie: %1").arg(m_manager.lastError()));
        return false;
    }
    if (m_categories.isEmpty()) {
        const QList<QPair<QString, QString>> defaults = {
            {tr("Stipendio"), QStringLiteral("in")},
            {tr("Regali"), QStringLiteral("in")},
            {tr("Altro"), QStringLiteral("in")},
            {tr("Alimentari"), QStringLiteral("out")},
            {tr("Trasporti"), QStringLiteral("out")},
            {tr("Casa"), QStringLiteral("out")},
            {tr("Svago"), QStringLiteral("out")},
            {tr("Salute"), QStringLiteral("out")},
            {tr("Altro"), QStringLiteral("out")}
        };
        for (const auto &entry : defaults) {
            if (!m_manager.addCategory(entry.first, entry.second)) {
                showError(tr("Impossibile creare le categorie iniziali: %1").arg(m_manager.lastError()));
                return false;
            }
        }
        if (!m_manager.getCategories(m_categories)) {
            showError(tr("Impossibile caricare le categorie: %1").arg(m_manager.lastError()));
            return false;
        }
    }
    return true;
}

void MainWindow::buildUi()
{
    auto *root = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *sidebar = new QFrame(root);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(232);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(18, 24, 18, 22);
    sideLayout->setSpacing(11);
    auto *brandRow = new QHBoxLayout;
    brandRow->setContentsMargins(8, 12, 8, 26);
    brandRow->setSpacing(9);
    auto *brandIcon = new QLabel(sidebar);
    brandIcon->setPixmap(expenseTrackerLogo(32).pixmap(QSize(32, 32)));
    brandIcon->setFixedSize(32, 32);
    auto *brand = new QLabel(QStringLiteral("Expense Tracker"), sidebar);
    brand->setObjectName(QStringLiteral("brand"));
    brand->setContentsMargins(0, 0, 0, 0);
    brandRow->addWidget(brandIcon);
    brandRow->addWidget(brand);
    brandRow->addStretch();
    sideLayout->addLayout(brandRow);

    m_accountSelector = new QComboBox(sidebar);
    polishComboPopup(m_accountSelector);
    m_accountSelector->setMinimumHeight(40);
    sideLayout->addWidget(m_accountSelector);
    auto *manageAccountsButton = new QPushButton(tr("Gestisci conti"), sidebar);
    manageAccountsButton->setObjectName(QStringLiteral("secondaryButton"));
    sideLayout->addWidget(manageAccountsButton);
    auto *transferButton = new QPushButton(tr("⇄ Trasferisci"), sidebar);
    transferButton->setObjectName(QStringLiteral("secondaryButton"));
    sideLayout->addWidget(transferButton);
    connect(manageAccountsButton, &QPushButton::clicked,
            this, &MainWindow::openAccountManagement);
    connect(transferButton, &QPushButton::clicked, this, &MainWindow::openTransfers);
    connect(m_accountSelector, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0)
            return;
        const int selectedAccountId = m_accountSelector->itemData(index).toInt();
        if (selectedAccountId == m_accountId)
            return;
        m_accountId = selectedAccountId;
        QSettings settings;
        settings.setValue(QStringLiteral("accounts/activeId"), m_accountId);
        settings.sync();
        if (settings.status() != QSettings::NoError)
            showError(tr("Impossibile salvare il conto attivo."));
        m_transactionPage = 0;
        refreshAll();
    });
    refreshAccounts();

    m_pages = new QStackedWidget(root);
    sideLayout->addSpacing(12);
    auto *navGroup = new QButtonGroup(sidebar);
    navGroup->setExclusive(true);
    const QStringList navLabels = {
        tr("Panoramica"), tr("Transazioni"), tr("Analisi"), tr("Categorie"), tr("Aspetto")};
    for (int i = 0; i < navLabels.size(); ++i) {
        auto *button = new QPushButton(navLabels[i], sidebar);
        button->setObjectName(QStringLiteral("navButton"));
        button->setCursor(Qt::PointingHandCursor);
        button->setCheckable(true);
        button->setChecked(i == 0);
        button->setShortcut(QKeySequence(
            QStringLiteral("Ctrl+%1").arg(i + 1)));
        button->setToolTip(
            tr("Vai a %1 (Ctrl+%2)").arg(navLabels[i]).arg(i + 1));
        navGroup->addButton(button, i);
        sideLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, i] {
            m_pages->setCurrentIndex(i);
        });
    }
    connect(m_pages, &QStackedWidget::currentChanged, this, [navGroup](int index) {
        if (QAbstractButton *button = navGroup->button(index))
            button->setChecked(true);
    });
    sideLayout->addStretch();
    auto *tutorialButton = new QPushButton(tr("◉  Tutorial"), sidebar);
    tutorialButton->setObjectName(QStringLiteral("secondaryButton"));
    tutorialButton->setMinimumHeight(40);
    tutorialButton->setCursor(Qt::PointingHandCursor);
    connect(tutorialButton, &QPushButton::clicked,
            this, &MainWindow::showTutorial);
    sideLayout->addWidget(tutorialButton);
    auto *footer = new QLabel(tr("Gestione personale\nDati salvati in locale"), sidebar);
    footer->setObjectName(QStringLiteral("sidebarFooter"));
    footer->setTextInteractionFlags(Qt::NoTextInteraction);
    sideLayout->addWidget(footer);

    m_pages->addWidget(buildDashboardPage());
    m_pages->addWidget(buildTransactionsPage());
    m_pages->addWidget(buildAnalysisPage());
    m_pages->addWidget(buildCategoriesPage());
    m_pages->addWidget(buildAppearancePage());
    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(m_pages, 1);
    setCentralWidget(root);
    auto *newTransactionShortcut = new QShortcut(QKeySequence::New, this);
    connect(newTransactionShortcut, &QShortcut::activated,
            this, &MainWindow::addTransaction);
    statusBar()->showMessage(tr("Database locale pronto"));
}

void MainWindow::showTutorial()
{
    if (findChild<QDialog *>(QStringLiteral("guidedTutorial")))
        return;

    const QStringList titles = {
        tr("La tua panoramica"),
        tr("Scegli il conto"),
        tr("Registra e trova i movimenti"),
        tr("Organizza le categorie"),
        tr("Esplora le tue analisi"),
        tr("Personalizza l'app")};
    const QStringList descriptions = {
        tr("Controlla saldo, entrate e uscite del mese, l'andamento degli ultimi sei mesi e i movimenti recenti del conto attivo."),
        tr("Seleziona un conto dalla barra laterale o fai doppio clic sul conto desiderato nella tabella della dashboard."),
        tr("Aggiungi una transazione con importo, categoria e note facoltative. Usa la ricerca per trovare anche il testo delle note."),
        tr("Consulta le categorie di entrata e uscita e aggiungine una quando ti serve."),
        tr("Confronta entrate e uscite, cambia periodo e consulta grafici e categorie per capire dove vanno i tuoi soldi."),
        tr("Scegli tema, colori e lingua nella pagina Aspetto. I dati restano salvati sul tuo dispositivo.")};

    const QList<int> pageIndexes = {0, 0, 1, 3, 2, 4};
    auto *dialog = new QDialog(this, Qt::Tool);
    dialog->setObjectName(QStringLiteral("guidedTutorial"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Tutorial Expense Tracker"));
    dialog->setModal(false);
    dialog->setMinimumWidth(350);
    dialog->resize(400, 280);
    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(22, 18, 22, 18);
    layout->setSpacing(10);

    auto *stepCounter = new QLabel(dialog);
    stepCounter->setObjectName(QStringLiteral("subTitle"));
    auto *heading = new QLabel(dialog);
    heading->setStyleSheet(QStringLiteral("font-size:17pt;font-weight:750;"));
    auto *description = new QLabel(dialog);
    description->setWordWrap(true);
    description->setMinimumHeight(100);
    description->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    auto *buttons = new QHBoxLayout;
    auto *previous = new QPushButton(tr("← Indietro"), dialog);
    previous->setObjectName(QStringLiteral("secondaryButton"));
    auto *next = new QPushButton(tr("Avanti →"), dialog);
    next->setObjectName(QStringLiteral("primaryButton"));
    auto *close = new QPushButton(tr("Chiudi"), dialog);
    close->setObjectName(QStringLiteral("secondaryButton"));
    buttons->addWidget(previous);
    buttons->addStretch();
    buttons->addWidget(next);
    buttons->addWidget(close);

    layout->addWidget(stepCounter, 0, Qt::AlignRight);
    layout->addWidget(heading);
    layout->addWidget(description);
    layout->addLayout(buttons);

    auto currentStep = std::make_shared<int>(0);
    auto updateStep = [this, dialog, stepCounter, heading, description,
                       previous, next, titles, descriptions, pageIndexes,
                       currentStep] {
        stepCounter->setText(tr("Passaggio %1 di %2")
                                 .arg(*currentStep + 1)
                                 .arg(titles.size()));
        heading->setText(titles.at(*currentStep));
        description->setText(descriptions.at(*currentStep));
        previous->setEnabled(*currentStep > 0);
        next->setText(*currentStep + 1 == titles.size()
                          ? tr("Fine")
                          : tr("Avanti →"));
        const int pageIndex = pageIndexes.at(*currentStep);
        m_pages->setCurrentIndex(pageIndex);
        if (pageIndex == 0) {
            refreshDashboard();
            if (*currentStep == 1) {
                if (auto *scrollArea =
                        qobject_cast<QScrollArea *>(m_pages->widget(0))) {
                    scrollArea->ensureWidgetVisible(m_dashboardAccountsTable);
                }
            }
        } else if (pageIndex == 1) {
            refreshTransactions();
        } else if (pageIndex == 2) {
            refreshAnalysis();
        } else if (pageIndex == 3) {
            refreshCategories();
        }
    };
    connect(previous, &QPushButton::clicked, dialog,
            [currentStep, updateStep] {
        if (*currentStep > 0) {
            --*currentStep;
            updateStep();
        }
    });
    connect(next, &QPushButton::clicked, dialog,
            [dialog, currentStep, updateStep, titles] {
        if (*currentStep + 1 < titles.size()) {
            ++*currentStep;
            updateStep();
        } else {
            dialog->close();
        }
    });
    connect(close, &QPushButton::clicked, dialog, &QDialog::close);
    updateStep();
    dialog->show();
    dialog->move(mapToGlobal(QPoint(
        width() - dialog->width() - 28,
        height() - dialog->height() - 58)));
    dialog->raise();
}

void MainWindow::refreshAccounts()
{
    if (!m_accountSelector)
        return;
    QList<QVariantMap> accounts;
    if (!m_manager.getAccounts(accounts)) {
        showError(tr("Impossibile caricare gli account: %1").arg(m_manager.lastError()));
        return;
    }
    const QSignalBlocker blocker(m_accountSelector);
    m_accountSelector->clear();
    int selectedIndex = -1;
    for (const QVariantMap &account : accounts) {
        const int id = account.value(QStringLiteral("ID_Account")).toInt();
        const QString type = account.value(QStringLiteral("Type")).toString();
        const QString name = account.value(QStringLiteral("Name")).toString();
        const qint64 balance = account.value(QStringLiteral("Balance")).toLongLong();
        m_accountSelector->addItem(
            accountTypeIcon(type),
            name,
            id);
        m_accountSelector->setItemData(
            m_accountSelector->count() - 1,
            QStringLiteral("%1 · %2\n%3")
                .arg(name, accountTypeLabel(type),
                     tr("Saldo: %1").arg(money(balance))),
            Qt::ToolTipRole);
        m_accountSelector->setItemData(
            m_accountSelector->count() - 1, type, Qt::UserRole + 1);
        if (id == m_accountId)
            selectedIndex = m_accountSelector->count() - 1;
    }
    if (selectedIndex < 0 && m_accountSelector->count() > 0) {
        selectedIndex = 0;
        m_accountId = m_accountSelector->itemData(0).toInt();
        QSettings settings;
        settings.setValue(QStringLiteral("accounts/activeId"), m_accountId);
        settings.sync();
        if (settings.status() != QSettings::NoError)
            showError(tr("Impossibile salvare il conto attivo."));
    }
    m_accountSelector->setCurrentIndex(selectedIndex);
}

void MainWindow::createAccount()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Aggiungi un conto"));
    dialog.setMinimumWidth(420);
    auto *form = new QFormLayout;
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(14);
    auto *name = new QLineEdit(&dialog);
    name->setMaxLength(80);
    auto *type = new QComboBox(&dialog);
    type->addItem(tr("Conto corrente"), QStringLiteral("checking"));
    type->addItem(tr("Carta di debito"), QStringLiteral("debit_card"));
    type->addItem(tr("Carta di credito"), QStringLiteral("credit_card"));
    type->addItem(tr("Conto di risparmio"), QStringLiteral("savings"));
    type->addItem(tr("Contanti"), QStringLiteral("cash"));
    type->addItem(tr("Altro"), QStringLiteral("other"));
    auto *openingBalance = new QDoubleSpinBox(&dialog);
    openingBalance->setRange(0.0, 999999999.99);
    openingBalance->setDecimals(2);
    openingBalance->setSingleStep(10.0);
    openingBalance->setSuffix(QStringLiteral(" €"));
    form->addRow(tr("Nome"), name);
    form->addRow(tr("Tipo di conto"), type);
    form->addRow(tr("Saldo iniziale"), openingBalance);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(18);
    layout->addLayout(form);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Nome mancante"), tr("Inserisci il nome del conto."));
        return;
    }

    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(QByteArrayLiteral("expense-tracker-local-account"),
                                 QCryptographicHash::Sha256).toHex());
    int newAccountId = 0;
    if (!m_manager.addAccount(
            name->text(),
            hash,
            QDate::currentDate().toString(Qt::ISODate),
            qRound64(openingBalance->value() * 100.0),
            &newAccountId,
            type->currentData().toString())) {
        showError(tr("Impossibile creare il conto: %1").arg(m_manager.lastError()));
        return;
    }
    m_accountId = newAccountId;
    QSettings settings;
    settings.setValue(QStringLiteral("accounts/activeId"), m_accountId);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        showError(tr("Impossibile salvare il conto attivo."));
    }
    refreshAccounts();
    refreshAll();
    statusBar()->showMessage(tr("Conto aggiunto"), 3000);
}

void MainWindow::openAccountManagement()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Gestione conti e carte"));
    dialog.setMinimumSize(820, 400);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(12);

    auto *title = pageTitle(tr("I tuoi conti"), &dialog);
    layout->addWidget(title);
    auto *subtitle = new QLabel(
        tr("Controlla i saldi e le attività, cambia conto attivo o aggiorna i dettagli."),
        &dialog);
    subtitle->setObjectName(QStringLiteral("subTitle"));
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    auto *table = makeTable(
        {tr("Conto"), tr("Tipo"), tr("Saldo"), tr("Transazioni"), tr("Trasferimenti")},
        &dialog);
    table->setMinimumHeight(100);
    table->setMaximumHeight(360);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    table->setItemDelegate(new ComboPopupDelegate(table));
    table->verticalHeader()->setDefaultSectionSize(50);
    table->setColumnWidth(0, 190);
    table->setColumnWidth(1, 140);
    table->setColumnWidth(2, 140);
    table->setColumnWidth(3, 100);
    table->setColumnWidth(4, 100);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    layout->addWidget(table, 1);

    auto *actions = new QHBoxLayout;
    actions->setSpacing(10);
    auto *add = new QPushButton(tr("+ Aggiungi"), &dialog);
    add->setObjectName(QStringLiteral("primaryButton"));
    auto *edit = new QPushButton(tr("Modifica"), &dialog);
    edit->setObjectName(QStringLiteral("secondaryButton"));
    auto *setActive = new QPushButton(tr("Imposta come attivo"), &dialog);
    setActive->setObjectName(QStringLiteral("secondaryButton"));
    auto *remove = new QPushButton(tr("Rimuovi conto vuoto"), &dialog);
    remove->setObjectName(QStringLiteral("secondaryButton"));
    auto *close = new QPushButton(tr("Chiudi"), &dialog);
    close->setObjectName(QStringLiteral("secondaryButton"));
    actions->addWidget(add);
    actions->addWidget(edit);
    actions->addWidget(setActive);
    actions->addWidget(remove);
    actions->addStretch();
    actions->addWidget(close);
    layout->addLayout(actions);

    auto updateSelection = [this, table, edit, setActive, remove] {
        const int row = table->currentRow();
        const int accountId = row >= 0
            ? table->item(row, 0)->data(Qt::UserRole).toInt()
            : 0;
        const int transactionCount = row >= 0
            ? table->item(row, 3)->text().toInt()
            : 0;
        const int transferCount = row >= 0
            ? table->item(row, 4)->text().toInt()
            : 0;
        const qint64 balance = row >= 0
            ? table->item(row, 2)->data(Qt::UserRole).toLongLong()
            : 0;
        edit->setEnabled(row >= 0);
        setActive->setEnabled(row >= 0 && accountId != m_accountId);
        remove->setEnabled(row >= 0 && table->rowCount() > 1 &&
                           transactionCount == 0 && transferCount == 0 && balance == 0);
    };
    auto refreshTable = [this, table, updateSelection] {
        QList<QVariantMap> accounts;
        if (!m_manager.getAccounts(accounts)) {
            showError(tr("Impossibile caricare gli account: %1").arg(m_manager.lastError()));
            return;
        }
        int activeRow = -1;
        const int selectedId = table->currentRow() >= 0
            ? table->item(table->currentRow(), 0)->data(Qt::UserRole).toInt()
            : m_accountId;
        const QSignalBlocker blocker(table);
        table->setRowCount(accounts.size());
        for (int row = 0; row < accounts.size(); ++row) {
            const QVariantMap &account = accounts.at(row);
            const int id = account.value(QStringLiteral("ID_Account")).toInt();
            const QString type = account.value(QStringLiteral("Type")).toString();
            auto *name = cell(account.value(QStringLiteral("Name")).toString());
            if (id == m_accountId) {
                name->setText(tr("%1 · Attivo").arg(name->text()));
                name->setToolTip(tr("Conto attivo"));
            }
            name->setIcon(accountTypeIcon(type));
            name->setData(Qt::UserRole, id);
            name->setFont(QFont(font().family(), 10, QFont::DemiBold));
            table->setItem(row, 0, name);
            table->setItem(row, 1, cell(accountTypeLabel(type)));
            auto *balance = cell(
                money(account.value(QStringLiteral("Balance")).toLongLong()),
                Qt::AlignRight | Qt::AlignVCenter);
            balance->setData(
                Qt::UserRole, account.value(QStringLiteral("Balance")).toLongLong());
            balance->setForeground(
                accountTypeColor(type).darker(
                    m_themeModeKey == QStringLiteral("light") ? 125 : 100));
            balance->setFont(QFont(font().family(), 10, QFont::DemiBold));
            table->setItem(row, 2, balance);
            table->setItem(
                row, 3,
                cell(QString::number(
                    account.value(QStringLiteral("TransactionCount")).toInt()),
                    Qt::AlignCenter));
            table->setItem(
                row, 4,
                cell(QString::number(
                    account.value(QStringLiteral("TransferCount")).toInt()),
                    Qt::AlignCenter));
            if (id == m_accountId)
                activeRow = row;
            if (id == selectedId)
                table->selectRow(row);
        }
        if (table->currentRow() < 0 && activeRow >= 0)
            table->selectRow(activeRow);
        const int contentHeight =
            table->horizontalHeader()->height() + accounts.size() * 50 + 14;
        table->setFixedHeight(qBound(100, contentHeight, 360));
        updateSelection();
    };
    auto selectedAccountId = [table] {
        const int row = table->currentRow();
        return row >= 0
            ? table->item(row, 0)->data(Qt::UserRole).toInt()
            : 0;
    };
    connect(table, &QTableWidget::itemSelectionChanged, &dialog, updateSelection);
    connect(add, &QPushButton::clicked, &dialog, [this, refreshTable] {
        createAccount();
        refreshTable();
    });
    connect(edit, &QPushButton::clicked, &dialog,
            [this, &dialog, table, selectedAccountId, refreshTable] {
        const int accountId = selectedAccountId();
        QList<QVariantMap> accounts;
        if (!m_manager.getAccounts(accounts)) {
            showError(tr("Impossibile caricare gli account: %1").arg(m_manager.lastError()));
            return;
        }
        const auto account = std::find_if(
            accounts.cbegin(), accounts.cend(), [accountId](const QVariantMap &entry) {
                return entry.value(QStringLiteral("ID_Account")).toInt() == accountId;
            });
        if (account == accounts.cend())
            return;

        QDialog editor(&dialog);
        editor.setWindowTitle(tr("Modifica conto"));
        auto *form = new QFormLayout(&editor);
        form->setContentsMargins(22, 20, 22, 14);
        form->setHorizontalSpacing(16);
        form->setVerticalSpacing(14);
        auto *name = new QLineEdit(
            account->value(QStringLiteral("Name")).toString(), &editor);
        name->setMaxLength(80);
        auto *type = new QComboBox(&editor);
        type->addItem(tr("Conto corrente"), QStringLiteral("checking"));
        type->addItem(tr("Carta di debito"), QStringLiteral("debit_card"));
        type->addItem(tr("Carta di credito"), QStringLiteral("credit_card"));
        type->addItem(tr("Conto di risparmio"), QStringLiteral("savings"));
        type->addItem(tr("Contanti"), QStringLiteral("cash"));
        type->addItem(tr("Altro"), QStringLiteral("other"));
        type->setCurrentIndex(type->findData(account->value(QStringLiteral("Type"))));
        form->addRow(tr("Nome"), name);
        form->addRow(tr("Tipo di conto"), type);
        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Save | QDialogButtonBox::Cancel, &editor);
        connect(buttons, &QDialogButtonBox::accepted, &editor, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &editor, &QDialog::reject);
        auto *editorLayout = new QVBoxLayout(&editor);
        editorLayout->setContentsMargins(0, 0, 0, 18);
        editorLayout->setSpacing(16);
        editorLayout->addLayout(form);
        editorLayout->addWidget(buttons);
        if (editor.exec() != QDialog::Accepted)
            return;
        if (name->text().trimmed().isEmpty()) {
            QMessageBox::warning(&editor, tr("Nome mancante"),
                                 tr("Inserisci il nome del conto."));
            return;
        }
        if (!m_manager.updateAccount(
                accountId, name->text(), type->currentData().toString())) {
            showError(tr("Impossibile aggiornare il conto: %1").arg(m_manager.lastError()));
            return;
        }
        refreshAccounts();
        refreshAll();
        refreshTable();
    });
    connect(setActive, &QPushButton::clicked, &dialog,
            [this, selectedAccountId, refreshTable] {
        const int accountId = selectedAccountId();
        if (accountId <= 0)
            return;
        m_accountId = accountId;
        QSettings settings;
        settings.setValue(QStringLiteral("accounts/activeId"), m_accountId);
        settings.sync();
        if (settings.status() != QSettings::NoError) {
            showError(tr("Impossibile salvare il conto attivo."));
        }
        refreshAccounts();
        refreshAll();
        refreshTable();
        statusBar()->showMessage(tr("Conto attivo aggiornato"), 2500);
    });
    connect(remove, &QPushButton::clicked, &dialog,
            [this, &dialog, selectedAccountId, refreshTable] {
        const int accountId = selectedAccountId();
        if (accountId <= 0)
            return;
        if (QMessageBox::question(
                &dialog,
                tr("Rimuovi conto vuoto"),
                tr("Vuoi rimuovere questo conto? Questa operazione non può essere annullata.")) !=
            QMessageBox::Yes) {
            return;
        }
        if (!m_manager.deleteAccount(accountId)) {
            showError(tr("Impossibile rimuovere il conto: %1").arg(m_manager.lastError()));
            return;
        }
        refreshAccounts();
        refreshAll();
        refreshTable();
    });
    connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(table, &QTableWidget::cellDoubleClicked, &dialog,
            [setActive] { if (setActive->isEnabled()) setActive->click(); });
    refreshTable();
    dialog.exec();
    refreshAccounts();
    refreshAll();
}

void MainWindow::openTransfers()
{
    QList<QVariantMap> accounts;
    if (!m_manager.getAccounts(accounts)) {
        showError(tr("Impossibile caricare gli account: %1").arg(m_manager.lastError()));
        return;
    }
    if (accounts.size() < 2) {
        QMessageBox::information(
            this,
            tr("Servono due conti"),
            tr("Aggiungi un altro conto prima di trasferire denaro."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Trasferimenti tra conti"));
    dialog.setMinimumSize(620, 500);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(18);
    auto *form = new QFormLayout;
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(14);
    auto *source = new QComboBox(&dialog);
    auto *destination = new QComboBox(&dialog);
    for (const QVariantMap &account : accounts) {
        const int id = account.value(QStringLiteral("ID_Account")).toInt();
        const QString label = QStringLiteral("%1 · %2").arg(
            account.value(QStringLiteral("Name")).toString(),
            accountTypeLabel(account.value(QStringLiteral("Type")).toString()));
        source->addItem(label, id);
        destination->addItem(label, id);
    }
    const int sourceIndex = source->findData(m_accountId);
    if (sourceIndex >= 0)
        source->setCurrentIndex(sourceIndex);
    auto updateDestinations = [destination, source, accounts] {
        const int previousDestination = destination->currentData().toInt();
        const int sourceId = source->currentData().toInt();
        destination->clear();
        for (const QVariantMap &account : accounts) {
            const int id = account.value(QStringLiteral("ID_Account")).toInt();
            if (id == sourceId)
                continue;
            destination->addItem(
                QStringLiteral("%1 · %2").arg(
                    account.value(QStringLiteral("Name")).toString(),
                    accountTypeLabel(account.value(QStringLiteral("Type")).toString())),
                id);
        }
        const int preferred = destination->findData(previousDestination);
        if (preferred >= 0)
            destination->setCurrentIndex(preferred);
    };
    updateDestinations();
    connect(source, &QComboBox::currentIndexChanged, &dialog, updateDestinations);
    auto *date = new QDateEdit(QDate::currentDate(), &dialog);
    date->setCalendarPopup(true);
    date->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    auto *amount = new QDoubleSpinBox(&dialog);
    amount->setRange(0.01, 999999999.99);
    amount->setDecimals(2);
    amount->setSingleStep(10.0);
    amount->setSuffix(QStringLiteral(" €"));
    form->addRow(tr("Conto di origine"), source);
    form->addRow(tr("Conto di destinazione"), destination);
    form->addRow(tr("Data"), date);
    form->addRow(tr("Importo"), amount);
    layout->addLayout(form);

    auto *historyTitle = new QLabel(tr("Storico trasferimenti del conto di partenza"), &dialog);
    historyTitle->setObjectName(QStringLiteral("pageTitle"));
    layout->addWidget(historyTitle);
    auto *history = makeTable(
        {tr("Data"), tr("Direzione"), tr("Altro conto"), tr("Importo")}, &dialog);
    history->setMinimumHeight(180);
    layout->addWidget(history, 1);
    auto loadHistory = [this, source, history] {
        QList<QVariantMap> transfers;
        if (!m_manager.getAccountTransfers(source->currentData().toInt(), transfers)) {
            showError(tr("Impossibile caricare i trasferimenti: %1").arg(m_manager.lastError()));
            return;
        }
        history->setRowCount(transfers.size());
        for (int row = 0; row < transfers.size(); ++row) {
            const QVariantMap &transfer = transfers.at(row);
            const bool outgoing =
                transfer.value(QStringLiteral("Direction")).toString() == QStringLiteral("out");
            history->setItem(
                row, 0,
                cell(QDate::fromString(
                    transfer.value(QStringLiteral("Date")).toString(), Qt::ISODate)
                         .toString(QStringLiteral("dd/MM/yyyy"))));
            auto *direction = cell(outgoing ? tr("In uscita") : tr("In entrata"));
            direction->setForeground(QColor(outgoing
                                                ? QStringLiteral("#b91c1c")
                                                : QStringLiteral("#047857")));
            history->setItem(row, 1, direction);
            history->setItem(
                row, 2, cell(transfer.value(QStringLiteral("OtherAccount")).toString()));
            auto *transferAmount = cell(
                (outgoing ? QStringLiteral("−") : QStringLiteral("+")) +
                    money(transfer.value(QStringLiteral("Amount")).toLongLong()),
                Qt::AlignVCenter | Qt::AlignRight);
            transferAmount->setForeground(direction->foreground());
            history->setItem(row, 3, transferAmount);
        }
    };
    connect(source, &QComboBox::currentIndexChanged, &dialog, loadHistory);
    loadHistory();

    auto *actions = new QHBoxLayout;
    auto *create = new QPushButton(tr("Esegui trasferimento"), &dialog);
    create->setObjectName(QStringLiteral("primaryButton"));
    auto *close = new QPushButton(tr("Chiudi"), &dialog);
    close->setObjectName(QStringLiteral("secondaryButton"));
    actions->addWidget(create);
    actions->addStretch();
    actions->addWidget(close);
    layout->addLayout(actions);
    connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(create, &QPushButton::clicked, &dialog, [this, &dialog, source,
                                                      destination, date, amount,
                                                      loadHistory] {
        const int sourceId = source->currentData().toInt();
        const int destinationId = destination->currentData().toInt();
        if (sourceId <= 0 || destinationId <= 0 || sourceId == destinationId) {
            QMessageBox::warning(&dialog, tr("Conti non validi"),
                                 tr("Scegli due conti diversi."));
            return;
        }
        if (!m_manager.addTransfer(
                sourceId,
                destinationId,
                date->date().toString(Qt::ISODate),
                qRound64(amount->value() * 100.0))) {
            showError(tr("Impossibile eseguire il trasferimento: %1")
                          .arg(m_manager.lastError()));
            return;
        }
        m_accountId = sourceId;
        refreshAccounts();
        refreshAll();
        loadHistory();
        statusBar()->showMessage(tr("Trasferimento registrato"), 3000);
    });
    dialog.exec();
}

QWidget *MainWindow::buildDashboardPage()
{
    auto *scrollArea = new QScrollArea;
    scrollArea->setObjectName(QStringLiteral("dashboardScrollArea"));
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    auto *page = new QWidget;
    scrollArea->setWidget(page);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 24, 30, 26);
    layout->setSpacing(14);

    auto *header = new QHBoxLayout;
    auto *titleGroup = new QVBoxLayout;
    titleGroup->setSpacing(4);
    titleGroup->addWidget(pageTitle(tr("Panoramica"), page));
    auto *subtitle = new QLabel(tr("Il tuo mese, il saldo e le ultime operazioni."), page);
    subtitle->setObjectName(QStringLiteral("subTitle"));
    titleGroup->addWidget(subtitle);
    header->addLayout(titleGroup, 1);
    auto *add = new QPushButton(tr("+ Nuova transazione"), page);
    add->setObjectName(QStringLiteral("primaryButton"));
    add->setMinimumHeight(42);
    add->setToolTip(tr("Aggiungi una transazione (Ctrl+N)"));
    header->addWidget(add, 0, Qt::AlignVCenter);
    auto *view = new QPushButton(tr("Vedi tutte"), page);
    view->setObjectName(QStringLiteral("secondaryButton"));
    view->setMinimumHeight(42);
    header->addWidget(view, 0, Qt::AlignVCenter);
    layout->addLayout(header);

    auto *metrics = new QHBoxLayout;
    metrics->setSpacing(14);
    metrics->addWidget(
        metricCard(tr("Saldo attuale"), QStringLiteral("balance"), &m_balanceValue, page), 1);
    metrics->addWidget(
        metricCard(tr("Entrate del mese"), QStringLiteral("income"), &m_incomeValue, page), 1);
    metrics->addWidget(
        metricCard(tr("Uscite del mese"), QStringLiteral("expenses"), &m_expensesValue, page), 1);
    metrics->addWidget(
        metricCard(tr("Saldo netto del mese"), QStringLiteral("month"), &m_monthValue, page), 1);
    layout->addLayout(metrics);

    auto *accountsPanel = new QFrame(page);
    accountsPanel->setObjectName(QStringLiteral("panel"));
    auto *accountsLayout = new QVBoxLayout(accountsPanel);
    accountsLayout->setContentsMargins(18, 14, 18, 14);
    accountsLayout->setSpacing(8);
    auto *accountsHeader = new QHBoxLayout;
    auto *accountsTitle = new QLabel(tr("Saldi di tutti i conti"), accountsPanel);
    accountsTitle->setStyleSheet(QStringLiteral("font-size:13pt;font-weight:700;"));
    accountsHeader->addWidget(accountsTitle);
    accountsHeader->addStretch();
    auto *totalCaption = new QLabel(tr("Saldo complessivo:"), accountsPanel);
    totalCaption->setObjectName(QStringLiteral("subTitle"));
    accountsHeader->addWidget(totalCaption);
    m_dashboardTotalBalance = new QLabel(QStringLiteral("—"), accountsPanel);
    m_dashboardTotalBalance->setStyleSheet(QStringLiteral("font-size:13pt;font-weight:800;"));
    accountsHeader->addWidget(m_dashboardTotalBalance);
    accountsLayout->addLayout(accountsHeader);

    m_dashboardAccountsTable = makeTable(
        {tr("Conto"), tr("Tipo"), tr("Saldo")}, accountsPanel);
    m_dashboardAccountsTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_dashboardAccountsTable->setFocusPolicy(Qt::NoFocus);
    m_dashboardAccountsTable->setToolTip(tr("Fai doppio clic su un conto per attivarlo"));
    m_dashboardAccountsTable->setMinimumHeight(260);
    m_dashboardAccountsTable->setMaximumHeight(300);
    m_dashboardAccountsTable->verticalHeader()->setDefaultSectionSize(40);
    m_dashboardAccountsTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    m_dashboardAccountsTable->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    m_dashboardAccountsTable->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    accountsLayout->addWidget(m_dashboardAccountsTable);
    connect(m_dashboardAccountsTable, &QTableWidget::cellDoubleClicked,
            this, [this](int row, int) {
        if (!m_dashboardAccountsTable || !m_accountSelector)
            return;
        const QTableWidgetItem *accountItem =
            m_dashboardAccountsTable->item(row, 0);
        if (!accountItem)
            return;
        const int accountId = accountItem->data(Qt::UserRole).toInt();
        const int accountIndex = m_accountSelector->findData(accountId);
        if (accountIndex >= 0)
            m_accountSelector->setCurrentIndex(accountIndex);
    });
    layout->addWidget(accountsPanel);

    auto *activityRow = new QHBoxLayout;
    activityRow->setSpacing(14);
    auto *trendPanel = new QFrame(page);
    trendPanel->setObjectName(QStringLiteral("panel"));
    auto *trendLayout = new QVBoxLayout(trendPanel);
    trendLayout->setContentsMargins(18, 14, 18, 12);
    trendLayout->setSpacing(6);
    auto *trendTitle = new QLabel(
        tr("Entrate e uscite · ultimi 6 mesi"), trendPanel);
    trendTitle->setStyleSheet(QStringLiteral("font-size:13pt;font-weight:700;"));
    trendLayout->addWidget(trendTitle);
    m_dashboardTrend = new ExpenseTrendChart(trendPanel);
    m_dashboardTrend->setMinimumHeight(210);
    trendLayout->addWidget(m_dashboardTrend, 1);
    activityRow->addWidget(trendPanel, 3);

    auto *recentPanel = new QFrame(page);
    recentPanel->setObjectName(QStringLiteral("panel"));
    auto *recentLayout = new QVBoxLayout(recentPanel);
    recentLayout->setContentsMargins(18, 14, 18, 16);
    recentLayout->setSpacing(10);
    auto *recentHeader = new QHBoxLayout;
    auto *heading = new QLabel(tr("Movimenti recenti"), recentPanel);
    heading->setStyleSheet(QStringLiteral("font-size:14pt;font-weight:700;"));
    recentHeader->addWidget(heading);
    recentHeader->addStretch();
    auto *viewRecent = new QPushButton(tr("Apri transazioni"), recentPanel);
    viewRecent->setObjectName(QStringLiteral("secondaryButton"));
    recentHeader->addWidget(viewRecent);
    recentLayout->addLayout(recentHeader);
    m_recentTable = makeTable({tr("Data"), tr("Categoria"), tr("Tipo"), tr("Importo")}, recentPanel);
    m_recentTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_recentTable->setFocusPolicy(Qt::NoFocus);
    m_recentTable->setMinimumHeight(260);
    recentLayout->addWidget(m_recentTable);
    activityRow->addWidget(recentPanel, 4);
    layout->addLayout(activityRow);

    connect(add, &QPushButton::clicked, this, &MainWindow::addTransaction);
    auto openTransactions = [this] {
        m_pages->setCurrentIndex(1);
    };
    connect(view, &QPushButton::clicked, this, openTransactions);
    connect(viewRecent, &QPushButton::clicked, this, openTransactions);
    return scrollArea;
}

QWidget *MainWindow::buildTransactionsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(28, 22, 28, 18);
    layout->setSpacing(10);
    auto *headingRow = new QHBoxLayout;
    auto *headingText = new QVBoxLayout;
    headingText->setSpacing(2);
    headingText->addWidget(pageTitle(tr("Transazioni"), page));
    auto *subtitle = new QLabel(
        tr("Tieni sotto controllo ogni entrata e uscita."),
        page);
    subtitle->setObjectName(QStringLiteral("subTitle"));
    subtitle->setTextInteractionFlags(Qt::NoTextInteraction);
    headingText->addWidget(subtitle);
    headingRow->addLayout(headingText, 1);
    auto *add = new QPushButton(tr("+ Nuova transazione"), page);
    add->setObjectName(QStringLiteral("primaryButton"));
    add->setMinimumHeight(42);
    add->setToolTip(tr("Aggiungi una transazione (Ctrl+N)"));
    headingRow->addWidget(add, 0, Qt::AlignVCenter);
    layout->addLayout(headingRow);

    auto *filters = new QFrame(page);
    filters->setObjectName(QStringLiteral("panel"));
    auto *filterLayout = new QVBoxLayout(filters);
    filterLayout->setContentsMargins(14, 12, 14, 12);
    filterLayout->setSpacing(10);

    auto *dateCategoryRow = new QHBoxLayout;
    dateCategoryRow->setSpacing(8);
    m_filterDateEnabled = new QCheckBox(tr("Date"), filters);
    m_filterDateEnabled->setToolTip(tr("Attiva il filtro per intervallo di date"));
    m_filterFrom = new QDateEdit(QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1), filters);
    m_filterTo = new QDateEdit(QDate::currentDate(), filters);
    m_filterFrom->setCalendarPopup(true);
    m_filterTo->setCalendarPopup(true);
    m_filterFrom->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_filterTo->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_filterFrom->setMinimumWidth(120);
    m_filterTo->setMinimumWidth(120);
    m_filterFrom->setEnabled(false);
    m_filterTo->setEnabled(false);
    m_filterCategory = new QComboBox(filters);
    polishComboPopup(m_filterCategory);
    m_filterCategory->setMinimumWidth(150);
    m_filterCategory->addItem(tr("Tutte le categorie"), 0);
    for (const QVariantMap &category : m_categories)
        m_filterCategory->addItem(
                                  translatedCategoryName(
                                      category.value(QStringLiteral("Name")).toString()),
                                  category.value(QStringLiteral("ID_Category")).toInt());
    m_filterType = new QComboBox(filters);
    polishComboPopup(m_filterType);
    m_filterType->setMinimumWidth(125);
    m_filterType->addItem(tr("Entrate e uscite"), QString());
    m_filterType->addItem(tr("Solo entrate"), QStringLiteral("in"));
    m_filterType->addItem(tr("Solo uscite"), QStringLiteral("out"));
    dateCategoryRow->addWidget(m_filterDateEnabled);
    dateCategoryRow->addWidget(new QLabel(tr("Dal"), filters));
    dateCategoryRow->addWidget(m_filterFrom);
    dateCategoryRow->addWidget(new QLabel(tr("Al"), filters));
    dateCategoryRow->addWidget(m_filterTo);
    dateCategoryRow->addWidget(m_filterCategory, 1);
    dateCategoryRow->addWidget(m_filterType);
    filterLayout->addLayout(dateCategoryRow);

    auto *amountActionRow = new QHBoxLayout;
    amountActionRow->setSpacing(8);
    m_filterMin = new QDoubleSpinBox(filters);
    m_filterMin->setRange(0, 999999999);
    m_filterMin->setPrefix(tr("Min "));
    m_filterMin->setSuffix(QStringLiteral(" €"));
    m_filterMin->setSpecialValueText(tr("Min €"));
    m_filterMin->setMinimumWidth(130);
    m_filterMin->setGroupSeparatorShown(true);
    m_filterMax = new QDoubleSpinBox(filters);
    m_filterMax->setRange(0, 999999999);
    m_filterMax->setPrefix(tr("Max "));
    m_filterMax->setSuffix(QStringLiteral(" €"));
    m_filterMax->setSpecialValueText(tr("Max €"));
    m_filterMax->setMinimumWidth(130);
    m_filterMax->setGroupSeparatorShown(true);
    auto *apply = new QPushButton(tr("Filtra"), filters);
    apply->setObjectName(QStringLiteral("secondaryButton"));
    apply->setToolTip(tr("Applica i filtri selezionati"));
    auto *reset = new QPushButton(tr("Reset"), filters);
    reset->setObjectName(QStringLiteral("secondaryButton"));
    reset->setToolTip(tr("Azzera tutti i filtri"));
    amountActionRow->addWidget(m_filterMin);
    amountActionRow->addWidget(m_filterMax);
    amountActionRow->addStretch();
    amountActionRow->addWidget(apply);
    amountActionRow->addWidget(reset);
    filterLayout->addLayout(amountActionRow);
    layout->addWidget(filters);

    auto *searchRow = new QHBoxLayout;
    m_transactionSearch = new QLineEdit(page);
    m_transactionSearch->setObjectName(QStringLiteral("transactionSearch"));
    m_transactionSearch->setPlaceholderText(
        tr("Cerca categoria, note, data, importo o ID…"));
    m_transactionSearch->setClearButtonEnabled(true);
    m_transactionSearch->setMinimumHeight(40);
    m_transactionSearchTimer = new QTimer(this);
    m_transactionSearchTimer->setSingleShot(true);
    m_transactionSearchTimer->setInterval(100);
    m_sortOrder = new QComboBox(page);
    polishComboPopup(m_sortOrder);
    m_sortOrder->setToolTip(tr("Ordina i risultati"));
    m_sortOrder->addItem(tr("Più recenti"), QStringLiteral("newest"));
    m_sortOrder->addItem(tr("Meno recenti"), QStringLiteral("oldest"));
    m_sortOrder->addItem(tr("Importo maggiore"), QStringLiteral("highest"));
    m_sortOrder->addItem(tr("Importo minore"), QStringLiteral("lowest"));
    m_sortOrder->setMinimumWidth(155);
    m_sortOrder->setMinimumHeight(40);
    m_transactionCount = new QLabel(page);
    m_transactionCount->setObjectName(QStringLiteral("subTitle"));
    m_transactionCount->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    searchRow->addWidget(m_transactionSearch, 1);
    searchRow->addWidget(m_sortOrder);
    searchRow->addWidget(m_transactionCount);
    layout->addLayout(searchRow);

    auto *summary = new QHBoxLayout;
    summary->setSpacing(10);
    const auto addSummary = [this, page, summary](
                                const QString &caption,
                                QLabel **valueLabel,
                                const QString &accent) {
        auto *card = new QFrame(page);
        card->setObjectName(QStringLiteral("transactionSummaryCard"));
        card->setProperty("summaryAccentDark", accent);
        card->setProperty(
            "summaryAccentLight",
            QColor(accent).darker(160).name());
        m_transactionSummaryCards.append(card);
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 8, 12, 8);
        cardLayout->setSpacing(3);
        auto *captionLabel = new QLabel(caption, card);
        captionLabel->setObjectName(QStringLiteral("transactionSummaryCaption"));
        captionLabel->setTextInteractionFlags(Qt::NoTextInteraction);
        *valueLabel = new QLabel(QStringLiteral("0,00 €"), card);
        (*valueLabel)->setObjectName(QStringLiteral("transactionSummaryValue"));
        (*valueLabel)->setTextInteractionFlags(Qt::NoTextInteraction);
        cardLayout->addWidget(captionLabel);
        cardLayout->addWidget(*valueLabel);
        summary->addWidget(card, 1);
    };
    addSummary(tr("ENTRATE FILTRATE"), &m_filteredIncome, QStringLiteral("#34d399"));
    addSummary(tr("USCITE FILTRATE"), &m_filteredExpenses, QStringLiteral("#fb7185"));
    addSummary(tr("SALDO RISULTATI"), &m_filteredBalance, QStringLiteral("#22d3ee"));
    layout->addLayout(summary);

    m_transactionsTable = makeTable(
        {tr("Data"), tr("Categoria"), tr("Tipo"), tr("Importo"),
         tr("Note"), tr("ID")}, page);
    m_transactionsTable->setColumnHidden(5, true);
    m_transactionsTable->setMinimumHeight(180);
    m_transactionsTable->verticalHeader()->setDefaultSectionSize(44);
    m_transactionsTable->setSortingEnabled(false);
    m_transactionsTable->setToolTip(tr("Fai doppio clic su una riga per modificarla"));
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_transactionsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    layout->addWidget(m_transactionsTable, 1);

    m_transactionEmpty = new QLabel(tr("Nessuna transazione corrisponde ai filtri."), page);
    m_transactionEmpty->setObjectName(QStringLiteral("subTitle"));
    m_transactionEmpty->setAlignment(Qt::AlignCenter);
    m_transactionEmpty->setMinimumHeight(180);
    m_transactionEmpty->hide();
    layout->addWidget(m_transactionEmpty, 1);

    auto *actions = new QHBoxLayout;
    m_editTransaction = new QPushButton(tr("Modifica"), page);
    m_editTransaction->setObjectName(QStringLiteral("secondaryButton"));
    m_deleteTransaction = new QPushButton(tr("Elimina"), page);
    m_deleteTransaction->setObjectName(QStringLiteral("secondaryButton"));
    auto *csv = new QPushButton(tr("Esporta CSV"), page);
    csv->setObjectName(QStringLiteral("secondaryButton"));
    m_editTransaction->setEnabled(false);
    m_deleteTransaction->setEnabled(false);
    m_editTransaction->setToolTip(tr("Seleziona una transazione da modificare"));
    m_deleteTransaction->setToolTip(tr("Seleziona una transazione da eliminare"));
    actions->addWidget(m_editTransaction);
    actions->addWidget(m_deleteTransaction);
    actions->addStretch();
    actions->addWidget(csv);
    layout->addLayout(actions);

    auto *pagination = new QHBoxLayout;
    auto *pageSizeLabel = new QLabel(tr("Righe per pagina"), page);
    m_pageSize = new QComboBox(page);
    polishComboPopup(m_pageSize);
    m_pageSize->addItem(QStringLiteral("10"), 10);
    m_pageSize->addItem(QStringLiteral("20"), 20);
    m_pageSize->addItem(QStringLiteral("50"), 50);
    m_pageSize->setCurrentIndex(1);
    m_pageSize->setMaximumWidth(86);
    m_previousPage = new QPushButton(tr("← Precedente"), page);
    m_previousPage->setObjectName(QStringLiteral("secondaryButton"));
    m_nextPage = new QPushButton(tr("Successiva →"), page);
    m_nextPage->setObjectName(QStringLiteral("secondaryButton"));
    m_pageInfo = new QLabel(page);
    m_pageInfo->setObjectName(QStringLiteral("subTitle"));
    m_pageInfo->setMinimumWidth(105);
    m_pageInfo->setAlignment(Qt::AlignCenter);
    pagination->addWidget(pageSizeLabel);
    pagination->addWidget(m_pageSize);
    pagination->addStretch();
    pagination->addWidget(m_previousPage);
    pagination->addWidget(m_pageInfo);
    pagination->addWidget(m_nextPage);
    layout->addLayout(pagination);

    connect(apply, &QPushButton::clicked, this, [this] {
        m_transactionPage = 0;
        refreshTransactions();
    });
    connect(reset, &QPushButton::clicked, this, [this] {
        m_filterDateEnabled->setChecked(false);
        m_filterFrom->setDate(QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1));
        m_filterTo->setDate(QDate::currentDate());
        m_filterCategory->setCurrentIndex(0);
        m_filterType->setCurrentIndex(0);
        m_filterMin->setValue(0);
        m_filterMax->setValue(0);
        m_transactionPage = 0;
        refreshTransactions();
    });
    connect(m_filterDateEnabled, &QCheckBox::toggled, this, [this](bool enabled) {
        m_filterFrom->setEnabled(enabled);
        m_filterTo->setEnabled(enabled);
    });
    connect(m_sortOrder, &QComboBox::currentIndexChanged, this, [this] {
        m_transactionPage = 0;
        refreshTransactions();
    });
    connect(m_transactionSearch, &QLineEdit::textChanged, this, [this] {
        m_transactionSearchTimer->start();
    });
    connect(m_transactionSearchTimer, &QTimer::timeout, this, [this] {
        m_transactionPage = 0;
        renderTransactionsPage();
    });
    connect(m_transactionSearch, &QLineEdit::returnPressed, this, [this] {
        m_transactionSearchTimer->stop();
        m_transactionPage = 0;
        renderTransactionsPage();
        if (m_transactionsTable->rowCount() == 0)
            return;
        m_transactionsTable->selectRow(0);
        m_transactionsTable->setFocus();
    });
    auto *clearSearchShortcut =
        new QShortcut(QKeySequence(Qt::Key_Escape), m_transactionSearch);
    clearSearchShortcut->setContext(Qt::WidgetShortcut);
    connect(clearSearchShortcut, &QShortcut::activated,
            m_transactionSearch, &QLineEdit::clear);
    connect(m_pageSize, &QComboBox::currentIndexChanged, this, [this] {
        m_transactionPage = 0;
        renderTransactionsPage();
    });
    connect(m_previousPage, &QPushButton::clicked, this, [this] {
        if (m_transactionPage > 0) {
            --m_transactionPage;
            renderTransactionsPage();
        }
    });
    connect(m_nextPage, &QPushButton::clicked, this, [this] {
        const int pageSize = m_pageSize->currentData().toInt();
        const int pageCount = (m_visibleTransactions.size() - 1) / pageSize + 1;
        if (m_transactionPage + 1 < pageCount) {
            ++m_transactionPage;
            renderTransactionsPage();
        }
    });
    connect(add, &QPushButton::clicked, this, &MainWindow::addTransaction);
    connect(m_editTransaction, &QPushButton::clicked, this, &MainWindow::editTransaction);
    connect(m_deleteTransaction, &QPushButton::clicked, this, &MainWindow::deleteTransaction);
    connect(csv, &QPushButton::clicked, this, &MainWindow::exportCsv);
    connect(m_transactionsTable, &QTableWidget::cellDoubleClicked, this,
            [this] { editTransaction(); });
    connect(m_transactionsTable, &QTableWidget::itemSelectionChanged, this, [this] {
        const bool hasSelection = m_transactionsTable->currentRow() >= 0;
        m_editTransaction->setEnabled(hasSelection);
        m_deleteTransaction->setEnabled(hasSelection);
    });
    auto *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, this, [this] {
        m_pages->setCurrentIndex(1);
        m_transactionSearch->setFocus();
        m_transactionSearch->selectAll();
    });
    auto *deleteShortcut = new QShortcut(QKeySequence::Delete, m_transactionsTable);
    deleteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(deleteShortcut, &QShortcut::activated, this, &MainWindow::deleteTransaction);
    auto *editShortcut = new QShortcut(QKeySequence(Qt::Key_Return), m_transactionsTable);
    editShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(editShortcut, &QShortcut::activated, this, &MainWindow::editTransaction);
    return page;
}

QWidget *MainWindow::buildCategoriesPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(16);
    layout->addWidget(pageTitle(tr("Categorie"), page));
    auto *subtitle = new QLabel(tr("Organizza entrate e uscite con categorie dedicate."), page);
    subtitle->setObjectName(QStringLiteral("subTitle"));
    layout->addWidget(subtitle);
    m_categoriesTable = makeTable({tr("Categoria"), tr("Tipo")}, page);
    m_categoriesTable->setObjectName(QStringLiteral("categoriesTable"));
    m_categoriesTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_categoriesTable->setFocusPolicy(Qt::NoFocus);
    m_categoriesTable->verticalHeader()->setDefaultSectionSize(48);
    layout->addWidget(m_categoriesTable, 1);
    auto *add = new QPushButton(tr("+ Nuova categoria"), page);
    add->setObjectName(QStringLiteral("primaryButton"));
    layout->addWidget(add, 0, Qt::AlignLeft);
    connect(add, &QPushButton::clicked, this, &MainWindow::addCategory);
    return page;
}

QWidget *MainWindow::buildAnalysisPage()
{
    auto *scrollArea = new QScrollArea;
    scrollArea->setObjectName(QStringLiteral("analysisScrollArea"));
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    auto *page = new QWidget;
    scrollArea->setWidget(page);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 26, 30, 26);
    layout->setSpacing(14);

    auto *header = new QHBoxLayout;
    header->addWidget(pageTitle(tr("Analisi delle spese"), page));
    header->addStretch();
    m_analysisPeriod = new QComboBox(page);
    m_analysisPeriod->setObjectName(QStringLiteral("analysisPeriod"));
    polishComboPopup(m_analysisPeriod);
    m_analysisPeriod->addItem(tr("Ultimo mese"), 1);
    m_analysisPeriod->addItem(tr("Ultimi 3 mesi"), 3);
    m_analysisPeriod->addItem(tr("Ultimi 6 mesi"), 6);
    m_analysisPeriod->addItem(tr("Tutti i periodi"), 0);
    QSettings settings;
    m_analysisMonths = settings.value(QStringLiteral("analysis/months"), 1).toInt();
    if (m_analysisMonths != 1 && m_analysisMonths != 3 && m_analysisMonths != 6)
        m_analysisMonths = 1;
    m_analysisPeriod->setCurrentIndex(m_analysisPeriod->findData(m_analysisMonths));
    header->addWidget(new QLabel(tr("Periodo:"), page));
    header->addWidget(m_analysisPeriod);
    m_analysisPrevious = new QPushButton(QStringLiteral("←"), page);
    m_analysisPrevious->setObjectName(QStringLiteral("secondaryButton"));
    m_analysisPrevious->setAccessibleName(tr("Vai al periodo precedente"));
    m_analysisPrevious->setToolTip(tr("Vai al periodo precedente di un mese"));
    m_analysisNext = new QPushButton(QStringLiteral("→"), page);
    m_analysisNext->setObjectName(QStringLiteral("secondaryButton"));
    m_analysisNext->setAccessibleName(tr("Vai al periodo successivo"));
    m_analysisNext->setToolTip(tr("Vai al periodo successivo di un mese"));
    m_analysisPrevious->setFixedWidth(42);
    m_analysisNext->setFixedWidth(42);
    header->addWidget(m_analysisPrevious);
    header->addWidget(m_analysisNext);
    layout->addLayout(header);

    auto *subtitle = new QLabel(
        tr("Entrate, uscite e tendenze del conto selezionato, con dettagli per categoria."),
        page);
    subtitle->setObjectName(QStringLiteral("subTitle"));
    subtitle->setTextInteractionFlags(Qt::NoTextInteraction);
    layout->addWidget(subtitle);

    m_analysisDateRange = new QLabel(page);
    m_analysisDateRange->setObjectName(QStringLiteral("subTitle"));
    m_analysisDateRange->setTextInteractionFlags(Qt::NoTextInteraction);
    layout->addWidget(m_analysisDateRange);

    auto *metrics = new QHBoxLayout;
    metrics->setSpacing(12);
    metrics->addWidget(metricCard(
        tr("Totale uscite"), QStringLiteral("expenses"), &m_analysisTotal, page), 1);
    metrics->addWidget(metricCard(
        tr("Totale entrate"), QStringLiteral("income"), &m_analysisIncome, page), 1);
    metrics->addWidget(metricCard(
        tr("Saldo netto del periodo"), QStringLiteral("balance"), &m_analysisNet, page), 1);
    metrics->addWidget(metricCard(
        tr("Media mensile delle uscite"), QStringLiteral("month"), &m_analysisAverage, page), 1);
    layout->addLayout(metrics);

    auto *insights = new QHBoxLayout;
    insights->setSpacing(12);
    auto addInsight = [page, insights](const QString &title, QLabel **value) {
        auto *card = new QFrame(page);
        card->setObjectName(QStringLiteral("panel"));
        card->setMinimumHeight(76);
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(15, 10, 15, 10);
        cardLayout->setSpacing(5);
        auto *caption = new QLabel(title, card);
        caption->setObjectName(QStringLiteral("subTitle"));
        *value = new QLabel(QStringLiteral("—"), card);
        (*value)->setStyleSheet(QStringLiteral("font-size:12pt;font-weight:700;"));
        (*value)->setTextInteractionFlags(Qt::NoTextInteraction);
        cardLayout->addWidget(caption);
        cardLayout->addWidget(*value);
        insights->addWidget(card, 1);
    };
    addInsight(tr("Categoria con più uscite"), &m_analysisTopCategory);
    addInsight(tr("Confronto con il periodo precedente"), &m_analysisComparison);
    addInsight(tr("Movimenti nel periodo"), &m_analysisActivity);
    layout->addLayout(insights);

    auto *visuals = new QHBoxLayout;
    visuals->setSpacing(16);

    auto *trendPanel = new QFrame(page);
    trendPanel->setObjectName(QStringLiteral("panel"));
    trendPanel->setMinimumHeight(350);
    auto *trendLayout = new QVBoxLayout(trendPanel);
    trendLayout->setContentsMargins(20, 18, 20, 16);
    trendLayout->setSpacing(10);
    m_analysisTrendTitle = new QLabel(
        tr("Entrate e uscite · ultimi 6 mesi"), trendPanel);
    m_analysisTrendTitle->setStyleSheet(QStringLiteral("font-size:14pt;font-weight:700;"));
    trendLayout->addWidget(m_analysisTrendTitle);
    m_expenseTrend = new ExpenseTrendChart(trendPanel);
    trendLayout->addWidget(m_expenseTrend, 1);
    visuals->addWidget(trendPanel, 3);

    auto *panel = new QFrame(page);
    panel->setObjectName(QStringLiteral("panel"));
    panel->setMinimumHeight(350);
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(20, 18, 20, 16);
    panelLayout->setSpacing(10);
    auto *heading = new QLabel(tr("Spese per categoria"), panel);
    heading->setStyleSheet(QStringLiteral("font-size:14pt;font-weight:700;"));
    heading->setTextInteractionFlags(Qt::NoTextInteraction);
    panelLayout->addWidget(heading);

    auto *content = new QHBoxLayout;
    content->setSpacing(12);
    m_expenseDonut = new ExpenseDonutChart(panel);
    m_expenseDonut->setMinimumSize(190, 220);
    content->addWidget(m_expenseDonut, 2);
    m_analysisTable = makeTable(
        {QString(), tr("Categoria"), tr("Totale"), tr("Quota")}, panel);
    m_analysisTable->setObjectName(QStringLiteral("analysisTable"));
    m_analysisTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_analysisTable->setFocusPolicy(Qt::NoFocus);
    m_analysisTable->verticalHeader()->setDefaultSectionSize(44);
    m_analysisTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_analysisTable->setColumnWidth(0, 28);
    m_analysisTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_analysisTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_analysisTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_analysisTable->setMinimumWidth(250);
    content->addWidget(m_analysisTable, 3);
    panelLayout->addLayout(content, 1);

    m_analysisEmpty = new QLabel(
        tr("Non ci sono ancora uscite da riepilogare.\nAggiungi una transazione di tipo Uscita per iniziare."),
        panel);
    m_analysisEmpty->setObjectName(QStringLiteral("subTitle"));
    m_analysisEmpty->setAlignment(Qt::AlignCenter);
    m_analysisEmpty->setTextInteractionFlags(Qt::NoTextInteraction);
    m_analysisEmpty->hide();
    panelLayout->addWidget(m_analysisEmpty);
    visuals->addWidget(panel, 2);
    layout->addLayout(visuals, 1);

    connect(m_analysisPeriod, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index >= 0) {
            const int months = m_analysisPeriod->currentData().toInt();
            if (months > 0) {
                m_analysisMonths = months;
                m_analysisMonthOffset = 0;
                QSettings settings;
                settings.setValue(QStringLiteral("analysis/months"), months);
                settings.sync();
                if (settings.status() != QSettings::NoError)
                    showError(tr("Impossibile salvare il periodo di analisi."));
            }
        }
        refreshAnalysis();
    });
    connect(m_analysisPrevious, &QPushButton::clicked, this, [this] {
        if (m_analysisMonthOffset < 1200) {
            ++m_analysisMonthOffset;
            refreshAnalysis();
        }
    });
    connect(m_analysisNext, &QPushButton::clicked, this, [this] {
        if (m_analysisMonthOffset > 0) {
            --m_analysisMonthOffset;
            refreshAnalysis();
        }
    });
    return scrollArea;
}

QWidget *MainWindow::buildAppearancePage()
{
    auto *scrollArea = new QScrollArea;
    scrollArea->setObjectName(QStringLiteral("appearanceScrollArea"));
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    auto *page = new QWidget;
    scrollArea->setWidget(page);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 26, 30, 24);
    layout->setSpacing(16);
    layout->addWidget(pageTitle(tr("Personalizza l'aspetto"), page));

    auto *subtitle = new QLabel(
        tr("Scegli un tema e un colore principale. Le modifiche si applicano subito e vengono salvate su questo PC."),
        page);
    subtitle->setObjectName(QStringLiteral("subTitle"));
    subtitle->setWordWrap(true);
    subtitle->setTextInteractionFlags(Qt::NoTextInteraction);
    layout->addWidget(subtitle);

    auto *settingsPanel = new QFrame(page);
    settingsPanel->setObjectName(QStringLiteral("panel"));
    auto *settingsLayout = new QFormLayout(settingsPanel);
    settingsLayout->setContentsMargins(22, 20, 22, 20);
    settingsLayout->setHorizontalSpacing(22);
    settingsLayout->setVerticalSpacing(18);
    auto *modeDescription = new QLabel(
        tr("Regola lo sfondo generale e il contrasto dell'interfaccia."),
        settingsPanel);
    modeDescription->setObjectName(QStringLiteral("subTitle"));
    modeDescription->setTextInteractionFlags(Qt::NoTextInteraction);
    m_themeMode = new QComboBox(settingsPanel);
    polishComboPopup(m_themeMode);
    m_themeMode->addItem(tr("Scuro"), QStringLiteral("dark"));
    m_themeMode->addItem(tr("Chiaro"), QStringLiteral("light"));
    m_themeMode->setCurrentIndex(
        qMax(0, m_themeMode->findData(m_themeModeKey)));
    auto *modeField = new QWidget(settingsPanel);
    auto *modeLayout = new QVBoxLayout(modeField);
    modeLayout->setContentsMargins(0, 0, 0, 0);
    modeLayout->setSpacing(6);
    modeLayout->addWidget(m_themeMode);
    modeLayout->addWidget(modeDescription);
    settingsLayout->addRow(tr("Tema"), modeField);

    auto *languageDescription = new QLabel(
        tr("La lingua scelta viene applicata al prossimo avvio."),
        settingsPanel);
    languageDescription->setObjectName(QStringLiteral("subTitle"));
    languageDescription->setTextInteractionFlags(Qt::NoTextInteraction);
    m_language = new QComboBox(settingsPanel);
    polishComboPopup(m_language);
    m_language->addItem(tr("Italiano"), QStringLiteral("it"));
    m_language->addItem(tr("Inglese"), QStringLiteral("en"));
    m_language->setCurrentIndex(qMax(0, m_language->findData(m_languageKey)));
    auto *languageField = new QWidget(settingsPanel);
    auto *languageLayout = new QVBoxLayout(languageField);
    languageLayout->setContentsMargins(0, 0, 0, 0);
    languageLayout->setSpacing(6);
    languageLayout->addWidget(m_language);
    languageLayout->addWidget(languageDescription);
    settingsLayout->addRow(tr("Lingua"), languageField);

    auto *paletteDescription = new QLabel(
        tr("Scegli una palette: aggiorna subito pulsanti, navigazione e grafici."),
        settingsPanel);
    paletteDescription->setObjectName(QStringLiteral("subTitle"));
    paletteDescription->setTextInteractionFlags(Qt::NoTextInteraction);
    auto *paletteField = new QWidget(settingsPanel);
    auto *paletteLayout = new QVBoxLayout(paletteField);
    paletteLayout->setContentsMargins(0, 0, 0, 0);
    paletteLayout->setSpacing(10);
    auto *paletteOptionsLayout = new QHBoxLayout;
    paletteOptionsLayout->setContentsMargins(0, 0, 0, 0);
    paletteOptionsLayout->setSpacing(10);
    const QList<QPair<QString, QString>> palettes = {
        {tr("Oceano"), QStringLiteral("ocean")},
        {tr("Smeraldo"), QStringLiteral("emerald")},
        {tr("Viola"), QStringLiteral("violet")},
        {tr("Corallo"), QStringLiteral("coral")}
    };
    m_paletteOptions.clear();
    for (const auto &palette : palettes) {
        auto *option = new QPushButton(palette.first, paletteField);
        option->setObjectName(QStringLiteral("paletteOption"));
        option->setProperty("paletteKey", palette.second);
        option->setCheckable(true);
        option->setCursor(Qt::PointingHandCursor);
        option->setMinimumHeight(48);
        option->setIcon(paletteIcon(
            palette.second, m_themeModeKey == QStringLiteral("light")));
        option->setIconSize(QSize(60, 16));
        m_paletteOptions.append(option);
        paletteOptionsLayout->addWidget(option, 1);
        connect(option, &QPushButton::clicked, this, [this, key = palette.second] {
            m_themePaletteKey = key;
            applyAppearance();
            saveAppearance();
        });
    }
    paletteLayout->addLayout(paletteOptionsLayout);
    paletteLayout->addWidget(paletteDescription);
    settingsLayout->addRow(tr("Colore principale"), paletteField);
    layout->addWidget(settingsPanel);

    auto *previewPanel = new QFrame(page);
    previewPanel->setObjectName(QStringLiteral("panel"));
    auto *previewLayout = new QVBoxLayout(previewPanel);
    previewLayout->setContentsMargins(22, 20, 22, 20);
    previewLayout->setSpacing(14);
    auto *previewHeading = new QLabel(tr("Anteprima"), previewPanel);
    previewHeading->setStyleSheet(QStringLiteral("font-size:14pt;font-weight:700;"));
    previewHeading->setTextInteractionFlags(Qt::NoTextInteraction);
    previewLayout->addWidget(previewHeading);
    m_appearancePreview = new QLabel(tr("Colore principale"), previewPanel);
    m_appearancePreview->setAlignment(Qt::AlignCenter);
    m_appearancePreview->setMinimumHeight(56);
    m_appearancePreview->setTextInteractionFlags(Qt::NoTextInteraction);
    previewLayout->addWidget(m_appearancePreview);
    auto *sampleButtons = new QHBoxLayout;
    auto *primaryPreview = new QPushButton(tr("Pulsante principale"), previewPanel);
    primaryPreview->setObjectName(QStringLiteral("primaryButton"));
    primaryPreview->setEnabled(false);
    auto *secondaryPreview = new QPushButton(tr("Pulsante secondario"), previewPanel);
    secondaryPreview->setObjectName(QStringLiteral("secondaryButton"));
    secondaryPreview->setEnabled(false);
    sampleButtons->addWidget(primaryPreview);
    sampleButtons->addWidget(secondaryPreview);
    sampleButtons->addStretch();
    previewLayout->addLayout(sampleButtons);
    auto *legendHeading = new QLabel(tr("Cosa significano i colori"), previewPanel);
    legendHeading->setStyleSheet(QStringLiteral("font-weight:700;"));
    legendHeading->setTextInteractionFlags(Qt::NoTextInteraction);
    previewLayout->addWidget(legendHeading);

    auto *legendLayout = new QGridLayout;
    legendLayout->setContentsMargins(0, 0, 0, 0);
    legendLayout->setHorizontalSpacing(10);
    legendLayout->setVerticalSpacing(6);
    const QList<QStringList> legend = {
        {QStringLiteral("appearanceLegendMain"), tr("Aa"),
         tr("Testo principale: titoli, categorie e importi.")},
        {QStringLiteral("appearanceLegendMuted"), tr("Aa"),
         tr("Testo secondario: descrizioni e suggerimenti.")},
        {QStringLiteral("appearanceLegendIncome"), tr("+"),
         tr("Entrate: denaro che ricevi.")},
        {QStringLiteral("appearanceLegendExpense"), tr("−"),
         tr("Uscite: spese e pagamenti.")},
        {QStringLiteral("appearanceLegendAccent"), tr("●"),
         tr("Colore scelto: pulsanti, selezione e grafico.")}
    };
    for (int row = 0; row < legend.size(); ++row) {
        const QStringList &entry = legend.at(row);
        auto *sample = new QLabel(entry.at(1), previewPanel);
        sample->setObjectName(entry.at(0));
        sample->setAlignment(Qt::AlignCenter);
        sample->setMinimumWidth(38);
        sample->setTextInteractionFlags(Qt::NoTextInteraction);
        auto *description = new QLabel(entry.at(2), previewPanel);
        description->setObjectName(QStringLiteral("subTitle"));
        description->setTextInteractionFlags(Qt::NoTextInteraction);
        legendLayout->addWidget(sample, row, 0, Qt::AlignLeft);
        legendLayout->addWidget(description, row, 1);
    }
    previewLayout->addLayout(legendLayout);
    layout->addWidget(previewPanel);
    layout->addStretch();

    connect(m_themeMode, &QComboBox::currentIndexChanged, this, [this] {
        m_themeModeKey = m_themeMode->currentData().toString();
        applyAppearance();
        saveAppearance();
    });
    connect(m_language, &QComboBox::currentIndexChanged, this, [this] {
        const QString requestedLanguage = m_language->currentData().toString();
        if (requestedLanguage == m_languageKey)
            return;
        QSettings settings;
        settings.setValue(QStringLiteral("appearance/language"), requestedLanguage);
        settings.sync();
        if (settings.status() != QSettings::NoError) {
            showError(tr("Impossibile salvare la lingua selezionata."));
            m_language->setCurrentIndex(
                qMax(0, m_language->findData(m_languageKey)));
            return;
        }
        m_languageKey = requestedLanguage;
        statusBar()->showMessage(
            tr("Lingua salvata. Riavvia l'app per applicare la modifica."), 8000);
    });
    return scrollArea;
}

TransactionFilter MainWindow::currentFilter() const
{
    TransactionFilter filter;
    if (m_filterDateEnabled->isChecked()) {
        filter.dateFrom = m_filterFrom->date().toString(Qt::ISODate);
        filter.dateTo = m_filterTo->date().toString(Qt::ISODate);
    }
    filter.type = m_filterType->currentData().toString();
    filter.categoryId = m_filterCategory->currentData().toInt();
    filter.minimumAmount = qRound64(m_filterMin->value() * 100.0);
    filter.maximumAmount = qRound64(m_filterMax->value() * 100.0);
    return filter;
}

QString MainWindow::categoryName(int categoryId) const
{
    for (const QVariantMap &category : m_categories) {
        if (category.value(QStringLiteral("ID_Category")).toInt() == categoryId)
            return translatedCategoryName(
                category.value(QStringLiteral("Name")).toString());
    }
    return tr("Senza categoria");
}

QString MainWindow::money(qint64 cents) const
{
    const bool negative = cents < 0;
    const quint64 magnitude = negative
        ? static_cast<quint64>(-(cents + 1)) + 1
        : static_cast<quint64>(cents);
    const QString digits = QString::number(magnitude / 100);
    QString grouped;
    for (int i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0)
            grouped += QLatin1Char('.');
        grouped += digits.at(i);
    }
    return QStringLiteral("%1%2,%3 €")
        .arg(negative ? QStringLiteral("-") : QString())
        .arg(grouped)
        .arg(magnitude % 100, 2, 10, QLatin1Char('0'));
}

void MainWindow::refreshAll()
{
    refreshAccounts();
    if (!m_manager.getCategories(m_categories)) {
        showError(m_manager.lastError());
        return;
    }
    if (m_filterCategory) {
        const int selected = m_filterCategory->currentData().toInt();
        m_filterCategory->blockSignals(true);
        m_filterCategory->clear();
        m_filterCategory->addItem(tr("Tutte le categorie"), 0);
        for (const QVariantMap &category : m_categories)
            m_filterCategory->addItem(category.value(QStringLiteral("Name")).toString(),
                                      category.value(QStringLiteral("ID_Category")).toInt());
        const int index = m_filterCategory->findData(selected);
        m_filterCategory->setCurrentIndex(index < 0 ? 0 : index);
        m_filterCategory->blockSignals(false);
    }
    refreshDashboard();
    refreshTransactions();
    refreshAnalysis();
    refreshCategories();
}

void MainWindow::refreshDashboard()
{
    if (!m_recentTable || !m_dashboardAccountsTable || !m_dashboardTrend)
        return;

    QList<Transaction> all;
    if (!m_manager.getTransactions(m_accountId, all)) {
        showError(m_manager.lastError());
        return;
    }
    auto addChecked = [](qint64 &total, qint64 amount) {
        if (amount > std::numeric_limits<qint64>::max() - total)
            return false;
        total += amount;
        return true;
    };

    const QDate today = QDate::currentDate();
    const QDate monthStartDate(today.year(), today.month(), 1);
    const QDate firstTrendMonth = monthStartDate.addMonths(-5);
    qint64 monthIncome = 0;
    qint64 monthExpenses = 0;
    QList<qint64> monthlyIncome(6, 0);
    QList<qint64> monthlyExpenses(6, 0);
    QStringList monthLabels;
    for (int index = 0; index < 6; ++index) {
        monthLabels.append(
            firstTrendMonth.addMonths(index).toString(QStringLiteral("MMM")));
    }

    QList<Transaction> recent;
    for (const Transaction &transaction : all) {
        const QDate date = QDate::fromString(transaction.getDate(), Qt::ISODate);
        if (!date.isValid())
            continue;
        if (date <= today)
            recent.append(transaction);

        if (date >= monthStartDate && date <= today) {
            qint64 &monthTotal = transaction.isIncome() ? monthIncome : monthExpenses;
            if (!addChecked(monthTotal, transaction.getAmount())) {
                showError(tr("Il totale mensile supera l'intervallo supportato."));
                return;
            }
        }

        const int monthIndex = (date.year() - firstTrendMonth.year()) * 12 +
                               date.month() - firstTrendMonth.month();
        if (date <= today && monthIndex >= 0 && monthIndex < 6) {
            qint64 &monthlyTotal = transaction.isIncome()
                ? monthlyIncome[monthIndex]
                : monthlyExpenses[monthIndex];
            if (!addChecked(monthlyTotal, transaction.getAmount())) {
                showError(tr("Il totale mensile supera l'intervallo supportato."));
                return;
            }
        }
    }
    qint64 balance = 0;
    if (!m_manager.getAccountBalance(m_accountId, balance)) {
        showError(m_manager.lastError());
        return;
    }

    QList<QVariantMap> accounts;
    if (!m_manager.getAccounts(accounts)) {
        showError(tr("Impossibile caricare i saldi dei conti: %1")
                      .arg(m_manager.lastError()));
        return;
    }
    std::stable_sort(accounts.begin(), accounts.end(),
                     [](const QVariantMap &a, const QVariantMap &b) {
        const QString aType = a.value(QStringLiteral("Type")).toString();
        const QString bType = b.value(QStringLiteral("Type")).toString();
        if (aType != bType)
            return aType < bType;
        return a.value(QStringLiteral("Name")).toString().toCaseFolded() <
               b.value(QStringLiteral("Name")).toString().toCaseFolded();
    });
    qint64 combinedBalance = 0;
    m_dashboardAccountsTable->setRowCount(accounts.size());
    for (int row = 0; row < accounts.size(); ++row) {
        const QVariantMap &account = accounts.at(row);
        const int accountId = account.value(QStringLiteral("ID_Account")).toInt();
        const QString name = account.value(QStringLiteral("Name")).toString();
        const QString type = account.value(QStringLiteral("Type")).toString();
        const qint64 accountBalance =
            account.value(QStringLiteral("Balance")).toLongLong();
        if ((accountBalance > 0 &&
             combinedBalance > std::numeric_limits<qint64>::max() - accountBalance) ||
            (accountBalance < 0 &&
             combinedBalance < std::numeric_limits<qint64>::min() - accountBalance)) {
            showError(tr("Il saldo complessivo dei conti supera l'intervallo supportato."));
            return;
        }
        combinedBalance += accountBalance;

        auto *accountName = cell(name);
        accountName->setData(Qt::UserRole, accountId);
        auto *accountTypeCell = cell(accountTypeLabel(type));
        accountTypeCell->setIcon(accountTypeIcon(type));
        accountTypeCell->setForeground(accountTypeColor(type));
        auto *accountAmount = cell(
            money(accountBalance), Qt::AlignRight | Qt::AlignVCenter);
        if (accountId == m_accountId) {
            const QColor accent = QColor(m_themeModeKey == QStringLiteral("light")
                                             ? QStringLiteral("#6d28d9")
                                             : QStringLiteral("#a78bfa"));
            accountName->setForeground(accent);
            accountAmount->setForeground(accent);
            QFont activeFont = accountName->font();
            activeFont.setWeight(QFont::DemiBold);
            accountName->setFont(activeFont);
            accountAmount->setFont(activeFont);
        }
        m_dashboardAccountsTable->setItem(row, 0, accountName);
        m_dashboardAccountsTable->setItem(row, 1, accountTypeCell);
        m_dashboardAccountsTable->setItem(row, 2, accountAmount);
    }
    m_dashboardTotalBalance->setText(money(combinedBalance));

    m_balanceValue->setText(money(balance));
    const qint64 monthNet = monthIncome - monthExpenses;
    m_incomeValue->setText(money(monthIncome));
    m_expensesValue->setText(money(monthExpenses));
    m_monthValue->setText(money(monthNet));
    const QColor monthNetColor = monthNet > 0
        ? incomeTextColor(m_themeModeKey == QStringLiteral("light"))
        : (monthNet < 0
               ? expenseTextColor(m_themeModeKey == QStringLiteral("light"))
               : palette().color(QPalette::WindowText));
    m_monthValue->setStyleSheet(
        QStringLiteral("color:%1;").arg(monthNetColor.name()));

    const bool lightMode = m_themeModeKey == QStringLiteral("light");
    m_dashboardTrend->setData(
        monthLabels, monthlyIncome, monthlyExpenses,
        incomeTextColor(lightMode), expenseTextColor(lightMode),
        QColor(lightMode ? QStringLiteral("#172033") : QStringLiteral("#f8fafc")),
        QColor(lightMode ? QStringLiteral("#526176") : QStringLiteral("#c0ccda")),
        QColor(lightMode ? QStringLiteral("#d8e0ea") : QStringLiteral("#334155")));

    QString balanceColor = QStringLiteral("#67e8f9");
    if (m_themeModeKey == QStringLiteral("light")) {
        balanceColor = balance < 0 ? QStringLiteral("#b91c1c")
                                   : QStringLiteral("#0e7490");
        if (balance >= 0 && m_themePaletteKey == QStringLiteral("emerald"))
            balanceColor = QStringLiteral("#047857");
        else if (balance >= 0 && m_themePaletteKey == QStringLiteral("violet"))
            balanceColor = QStringLiteral("#6d28d9");
        else if (balance >= 0 && m_themePaletteKey == QStringLiteral("coral"))
            balanceColor = QStringLiteral("#be123c");
    } else if (balance < 0) {
        balanceColor = QStringLiteral("#fb7185");
    }
    m_balanceValue->setStyleSheet(QStringLiteral("color:%1;").arg(balanceColor));

    std::sort(recent.begin(), recent.end(), [](const Transaction &a, const Transaction &b) {
        return a.getDate() > b.getDate() ||
               (a.getDate() == b.getDate() && a.getId() > b.getId());
    });
    m_recentTable->setRowCount(qMin(6, static_cast<int>(recent.size())));
    for (int row = 0; row < m_recentTable->rowCount(); ++row) {
        const Transaction &t = recent.at(row);
        m_recentTable->setItem(row, 0, cell(QDate::fromString(t.getDate(), Qt::ISODate).toString(QStringLiteral("dd/MM/yyyy"))));
        m_recentTable->setItem(row, 1, cell(categoryName(t.getCategoryId())));
        m_recentTable->setItem(
            row, 2, transactionTypeCell(
                        t.isIncome(), m_themeModeKey == QStringLiteral("light")));
        auto *amount = cell((t.isIncome() ? QStringLiteral("+") : QStringLiteral("−")) + money(t.getAmount()),
                            Qt::AlignVCenter | Qt::AlignRight);
        amount->setForeground(t.isIncome()
                                   ? incomeTextColor(m_themeModeKey == QStringLiteral("light"))
                                   : expenseTextColor(m_themeModeKey == QStringLiteral("light")));
        m_recentTable->setItem(row, 3, amount);
    }
}

void MainWindow::refreshTransactions()
{
    if (!m_transactionsTable)
        return;
    if (!m_manager.getTransactions(
            m_accountId, m_filteredTransactions, currentFilter())) {
        showError(m_manager.lastError());
        return;
    }
    std::sort(m_filteredTransactions.begin(), m_filteredTransactions.end(),
              [this](const Transaction &a, const Transaction &b) {
        const QString order = m_sortOrder->currentData().toString();
        if (order == QStringLiteral("highest") && a.getAmount() != b.getAmount())
            return a.getAmount() > b.getAmount();
        if (order == QStringLiteral("lowest") && a.getAmount() != b.getAmount())
            return a.getAmount() < b.getAmount();
        const bool newestFirst = order != QStringLiteral("oldest");
        return newestFirst
            ? (a.getDate() > b.getDate() ||
               (a.getDate() == b.getDate() && a.getId() > b.getId()))
            : (a.getDate() < b.getDate() ||
               (a.getDate() == b.getDate() && a.getId() < b.getId()));
    });
    renderTransactionsPage();
}

void MainWindow::renderTransactionsPage()
{
    if (!m_transactionsTable || !m_pageSize)
        return;

    m_visibleTransactions.clear();
    const QString search = m_transactionSearch
        ? m_transactionSearch->text().trimmed()
        : QString();
    for (const Transaction &transaction : m_filteredTransactions) {
        const QString type = transaction.isIncome()
            ? tr("Entrata")
            : tr("Uscita");
        const QString amount = money(transaction.getAmount());
        const QString localizedAmount = QLocale().toString(
            static_cast<double>(transaction.getAmount()) / 100.0, 'f', 2);
        const QString searchable = QStringLiteral("%1 %2 %3 %4 %5 %6 %7 %8 %9")
            .arg(categoryName(transaction.getCategoryId()),
                 type,
                 transaction.getDate(),
                 QDate::fromString(transaction.getDate(), Qt::ISODate)
                     .toString(QStringLiteral("dd/MM/yyyy")),
                 amount,
                 localizedAmount,
                 QString::number(static_cast<double>(transaction.getAmount()) / 100.0, 'f', 2),
                 QString::number(transaction.getId()),
                 transaction.getNotes());
        if (search.isEmpty() || searchable.contains(search, Qt::CaseInsensitive))
            m_visibleTransactions.append(transaction);
    }

    const int pageSize = qMax(1, m_pageSize->currentData().toInt());
    const int pageCount = m_visibleTransactions.isEmpty()
        ? 1
        : (m_visibleTransactions.size() - 1) / pageSize + 1;
    m_transactionPage = qBound(0, m_transactionPage, pageCount - 1);
    const int firstIndex = m_transactionPage * pageSize;
    const int lastIndex = qMin(firstIndex + pageSize, m_visibleTransactions.size());

    qint64 incomeTotal = 0;
    qint64 expenseTotal = 0;
    for (const Transaction &transaction : m_visibleTransactions) {
        qint64 &total = transaction.isIncome() ? incomeTotal : expenseTotal;
        if (transaction.getAmount() > std::numeric_limits<qint64>::max() - total) {
            showError(tr("Il totale dei risultati supera l'intervallo supportato."));
            return;
        }
        total += transaction.getAmount();
    }
    m_filteredIncome->setText(money(incomeTotal));
    m_filteredExpenses->setText(money(expenseTotal));
    m_filteredBalance->setText(money(incomeTotal - expenseTotal));

    m_transactionsTable->setVisible(!m_visibleTransactions.isEmpty());
    m_transactionEmpty->setVisible(m_visibleTransactions.isEmpty());
    m_transactionsTable->setRowCount(qMax(0, lastIndex - firstIndex));
    for (int row = 0; row < m_transactionsTable->rowCount(); ++row) {
        const Transaction &t = m_visibleTransactions.at(firstIndex + row);
        auto *date = cell(QDate::fromString(t.getDate(), Qt::ISODate)
                              .toString(QStringLiteral("dd MMM yyyy")));
        date->setForeground(QColor(m_themeModeKey == QStringLiteral("light")
                                       ? QStringLiteral("#526176")
                                       : QStringLiteral("#c0ccda")));
        m_transactionsTable->setItem(row, 0, date);
        auto *category = cell(categoryName(t.getCategoryId()));
        QFont categoryFont = category->font();
        categoryFont.setWeight(QFont::DemiBold);
        category->setFont(categoryFont);
        m_transactionsTable->setItem(row, 1, category);
        m_transactionsTable->setItem(
            row, 2, transactionTypeCell(
                        t.isIncome(), m_themeModeKey == QStringLiteral("light")));
        auto *amount = cell((t.isIncome() ? QStringLiteral("+") : QStringLiteral("−")) + money(t.getAmount()),
                            Qt::AlignVCenter | Qt::AlignRight);
        amount->setForeground(t.isIncome()
                                   ? incomeTextColor(m_themeModeKey == QStringLiteral("light"))
                                   : expenseTextColor(m_themeModeKey == QStringLiteral("light")));
        QFont amountFont = amount->font();
        amountFont.setWeight(QFont::Bold);
        amount->setFont(amountFont);
        m_transactionsTable->setItem(row, 3, amount);
        auto *notes = cell(t.getNotes());
        notes->setToolTip(t.getNotes());
        m_transactionsTable->setItem(row, 4, notes);
        m_transactionsTable->setItem(row, 5, cell(QString::number(t.getId())));
        m_transactionsTable->item(row, 0)->setToolTip(t.getDate());
        m_transactionsTable->item(row, 1)->setToolTip(
            tr("%1 · %2")
                .arg(categoryName(t.getCategoryId()),
                     t.isIncome() ? tr("Entrata") : tr("Uscita")));
        m_transactionsTable->item(row, 3)->setToolTip(
            tr("Importo originale: %1 centesimi").arg(t.getAmount()));
    }

    const int firstDisplayed = m_visibleTransactions.isEmpty() ? 0 : firstIndex + 1;
    m_transactionCount->setText(
        tr("%1–%2 di %3 movimenti")
            .arg(firstDisplayed)
            .arg(lastIndex)
            .arg(m_visibleTransactions.size()));
    m_pageInfo->setText(tr("Pagina %1 di %2").arg(m_transactionPage + 1).arg(pageCount));
    m_previousPage->setEnabled(!m_visibleTransactions.isEmpty() && m_transactionPage > 0);
    m_nextPage->setEnabled(!m_visibleTransactions.isEmpty() && m_transactionPage + 1 < pageCount);
    m_editTransaction->setEnabled(false);
    m_deleteTransaction->setEnabled(false);
}

void MainWindow::refreshCategories()
{
    if (!m_categoriesTable)
        return;
    m_categoriesTable->setRowCount(m_categories.size());
    for (int row = 0; row < m_categories.size(); ++row) {
        const QVariantMap &category = m_categories.at(row);
        auto *name = cell(translatedCategoryName(
            category.value(QStringLiteral("Name")).toString()));
        name->setFlags(Qt::ItemIsEnabled);
        QFont nameFont = name->font();
        nameFont.setWeight(QFont::DemiBold);
        name->setFont(nameFont);
        m_categoriesTable->setItem(row, 0, name);
        const bool income = category.value(QStringLiteral("Type")).toString() == QStringLiteral("in");
        auto *type = cell(income ? tr("Entrate") : tr("Uscite"));
        type->setFlags(Qt::ItemIsEnabled);
        type->setForeground(income
                                ? incomeTextColor(m_themeModeKey == QStringLiteral("light"))
                                : expenseTextColor(m_themeModeKey == QStringLiteral("light")));
        type->setFont(nameFont);
        m_categoriesTable->setItem(row, 1, type);
    }
}

void MainWindow::refreshAnalysis()
{
    if (!m_analysisTable || !m_expenseDonut || !m_analysisPeriod || !m_expenseTrend)
        return;

    QList<Transaction> transactions;
    if (!m_manager.getTransactions(m_accountId, transactions)) {
        showError(m_manager.lastError());
        return;
    }

    const QString accountType =
        m_accountSelector->currentData(Qt::UserRole + 1).toString();
    const bool allTimeAccount =
        accountType == QStringLiteral("cash") ||
        accountType == QStringLiteral("savings");
    m_analysisPrevious->setEnabled(!allTimeAccount && m_analysisMonthOffset < 1200);
    m_analysisNext->setEnabled(!allTimeAccount && m_analysisMonthOffset > 0);
    const int savedMonths = m_analysisPeriod->currentData().toInt();
    m_analysisPeriod->setEnabled(!allTimeAccount);
    if (savedMonths > 0)
        m_analysisMonths = savedMonths;
    const int months = allTimeAccount ? 0 : m_analysisMonths;
    const int selectedPeriodIndex = m_analysisPeriod->findData(
        allTimeAccount ? 0 : months);
    if (selectedPeriodIndex >= 0 &&
        selectedPeriodIndex != m_analysisPeriod->currentIndex()) {
        const QSignalBlocker blocker(m_analysisPeriod);
        m_analysisPeriod->setCurrentIndex(selectedPeriodIndex);
    }
    const int previousIndex = m_analysisPeriod->findData(0);
    if (previousIndex >= 0) {
        auto *model = qobject_cast<QStandardItemModel *>(m_analysisPeriod->model());
        if (model && model->item(previousIndex))
            model->item(previousIndex)->setEnabled(allTimeAccount);
    }

    const QDate today = QDate::currentDate();
    const QDate currentMonthStart(today.year(), today.month(), 1);
    const QDate periodMonthStart =
        currentMonthStart.addMonths(-m_analysisMonthOffset);
    const QDate startDate = months > 0
        ? periodMonthStart.addMonths(1 - months)
        : QDate();
    const QDate periodEnd = months > 0
        ? (m_analysisMonthOffset == 0
               ? today
               : periodMonthStart.addMonths(1).addDays(-1))
        : today;
    QList<Transaction> selectedTransactions;
    selectedTransactions.reserve(transactions.size());
    for (const Transaction &transaction : transactions) {
        const QDate date = QDate::fromString(transaction.getDate(), Qt::ISODate);
        if (date.isValid() &&
            (months == 0 || (date >= startDate && date <= periodEnd)))
            selectedTransactions.append(transaction);
    }

    QHash<int, qint64> totalsByCategory;
    qint64 totalExpenses = 0;
    qint64 totalIncome = 0;
    auto addChecked = [](qint64 &total, qint64 amount) {
        if (amount > std::numeric_limits<qint64>::max() - total)
            return false;
        total += amount;
        return true;
    };
    for (const Transaction &transaction : selectedTransactions) {
        qint64 &total = transaction.isIncome() ? totalIncome : totalExpenses;
        if (!addChecked(total, transaction.getAmount())) {
            showError(tr("Il totale delle transazioni supera l'intervallo supportato."));
            return;
        }
        if (!transaction.isIncome()) {
            qint64 &categoryTotal = totalsByCategory[transaction.getCategoryId()];
            if (!addChecked(categoryTotal, transaction.getAmount())) {
                showError(tr("Il totale delle uscite supera l'intervallo supportato."));
                return;
            }
        }
    }

    QList<QPair<QString, qint64>> segments;
    for (auto it = totalsByCategory.cbegin(); it != totalsByCategory.cend(); ++it)
        segments.append(qMakePair(categoryName(it.key()), it.value()));
    std::sort(segments.begin(), segments.end(), [](const auto &left, const auto &right) {
        return left.second > right.second;
    });

    m_analysisTotal->setText(money(totalExpenses));
    m_analysisIncome->setText(money(totalIncome));
    m_analysisNet->setText(money(totalIncome - totalExpenses));
    QDate earliestDate;
    for (const Transaction &transaction : selectedTransactions) {
        const QDate date = QDate::fromString(transaction.getDate(), Qt::ISODate);
        if (date.isValid() && date <= periodEnd &&
            (!earliestDate.isValid() || date < earliestDate)) {
            earliestDate = date;
        }
    }
    const QDate averageStartDate = months > 0 ? startDate : earliestDate;
    if (months == 0 && !earliestDate.isValid()) {
        m_analysisDateRange->setText(tr("Nessuna transazione registrata."));
        m_analysisAverage->setText(money(0));
    } else {
        m_analysisDateRange->setText(
            months > 0
                ? tr("Periodo analizzato: %1 – %2")
                      .arg(QLocale().toString(startDate, QLocale::LongFormat),
                           QLocale().toString(periodEnd, QLocale::LongFormat))
                : tr("Periodo analizzato: tutte le transazioni (%1 – %2)")
                      .arg(QLocale().toString(earliestDate, QLocale::LongFormat),
                           QLocale().toString(periodEnd, QLocale::LongFormat)));
        const qint64 elapsedDays = averageStartDate.daysTo(periodEnd) + 1;
        const long double elapsedMonths =
            static_cast<long double>(elapsedDays) * 12.0L / 365.2425L;
        const long double monthlyAverage =
            elapsedMonths > 0
                ? static_cast<long double>(totalExpenses) / elapsedMonths
                : 0.0L;
        if (monthlyAverage > std::numeric_limits<qint64>::max() - 1.0L) {
            showError(tr("La media mensile supera l'intervallo supportato."));
            return;
        }
        m_analysisAverage->setText(
            money(static_cast<qint64>(monthlyAverage + 0.5L)));
    }

    if (segments.isEmpty()) {
        m_analysisTopCategory->setText(tr("Nessuna spesa"));
    } else {
        const double topShare = totalExpenses > 0
            ? static_cast<double>(
                  static_cast<long double>(segments.first().second) * 100.0L /
                  totalExpenses)
            : 0.0;
        m_analysisTopCategory->setText(
            QStringLiteral("%1 · %2%")
                .arg(segments.first().first,
                     QLocale().toString(topShare, 'f', 1)));
    }
    if (months == 0) {
        m_analysisComparison->setText(
            tr("Confronto non disponibile per l'intero periodo"));
    } else {
        const QDate previousEnd = startDate.addDays(-1);
        const QDate previousStart = startDate.addMonths(-months);
        qint64 previousExpenses = 0;
        for (const Transaction &transaction : transactions) {
            if (transaction.isIncome())
                continue;
            const QDate date = QDate::fromString(transaction.getDate(), Qt::ISODate);
            if (date >= previousStart && date <= previousEnd &&
                !addChecked(previousExpenses, transaction.getAmount())) {
                showError(tr("Il totale delle uscite supera l'intervallo supportato."));
                return;
            }
        }
        if (previousExpenses == 0) {
            m_analysisComparison->setText(
                totalExpenses > 0 ? tr("Nuove uscite nel periodo")
                                  : tr("Nessuna uscita nei due periodi"));
        } else {
            const long double change =
                (static_cast<long double>(totalExpenses) - previousExpenses) *
                100.0L / previousExpenses;
            const QString sign = change > 0 ? QStringLiteral("+") : QString();
            m_analysisComparison->setText(
                QStringLiteral("%1% · %2")
                .arg(sign + QLocale().toString(static_cast<double>(change), 'f', 1),
                         tr("rispetto al periodo precedente")));
        }
    }
    const int expenseCount = static_cast<int>(std::count_if(
        selectedTransactions.cbegin(), selectedTransactions.cend(),
        [](const Transaction &transaction) { return !transaction.isIncome(); }));
    m_analysisActivity->setText(
        tr("%1 movimenti · %2 uscite")
            .arg(selectedTransactions.size())
            .arg(expenseCount));

    const QDate firstTrendMonth =
        QDate(periodEnd.year(), periodEnd.month(), 1).addMonths(-5);
    const int trendMonthCount =
        (periodEnd.year() - firstTrendMonth.year()) * 12 +
        periodEnd.month() - firstTrendMonth.month() + 1;
    m_analysisTrendTitle->setText(
        tr("Entrate e uscite · 6 mesi fino a %1")
            .arg(QLocale().toString(periodEnd, QStringLiteral("MMMM yyyy"))));
    QStringList monthLabels;
    QList<qint64> monthlyIncome(trendMonthCount, 0);
    QList<qint64> monthlyExpenses(trendMonthCount, 0);
    for (int index = 0; index < trendMonthCount; ++index)
        monthLabels.append(firstTrendMonth.addMonths(index).toString(QStringLiteral("MMM")));
    for (const Transaction &transaction : transactions) {
        const QDate date = QDate::fromString(transaction.getDate(), Qt::ISODate);
        if (!date.isValid() || date < firstTrendMonth || date > periodEnd)
            continue;
        const int index = (date.year() - firstTrendMonth.year()) * 12 +
                          date.month() - firstTrendMonth.month();
        if (index < 0 || index >= trendMonthCount)
            continue;
        qint64 &monthlyTotal = transaction.isIncome()
            ? monthlyIncome[index]
            : monthlyExpenses[index];
        if (!addChecked(monthlyTotal, transaction.getAmount())) {
            showError(tr("Il totale mensile supera l'intervallo supportato."));
            return;
        }
    }
    const bool lightMode = m_themeModeKey == QStringLiteral("light");
    m_expenseTrend->setData(
        monthLabels, monthlyIncome, monthlyExpenses,
        incomeTextColor(lightMode), expenseTextColor(lightMode),
        QColor(lightMode ? QStringLiteral("#172033") : QStringLiteral("#f8fafc")),
        QColor(lightMode ? QStringLiteral("#526176") : QStringLiteral("#c0ccda")),
        QColor(lightMode ? QStringLiteral("#d8e0ea") : QStringLiteral("#334155")));

    m_expenseDonut->setData(segments, totalExpenses);
    const bool hasExpenses = totalExpenses > 0;
    m_expenseDonut->setVisible(hasExpenses);
    m_analysisTable->setVisible(hasExpenses);
    m_analysisEmpty->setVisible(!hasExpenses);
    m_analysisTable->setRowCount(segments.size());
    const QList<QColor> colors = chartColors(
        m_themePaletteKey, m_themeModeKey == QStringLiteral("light"));
    for (int row = 0; row < segments.size(); ++row) {
        const auto &segment = segments.at(row);
        const QColor color = colors.at(row % colors.size());
        auto *swatch = cell(QString());
        swatch->setFlags(Qt::ItemIsEnabled);
        swatch->setBackground(color);
        m_analysisTable->setItem(row, 0, swatch);

        auto *name = cell(segment.first);
        name->setFlags(Qt::ItemIsEnabled);
        m_analysisTable->setItem(row, 1, name);

        auto *amount = cell(money(segment.second), Qt::AlignRight | Qt::AlignVCenter);
        amount->setFlags(Qt::ItemIsEnabled);
        amount->setFont(QFont(font().family(), 10, QFont::DemiBold));
        m_analysisTable->setItem(row, 2, amount);

        const double percent = static_cast<double>(
            static_cast<long double>(segment.second) * 100.0L / totalExpenses);
        auto *share = cell(
            QLocale().toString(percent, 'f', 1) + QLatin1Char('%'),
            Qt::AlignRight | Qt::AlignVCenter);
        share->setFlags(Qt::ItemIsEnabled);
        share->setForeground(color);
        share->setFont(QFont(font().family(), 10, QFont::DemiBold));
        m_analysisTable->setItem(row, 3, share);
    }
}

void MainWindow::addTransaction()
{
    TransactionDialog dialog(m_categories, m_accountId, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!m_manager.addTransaction(dialog.transaction())) {
        showError(tr("Impossibile salvare la transazione: %1").arg(m_manager.lastError()));
        return;
    }
    m_transactionPage = 0;
    refreshAll();
    statusBar()->showMessage(tr("Transazione aggiunta"), 3000);
}

void MainWindow::editTransaction()
{
    const int row = m_transactionsTable ? m_transactionsTable->currentRow() : -1;
    if (row < 0) {
        QMessageBox::information(this, tr("Seleziona una transazione"), tr("Seleziona una riga da modificare."));
        return;
    }
    const int id = m_transactionsTable->item(row, 5)->text().toInt();
    QList<Transaction> transactions;
    if (!m_manager.getTransactions(m_accountId, transactions, currentFilter())) {
        showError(m_manager.lastError());
        return;
    }
    auto it = std::find_if(transactions.begin(), transactions.end(),
                           [id](const Transaction &t) { return t.getId() == id; });
    if (it == transactions.end()) {
        showError(tr("La transazione selezionata non è più disponibile."));
        return;
    }
    TransactionDialog dialog(m_categories, m_accountId, this);
    dialog.setTransaction(*it);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!m_manager.updateTransaction(dialog.transaction())) {
        showError(tr("Impossibile modificare la transazione: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
}

void MainWindow::deleteTransaction()
{
    const int row = m_transactionsTable ? m_transactionsTable->currentRow() : -1;
    if (row < 0) {
        QMessageBox::information(this, tr("Seleziona una transazione"), tr("Seleziona una riga da eliminare."));
        return;
    }
    const int id = m_transactionsTable->item(row, 5)->text().toInt();
    if (QMessageBox::question(this, tr("Conferma eliminazione"),
                              tr("Vuoi eliminare questa transazione? Il saldo verrà aggiornato.")) != QMessageBox::Yes)
        return;
    if (!m_manager.deleteTransaction(id)) {
        showError(tr("Impossibile eliminare la transazione: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
}

void MainWindow::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Esporta transazioni"), QDir::home().filePath(QStringLiteral("transazioni.csv")),
        tr("File CSV (*.csv)"));
    if (path.isEmpty())
        return;
    if (QMessageBox::warning(
            this,
            tr("Esportazione non cifrata"),
            tr("Il file CSV conterrà dati finanziari e note in chiaro. "
               "Chiunque possa leggere il file potrà visualizzarli. Continuare?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    if (!m_manager.exportTransactionsToCsv(m_accountId, path, currentFilter())) {
        showError(tr("Esportazione non riuscita: %1").arg(m_manager.lastError()));
        return;
    }
    statusBar()->showMessage(tr("CSV esportato correttamente"), 4000);
}

void MainWindow::addCategory()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Nuova categoria"),
                                               tr("Nome categoria:"), QLineEdit::Normal,
                                               QString(), &ok).trimmed();
    if (!ok)
        return;
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Nome mancante"), tr("Inserisci un nome per la categoria."));
        return;
    }
    const QStringList types = {tr("Uscita"), tr("Entrata")};
    const QString selected = QInputDialog::getItem(this, tr("Tipo categoria"),
                                                   tr("Questa categoria sarà usata per:"), types, 0, false, &ok);
    if (!ok)
        return;
    const QString type = selected == tr("Entrata") ? QStringLiteral("in") : QStringLiteral("out");
    if (!m_manager.addCategory(name, type)) {
        showError(tr("Impossibile creare la categoria: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    statusBar()->showMessage(tr("Categoria aggiunta"), 3000);
}

void MainWindow::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Expense Tracker"), message);
}

#include "MainWindow.moc"
