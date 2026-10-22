#pragma once

#include <QString>

// A rule that creates an income or an expense on a schedule.
struct RecurringRule
{
    int id = 0;
    int accountId = 0;
    int categoryId = 0;
    qint64 amount = 0; // cents, always positive
    bool isIncome = false;
    QString note;
    QString frequency = QStringLiteral("monthly"); // daily | weekly | monthly | yearly
    int interval = 1;       // every N days/weeks/months/years
    QString startDate;      // anchor used to compute every occurrence
    QString nextDate;       // next occurrence still to be created
    QString endDate;        // empty = never ends
    int generated = 0;      // occurrences created since startDate
    bool active = true;
};
