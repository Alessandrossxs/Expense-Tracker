#include <QCoreApplication>
#include <QTranslator>
#include <QString>

#include <iostream>

namespace
{
bool expectEqual(const QString &actual, const QString &expected, const char *message)
{
    if (actual == expected)
        return true;

    std::cerr << message << ": expected \"" << expected.toStdString()
              << "\", got \"" << actual.toStdString() << "\"\n";
    return false;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    if (argc != 3) {
        std::cerr << "Expected English and Italian catalog paths\n";
        return 1;
    }

    QTranslator english;
    if (!english.load(QString::fromLocal8Bit(argv[1])) ||
        !application.installTranslator(&english)) {
        std::cerr << "Could not load English translation catalog\n";
        return 1;
    }
    if (!expectEqual(QCoreApplication::translate("MainWindow", "Panoramica"),
                     QStringLiteral("Dashboard"), "English navigation") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Conto di risparmio"),
                     QStringLiteral("Savings account"), "English account type") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Trasferimenti tra conti"),
                     QStringLiteral("Transfers between accounts"), "English transfers") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Gestisci conti"),
                     QStringLiteral("Manage accounts"), "English account management") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Saldo netto del mese"),
                     QStringLiteral("Net balance this month"), "English dashboard insight") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Entrate del mese"),
                     QStringLiteral("This month's income"), "English monthly dashboard income") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Saldi di tutti i conti"),
                     QStringLiteral("Balances across accounts"), "English account balances") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Note"),
                     QStringLiteral("Notes"), "English transaction notes") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Cerca categoria, note, data, importo o ID…"),
                     QStringLiteral("Search category, notes, date, amount, or ID…"),
                     "English note search placeholder") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Vai a %1 (Ctrl+%2)")
                         .arg(QStringLiteral("Dashboard"))
                         .arg(1),
                     QStringLiteral("Go to Dashboard (Ctrl+1)"),
                     "English navigation shortcut tooltip") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Aggiungi una transazione (Ctrl+N)"),
                     QStringLiteral("Add a transaction (Ctrl+N)"),
                     "English new transaction shortcut tooltip") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "◉  Tutorial"),
                     QStringLiteral("◉  Tutorial"), "English tutorial button") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Passaggio %1 di %2")
                         .arg(2)
                         .arg(5),
                     QStringLiteral("Step 2 of 5"), "English tutorial progress") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Ultimo mese"),
                     QStringLiteral("Last month"), "English analysis period") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Media mensile delle uscite"),
                     QStringLiteral("Average monthly expenses"), "English analysis insight") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Periodo analizzato: %1 – %2"),
                     QStringLiteral("Analysis period: %1 – %2"), "English analysis dates") ||
        !expectEqual(QCoreApplication::translate("CategoryNames", "Stipendio"),
                     QStringLiteral("Salary"), "English category") ||
        !expectEqual(QCoreApplication::translate("TransactionDialog", "Uscita"),
                     QStringLiteral("Expense"), "English transaction type") ||
        !expectEqual(QCoreApplication::translate("TransactionDialog", "Note"),
                     QStringLiteral("Notes"), "English transaction note field")) {
        return 1;
    }
    application.removeTranslator(&english);

    QTranslator italian;
    if (!italian.load(QString::fromLocal8Bit(argv[2])) ||
        !application.installTranslator(&italian)) {
        std::cerr << "Could not load Italian translation catalog\n";
        return 1;
    }
    if (!expectEqual(QCoreApplication::translate("MainWindow", "Reset"),
                     QStringLiteral("Azzera"), "Italian filter reset") ||
        !expectEqual(QCoreApplication::translate("MainWindow", "Date"),
                     QStringLiteral("Data"), "Italian date filter")) {
        return 1;
    }
    return 0;
}
