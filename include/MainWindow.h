#pragma once

#include "ExpenseManager.h"

#include <QMainWindow>
#include <QList>
#include <QTranslator>
#include <QVariantMap>

class QLabel;
class QFrame;
class QComboBox;
class QCheckBox;
class QDateEdit;
class QDoubleSpinBox;
class ExpenseDonutChart;
class ExpenseTrendChart;
class QLineEdit;
class QStackedWidget;
class QTableWidget;
class QWidget;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    ExpenseManager m_manager;
    int m_accountId = 0;
    QList<QVariantMap> m_categories;

    QStackedWidget *m_pages = nullptr;
    QTableWidget *m_recentTable = nullptr;
    QTableWidget *m_transactionsTable = nullptr;
    QTableWidget *m_categoriesTable = nullptr;
    QTableWidget *m_analysisTable = nullptr;
    QTableWidget *m_dashboardAccountsTable = nullptr;
    ExpenseDonutChart *m_expenseDonut = nullptr;
    ExpenseTrendChart *m_expenseTrend = nullptr;
    ExpenseTrendChart *m_dashboardTrend = nullptr;
    QLabel *m_balanceValue = nullptr;
    QLabel *m_incomeValue = nullptr;
    QLabel *m_expensesValue = nullptr;
    QLabel *m_monthValue = nullptr;
    QLabel *m_transactionCount = nullptr;
    QLabel *m_filteredIncome = nullptr;
    QLabel *m_filteredExpenses = nullptr;
    QLabel *m_filteredBalance = nullptr;
    QLabel *m_transactionEmpty = nullptr;
    QLabel *m_pageInfo = nullptr;
    QLabel *m_analysisTotal = nullptr;
    QLabel *m_analysisIncome = nullptr;
    QLabel *m_analysisNet = nullptr;
    QLabel *m_analysisAverage = nullptr;
    QLabel *m_analysisTopCategory = nullptr;
    QLabel *m_analysisComparison = nullptr;
    QLabel *m_analysisActivity = nullptr;
    QLabel *m_analysisTrendTitle = nullptr;
    QLabel *m_analysisDateRange = nullptr;
    QLabel *m_analysisEmpty = nullptr;
    QLabel *m_dashboardTotalBalance = nullptr;
    QLabel *m_appearancePreview = nullptr;
    QLabel *m_activeAccountSummary = nullptr;
    QComboBox *m_filterCategory = nullptr;
    QComboBox *m_filterType = nullptr;
    QComboBox *m_accountSelector = nullptr;
    QComboBox *m_analysisPeriod = nullptr;
    QComboBox *m_sortOrder = nullptr;
    QComboBox *m_pageSize = nullptr;
    QComboBox *m_themeMode = nullptr;
    QComboBox *m_language = nullptr;
    QList<QPushButton *> m_paletteOptions;
    QList<QFrame *> m_transactionSummaryCards;
    QCheckBox *m_filterDateEnabled = nullptr;
    QDateEdit *m_filterFrom = nullptr;
    QDateEdit *m_filterTo = nullptr;
    QDoubleSpinBox *m_filterMin = nullptr;
    QDoubleSpinBox *m_filterMax = nullptr;
    QLineEdit *m_transactionSearch = nullptr;
    QPushButton *m_previousPage = nullptr;
    QPushButton *m_nextPage = nullptr;
    QPushButton *m_analysisPrevious = nullptr;
    QPushButton *m_analysisNext = nullptr;
    QPushButton *m_editTransaction = nullptr;
    QPushButton *m_deleteTransaction = nullptr;
    QList<Transaction> m_filteredTransactions;
    QList<Transaction> m_visibleTransactions;
    QString m_baseStyleSheet;
    QString m_fontFamily;
    QString m_themeModeKey;
    QString m_themePaletteKey;
    QString m_languageKey;
    QTranslator m_translator;
    int m_transactionPage = 0;
    int m_analysisMonths = 1;
    int m_analysisMonthOffset = 0;

    bool initializeData();
    void buildUi();
    void refreshAccounts();
    void createAccount();
    void openAccountManagement();
    void openTransfers();
    QWidget *buildDashboardPage();
    QWidget *buildTransactionsPage();
    QWidget *buildCategoriesPage();
    QWidget *buildAnalysisPage();
    QWidget *buildAppearancePage();
    void applyAppearance();
    void saveAppearance();
    void refreshAll();
    void refreshDashboard();
    void refreshTransactions();
    void renderTransactionsPage();
    void refreshCategories();
    void refreshAnalysis();
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