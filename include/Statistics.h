#pragma once

#include "Transaction.h"

#include <QList>

class Statistics
{
private:
    qint64 m_totalIncome = 0;
    qint64 m_totalExpenses = 0;

public:
    Statistics() = default;

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