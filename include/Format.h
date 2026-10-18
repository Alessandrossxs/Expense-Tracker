#pragma once

#include <QString>

// Formats an amount in cents as "1.234,56 €" (Italian style).
inline QString formatMoney(qint64 cents)
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
