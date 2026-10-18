#include "Database.h"
#include "ExpenseManager.h"
#include "Transaction.h"

#include <QCoreApplication>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <iostream>

namespace
{
int g_failures = 0;

void check(bool condition, const char *message, int line = __builtin_LINE())
{
    if (!condition) {
        std::cerr << "line " << line << ": " << message << '\n';
        ++g_failures;
    }
}

void testMigration(const QString &path)
{
    {
        QSqlDatabase old = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("old"));
        old.setDatabaseName(path);
        check(old.open(), "cannot create legacy database");
        QSqlQuery q(old);
        const QStringList statements = {
            "CREATE TABLE Bank_Account (ID_Account INTEGER PRIMARY KEY AUTOINCREMENT, Creation_date TEXT NOT NULL, "
            "Name TEXT NOT NULL, Password_Hash TEXT NOT NULL, Initial_Credit INTEGER NOT NULL, Credit INTEGER NOT NULL)",
            "CREATE TABLE Category (ID_Category INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT NOT NULL, Type TEXT NOT NULL)",
            "CREATE TABLE Transactions (ID_Transaction INTEGER PRIMARY KEY AUTOINCREMENT, Amount INTEGER NOT NULL, "
            "Date TEXT NOT NULL, Type TEXT NOT NULL, ID_Category INTEGER NOT NULL, ID_Account INTEGER NOT NULL)",
            "INSERT INTO Bank_Account VALUES (1, '2026-01-01', 'Old', 'h', 0, 500)",
            "INSERT INTO Category VALUES (1, 'Salary', 'in')",
            "INSERT INTO Transactions VALUES (1, 500, '2026-01-02', 'in', 1, 1)"};
        for (const QString &sql : statements) {
            check(q.exec(sql), "legacy statement failed");
        }
        old.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("old"));

    Database db(path);
    check(db.open(), "migration failed");
    QList<Transaction> list;
    check(db.getTransactions(1, list) && list.size() == 1, "legacy transaction lost");
    check(list.value(0).getNote().isEmpty(), "legacy note should be empty");
    check(db.addTransaction(Transaction(0, 1, 1, QStringLiteral("2026-01-03"), 100, true, QStringLiteral("after"))),
          "add after migration failed");
}

void testNotesAndCsv(const QString &dir)
{
    ExpenseManager m(dir + QStringLiteral("/notes.sqlite"));
    check(m.open(), "open");
    int acc = 0, cat = 0;
    check(m.addAccount(QStringLiteral("A"), QStringLiteral("h"), QStringLiteral("2026-01-01"), 0, &acc), "account");
    check(m.addCategory(QStringLiteral("Food"), QStringLiteral("out"), &cat), "category");
    check(m.addTransaction(Transaction(0, acc, cat, QStringLiteral("2026-03-01"), 900, false,
                                       QStringLiteral("  Pizza, \"bis\"  "))), "add with note");
    QList<Transaction> list;
    check(m.getTransactions(acc, list) && list.size() == 1, "list");
    check(list.first().getNote() == QStringLiteral("Pizza, \"bis\""), "note not trimmed/stored");
    Transaction t = list.first();
    t.setNote(QStringLiteral("Cena"));
    check(m.updateTransaction(t), "update note");
    check(m.getTransactions(acc, list) && list.first().getNote() == QStringLiteral("Cena"), "updated note");
    t.setNote(QString(501, QLatin1Char('x')));
    check(!m.updateTransaction(t), "too long note accepted");
    t.setNote(QStringLiteral("a,\"b\""));
    check(m.updateTransaction(t), "update");
    const QString csv = dir + QStringLiteral("/n.csv");
    check(m.exportTransactionsToCsv(acc, csv), "csv");
    QFile f(csv);
    check(f.open(QIODevice::ReadOnly) && f.readAll().contains(",expense,900,\"a,\"\"b\"\"\"\r\n"), "csv note quoting");
}

