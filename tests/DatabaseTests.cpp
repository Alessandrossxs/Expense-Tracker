#include "Database.h"
#include "Transaction.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>

namespace
{
bool expect(bool condition, const QString &message)
{
    if (!condition) {
        std::cerr << message.toStdString() << '\n';
        return false;
    }
    return true;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QTemporaryDir temporaryDirectory;
    if (!expect(temporaryDirectory.isValid(), QStringLiteral("Could not create temporary directory"))) {
        return 1;
    }

    Database database(temporaryDirectory.filePath(QStringLiteral("database-test.sqlite")));
    if (!expect(database.open(), database.lastError())) {
        return 1;
    }

    int firstAccountId = 0;
    int secondAccountId = 0;
    if (!expect(
            database.addAccount(
                QStringLiteral("First account"),
                QStringLiteral("test-hash"),
                QStringLiteral("2026-01-01"),
                10000,
                &firstAccountId),
            database.lastError()) ||
        !expect(
            database.addAccount(
                QStringLiteral("Second account"),
                QStringLiteral("test-hash"),
                QStringLiteral("2026-01-01"),
                500,
                &secondAccountId),
            database.lastError())) {
        return 1;
    }

    int incomeCategoryId = 0;
    int expenseCategoryId = 0;
    if (!expect(
            database.addCategory(QStringLiteral("Salary"), QStringLiteral("in"), &incomeCategoryId),
            database.lastError()) ||
        !expect(
            database.addCategory(QStringLiteral("Food"), QStringLiteral("out"), &expenseCategoryId),
            database.lastError())) {
        return 1;
    }

    const Transaction income(
        0, firstAccountId, incomeCategoryId, QStringLiteral("2026-01-02"), 2000, true);
    const Transaction expense(
        0, firstAccountId, expenseCategoryId, QStringLiteral("2026-01-03"), 3000, false);
    if (!expect(database.addTransaction(income), database.lastError()) ||
        !expect(database.addTransaction(expense), database.lastError())) {
        return 1;
    }

    qint64 firstBalance = 0;
    qint64 secondBalance = 0;
    if (!expect(database.getAccountBalance(firstAccountId, firstBalance), database.lastError()) ||
        !expect(firstBalance == 9000, QStringLiteral("Income and expense balances are incorrect"))) {
        return 1;
    }

    const Transaction invalidTransaction(
        0, firstAccountId, expenseCategoryId, QStringLiteral("2026-01-04"), 100, true);
    if (!expect(
            !database.addTransaction(invalidTransaction),
            QStringLiteral("Mismatched category type was accepted")) ||
        !expect(database.getAccountBalance(firstAccountId, firstBalance), database.lastError()) ||
        !expect(firstBalance == 9000, QStringLiteral("Rejected transaction changed the balance"))) {
        return 1;
    }

    QList<Transaction> transactions;
    if (!expect(
            database.getTransactions(firstAccountId, transactions),
            database.lastError()) ||
        !expect(transactions.size() == 2, QStringLiteral("Unexpected transaction count"))) {
        return 1;
    }

    Transaction updatedExpense = transactions.at(1);
    updatedExpense.setAccountId(secondAccountId);
    updatedExpense.setAmount(600);
    if (!expect(database.updateTransaction(updatedExpense), database.lastError()) ||
        !expect(database.getAccountBalance(firstAccountId, firstBalance), database.lastError()) ||
        !expect(database.getAccountBalance(secondAccountId, secondBalance), database.lastError()) ||
        !expect(firstBalance == 12000, QStringLiteral("Updated source account balance is incorrect")) ||
        !expect(secondBalance == -100, QStringLiteral("Updated destination account balance is incorrect"))) {
        return 1;
    }

    if (!expect(
            database.deleteTransaction(updatedExpense.getId()),
            database.lastError()) ||
        !expect(database.getAccountBalance(secondAccountId, secondBalance), database.lastError()) ||
        !expect(secondBalance == 500, QStringLiteral("Deleted transaction did not restore balance"))) {
        return 1;
    }

    return 0;
}
