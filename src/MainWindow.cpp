#include "MainWindow.h"

#include "TransactionDialog.h"

#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QDir>
#include <QUuid>
#include <QDateTime>
#include <QTimer>

#include <algorithm>

namespace
{
QLabel *pageTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("pageTitle"));
    return label;
}

QFrame *metricCard(const QString &title, QLabel **valueLabel, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("metricCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(8);
    auto *caption = new QLabel(title, card);
    caption->setObjectName(QStringLiteral("metricCaption"));
    *valueLabel = new QLabel(QStringLiteral("0,00 €"), card);
    (*valueLabel)->setObjectName(QStringLiteral("metricValue"));
    layout->addWidget(caption);
    layout->addWidget(*valueLabel);
    return card;
}

QTableWidget *makeTable(const QStringList &headers, QWidget *parent)
{
    auto *table = new QTableWidget(parent);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setShowGrid(false);
    table->setMinimumHeight(220);
    return table;
}

QTableWidgetItem *cell(const QString &text, int alignment = Qt::AlignVCenter | Qt::AlignLeft)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(alignment);
    return item;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Expense Tracker"));
    resize(1180, 760);
    setMinimumSize(900, 620);
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget { background: #f5f7fb; color: #172033; font-family: "Segoe UI"; font-size: 10pt; }
        QFrame#sidebar { background: #171a2b; border: none; }
        QLabel#brand { color: #ffffff; font-size: 18pt; font-weight: 700; padding: 12px 8px 22px 8px; }
        QPushButton#navButton { color: #c6c9d7; background: transparent; text-align: left; border: 0; border-radius: 9px; padding: 12px 14px; font-weight: 600; }
        QPushButton#navButton:hover { background: #292d45; color: #ffffff; }
        QPushButton#primaryButton { background: #6658d3; color: white; border: none; border-radius: 8px; padding: 10px 16px; font-weight: 600; }
        QPushButton#primaryButton:hover { background: #5648c0; }
        QPushButton#secondaryButton { background: white; color: #30384c; border: 1px solid #dfe3ed; border-radius: 8px; padding: 9px 14px; font-weight: 600; }
        QPushButton#secondaryButton:hover { background: #f0efff; }
        QLabel#pageTitle { font-size: 23pt; font-weight: 700; color: #171b2b; }
        QLabel#subTitle { color: #778097; font-size: 10pt; }
        QFrame#metricCard, QFrame#panel { background: white; border: 1px solid #e7eaf1; border-radius: 13px; }
        QLabel#metricCaption { color: #7b8398; font-size: 10pt; }
        QLabel#metricValue { color: #171b2b; font-size: 21pt; font-weight: 700; }
        QTableWidget { background: white; alternate-background-color: #fafbfe; border: 1px solid #e7eaf1; border-radius: 10px; padding: 5px; selection-background-color: #ebe9ff; selection-color: #242044; }
        QHeaderView::section { background: #f8f9fc; color: #778097; border: none; border-bottom: 1px solid #e7eaf1; padding: 11px 8px; font-weight: 600; }
        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox { background: white; border: 1px solid #dfe3ed; border-radius: 7px; padding: 7px; }
        QStatusBar { background: white; color: #778097; }
    )"));

    if (!initializeData()) {
        QTimer::singleShot(0, this, [this] { close(); });
        return;
    }
    buildUi();
    refreshAll();
}

bool MainWindow::initializeData()
{
    if (!m_manager.open()) {
        showError(tr("Impossibile aprire il database: %1").arg(m_manager.lastError()));
        return false;
    }

    // The current backend exposes account creation but not account listing.
    // Read only the account IDs through a short-lived SQLite connection.
    const QString connectionName = QStringLiteral("ui_accounts_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    int firstAccountId = 0;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QDir::current().filePath(QStringLiteral("expense_tracker.db")));
        if (!db.open()) {
            const QString error = db.lastError().text();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connectionName);
            showError(tr("Impossibile leggere gli account: %1").arg(error));
            return false;
        }
        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("SELECT ID_Account FROM Bank_Account ORDER BY ID_Account LIMIT 1"))) {
            const QString error = query.lastError().text();
            query.finish();
            db.close();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connectionName);
            showError(tr("Impossibile leggere gli account: %1").arg(error));
            return false;
        }
        if (query.next())
            firstAccountId = query.value(0).toInt();
        query.finish();
        db.close();
        db = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(connectionName);

    if (firstAccountId == 0) {
        const QString hash = QString::fromLatin1(
            QCryptographicHash::hash(QByteArrayLiteral("expense-tracker-local-account"),
                                     QCryptographicHash::Sha256).toHex());
        if (!m_manager.addAccount(tr("Personale"), hash,
                                  QDate::currentDate().toString(Qt::ISODate), 0,
                                  &firstAccountId)) {
            showError(tr("Impossibile creare l'account iniziale: %1").arg(m_manager.lastError()));
            return false;
        }
    }
    m_accountId = firstAccountId;

    if (!m_manager.getCategories(m_categories)) {
        showError(tr("Impossibile caricare le categorie: %1").arg(m_manager.lastError()));
        return false;
    }
    if (m_categories.isEmpty()) {
        const QList<QPair<QString, QString>> defaults = {
            {tr("Stipendio"), QStringLiteral("in")},
            {tr("Regali"), QStringLiteral("in")},
            {tr("Altro"), QStringLiteral("in")},
            {tr("Alimentari"), QStringLiteral("out")},
            {tr("Trasporti"), QStringLiteral("out")},
            {tr("Casa"), QStringLiteral("out")},
            {tr("Svago"), QStringLiteral("out")},
            {tr("Salute"), QStringLiteral("out")},
            {tr("Altro"), QStringLiteral("out")}
        };
        for (const auto &entry : defaults) {
            if (!m_manager.addCategory(entry.first, entry.second)) {
                showError(tr("Impossibile creare le categorie iniziali: %1").arg(m_manager.lastError()));
                return false;
            }
        }
        if (!m_manager.getCategories(m_categories)) {
            showError(tr("Impossibile caricare le categorie: %1").arg(m_manager.lastError()));
            return false;
        }
    }
    return true;
}

void MainWindow::buildUi()
{
    auto *root = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *sidebar = new QFrame(root);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(220);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(16, 22, 16, 20);
    sideLayout->setSpacing(8);
    auto *brand = new QLabel(QStringLiteral("◈  Expense Tracker"), sidebar);
    brand->setObjectName(QStringLiteral("brand"));
    sideLayout->addWidget(brand);

    m_pages = new QStackedWidget(root);
    const QStringList navLabels = {tr("Panoramica"), tr("Transazioni"), tr("Categorie")};
    for (int i = 0; i < navLabels.size(); ++i) {
        auto *button = new QPushButton(navLabels[i], sidebar);
        button->setObjectName(QStringLiteral("navButton"));
        button->setCursor(Qt::PointingHandCursor);
        sideLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, i] {
            m_pages->setCurrentIndex(i);
            if (i == 1) refreshTransactions();
            if (i == 2) refreshCategories();
        });
    }
    sideLayout->addStretch();
    auto *footer = new QLabel(tr("Gestione personale\nDati salvati in locale"), sidebar);
    footer->setStyleSheet(QStringLiteral("color:#858aa2; padding:8px;"));
    sideLayout->addWidget(footer);

    m_pages->addWidget(buildDashboardPage());
    m_pages->addWidget(buildTransactionsPage());
    m_pages->addWidget(buildCategoriesPage());
    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(m_pages, 1);
    setCentralWidget(root);
    statusBar()->showMessage(tr("Database locale pronto"));
}

