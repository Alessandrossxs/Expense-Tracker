#include "MainWindow.h"

#include "ChartWidgets.h"
#include "Format.h"
#include "TransactionDialog.h"

#include <QAbstractItemView>
#include <QButtonGroup>
#include <QColor>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <stdexcept>

namespace
{
constexpr int SortRole = Qt::UserRole + 1;
constexpr int PeriodCustom = 4;

// Table cell that sorts on a dedicated key (ISO date, signed cents, ...)
// instead of on its displayed text.
class SortItem : public QTableWidgetItem
{
public:
    using QTableWidgetItem::QTableWidgetItem;
    bool operator<(const QTableWidgetItem &other) const override
    {
        const QVariant a = data(SortRole);
        const QVariant b = other.data(SortRole);
        if (a.isValid() && b.isValid()) {
            if (a.typeId() == QMetaType::LongLong && b.typeId() == QMetaType::LongLong)
                return a.toLongLong() < b.toLongLong();
            return QString::compare(a.toString(), b.toString(), Qt::CaseInsensitive) < 0;
        }
        return QTableWidgetItem::operator<(other);
    }
};

SortItem *cell(const QString &text,
               Qt::Alignment alignment = Qt::AlignVCenter | Qt::AlignLeft,
               const QVariant &sortKey = QVariant())
{
    auto *item = new SortItem(text);
    item->setTextAlignment(alignment);
    if (sortKey.isValid())
        item->setData(SortRole, sortKey);
    return item;
}

QLabel *pageTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("pageTitle"));
    return label;
}

QLabel *subTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("subTitle"));
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

