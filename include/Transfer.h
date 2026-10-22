#pragma once

#include <QString>

// Money moved between two accounts. Stored apart from income/expense
// transactions, so transfers never count as earnings or spending.
struct Transfer
{
    int id = 0;
    int fromAccountId = 0;
    int toAccountId = 0;
    QString date; // YYYY-MM-DD
    qint64 amount = 0; // cents, always positive
    QString note;
};