QWidget *MainWindow::buildDashboardPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(20);

    layout->addWidget(pageTitle(tr("Panoramica"), page));
    auto *subtitle = new QLabel(tr("Un riepilogo chiaro delle tue finanze."), page);
    subtitle->setObjectName(QStringLiteral("subTitle"));
    layout->addWidget(subtitle);

    auto *metrics = new QHBoxLayout;
    metrics->setSpacing(14);
    metrics->addWidget(metricCard(tr("Saldo attuale"), &m_balanceValue, page), 1);
    metrics->addWidget(metricCard(tr("Entrate totali"), &m_incomeValue, page), 1);
    metrics->addWidget(metricCard(tr("Uscite totali"), &m_expensesValue, page), 1);
    metrics->addWidget(metricCard(tr("Uscite del mese"), &m_monthValue, page), 1);
    layout->addLayout(metrics);

    auto *recentPanel = new QFrame(page);
    recentPanel->setObjectName(QStringLiteral("panel"));
    auto *recentLayout = new QVBoxLayout(recentPanel);
    recentLayout->setContentsMargins(18, 18, 18, 18);
    auto *heading = new QLabel(tr("Movimenti recenti"), recentPanel);
    heading->setStyleSheet(QStringLiteral("font-size:14pt;font-weight:700;"));
    recentLayout->addWidget(heading);
    m_recentTable = makeTable({tr("Data"), tr("Categoria"), tr("Tipo"), tr("Importo")}, recentPanel);
    m_recentTable->setMinimumHeight(260);
    recentLayout->addWidget(m_recentTable);
    layout->addWidget(recentPanel, 1);

    auto *actions = new QHBoxLayout;
    auto *add = new QPushButton(tr("+ Nuova transazione"), page);
    add->setObjectName(QStringLiteral("primaryButton"));
    auto *view = new QPushButton(tr("Vedi tutte"), page);
    view->setObjectName(QStringLiteral("secondaryButton"));
    actions->addWidget(add);
    actions->addWidget(view);
    actions->addStretch();
    layout->addLayout(actions);
    connect(add, &QPushButton::clicked, this, &MainWindow::addTransaction);
    connect(view, &QPushButton::clicked, this, [this] {
        m_pages->setCurrentIndex(1);
        refreshTransactions();
    });
    return page;
}