QFrame *makePanel(const QString &heading, QWidget *parent, QVBoxLayout **layoutOut)
{
    auto *panel = new QFrame(parent);
    panel->setObjectName(QStringLiteral("panel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);
    if (!heading.isEmpty()) {
        auto *label = new QLabel(heading, panel);
        label->setObjectName(QStringLiteral("panelHeading"));
        layout->addWidget(label);
    }
    *layoutOut = layout;
    return panel;
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
    table->setMinimumHeight(200);
    return table;
}

// Table + placeholder message shown instead of it when there are no rows.
QStackedWidget *withEmptyState(QTableWidget *table, QLabel **emptyLabel, QWidget *parent)
{
    auto *stack = new QStackedWidget(parent);
    stack->setObjectName(QStringLiteral("transparentBox"));
    *emptyLabel = new QLabel(stack);
    (*emptyLabel)->setObjectName(QStringLiteral("emptyState"));
    (*emptyLabel)->setAlignment(Qt::AlignCenter);
    (*emptyLabel)->setWordWrap(true);
    stack->addWidget(table);
    stack->addWidget(*emptyLabel);
    return stack;
}

QPushButton *button(const QString &text, const char *objectName, QWidget *parent,
                    const QString &tooltip = QString())
{
    auto *b = new QPushButton(text, parent);
    b->setObjectName(QLatin1String(objectName));
    b->setCursor(Qt::PointingHandCursor);
    if (!tooltip.isEmpty())
        b->setToolTip(tooltip);
    return b;
}

struct Theme
{
    QString bg, text, sidebar, navText, navHover, accent, accentHover, card, border, muted, alt,
        selection, selectionText, input, inputBorder, header, secondaryText, secondaryHover;
};

Theme lightTheme()
{
    return {"#f5f7fb", "#172033", "#171a2b", "#c6c9d7", "#292d45", "#6658d3", "#5648c0", "#ffffff",
            "#e7eaf1", "#778097", "#fafbfe", "#ebe9ff", "#242044", "#ffffff", "#dfe3ed", "#f8f9fc",
            "#30384c", "#f0efff"};
}

Theme darkTheme()
{
    return {"#12141f", "#e6e8f2", "#0c0e18", "#aeb3c7", "#1d2036", "#7a6cf0", "#8b7df5", "#1b1e2e",
            "#2a2e45", "#8b92ab", "#1f2335", "#38336b", "#ffffff", "#1b1e2e", "#343954", "#171a28",
            "#dfe2f0", "#26293f"};
}

QString buildStyleSheet(const Theme &t)
{
    QString css = QStringLiteral(R"(
        QMainWindow, QWidget { background: {{bg}}; color: {{text}}; font-size: 10pt; }
        QLabel { background: transparent; }
        QScrollArea { border: none; }
        QWidget#transparentBox { background: transparent; }
        QFrame#sidebar { background: {{sidebar}}; border: none; }
        QLabel#brand { color: #ffffff; font-size: 12pt; font-weight: 700; padding: 12px 4px 20px 4px; }
        QLabel#sidebarFooter { color: #858aa2; padding: 8px; }
        QPushButton#navButton { color: {{navText}}; background: transparent; text-align: left; border: 0; border-radius: 9px; padding: 12px 14px; font-weight: 600; }
        QPushButton#navButton:hover { background: {{navHover}}; color: #ffffff; }
        QPushButton#navButton:checked { background: {{accent}}; color: #ffffff; }
        QPushButton#themeButton { color: {{navText}}; background: transparent; text-align: left; border: 1px solid {{navHover}}; border-radius: 9px; padding: 10px 14px; font-weight: 600; }
        QPushButton#themeButton:hover { background: {{navHover}}; color: #ffffff; }
        QPushButton#primaryButton { background: {{accent}}; color: white; border: none; border-radius: 8px; padding: 10px 16px; font-weight: 600; }
        QPushButton#primaryButton:hover { background: {{accentHover}}; }
        QPushButton#secondaryButton { background: {{card}}; color: {{secondaryText}}; border: 1px solid {{inputBorder}}; border-radius: 8px; padding: 9px 14px; font-weight: 600; }
        QPushButton#secondaryButton:hover { background: {{secondaryHover}}; }
        QLabel#pageTitle { font-size: 23pt; font-weight: 700; color: {{text}}; }
        QLabel#subTitle, QLabel#summaryLine { color: {{muted}}; }
        QLabel#panelHeading { font-size: 13pt; font-weight: 700; }
        QLabel#emptyState { color: {{muted}}; font-size: 11pt; padding: 30px; }
        QLabel#warning { color: #d94d64; font-weight: 600; }
        QFrame#metricCard, QFrame#panel { background: {{card}}; border: 1px solid {{border}}; border-radius: 13px; }
        QLabel#metricCaption { color: {{muted}}; }
        QLabel#metricValue { color: {{text}}; font-size: 19pt; font-weight: 700; }
        QTableWidget { background: {{card}}; alternate-background-color: {{alt}}; border: 1px solid {{border}}; border-radius: 10px; padding: 5px; selection-background-color: {{selection}}; selection-color: {{selectionText}}; }
        QHeaderView::section { background: {{header}}; color: {{muted}}; border: none; border-bottom: 1px solid {{border}}; padding: 11px 8px; font-weight: 600; }
        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox { background: {{input}}; border: 1px solid {{inputBorder}}; border-radius: 7px; padding: 7px; }
        QComboBox QAbstractItemView { background: {{input}}; color: {{text}}; selection-background-color: {{selection}}; selection-color: {{selectionText}}; border: 1px solid {{inputBorder}}; }
        QStatusBar { background: {{card}}; color: {{muted}}; }
    )");
    const QList<QPair<QString, QString>> tokens = {
        {"bg", t.bg}, {"text", t.text}, {"sidebar", t.sidebar}, {"navText", t.navText},
        {"navHover", t.navHover}, {"accentHover", t.accentHover}, {"accent", t.accent},
        {"card", t.card}, {"border", t.border}, {"muted", t.muted}, {"alt", t.alt},
        {"selectionText", t.selectionText}, {"selection", t.selection}, {"inputBorder", t.inputBorder},
        {"input", t.input}, {"header", t.header}, {"secondaryText", t.secondaryText},
        {"secondaryHover", t.secondaryHover}};
    for (const auto &token : tokens)
        css.replace(QStringLiteral("{{%1}}").arg(token.first), token.second);
    return css;
}

QString italianMonth(const QDate &date)
{
    QString name = QLocale(QLocale::Italian).monthName(date.month(), QLocale::ShortFormat);
    name.remove(QLatin1Char('.'));
    return name.left(1).toUpper() + name.mid(1);
}
}

MainWindow::MainWindow(const QString &databasePath, QWidget *parent)
    : QMainWindow(parent), m_manager(databasePath), m_databasePath(databasePath)
{
    setWindowTitle(tr("Expense Tracker"));
    resize(1180, 780);
    setMinimumSize(940, 640);

    if (!initializeData()) {
        QTimer::singleShot(0, this, [this] { close(); });
        return;
    }
    buildUi();
    applyTheme(QSettings().value(QStringLiteral("theme/dark"), false).toBool());

    new QShortcut(QKeySequence::New, this, [this] { addTransaction(); });
    new QShortcut(QKeySequence(Qt::Key_Delete), m_transactionsTable,
                  [this] { deleteTransaction(); }, Qt::WidgetShortcut);
    new QShortcut(QKeySequence(Qt::Key_Return), m_transactionsTable,
                  [this] { editTransaction(); }, Qt::WidgetShortcut);
    new QShortcut(QKeySequence(Qt::Key_Enter), m_transactionsTable,
                  [this] { editTransaction(); }, Qt::WidgetShortcut);

    refreshAll();
}

bool MainWindow::initializeData()
{
    if (!m_manager.open()) {
        showError(tr("Impossibile aprire il database: %1").arg(m_manager.lastError()));
        return false;
    }

    int firstAccountId = 0;
    if (!m_manager.getFirstAccountId(firstAccountId)) {
        showError(tr("Impossibile leggere gli account: %1").arg(m_manager.lastError()));
        return false;
    }
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
            {tr("Stipendio"), QStringLiteral("in")},   {tr("Regali"), QStringLiteral("in")},
            {tr("Altro"), QStringLiteral("in")},       {tr("Alimentari"), QStringLiteral("out")},
            {tr("Trasporti"), QStringLiteral("out")},  {tr("Casa"), QStringLiteral("out")},
            {tr("Svago"), QStringLiteral("out")},      {tr("Salute"), QStringLiteral("out")},
            {tr("Altro"), QStringLiteral("out")}};
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

void MainWindow::applyTheme(bool dark)
{
    m_darkTheme = dark;
    setStyleSheet(buildStyleSheet(dark ? darkTheme() : lightTheme()));
    if (m_donut) m_donut->setDarkMode(dark);
    if (m_bars) m_bars->setDarkMode(dark);
    if (m_themeButton)
        m_themeButton->setText(dark ? tr("☀  Tema chiaro") : tr("☾  Tema scuro"));
    QSettings().setValue(QStringLiteral("theme/dark"), dark);
    if (m_balanceValue)
        refreshDashboard();
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
    auto *group = new QButtonGroup(this);
    group->setExclusive(true);
    const QStringList navLabels = {tr("Panoramica"), tr("Transazioni"), tr("Categorie")};
    for (int i = 0; i < navLabels.size(); ++i) {
        auto *b = button(navLabels[i], "navButton", sidebar);
        b->setCheckable(true);
        group->addButton(b);
        m_navButtons.append(b);
        sideLayout->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, i] { showPage(i); });
    }
    m_navButtons.first()->setChecked(true);
    sideLayout->addStretch();

    m_themeButton = button(QString(), "themeButton", sidebar);
    connect(m_themeButton, &QPushButton::clicked, this, [this] { applyTheme(!m_darkTheme); });
    sideLayout->addWidget(m_themeButton);
    auto *footer = new QLabel(tr("Gestione personale\nDati salvati in locale"), sidebar);
    footer->setObjectName(QStringLiteral("sidebarFooter"));
    footer->setToolTip(QDir::toNativeSeparators(m_databasePath));
    sideLayout->addWidget(footer);

    m_pages->addWidget(buildDashboardPage());
    m_pages->addWidget(buildTransactionsPage());
    m_pages->addWidget(buildCategoriesPage());
    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(m_pages, 1);
    setCentralWidget(root);
    statusBar()->showMessage(tr("Dati salvati in: %1").arg(QDir::toNativeSeparators(m_databasePath)));
}

