#include "Database.h"
#include "ExpenseManager.h"
#include "Statistics.h"
#include "Transaction.h"

#include <QCoreApplication>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <iostream>
#include <limits>
#include <stdexcept>

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
                &secondAccountId,
                QStringLiteral("savings")),
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

    QList<QVariantMap> accounts;
    QList<QVariantMap> transfers;
    if (!expect(database.getAccounts(accounts), database.lastError()) ||
        !expect(accounts.size() == 2, QStringLiteral("Accounts were not retrieved")) ||
        !expect(accounts.first().value(QStringLiteral("TransactionCount")).toInt() == 1,
                QStringLiteral("Account transaction count is incorrect")) ||
        !expect(accounts.first().value(QStringLiteral("TransferCount")).toInt() == 0,
                QStringLiteral("Account transfer count is incorrect")) ||
        !expect(accounts.at(1).value(QStringLiteral("Type")).toString() ==
                    QStringLiteral("savings"),
                QStringLiteral("Account type was not saved")) ||
        !expect(
            database.addTransfer(
                firstAccountId, secondAccountId, QStringLiteral("2026-01-06"), 1000),
            database.lastError()) ||
        !expect(database.getAccountBalance(firstAccountId, firstBalance), database.lastError()) ||
        !expect(database.getAccountBalance(secondAccountId, secondBalance), database.lastError()) ||
        !expect(firstBalance == 11000, QStringLiteral("Transfer did not debit source account")) ||
        !expect(secondBalance == 1500, QStringLiteral("Transfer did not credit destination account")) ||
        !expect(database.getAccounts(accounts), database.lastError()) ||
        !expect(accounts.first().value(QStringLiteral("TransferCount")).toInt() == 1,
                QStringLiteral("Source account transfer count is incorrect")) ||
        !expect(accounts.at(1).value(QStringLiteral("TransferCount")).toInt() == 1,
                QStringLiteral("Destination account transfer count is incorrect")) ||
        !expect(database.getAccountTransfers(firstAccountId, transfers), database.lastError()) ||
        !expect(transfers.size() == 1, QStringLiteral("Transfer history was not retrieved")) ||
        !expect(transfers.first().value(QStringLiteral("Direction")).toString() ==
                    QStringLiteral("out"),
                QStringLiteral("Outgoing transfer direction is incorrect")) ||
        !expect(transfers.first().value(QStringLiteral("OtherAccount")).toString() ==
                    QStringLiteral("Second account"),
                QStringLiteral("Transfer destination is incorrect")) ||
        !expect(database.getTransactions(firstAccountId, transactions), database.lastError()) ||
        !expect(transactions.size() == 1,
                QStringLiteral("Internal transfer was counted as an income or expense")) ||
        !expect(
            !database.addTransfer(
                firstAccountId, firstAccountId, QStringLiteral("2026-01-06"), 1000),
            QStringLiteral("Self-transfer was accepted"))) {
        return 1;
    }

    if (!expect(
            database.updateAccount(
                secondAccountId,
                QStringLiteral("Rainy day"),
                QStringLiteral("savings")),
            database.lastError()) ||
        !expect(database.getAccounts(accounts), database.lastError()) ||
        !expect(accounts.at(1).value(QStringLiteral("Name")).toString() ==
                    QStringLiteral("Rainy day"),
                QStringLiteral("Account name was not updated")) ||
        !expect(
            !database.updateAccount(
                secondAccountId, QStringLiteral("Invalid"), QStringLiteral("unknown")),
            QStringLiteral("Invalid account type was accepted"))) {
        return 1;
    }

    int emptyAccountId = 0;
    int fundedAccountId = 0;
    if (!expect(
            database.addAccount(
                QStringLiteral("Empty account"),
                QStringLiteral("test-hash"),
                QStringLiteral("2026-01-07"),
                0,
                &emptyAccountId),
            database.lastError()) ||
        !expect(
            database.addAccount(
                QStringLiteral("Funded account"),
                QStringLiteral("test-hash"),
                QStringLiteral("2026-01-07"),
                500,
                &fundedAccountId),
            database.lastError()) ||
        !expect(
            !database.deleteAccount(fundedAccountId),
            QStringLiteral("Account with remaining funds was deleted")) ||
        !expect(database.deleteAccount(emptyAccountId), database.lastError()) ||
        !expect(
            !database.deleteAccount(secondAccountId),
            QStringLiteral("Account with transfer history was deleted")) ||
        !expect(
            !database.deleteAccount(firstAccountId),
            QStringLiteral("Account with transactions was deleted"))) {
        return 1;
    }

    const Statistics statistics({income, expense});
    if (!expect(statistics.getTotalIncome() == 2000, QStringLiteral("Income total is incorrect")) ||
        !expect(statistics.getTotalExpenses() == 3000, QStringLiteral("Expense total is incorrect")) ||
        !expect(statistics.getBalance() == -1000, QStringLiteral("Transaction balance is incorrect"))) {
        return 1;
    }

    const Statistics emptyStatistics(QList<Transaction>{});
    if (!expect(emptyStatistics.getTotalIncome() == 0, QStringLiteral("Empty income total is not zero")) ||
        !expect(emptyStatistics.getTotalExpenses() == 0, QStringLiteral("Empty expense total is not zero")) ||
        !expect(emptyStatistics.getBalance() == 0, QStringLiteral("Empty balance is not zero"))) {
        return 1;
    }

    const QList<Transaction> overflowingTransactions = {
        Transaction(0, firstAccountId, incomeCategoryId, QStringLiteral("2026-01-04"),
                    std::numeric_limits<qint64>::max(), true),
        Transaction(0, firstAccountId, incomeCategoryId, QStringLiteral("2026-01-05"), 1, true)};
    bool overflowDetected = false;
    try {
        const Statistics overflowingStatistics(overflowingTransactions);
        Q_UNUSED(overflowingStatistics);
    } catch (const std::overflow_error &) {
        overflowDetected = true;
    }
    if (!expect(overflowDetected, QStringLiteral("Income total overflow was not reported"))) {
        return 1;
    }

    ExpenseManager manager(temporaryDirectory.filePath(QStringLiteral("manager-test.sqlite")));
    if (!expect(manager.open(), manager.lastError())) {
        return 1;
    }

    int managedAccountId = 0;
    int managedIncomeCategoryId = 0;
    int managedExpenseCategoryId = 0;
    if (!expect(
            manager.addAccount(
                QStringLiteral("Managed account"),
                QStringLiteral("test-hash"),
                QStringLiteral("2026-02-01"),
                10000,
                &managedAccountId),
            manager.lastError()) ||
        !expect(
            manager.addCategory(
                QStringLiteral("Managed salary"),
                QStringLiteral("in"),
                &managedIncomeCategoryId),
            manager.lastError()) ||
        !expect(
            manager.addCategory(
                QStringLiteral("Managed food"),
                QStringLiteral("out"),
                &managedExpenseCategoryId),
            manager.lastError())) {
        return 1;
    }

    if (!expect(
            manager.addTransaction(Transaction(
                0,
                managedAccountId,
                managedIncomeCategoryId,
                QStringLiteral("2026-02-02"),
                3000,
                true)),
            manager.lastError()) ||
        !expect(
            manager.addTransaction(Transaction(
                0,
                managedAccountId,
                managedExpenseCategoryId,
                QStringLiteral("2026-02-03"),
                1500,
                false)),
            manager.lastError())) {
        return 1;
    }

    qint64 managedBalance = 0;
    QList<QVariantMap> managedCategories;
    QList<Transaction> managedTransactions;
    if (!expect(manager.getCategories(managedCategories), manager.lastError()) ||
        !expect(managedCategories.size() == 2, QStringLiteral("Managed categories were not retrieved")) ||
        !expect(
            manager.getTransactions(managedAccountId, managedTransactions),
            manager.lastError()) ||
        !expect(managedTransactions.size() == 2, QStringLiteral("Managed transactions were not retrieved"))) {
        return 1;
    }
    Transaction updatedIncome = managedTransactions.first();
    updatedIncome.setAmount(3500);
    if (!expect(manager.updateTransaction(updatedIncome), manager.lastError()) ||
        !expect(manager.getAccountBalance(managedAccountId, managedBalance), manager.lastError()) ||
        !expect(managedBalance == 12000, QStringLiteral("Managed transaction update did not adjust balance")) ||
        !expect(manager.deleteTransaction(updatedIncome.getId()), manager.lastError()) ||
        !expect(manager.getAccountBalance(managedAccountId, managedBalance), manager.lastError()) ||
        !expect(managedBalance == 8500, QStringLiteral("Managed transaction deletion did not adjust balance"))) {
        return 1;
    }

    TransactionFilter filter;
    filter.dateFrom = QStringLiteral("2026-02-03");
    filter.dateTo = QStringLiteral("2026-02-03");
    filter.type = QStringLiteral("out");
    filter.categoryId = managedExpenseCategoryId;
    filter.minimumAmount = 1000;
    filter.maximumAmount = 2000;
    QList<Transaction> filteredTransactions;
    if (!expect(
            manager.getTransactions(managedAccountId, filteredTransactions, filter),
            manager.lastError()) ||
        !expect(filteredTransactions.size() == 1, QStringLiteral("Transaction filters returned the wrong count")) ||
        !expect(filteredTransactions.first().getAmount() == 1500, QStringLiteral("Transaction filters returned the wrong amount"))) {
        return 1;
    }

    Statistics filteredStatistics;
    if (!expect(
            manager.getStatistics(managedAccountId, filteredStatistics, filter),
            manager.lastError()) ||
        !expect(filteredStatistics.getTotalIncome() == 0, QStringLiteral("Filtered income total is incorrect")) ||
        !expect(filteredStatistics.getTotalExpenses() == 1500, QStringLiteral("Filtered expense total is incorrect")) ||
        !expect(filteredStatistics.getBalance() == -1500, QStringLiteral("Filtered balance is incorrect")) ||
        !expect(manager.getAccountBalance(managedAccountId, managedBalance), manager.lastError()) ||
        !expect(managedBalance == 8500, QStringLiteral("Managed account balance is incorrect"))) {
        return 1;
    }

    const QString csvPath = temporaryDirectory.filePath(QStringLiteral("transactions.csv"));
    if (!expect(
            manager.exportTransactionsToCsv(managedAccountId, csvPath, filter),
            manager.lastError())) {
        return 1;
    }
    QFile csvFile(csvPath);
    if (!expect(csvFile.open(QIODevice::ReadOnly), csvFile.errorString())) {
        return 1;
    }
    const QByteArray csvContents = csvFile.readAll();
    if (!expect(
            csvContents.startsWith(
                QByteArrayLiteral("transaction_id,account_id,category_id,date,type,amount_cents\r\n")),
            QStringLiteral("CSV header is incorrect")) ||
        !expect(csvContents.count('\n') == 2, QStringLiteral("CSV export contains unexpected rows")) ||
        !expect(
            csvContents.contains(QByteArrayLiteral(",\"expense\",\"1500\"\r\n")),
            QStringLiteral("CSV row is incorrect"))) {
        return 1;
    }

    TransactionFilter invalidFilter;
    invalidFilter.dateFrom = QStringLiteral("2026-02-04");
    invalidFilter.dateTo = QStringLiteral("2026-02-03");
    if (!expect(
            !manager.getTransactions(managedAccountId, filteredTransactions, invalidFilter),
            QStringLiteral("Invalid transaction filter was accepted")) ||
        !expect(!manager.lastError().isEmpty(), QStringLiteral("Invalid filter did not report an error"))) {
        return 1;
    }

    invalidFilter = TransactionFilter{};
    invalidFilter.type = QStringLiteral("income");
    if (!expect(
            !manager.getTransactions(managedAccountId, filteredTransactions, invalidFilter),
            QStringLiteral("Invalid transaction type filter was accepted")) ||
        !expect(!manager.lastError().isEmpty(), QStringLiteral("Invalid type filter did not report an error"))) {
        return 1;
    }

    const QString legacyPath =
        temporaryDirectory.filePath(QStringLiteral("legacy-database.sqlite"));
    const QString legacyConnection = QStringLiteral("legacy_fixture_connection");
    {
        QSqlDatabase legacy = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), legacyConnection);
        legacy.setDatabaseName(legacyPath);
        if (!expect(legacy.open(), legacy.lastError().text()))
            return 1;
        {
            QSqlQuery query(legacy);
            if (!expect(query.exec(QStringLiteral(
                    "CREATE TABLE Bank_Account ("
                    "ID_Account INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "Creation_date TEXT NOT NULL, Name TEXT NOT NULL, "
                    "Password_Hash TEXT NOT NULL, Initial_Credit INTEGER NOT NULL, "
                    "Credit INTEGER NOT NULL)")), query.lastError().text()) ||
                !expect(query.exec(QStringLiteral(
                    "INSERT INTO Bank_Account "
                    "(Creation_date, Name, Password_Hash, Initial_Credit, Credit) "
                    "VALUES ('2026-01-01', 'Legacy', 'hash', 1000, 1000)")),
                    query.lastError().text())) {
                return 1;
            }
        }
        legacy.close();
        legacy = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(legacyConnection);
    Database migratedDatabase(legacyPath);
    QList<QVariantMap> migratedAccounts;
    if (!expect(migratedDatabase.open(), migratedDatabase.lastError()) ||
        !expect(migratedDatabase.getAccounts(migratedAccounts), migratedDatabase.lastError()) ||
        !expect(migratedAccounts.size() == 1, QStringLiteral("Legacy account was not preserved")) ||
        !expect(migratedAccounts.first().value(QStringLiteral("Type")).toString() ==
                    QStringLiteral("checking"),
                QStringLiteral("Legacy account type default was not applied"))) {
        return 1;
    }

    Database singleAccountDatabase(
        temporaryDirectory.filePath(QStringLiteral("single-account.sqlite")));
    int singleAccountId = 0;
    if (!expect(singleAccountDatabase.open(), singleAccountDatabase.lastError()) ||
        !expect(
            singleAccountDatabase.addAccount(
                QStringLiteral("Only account"),
                QStringLiteral("test-hash"),
                QStringLiteral("2026-02-01"),
                0,
                &singleAccountId),
            singleAccountDatabase.lastError()) ||
        !expect(
            !singleAccountDatabase.deleteAccount(singleAccountId),
            QStringLiteral("The last remaining account was deleted"))) {
        return 1;
    }

    return 0;
}