QWidget *MainWindow::buildTransactionsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(16);
    layout->addWidget(pageTitle(tr("Transazioni"), page));

    auto *filters = new QFrame(page);
    filters->setObjectName(QStringLiteral("panel"));
    auto *filterLayout = new QHBoxLayout(filters);
    filterLayout->setContentsMargins(12, 12, 12, 12);
    filterLayout->setSpacing(8);
    m_filterFrom = new QDateEdit(QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1), filters);
    m_filterTo = new QDateEdit(QDate::currentDate(), filters);
    m_filterFrom->setCalendarPopup(true);
    m_filterTo->setCalendarPopup(true);
    m_filterFrom->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_filterTo->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_filterCategory = new QComboBox(filters);
    m_filterCategory->addItem(tr("Tutte le categorie"), 0);
    for (const QVariantMap &category : m_categories)
        m_filterCategory->addItem(category.value(QStringLiteral("Name")).toString(),
                                  category.value(QStringLiteral("ID_Category")).toInt());
    m_filterMin = new QDoubleSpinBox(filters);
    m_filterMin->setRange(0, 999999999);
    m_filterMin->setPrefix(tr("Min "));
    m_filterMin->setSuffix(QStringLiteral(" €"));
    m_filterMin->setSpecialValueText(tr("Min €"));
    m_filterMax = new QDoubleSpinBox(filters);
    m_filterMax->setRange(0, 999999999);
    m_filterMax->setPrefix(tr("Max "));
    m_filterMax->setSuffix(QStringLiteral(" €"));
    m_filterMax->setSpecialValueText(tr("Max €"));
    auto *apply = new QPushButton(tr("Filtra"), filters);
    apply->setObjectName(QStringLiteral("secondaryButton"));
    auto *reset = new QPushButton(tr("Reset"), filters);
    reset->setObjectName(QStringLiteral("secondaryButton"));
    filterLayout->addWidget(new QLabel(tr("Dal"), filters));
    filterLayout->addWidget(m_filterFrom);
    filterLayout->addWidget(new QLabel(tr("Al"), filters));
    filterLayout->addWidget(m_filterTo);
    filterLayout->addWidget(m_filterCategory, 1);
    filterLayout->addWidget(m_filterMin);
    filterLayout->addWidget(m_filterMax);
    filterLayout->addWidget(apply);
    filterLayout->addWidget(reset);
    layout->addWidget(filters);

    m_transactionsTable = makeTable(
        {tr("Data"), tr("Categoria"), tr("Tipo"), tr("Importo"), tr("ID")}, page);
    m_transactionsTable->setColumnHidden(4, true);
    layout->addWidget(m_transactionsTable, 1);

    auto *actions = new QHBoxLayout;
    auto *add = new QPushButton(tr("+ Aggiungi"), page);
    add->setObjectName(QStringLiteral("primaryButton"));
    auto *edit = new QPushButton(tr("Modifica"), page);
    edit->setObjectName(QStringLiteral("secondaryButton"));
    auto *remove = new QPushButton(tr("Elimina"), page);
    remove->setObjectName(QStringLiteral("secondaryButton"));
    auto *csv = new QPushButton(tr("Esporta CSV"), page);
    csv->setObjectName(QStringLiteral("secondaryButton"));
    actions->addWidget(add);
    actions->addWidget(edit);
    actions->addWidget(remove);
    actions->addStretch();
    actions->addWidget(csv);
    layout->addLayout(actions);

    connect(apply, &QPushButton::clicked, this, &MainWindow::refreshTransactions);
    connect(reset, &QPushButton::clicked, this, [this] {
        m_filterFrom->setDate(QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1));
        m_filterTo->setDate(QDate::currentDate());
        m_filterCategory->setCurrentIndex(0);
        m_filterMin->setValue(0);
        m_filterMax->setValue(0);
        refreshTransactions();
    });
    connect(add, &QPushButton::clicked, this, &MainWindow::addTransaction);
    connect(edit, &QPushButton::clicked, this, &MainWindow::editTransaction);
    connect(remove, &QPushButton::clicked, this, &MainWindow::deleteTransaction);
    connect(csv, &QPushButton::clicked, this, &MainWindow::exportCsv);
    connect(m_transactionsTable, &QTableWidget::cellDoubleClicked, this,
            [this] { editTransaction(); });
    return page;
}