void MainWindow::showPage(int index)
{
    m_pages->setCurrentIndex(index);
    if (index >= 0 && index < m_navButtons.size())
        m_navButtons.at(index)->setChecked(true);
    if (index == 1) refreshTransactions();
    if (index == 2) refreshCategories();
}

QWidget *MainWindow::buildDashboardPage()
{
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *page = new QWidget;
    scroll->setWidget(page);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(18);

    auto *header = new QHBoxLayout;
    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    titles->addWidget(pageTitle(tr("Panoramica"), page));
    titles->addWidget(subTitle(tr("Un riepilogo chiaro delle tue finanze."), page));
    header->addLayout(titles, 1);
    auto *initial = button(tr("Saldo iniziale…"), "secondaryButton", page,
                           tr("Imposta il saldo di partenza del conto"));
    header->addWidget(initial, 0, Qt::AlignTop);
    layout->addLayout(header);
    connect(initial, &QPushButton::clicked, this, &MainWindow::setInitialBalance);

    auto *metrics = new QHBoxLayout;
    metrics->setSpacing(14);
    metrics->addWidget(metricCard(tr("Saldo attuale"), &m_balanceValue, page), 1);
    metrics->addWidget(metricCard(tr("Entrate totali"), &m_incomeValue, page), 1);
    metrics->addWidget(metricCard(tr("Uscite totali"), &m_expensesValue, page), 1);
    metrics->addWidget(metricCard(tr("Uscite del mese"), &m_monthValue, page), 1);
    layout->addLayout(metrics);

    auto *charts = new QHBoxLayout;
    charts->setSpacing(14);
    QVBoxLayout *donutLayout = nullptr;
    QVBoxLayout *barsLayout = nullptr;
    auto *donutPanel = makePanel(tr("Spese per categoria · questo mese"), page, &donutLayout);
    m_donut = new DonutChart(donutPanel);
    m_donut->setEmptyText(tr("Nessuna spesa registrata questo mese."));
    donutLayout->addWidget(m_donut, 1);
    auto *barsPanel = makePanel(tr("Entrate e uscite · ultimi 6 mesi"), page, &barsLayout);
    m_bars = new MonthlyBarChart(barsPanel);
    m_bars->setEmptyText(tr("Ancora nessun dato negli ultimi 6 mesi."));
    barsLayout->addWidget(m_bars, 1);
    donutPanel->setMinimumHeight(250);
    barsPanel->setMinimumHeight(250);
    charts->addWidget(donutPanel, 1);
    charts->addWidget(barsPanel, 1);
    layout->addLayout(charts);

    QVBoxLayout *recentLayout = nullptr;
    auto *recentPanel = makePanel(tr("Movimenti recenti"), page, &recentLayout);
    m_recentTable = makeTable({tr("Data"), tr("Categoria"), tr("Nota"), tr("Tipo"), tr("Importo")}, recentPanel);
    m_recentTable->setMinimumHeight(250);
    QLabel *recentEmpty = nullptr;
    m_recentStack = withEmptyState(m_recentTable, &recentEmpty, recentPanel);
    recentEmpty->setText(tr("Nessuna transazione.\nPremi «+ Nuova transazione» per aggiungere la prima."));
    recentLayout->addWidget(m_recentStack);
    layout->addWidget(recentPanel, 1);

    auto *actions = new QHBoxLayout;
    auto *add = button(tr("+ Nuova transazione"), "primaryButton", page, tr("Ctrl+N"));
    auto *view = button(tr("Vedi tutte"), "secondaryButton", page);
    actions->addWidget(add);
    actions->addWidget(view);
    actions->addStretch();
    layout->addLayout(actions);
    connect(add, &QPushButton::clicked, this, &MainWindow::addTransaction);
    connect(view, &QPushButton::clicked, this, [this] { showPage(1); });
    return scroll;
}

