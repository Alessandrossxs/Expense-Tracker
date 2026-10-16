#pragma once

#include "ExpenseManager.h"

#include <QMainWindow>
#include <QList>
#include <QVariantMap>

class QLabel;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QStackedWidget;
class QTableWidget;
class QWidget;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    ExpenseManager m_manager;
    int m_accountId = 0;
    QList<QVariantMap> m_categories;

    QStackedWidget *m_pages = nullptr;
    QTableWidget *m_recentTable = nullptr;
    QTableWidget *m_transactionsTable = nullptr;
    QTableWidget *m_categoriesTable = nullptr;
    QLabel *m_balanceValue = nullptr;
    QLabel *m_incomeValue = nullptr;
    QLabel *m_expensesValue = nullptr;
    QLabel *m_monthValue = nullptr;
    QComboBox *m_filterCategory = nullptr;
    QDateEdit *m_filterFrom = nullptr;
    QDateEdit *m_filterTo = nullptr;
    QDoubleSpinBox *m_filterMin = nullptr;
    QDoubleSpinBox *m_filterMax = nullptr;

    bool initializeData();
    void buildUi();
    QWidget *buildDashboardPage();
    QWidget *buildTransactionsPage();
    QWidget *buildCategoriesPage();
    void refreshAll();
    void refreshDashboard();
    void refreshTransactions();
    void refreshCategories();
    TransactionFilter currentFilter() const;
    QString categoryName(int categoryId) const;
    QString money(qint64 cents) const;
    void addTransaction();
    void editTransaction();
    void deleteTransaction();
    void exportCsv();
    void addCategory();
    void showError(const QString &message);
};