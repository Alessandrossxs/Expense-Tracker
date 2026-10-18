#pragma once

#include "Transaction.h"

#include <QList>
#include <QString>

struct CategoryTotal
{
    int categoryId = 0;
    qint64 amount = 0;
};

struct MonthlyTotal
{
    QString month; ///< "YYYY-MM"
    qint64 income = 0;
    qint64 expenses = 0;
};

class Statistics
{
private:
    qint64 m_totalIncome = 0;
    qint64 m_totalExpenses = 0;

public:
    Statistics() = default;

    /** @brief Builds statistics from already aggregated totals (in cents). */
    static Statistics fromTotals(qint64 totalIncome, qint64 totalExpenses)
    {
        Statistics statistics;
        statistics.m_totalIncome = totalIncome;
        statistics.m_totalExpenses = totalExpenses;
        return statistics;
    }

    /**
     * @throws std::overflow_error if a total exceeds the supported range.
     */
    explicit Statistics(const QList<Transaction> &transactions);

    /**
     * @return Total income in cents.
     */
    qint64 getTotalIncome() const;

    /**
     * @return Total expenses in cents.
     */
    qint64 getTotalExpenses() const;

    /**
     * @return Income minus expenses in cents.
     * @throws std::overflow_error if the result exceeds the supported range.
     */
    qint64 getBalance() const;
};