QWidget *MainWindow::buildTransactionsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(14);
    layout->addWidget(pageTitle(tr("Transazioni"), page));

    QVBoxLayout *filterLayout = nullptr;
    auto *filters = makePanel(QString(), page, &filterLayout);
    filterLayout->setContentsMargins(12, 12, 12, 12);
    filterLayout->setSpacing(8);

    auto *row = new QHBoxLayout;
    row->setSpacing(8);
    m_filterPeriod = new QComboBox(filters);
    m_filterPeriod->addItems({tr("Tutto il periodo"), tr("Questo mese"), tr("Ultimi 30 giorni"),
                              tr("Quest'anno"), tr("Personalizzato…")});
    m_filterCategory = new QComboBox(filters);
    m_filterCategory->addItem(tr("Tutte le categorie"), 0);
    for (const QVariantMap &category : m_categories)
        m_filterCategory->addItem(category.value(QStringLiteral("Name")).toString(),
                                  category.value(QStringLiteral("ID_Category")).toInt());
    m_filterMin = new QDoubleSpinBox(filters);
    m_filterMax = new QDoubleSpinBox(filters);
    for (QDoubleSpinBox *spin : {m_filterMin, m_filterMax}) {
        spin->setRange(0, 999999999.99);
        spin->setDecimals(2);
        spin->setSuffix(QStringLiteral(" €"));
    }
    m_filterMin->setPrefix(tr("Min "));
    m_filterMin->setSpecialValueText(tr("Min €"));
    m_filterMax->setPrefix(tr("Max "));
    m_filterMax->setSpecialValueText(tr("Max €"));
    auto *reset = button(tr("Reset"), "secondaryButton", filters);
    row->addWidget(m_filterPeriod);
    row->addWidget(m_filterCategory, 1);
    row->addWidget(m_filterMin);
    row->addWidget(m_filterMax);
    row->addWidget(reset);
    filterLayout->addLayout(row);

    m_customRange = new QWidget(filters);
    m_customRange->setObjectName(QStringLiteral("transparentBox"));
    auto *rangeLayout = new QHBoxLayout(m_customRange);
    rangeLayout->setContentsMargins(0, 0, 0, 0);
    const QDate today = QDate::currentDate();
    m_filterFrom = new QDateEdit(QDate(today.year(), today.month(), 1), m_customRange);
    m_filterTo = new QDateEdit(today, m_customRange);
    for (QDateEdit *edit : {m_filterFrom, m_filterTo}) {
        edit->setCalendarPopup(true);
        edit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    }
    rangeLayout->addWidget(new QLabel(tr("Dal"), m_customRange));
    rangeLayout->addWidget(m_filterFrom);
    rangeLayout->addWidget(new QLabel(tr("Al"), m_customRange));
    rangeLayout->addWidget(m_filterTo);
    rangeLayout->addStretch();
    m_customRange->setVisible(false);
    filterLayout->addWidget(m_customRange);

    m_filterHint = new QLabel(filters);
    m_filterHint->setObjectName(QStringLiteral("warning"));
    m_filterHint->setVisible(false);
    filterLayout->addWidget(m_filterHint);
    layout->addWidget(filters);

    m_transactionsTable = makeTable(
        {tr("Data"), tr("Categoria"), tr("Nota"), tr("Tipo"), tr("Importo")}, page);
    m_transactionsTable->horizontalHeader()->setSortIndicator(0, Qt::DescendingOrder);
    m_transactionsTable->setSortingEnabled(true);
    m_transactionsStack = withEmptyState(m_transactionsTable, &m_transactionsEmpty, page);
    layout->addWidget(m_transactionsStack, 1);

    m_transactionsSummary = new QLabel(page);
    m_transactionsSummary->setObjectName(QStringLiteral("summaryLine"));
    layout->addWidget(m_transactionsSummary);

    auto *actions = new QHBoxLayout;
    auto *add = button(tr("+ Aggiungi"), "primaryButton", page, tr("Ctrl+N"));
    auto *edit = button(tr("Modifica"), "secondaryButton", page, tr("Invio o doppio clic"));
    auto *remove = button(tr("Elimina"), "secondaryButton", page, tr("Canc"));
    auto *csv = button(tr("Esporta CSV"), "secondaryButton", page,
                       tr("Esporta le transazioni attualmente filtrate"));
    actions->addWidget(add);
    actions->addWidget(edit);
    actions->addWidget(remove);
    actions->addStretch();
    actions->addWidget(csv);
    layout->addLayout(actions);

    connect(m_filterPeriod, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_customRange->setVisible(index == PeriodCustom);
        refreshTransactions();
    });
    connect(m_filterCategory, &QComboBox::currentIndexChanged, this, [this] { refreshTransactions(); });
    connect(m_filterFrom, &QDateEdit::dateChanged, this, [this] { refreshTransactions(); });
    connect(m_filterTo, &QDateEdit::dateChanged, this, [this] { refreshTransactions(); });
    connect(m_filterMin, &QDoubleSpinBox::editingFinished, this, [this] { refreshTransactions(); });
    connect(m_filterMax, &QDoubleSpinBox::editingFinished, this, [this] { refreshTransactions(); });
    connect(reset, &QPushButton::clicked, this, [this] {
        {
            const QSignalBlocker b1(m_filterPeriod), b2(m_filterCategory), b3(m_filterMin), b4(m_filterMax);
            m_filterPeriod->setCurrentIndex(0);
            m_filterCategory->setCurrentIndex(0);
            m_filterMin->setValue(0);
            m_filterMax->setValue(0);
        }
        m_customRange->setVisible(false);
        refreshTransactions();
    });
    connect(add, &QPushButton::clicked, this, &MainWindow::addTransaction);
    connect(edit, &QPushButton::clicked, this, &MainWindow::editTransaction);
    connect(remove, &QPushButton::clicked, this, &MainWindow::deleteTransaction);
    connect(csv, &QPushButton::clicked, this, &MainWindow::exportCsv);
    connect(m_transactionsTable, &QTableWidget::cellDoubleClicked, this, [this] { editTransaction(); });
    return page;
}