void testCategories(const QString &dir)
{
    Database db(dir + QStringLiteral("/cat.sqlite"));
    check(db.open(), "open");
    int acc = 0, food = 0, other = 0, pay = 0;
    db.addAccount(QStringLiteral("A"), QStringLiteral("h"), QStringLiteral("2026-01-01"), 0, &acc);
    check(db.addCategory(QStringLiteral("Food"), QStringLiteral("out"), &food), "add");
    check(!db.addCategory(QStringLiteral(" food "), QStringLiteral("out")), "duplicate accepted");
    check(db.addCategory(QStringLiteral("Food"), QStringLiteral("in"), &pay), "same name other type must work");
    check(db.addCategory(QStringLiteral("Fun"), QStringLiteral("out"), &other), "add 2");
    check(db.updateCategory(other, QStringLiteral("Svago")), "rename");
    check(!db.updateCategory(other, QStringLiteral("FOOD")), "rename to duplicate accepted");
    check(!db.updateCategory(other, QStringLiteral("  ")), "empty rename accepted");
    db.addTransaction(Transaction(0, acc, food, QStringLiteral("2026-01-02"), 10, false));
    int usage = 0;
    check(db.getCategoryUsage(food, usage) && usage == 1, "usage");
    check(!db.deleteCategory(food), "used category deleted");
    check(db.deleteCategory(other), "unused category not deleted");
    check(!db.deleteCategory(other), "deleting twice succeeded");
}

void testInitialCreditAndAggregates(const QString &dir)
{
    Database db(dir + QStringLiteral("/agg.sqlite"));
    check(db.open(), "open");
    int first = -1;
    check(db.getFirstAccountId(first) && first == 0, "no account expected");
    int acc = 0, sal = 0, food = 0, fun = 0;
    db.addAccount(QStringLiteral("A"), QStringLiteral("h"), QStringLiteral("2026-01-01"), 1000, &acc);
    check(db.getFirstAccountId(first) && first == acc, "first account");
    db.addCategory(QStringLiteral("Salary"), QStringLiteral("in"), &sal);
    db.addCategory(QStringLiteral("Food"), QStringLiteral("out"), &food);
    db.addCategory(QStringLiteral("Fun"), QStringLiteral("out"), &fun);
    auto add = [&](const char *d, qint64 a, bool in, int c) {
        check(db.addTransaction(Transaction(0, acc, c, QString::fromLatin1(d), a, in)), "add");
    };
    add("2026-01-05", 500, true, sal);
    add("2026-01-20", 200, false, food);
    add("2026-02-03", 300, false, fun);
    add("2026-02-10", 700, false, food);
    add("2026-02-11", 100, true, sal);

    qint64 balance = 0;
    check(db.getAccountBalance(acc, balance) && balance == 1000 + 600 - 1200, "balance");
    check(db.setAccountInitialCredit(acc, 2000) && db.getAccountBalance(acc, balance) && balance == 1400,
          "initial credit must shift the balance");
    qint64 initial = 0;
    check(db.getAccountInitialCredit(acc, initial) && initial == 2000, "initial credit");
    check(!db.setAccountInitialCredit(acc, -1), "negative initial credit accepted");
    check(!db.setAccountInitialCredit(999, 5), "unknown account accepted");

    qint64 in = 0, out = 0;
    check(db.getTotals(acc, in, out) && in == 600 && out == 1200, "totals");
    TransactionFilter feb;
    feb.dateFrom = QStringLiteral("2026-02-01");
    feb.dateTo = QStringLiteral("2026-02-28");
    check(db.getTotals(acc, in, out, feb) && in == 100 && out == 1000, "filtered totals");
    TransactionFilter bad;
    bad.dateFrom = QStringLiteral("2026-03-01");
    bad.dateTo = QStringLiteral("2026-02-01");
    check(!db.getTotals(acc, in, out, bad), "invalid filter accepted");

    QList<CategoryTotal> byCat;
    check(db.getExpensesByCategory(acc, TransactionFilter{}, byCat) && byCat.size() == 2, "by category size");
    check(byCat.value(0).categoryId == food && byCat.value(0).amount == 900, "by category order/amount");

    QList<MonthlyTotal> months;
    check(db.getMonthlyTotals(acc, QStringLiteral("2026-01-01"), months) && months.size() == 2, "months");
    check(months.value(0).month == QStringLiteral("2026-01") && months.value(0).income == 500 &&
              months.value(0).expenses == 200, "january");
    check(months.value(1).income == 100 && months.value(1).expenses == 1000, "february");
    check(db.getMonthlyTotals(acc, QStringLiteral("2026-02-01"), months) && months.size() == 1, "months from");

    QList<Transaction> recent;
    check(db.getRecentTransactions(acc, 2, recent) && recent.size() == 2, "recent size");
    check(recent.value(0).getDate() == QStringLiteral("2026-02-11"), "recent order");
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QTemporaryDir dir;
    check(dir.isValid(), "temp dir");
    testMigration(dir.filePath(QStringLiteral("legacy.sqlite")));
    testNotesAndCsv(dir.path());
    testCategories(dir.path());
    testInitialCreditAndAggregates(dir.path());
    return g_failures == 0 ? 0 : 1;
}