QWidget *MainWindow::buildCategoriesPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(16);
    layout->addWidget(pageTitle(tr("Categorie"), page));
    auto *subtitle = new QLabel(tr("Organizza entrate e uscite con categorie dedicate."), page);
    subtitle->setObjectName(QStringLiteral("subTitle"));
    layout->addWidget(subtitle);
    m_categoriesTable = makeTable({tr("Categoria"), tr("Tipo")}, page);
    layout->addWidget(m_categoriesTable, 1);
    auto *add = new QPushButton(tr("+ Nuova categoria"), page);
    add->setObjectName(QStringLiteral("primaryButton"));
    layout->addWidget(add, 0, Qt::AlignLeft);
    connect(add, &QPushButton::clicked, this, &MainWindow::addCategory);
    return page;
}

TransactionFilter MainWindow::currentFilter() const
{
    TransactionFilter filter;
    filter.dateFrom = m_filterFrom->date().toString(Qt::ISODate);
    filter.dateTo = m_filterTo->date().toString(Qt::ISODate);
    filter.categoryId = m_filterCategory->currentData().toInt();
    filter.minimumAmount = qRound64(m_filterMin->value() * 100.0);
    filter.maximumAmount = qRound64(m_filterMax->value() * 100.0);
    return filter;
}

QString MainWindow::categoryName(int categoryId) const
{
    for (const QVariantMap &category : m_categories) {
        if (category.value(QStringLiteral("ID_Category")).toInt() == categoryId)
            return category.value(QStringLiteral("Name")).toString();
    }
    return tr("Senza categoria");
}

QString MainWindow::money(qint64 cents) const
{
    const bool negative = cents < 0;
    const quint64 magnitude = negative
        ? static_cast<quint64>(-(cents + 1)) + 1
        : static_cast<quint64>(cents);
    const QString digits = QString::number(magnitude / 100);
    QString grouped;
    for (int i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0)
            grouped += QLatin1Char('.');
        grouped += digits.at(i);
    }
    return QStringLiteral("%1%2,%3 €")
        .arg(negative ? QStringLiteral("-") : QString())
        .arg(grouped)
        .arg(magnitude % 100, 2, 10, QLatin1Char('0'));
}

void MainWindow::refreshAll()
{
    if (!m_manager.getCategories(m_categories)) {
        showError(m_manager.lastError());
        return;
    }
    if (m_filterCategory) {
        const int selected = m_filterCategory->currentData().toInt();
        m_filterCategory->blockSignals(true);
        m_filterCategory->clear();
        m_filterCategory->addItem(tr("Tutte le categorie"), 0);
        for (const QVariantMap &category : m_categories)
            m_filterCategory->addItem(category.value(QStringLiteral("Name")).toString(),
                                      category.value(QStringLiteral("ID_Category")).toInt());
        const int index = m_filterCategory->findData(selected);
        m_filterCategory->setCurrentIndex(index < 0 ? 0 : index);
        m_filterCategory->blockSignals(false);
    }
    refreshDashboard();
    refreshTransactions();
    refreshCategories();
}