QWidget *MainWindow::buildCategoriesPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(16);
    layout->addWidget(pageTitle(tr("Categorie"), page));
    layout->addWidget(subTitle(tr("Organizza entrate e uscite con categorie dedicate."), page));
    m_categoriesTable = makeTable({tr("Categoria"), tr("Tipo")}, page);
    layout->addWidget(m_categoriesTable, 1);

    auto *actions = new QHBoxLayout;
    auto *add = button(tr("+ Nuova categoria"), "primaryButton", page);
    auto *rename = button(tr("Rinomina"), "secondaryButton", page, tr("Invio o doppio clic"));
    auto *remove = button(tr("Elimina"), "secondaryButton", page,
                          tr("Si possono eliminare solo le categorie non usate"));
    actions->addWidget(add);
    actions->addWidget(rename);
    actions->addWidget(remove);
    actions->addStretch();
    layout->addLayout(actions);
    connect(add, &QPushButton::clicked, this, &MainWindow::addCategory);
    connect(rename, &QPushButton::clicked, this, &MainWindow::renameCategory);
    connect(remove, &QPushButton::clicked, this, &MainWindow::deleteCategory);
    connect(m_categoriesTable, &QTableWidget::cellDoubleClicked, this, [this] { renameCategory(); });
    return page;
}

TransactionFilter MainWindow::currentFilter() const
{
    TransactionFilter filter;
    const QDate today = QDate::currentDate();
    QDate from;
    QDate to;
    switch (m_filterPeriod->currentIndex()) {
    case 1:
        from = QDate(today.year(), today.month(), 1);
        to = QDate(today.year(), today.month(), today.daysInMonth());
        break;
    case 2:
        from = today.addDays(-29);
        to = today;
        break;
    case 3:
        from = QDate(today.year(), 1, 1);
        to = QDate(today.year(), 12, 31);
        break;
    case PeriodCustom:
        from = m_filterFrom->date();
        to = m_filterTo->date();
        break;
    default:
        break;
    }
    if (from.isValid()) filter.dateFrom = from.toString(Qt::ISODate);
    if (to.isValid()) filter.dateTo = to.toString(Qt::ISODate);
    filter.categoryId = m_filterCategory->currentData().toInt();
    filter.minimumAmount = qRound64(m_filterMin->value() * 100.0);
    filter.maximumAmount = qRound64(m_filterMax->value() * 100.0);
    return filter;
}

