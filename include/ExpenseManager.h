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
        int *newId = nullptr);
    bool getAccountBalance(int accountId, qint64 &balance);
    bool getFirstAccountId(int &accountId);
    bool getAccounts(QList<QVariantMap> &accounts);
    bool renameAccount(int accountId, const QString &name);
    bool deleteAccount(int accountId);
    bool addTransfer(int fromAccountId, int toAccountId, qint64 amount, const QString &date,
                     const QString &note, int *newId = nullptr);
    bool deleteTransfer(int transferId);
    bool addRecurring(const RecurringRule &rule, int *newId = nullptr);
    bool updateRecurring(const RecurringRule &rule);
    bool deleteRecurring(int recurringId);
    bool setRecurringActive(int recurringId, bool active, const QString &today);
    bool getRecurring(int accountId, QList<RecurringRule> &rules);
    bool processRecurring(const QString &today, int &created);
    bool getTransfers(int accountId, QList<Transfer> &transfers,
                      const TransactionFilter &filter = TransactionFilter{});
    bool getAccountInitialCredit(int accountId, qint64 &initialCredit);
    bool setAccountInitialCredit(int accountId, qint64 initialCredit);

    bool addCategory(
        const QString &name,
        const QString &type,
        int *newId = nullptr);
    bool getCategories(QList<QVariantMap> &categories);
    bool updateCategory(int categoryId, const QString &name);
    bool deleteCategory(int categoryId);
    bool getCategoryUsage(int categoryId, int &transactionCount);

    bool addTransaction(const Transaction &transaction);
    bool updateTransaction(const Transaction &transaction);
    bool deleteTransaction(int transactionId);
    bool getTransactions(
        int accountId,
        QList<Transaction> &transactions,
        const TransactionFilter &filter = TransactionFilter{});
    bool getRecentTransactions(int accountId, int count, QList<Transaction> &transactions);
    bool getExpensesByCategory(
        int accountId,
        const TransactionFilter &filter,
        QList<CategoryTotal> &totals);
    bool getMonthlyTotals(int accountId, const QString &fromDate, QList<MonthlyTotal> &totals);
    bool getStatistics(
        int accountId,
        Statistics &statistics,
        const TransactionFilter &filter = TransactionFilter{});
    bool exportTransactionsToCsv(
        int accountId,
        const QString &filePath,
        const TransactionFilter &filter = TransactionFilter{});
};