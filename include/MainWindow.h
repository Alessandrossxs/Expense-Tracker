#pragma once

#include "ExpenseManager.h"

#include <QMainWindow>
#include <QList>
#include <QVariantMap>

class QButtonGroup;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QWidget;
class DonutChart;
class MonthlyBarChart;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(const QString &databasePath, QWidget *parent = nullptr);

private:
    ExpenseManager m_manager;
    QString m_databasePath;
    int m_accountId = 0;
    bool m_darkTheme = false;
    QList<QVariantMap> m_categories;
    QList<Transaction> m_visibleTransactions;

    QStackedWidget *m_pages = nullptr;
    QList<QPushButton *> m_navButtons;
    QPushButton *m_themeButton = nullptr;
    QTableWidget *m_recentTable = nullptr;
    QStackedWidget *m_recentStack = nullptr;
    QTableWidget *m_transactionsTable = nullptr;
    QStackedWidget *m_transactionsStack = nullptr;
    QLabel *m_transactionsEmpty = nullptr;
    QLabel *m_transactionsSummary = nullptr;
    QTableWidget *m_categoriesTable = nullptr;
    QLabel *m_balanceValue = nullptr;
    QLabel *m_incomeValue = nullptr;
    QLabel *m_expensesValue = nullptr;
    QLabel *m_monthValue = nullptr;
    DonutChart *m_donut = nullptr;
    MonthlyBarChart *m_bars = nullptr;
    QComboBox *m_filterPeriod = nullptr;
    QComboBox *m_filterCategory = nullptr;
    QWidget *m_customRange = nullptr;
    QDateEdit *m_filterFrom = nullptr;
    QDateEdit *m_filterTo = nullptr;
    QDoubleSpinBox *m_filterMin = nullptr;
    QDoubleSpinBox *m_filterMax = nullptr;
    QLabel *m_filterHint = nullptr;

    bool initializeData();
    void buildUi();
    QWidget *buildDashboardPage();
    QWidget *buildTransactionsPage();
    QWidget *buildCategoriesPage();
    void applyTheme(bool dark);
    void showPage(int index);
    void refreshAll();
    void refreshDashboard();
    void refreshTransactions();
    void refreshCategories();
    void fillTransactionRow(QTableWidget *table, int row, const Transaction &t) const;
    TransactionFilter currentFilter() const;
    QString filterProblem() const;
    bool filterIsActive() const;
    QString categoryName(int categoryId) const;
    int selectedTransactionId() const;
    int selectedCategoryId() const;
    bool categoryNameTaken(const QString &name, const QString &type, int excludeId) const;
    void addTransaction();
    void editTransaction();
    void deleteTransaction();
    void exportCsv();
    void addCategory();
    void renameCategory();
    void deleteCategory();
    void setInitialBalance();
    void showError(const QString &message);
};
