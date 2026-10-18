#pragma once

#include "ExpenseManager.h"

#include <QColor>
#include <QList>
#include <QMainWindow>
#include <QVariantMap>

class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTimer;
class QWidget;
class DonutChart;
class MonthlyBarChart;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(const QString &databasePath, QWidget *parent = nullptr);

private:
    struct RowRef
    {
        int id = 0;
        bool transfer = false;
    };

    ExpenseManager m_manager;
    QString m_databasePath;
    int m_accountId = 0;
    bool m_darkTheme = false;
    QColor m_accent;
    QList<QVariantMap> m_categories;
    QList<QVariantMap> m_accounts;
    QList<Transaction> m_visibleTransactions;

    QStackedWidget *m_pages = nullptr;
    QList<QPushButton *> m_navButtons;
    QComboBox *m_accountCombo = nullptr;
    QLabel *m_toast = nullptr;
    QTimer *m_toastTimer = nullptr;
    // Dashboard
    QLabel *m_dashSubtitle = nullptr;
    QLabel *m_balanceValue = nullptr;
    QLabel *m_incomeValue = nullptr;
    QLabel *m_expensesValue = nullptr;
    DonutChart *m_donut = nullptr;
    MonthlyBarChart *m_bars = nullptr;
    QTableWidget *m_recentTable = nullptr;
    QStackedWidget *m_recentStack = nullptr;
    // Transactions
    QTableWidget *m_transactionsTable = nullptr;
    QStackedWidget *m_transactionsStack = nullptr;
    QLabel *m_transactionsEmpty = nullptr;
    QLabel *m_transactionsSummary = nullptr;
    QComboBox *m_filterPeriod = nullptr;
    QComboBox *m_filterCategory = nullptr;
    QWidget *m_customRange = nullptr;
    QDateEdit *m_filterFrom = nullptr;
    QDateEdit *m_filterTo = nullptr;
    QDoubleSpinBox *m_filterMin = nullptr;
    QDoubleSpinBox *m_filterMax = nullptr;
    QLabel *m_filterHint = nullptr;
    // Categories, accounts, appearance
    QTableWidget *m_categoriesTable = nullptr;
    QTableWidget *m_accountsTable = nullptr;
    QLabel *m_totalValue = nullptr;
    QList<QPushButton *> m_swatches;
    QPushButton *m_lightButton = nullptr;
    QPushButton *m_darkButton = nullptr;
    QLineEdit *m_hexEdit = nullptr;
    QLabel *m_hexHint = nullptr;

    bool initializeData();
    void buildUi();
    QWidget *buildDashboardPage();
    QWidget *buildTransactionsPage();
    QWidget *buildCategoriesPage();
    QWidget *buildAccountsPage();
    QWidget *buildAppearancePage();
    void applyTheme();
    void setAccent(const QColor &color);
    void setDarkTheme(bool dark);
    void syncAppearanceControls();
    void showPage(int index);
    void showToast(const QString &text);
    void showError(const QString &message);

    void refreshAll();
    void refreshAccounts();
    void refreshDashboard();
    void refreshTransactions();
    void refreshCategories();
    void fillTransactionRow(QTableWidget *table, int row, const Transaction &t) const;
    void fillTransferRow(QTableWidget *table, int row, const Transfer &t) const;
    TransactionFilter currentFilter() const;
    QString filterProblem() const;
    bool filterIsActive() const;
    QString categoryName(int categoryId) const;
    QString accountName(int accountId) const;
    RowRef selectedTransactionRow() const;
    int selectedCategoryId() const;
    int selectedAccountId() const;
    bool categoryNameTaken(const QString &name, const QString &type, int excludeId) const;

    void addTransaction();
    void editTransaction();
    void deleteTransaction();
    void exportCsv();
    void addCategory();
    void renameCategory();
    void deleteCategory();
    void addAccount();
    void renameAccount();
    void deleteAccount();
    void setInitialBalance(int accountId);
    void addTransfer();
    void switchAccount(int accountId);
};