QString MainWindow::filterProblem() const
{
    if (m_filterPeriod->currentIndex() == PeriodCustom && m_filterFrom->date() > m_filterTo->date())
        return tr("La data iniziale è successiva alla data finale.");
    if (m_filterMax->value() > 0 && m_filterMin->value() > m_filterMax->value())
        return tr("L'importo minimo è maggiore del massimo.");
    return QString();
}

bool MainWindow::filterIsActive() const
{
    return m_filterPeriod->currentIndex() != 0 || m_filterCategory->currentData().toInt() != 0 ||
           m_filterMin->value() > 0 || m_filterMax->value() > 0;
}

QString MainWindow::categoryName(int categoryId) const
{
    for (const QVariantMap &category : m_categories) {
        if (category.value(QStringLiteral("ID_Category")).toInt() == categoryId)
            return category.value(QStringLiteral("Name")).toString();
    }
    return tr("Senza categoria");
}

void MainWindow::refreshAll()
{
    if (!m_manager.getCategories(m_categories)) {
        showError(m_manager.lastError());
        return;
    }
    {
        const QSignalBlocker blocker(m_filterCategory);
        const int selected = m_filterCategory->currentData().toInt();
        m_filterCategory->clear();
        m_filterCategory->addItem(tr("Tutte le categorie"), 0);
        for (const QVariantMap &category : m_categories)
            m_filterCategory->addItem(category.value(QStringLiteral("Name")).toString(),
                                      category.value(QStringLiteral("ID_Category")).toInt());
        const int index = m_filterCategory->findData(selected);
        m_filterCategory->setCurrentIndex(index < 0 ? 0 : index);
    }
    refreshDashboard();
    refreshTransactions();
    refreshCategories();
}

void MainWindow::fillTransactionRow(QTableWidget *table, int row, const Transaction &t) const
{
    const QString isoKey = QStringLiteral("%1 %2").arg(t.getDate()).arg(t.getId(), 10, 10, QLatin1Char('0'));
    auto *date = cell(QDate::fromString(t.getDate(), Qt::ISODate).toString(QStringLiteral("dd/MM/yyyy")),
                      Qt::AlignVCenter | Qt::AlignLeft, isoKey);
    date->setData(Qt::UserRole, t.getId());
    table->setItem(row, 0, date);
    table->setItem(row, 1, cell(categoryName(t.getCategoryId())));
    auto *note = cell(t.getNote());
    note->setToolTip(t.getNote());
    table->setItem(row, 2, note);
    table->setItem(row, 3, cell(t.isIncome() ? tr("Entrata") : tr("Uscita")));
    const qint64 signedAmount = t.isIncome() ? t.getAmount() : -t.getAmount();
    auto *amount = cell((t.isIncome() ? QStringLiteral("+") : QStringLiteral("−")) + formatMoney(t.getAmount()),
                        Qt::AlignVCenter | Qt::AlignRight, QVariant::fromValue<qint64>(signedAmount));
    amount->setForeground(QColor(t.isIncome() ? QStringLiteral("#16845b") : QStringLiteral("#d94d64")));
    table->setItem(row, 4, amount);
}

void MainWindow::refreshDashboard()
{
    Statistics totals;
    qint64 balance = 0;
    if (!m_manager.getStatistics(m_accountId, totals) ||
        !m_manager.getAccountBalance(m_accountId, balance)) {
        showError(m_manager.lastError());
        return;
    }

    const QDate today = QDate::currentDate();
    const QDate monthStart(today.year(), today.month(), 1);
    TransactionFilter month;
    month.dateFrom = monthStart.toString(Qt::ISODate);
    month.dateTo = QDate(today.year(), today.month(), today.daysInMonth()).toString(Qt::ISODate);
    Statistics monthTotals;
    QList<CategoryTotal> byCategory;
    QList<MonthlyTotal> monthly;
    QList<Transaction> recent;
    const QDate firstMonth = monthStart.addMonths(-5);
    if (!m_manager.getStatistics(m_accountId, monthTotals, month) ||
        !m_manager.getExpensesByCategory(m_accountId, month, byCategory) ||
        !m_manager.getMonthlyTotals(m_accountId, firstMonth.toString(Qt::ISODate), monthly) ||
        !m_manager.getRecentTransactions(m_accountId, 6, recent)) {
        showError(m_manager.lastError());
        return;
    }

    m_balanceValue->setText(formatMoney(balance));
    m_incomeValue->setText(formatMoney(totals.getTotalIncome()));
    m_expensesValue->setText(formatMoney(totals.getTotalExpenses()));
    m_monthValue->setText(formatMoney(monthTotals.getTotalExpenses()));
    m_balanceValue->setStyleSheet(
        balance < 0 ? QStringLiteral("color:#d94d64;")
                    : QStringLiteral("color:%1;").arg(m_darkTheme ? "#9a8cff" : "#6658d3"));

    m_recentTable->setRowCount(recent.size());
    for (int row = 0; row < recent.size(); ++row)
        fillTransactionRow(m_recentTable, row, recent.at(row));
    m_recentStack->setCurrentIndex(recent.isEmpty() ? 1 : 0);

    // Top 5 expense categories of the month, everything else grouped as "Altre".
    QList<DonutSlice> slices;
    qint64 others = 0;
    for (int i = 0; i < byCategory.size(); ++i) {
        if (i < 5)
            slices.append({categoryName(byCategory.at(i).categoryId), byCategory.at(i).amount,
                           formatMoney(byCategory.at(i).amount), false});
        else
            others += byCategory.at(i).amount;
    }
    if (others > 0)
        slices.append({tr("Altre"), others, formatMoney(others), true});
    m_donut->setData(slices, formatMoney(monthTotals.getTotalExpenses()), tr("spese del mese"));

    QList<BarGroup> groups;
    qint64 maxValue = 0;
    for (int i = 0; i < 6; ++i) {
        const QDate d = firstMonth.addMonths(i);
        BarGroup group;
        group.label = italianMonth(d);
        const QString key = d.toString(QStringLiteral("yyyy-MM"));
        for (const MonthlyTotal &m : monthly) {
            if (m.month == key) {
                group.income = m.income;
                group.expenses = m.expenses;
            }
        }
        maxValue = std::max({maxValue, group.income, group.expenses});
        groups.append(group);
    }
    m_bars->setData(groups, formatMoney(maxValue));
}

