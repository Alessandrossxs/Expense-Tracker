#pragma once

#include "Database.h"
#include "Statistics.h"
#include "Transaction.h"

#include <QList>
#include <QVariant>

class ExpenseManager
{
private:
    Database m_database;
    QString m_lastError;

    bool recordResult(bool success);

public:
    explicit ExpenseManager(const QString &databasePath = QStringLiteral("expense_tracker.db"));

    bool open();
    bool isOpen() const;
    QString lastError() const;

    bool addAccount(
        const QString &name,
        const QString &passwordHash,
        const QString &creationDate,
        qint64 initialCredit,
        int *newId = nullptr,
        const QString &accountType = QStringLiteral("checking"));
    bool getAccounts(QList<QVariantMap> &accounts);
    bool updateAccount(int accountId, const QString &name, const QString &accountType);
    bool deleteAccount(int accountId);
    bool getAccountBalance(int accountId, qint64 &balance);
    bool addTransfer(
        int sourceAccountId,
        int destinationAccountId,
        const QString &date,
        qint64 amount);
    bool getAccountTransfers(int accountId, QList<QVariantMap> &transfers);

    bool addCategory(
        const QString &name,
        const QString &type,
        int *newId = nullptr);
    bool getCategories(QList<QVariantMap> &categories);

    bool addTransaction(const Transaction &transaction);
    bool updateTransaction(const Transaction &transaction);
    bool deleteTransaction(int transactionId);
    bool getTransactions(
        int accountId,
        QList<Transaction> &transactions,
        const TransactionFilter &filter = TransactionFilter{});
    bool getStatistics(
        int accountId,
        Statistics &statistics,
        const TransactionFilter &filter = TransactionFilter{});
    bool exportTransactionsToCsv(
        int accountId,
        const QString &filePath,
        const TransactionFilter &filter = TransactionFilter{});
};