void MainWindow::refreshDashboard()
{
    QList<Transaction> all;
    if (!m_manager.getTransactions(m_accountId, all)) {
        showError(m_manager.lastError());
        return;
    }
    qint64 income = 0;
    qint64 expenses = 0;
    QList<Transaction> recent;
    for (const Transaction &transaction : all) {
        if (transaction.isIncome())
            income += transaction.getAmount();
        else
            expenses += transaction.getAmount();
        if (transaction.getDate() >= QDate::currentDate().addDays(-30).toString(Qt::ISODate))
            recent.append(transaction);
    }
    qint64 balance = 0;
    if (!m_manager.getAccountBalance(m_accountId, balance)) {
        showError(m_manager.lastError());
        return;
    }
    qint64 monthExpenses = 0;
    const QString monthStart = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1)
                                   .toString(Qt::ISODate);
    for (const Transaction &transaction : all) {
        if (!transaction.isIncome() && transaction.getDate() >= monthStart)
            monthExpenses += transaction.getAmount();
    }
    m_balanceValue->setText(money(balance));
    m_incomeValue->setText(money(income));
    m_expensesValue->setText(money(expenses));
    m_monthValue->setText(money(monthExpenses));
    m_balanceValue->setStyleSheet(balance < 0 ? QStringLiteral("color:#d94d64;") :
                                               QStringLiteral("color:#6658d3;"));

    std::sort(recent.begin(), recent.end(), [](const Transaction &a, const Transaction &b) {
        return a.getDate() > b.getDate() ||
               (a.getDate() == b.getDate() && a.getId() > b.getId());
    });
    m_recentTable->setRowCount(qMin(6, recent.size()));
    for (int row = 0; row < m_recentTable->rowCount(); ++row) {
        const Transaction &t = recent.at(row);
        m_recentTable->setItem(row, 0, cell(QDate::fromString(t.getDate(), Qt::ISODate).toString(QStringLiteral("dd/MM/yyyy"))));
        m_recentTable->setItem(row, 1, cell(categoryName(t.getCategoryId())));
        m_recentTable->setItem(row, 2, cell(t.isIncome() ? tr("Entrata") : tr("Uscita")));
        auto *amount = cell((t.isIncome() ? QStringLiteral("+") : QStringLiteral("−")) + money(t.getAmount()),
                            Qt::AlignVCenter | Qt::AlignRight);
        amount->setForeground(t.isIncome() ? QColor(QStringLiteral("#16845b")) : QColor(QStringLiteral("#d94d64")));
        m_recentTable->setItem(row, 3, amount);
    }
}

void MainWindow::refreshTransactions()
{
    if (!m_transactionsTable)
        return;
    QList<Transaction> transactions;
    if (!m_manager.getTransactions(m_accountId, transactions, currentFilter())) {
        showError(m_manager.lastError());
        return;
    }
    std::sort(transactions.begin(), transactions.end(), [](const Transaction &a, const Transaction &b) {
        return a.getDate() > b.getDate() ||
               (a.getDate() == b.getDate() && a.getId() > b.getId());
    });
    m_transactionsTable->setRowCount(transactions.size());
    for (int row = 0; row < transactions.size(); ++row) {
        const Transaction &t = transactions.at(row);
        m_transactionsTable->setItem(row, 0, cell(QDate::fromString(t.getDate(), Qt::ISODate).toString(QStringLiteral("dd/MM/yyyy"))));
        m_transactionsTable->setItem(row, 1, cell(categoryName(t.getCategoryId())));
        m_transactionsTable->setItem(row, 2, cell(t.isIncome() ? tr("Entrata") : tr("Uscita")));
        auto *amount = cell((t.isIncome() ? QStringLiteral("+") : QStringLiteral("−")) + money(t.getAmount()),
                            Qt::AlignVCenter | Qt::AlignRight);
        amount->setForeground(t.isIncome() ? QColor(QStringLiteral("#16845b")) : QColor(QStringLiteral("#d94d64")));
        m_transactionsTable->setItem(row, 3, amount);
        m_transactionsTable->setItem(row, 4, cell(QString::number(t.getId())));
    }
}