void MainWindow::refreshTransactions()
{
    if (!m_transactionsTable)
        return;
    const QString problem = filterProblem();
    m_filterHint->setText(problem);
    m_filterHint->setVisible(!problem.isEmpty());
    if (!problem.isEmpty())
        return;

    QList<Transaction> transactions;
    if (!m_manager.getTransactions(m_accountId, transactions, currentFilter())) {
        showError(m_manager.lastError());
        return;
    }
    m_visibleTransactions = transactions;

    m_transactionsTable->setSortingEnabled(false);
    m_transactionsTable->setRowCount(transactions.size());
    for (int row = 0; row < transactions.size(); ++row)
        fillTransactionRow(m_transactionsTable, row, transactions.at(row));
    m_transactionsTable->setSortingEnabled(true); // re-applies the current sort indicator

    m_transactionsEmpty->setText(filterIsActive()
        ? tr("Nessuna transazione corrisponde ai filtri.\nProva a modificarli o premi Reset.")
        : tr("Nessuna transazione.\nPremi «+ Aggiungi» o Ctrl+N per inserire la prima."));
    m_transactionsStack->setCurrentIndex(transactions.isEmpty() ? 1 : 0);

    QString summary = transactions.size() == 1 ? tr("1 transazione")
                                               : tr("%1 transazioni").arg(transactions.size());
    try {
        const Statistics stats(transactions);
        summary += tr("   ·   Entrate %1   ·   Uscite %2   ·   Saldo %3")
                       .arg(formatMoney(stats.getTotalIncome()), formatMoney(stats.getTotalExpenses()),
                            formatMoney(stats.getBalance()));
    } catch (const std::overflow_error &) {
    }
    m_transactionsSummary->setText(summary);
}

void MainWindow::refreshCategories()
{
    if (!m_categoriesTable)
        return;
    m_categoriesTable->setRowCount(m_categories.size());
    for (int row = 0; row < m_categories.size(); ++row) {
        const QVariantMap &category = m_categories.at(row);
        auto *name = cell(category.value(QStringLiteral("Name")).toString());
        name->setData(Qt::UserRole, category.value(QStringLiteral("ID_Category")).toInt());
        m_categoriesTable->setItem(row, 0, name);
        const bool income = category.value(QStringLiteral("Type")).toString() == QStringLiteral("in");
        m_categoriesTable->setItem(row, 1, cell(income ? tr("Entrate") : tr("Uscite")));
    }
}

int MainWindow::selectedTransactionId() const
{
    const int row = m_transactionsTable ? m_transactionsTable->currentRow() : -1;
    if (row < 0 || !m_transactionsTable->item(row, 0))
        return 0;
    return m_transactionsTable->item(row, 0)->data(Qt::UserRole).toInt();
}

int MainWindow::selectedCategoryId() const
{
    const int row = m_categoriesTable ? m_categoriesTable->currentRow() : -1;
    if (row < 0 || !m_categoriesTable->item(row, 0))
        return 0;
    return m_categoriesTable->item(row, 0)->data(Qt::UserRole).toInt();
}

