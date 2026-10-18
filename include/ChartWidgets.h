#pragma once

#include <QColor>
#include <QList>
#include <QString>
#include <QWidget>

struct DonutSlice
{
    QString label;
    qint64 value = 0;
    QString valueText;
    bool other = false; ///< Drawn in neutral gray (aggregate of small slices).
};

struct BarGroup
{
    QString label;
    qint64 income = 0;
    qint64 expenses = 0;
};

// Ring chart with a legend, painted with QPainter (no extra Qt modules needed).
class DonutChart : public QWidget
{
public:
    explicit DonutChart(QWidget *parent = nullptr);
    void setData(const QList<DonutSlice> &slices, const QString &centerValue,
                 const QString &centerCaption);
    void setEmptyText(const QString &text);
    void setDarkMode(bool dark);
    void setAccent(const QColor &accent);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_accent{QStringLiteral("#6658d3")};
    QList<DonutSlice> m_slices;
    QString m_centerValue;
    QString m_centerCaption;
    QString m_emptyText;
    bool m_dark = false;
};

// Grouped bar chart: income vs expenses for each month.
class MonthlyBarChart : public QWidget
{
public:
    explicit MonthlyBarChart(QWidget *parent = nullptr);
    void setData(const QList<BarGroup> &groups, const QString &maxLabel);
    void setEmptyText(const QString &text);
    void setDarkMode(bool dark);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<BarGroup> m_groups;
    QString m_maxLabel;
    QString m_emptyText;
    bool m_dark = false;
};