void MainWindow::refreshCategories()
{
    if (!m_categoriesTable)
        return;
    m_categoriesTable->setRowCount(m_categories.size());
    for (int row = 0; row < m_categories.size(); ++row) {
        const QVariantMap &category = m_categories.at(row);
        m_categoriesTable->setItem(row, 0, cell(category.value(QStringLiteral("Name")).toString()));
        const bool income = category.value(QStringLiteral("Type")).toString() == QStringLiteral("in");
        m_categoriesTable->setItem(row, 1, cell(income ? tr("Entrate") : tr("Uscite")));
    }
}

void MainWindow::addTransaction()
{
    TransactionDialog dialog(m_categories, m_accountId, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!m_manager.addTransaction(dialog.transaction())) {
        showError(tr("Impossibile salvare la transazione: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    statusBar()->showMessage(tr("Transazione aggiunta"), 3000);
}

void MainWindow::editTransaction()
{
    const int row = m_transactionsTable ? m_transactionsTable->currentRow() : -1;
    if (row < 0) {
        QMessageBox::information(this, tr("Seleziona una transazione"), tr("Seleziona una riga da modificare."));
        return;
    }
    const int id = m_transactionsTable->item(row, 4)->text().toInt();
    QList<Transaction> transactions;
    if (!m_manager.getTransactions(m_accountId, transactions, currentFilter())) {
        showError(m_manager.lastError());
        return;
    }
    auto it = std::find_if(transactions.begin(), transactions.end(),
                           [id](const Transaction &t) { return t.getId() == id; });
    if (it == transactions.end()) {
        showError(tr("La transazione selezionata non è più disponibile."));
        return;
    }
    TransactionDialog dialog(m_categories, m_accountId, this);
    dialog.setTransaction(*it);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!m_manager.updateTransaction(dialog.transaction())) {
        showError(tr("Impossibile modificare la transazione: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
}

void MainWindow::deleteTransaction()
{
    const int row = m_transactionsTable ? m_transactionsTable->currentRow() : -1;
    if (row < 0) {
        QMessageBox::information(this, tr("Seleziona una transazione"), tr("Seleziona una riga da eliminare."));
        return;
    }
    const int id = m_transactionsTable->item(row, 4)->text().toInt();
    if (QMessageBox::question(this, tr("Conferma eliminazione"),
                              tr("Vuoi eliminare questa transazione? Il saldo verrà aggiornato.")) != QMessageBox::Yes)
        return;
    if (!m_manager.deleteTransaction(id)) {
        showError(tr("Impossibile eliminare la transazione: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
}

void MainWindow::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Esporta transazioni"), QDir::home().filePath(QStringLiteral("transazioni.csv")),
        tr("File CSV (*.csv)"));
    if (path.isEmpty())
        return;
    if (!m_manager.exportTransactionsToCsv(m_accountId, path, currentFilter())) {
        showError(tr("Esportazione non riuscita: %1").arg(m_manager.lastError()));
        return;
    }
    statusBar()->showMessage(tr("CSV esportato correttamente"), 4000);
}

void MainWindow::addCategory()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Nuova categoria"),
                                               tr("Nome categoria:"), QLineEdit::Normal,
                                               QString(), &ok).trimmed();
    if (!ok)
        return;
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Nome mancante"), tr("Inserisci un nome per la categoria."));
        return;
    }
    const QStringList types = {tr("Uscita"), tr("Entrata")};
    const QString selected = QInputDialog::getItem(this, tr("Tipo categoria"),
                                                   tr("Questa categoria sarà usata per:"), types, 0, false, &ok);
    if (!ok)
        return;
    const QString type = selected == tr("Entrata") ? QStringLiteral("in") : QStringLiteral("out");
    if (!m_manager.addCategory(name, type)) {
        showError(tr("Impossibile creare la categoria: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    statusBar()->showMessage(tr("Categoria aggiunta"), 3000);
}

void MainWindow::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Expense Tracker"), message);
}