bool MainWindow::categoryNameTaken(const QString &name, const QString &type, int excludeId) const
{
    for (const QVariantMap &category : m_categories) {
        if (category.value(QStringLiteral("ID_Category")).toInt() != excludeId &&
            category.value(QStringLiteral("Type")).toString() == type &&
            category.value(QStringLiteral("Name")).toString().compare(name.trimmed(), Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
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
    const int id = selectedTransactionId();
    if (id == 0) {
        QMessageBox::information(this, tr("Seleziona una transazione"), tr("Seleziona una riga da modificare."));
        return;
    }
    const auto it = std::find_if(m_visibleTransactions.cbegin(), m_visibleTransactions.cend(),
                                 [id](const Transaction &t) { return t.getId() == id; });
    if (it == m_visibleTransactions.cend()) {
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
    statusBar()->showMessage(tr("Transazione modificata"), 3000);
}

void MainWindow::deleteTransaction()
{
    const int id = selectedTransactionId();
    if (id == 0) {
        QMessageBox::information(this, tr("Seleziona una transazione"), tr("Seleziona una riga da eliminare."));
        return;
    }
    if (QMessageBox::question(this, tr("Conferma eliminazione"),
                              tr("Vuoi eliminare questa transazione? Il saldo verrà aggiornato.")) != QMessageBox::Yes)
        return;
    if (!m_manager.deleteTransaction(id)) {
        showError(tr("Impossibile eliminare la transazione: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    statusBar()->showMessage(tr("Transazione eliminata"), 3000);
}

void MainWindow::exportCsv()
{
    const QString problem = filterProblem();
    if (!problem.isEmpty()) {
        QMessageBox::warning(this, tr("Filtri non validi"), problem);
        return;
    }
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
    const QString name = QInputDialog::getText(this, tr("Nuova categoria"), tr("Nome categoria:"),
                                               QLineEdit::Normal, QString(), &ok).trimmed();
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
    if (categoryNameTaken(name, type, 0)) {
        QMessageBox::warning(this, tr("Categoria già esistente"),
                             tr("Esiste già una categoria «%1» di questo tipo.").arg(name));
        return;
    }
    if (!m_manager.addCategory(name, type)) {
        showError(tr("Impossibile creare la categoria: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    statusBar()->showMessage(tr("Categoria aggiunta"), 3000);
}

void MainWindow::renameCategory()
{
    const int id = selectedCategoryId();
    if (id == 0) {
        QMessageBox::information(this, tr("Seleziona una categoria"), tr("Seleziona una categoria da rinominare."));
        return;
    }
    QString current;
    QString type;
    for (const QVariantMap &category : m_categories) {
        if (category.value(QStringLiteral("ID_Category")).toInt() == id) {
            current = category.value(QStringLiteral("Name")).toString();
            type = category.value(QStringLiteral("Type")).toString();
        }
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Rinomina categoria"), tr("Nuovo nome:"),
                                               QLineEdit::Normal, current, &ok).trimmed();
    if (!ok || name == current)
        return;
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Nome mancante"), tr("Inserisci un nome per la categoria."));
        return;
    }
    if (categoryNameTaken(name, type, id)) {
        QMessageBox::warning(this, tr("Categoria già esistente"),
                             tr("Esiste già una categoria «%1» di questo tipo.").arg(name));
        return;
    }
    if (!m_manager.updateCategory(id, name)) {
        showError(tr("Impossibile rinominare la categoria: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    statusBar()->showMessage(tr("Categoria rinominata"), 3000);
}

void MainWindow::deleteCategory()
{
    const int id = selectedCategoryId();
    if (id == 0) {
        QMessageBox::information(this, tr("Seleziona una categoria"), tr("Seleziona una categoria da eliminare."));
        return;
    }
    const QString name = categoryName(id);
    int usage = 0;
    if (!m_manager.getCategoryUsage(id, usage)) {
        showError(m_manager.lastError());
        return;
    }
    if (usage > 0) {
        QMessageBox::information(this, tr("Categoria in uso"),
                                 tr("La categoria «%1» è usata da %2 transazioni e non può essere eliminata.\n"
                                    "Modifica o elimina prima quelle transazioni.").arg(name).arg(usage));
        return;
    }
    if (QMessageBox::question(this, tr("Conferma eliminazione"),
                              tr("Vuoi eliminare la categoria «%1»?").arg(name)) != QMessageBox::Yes)
        return;
    if (!m_manager.deleteCategory(id)) {
        showError(tr("Impossibile eliminare la categoria: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    statusBar()->showMessage(tr("Categoria eliminata"), 3000);
}

void MainWindow::setInitialBalance()
{
    qint64 current = 0;
    if (!m_manager.getAccountInitialCredit(m_accountId, current)) {
        showError(m_manager.lastError());
        return;
    }
    bool ok = false;
    const double value = QInputDialog::getDouble(
        this, tr("Saldo iniziale"),
        tr("Saldo del conto prima della prima transazione (€).\nIl saldo attuale verrà aggiornato di conseguenza:"),
        current / 100.0, 0.0, 999999999.99, 2, &ok);
    if (!ok)
        return;
    if (!m_manager.setAccountInitialCredit(m_accountId, qRound64(value * 100.0))) {
        showError(tr("Impossibile aggiornare il saldo iniziale: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    statusBar()->showMessage(tr("Saldo iniziale aggiornato"), 3000);
}

void MainWindow::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Expense Tracker"), message);
}
