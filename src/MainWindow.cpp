#include "MainWindow.h"

#include "ChartWidgets.h"
#include "Format.h"
#include "Logo.h"
#include "Popup.h"
#include "RecurringForm.h"
#include "Theme.h"
#include "TransactionForm.h"

#include <QAbstractItemView>
#include <QButtonGroup>
#include <QColor>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
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
#include <QStyledItemDelegate>
#include <QStandardPaths>
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

// Selected rows use the highlight text color, even for cells that set their own
// foreground (green/red/blue amounts), so the selection always has full contrast.
class SelectionDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        if (option->state & QStyle::State_Selected) {
            const QBrush highlighted = option->palette.brush(QPalette::HighlightedText);
            option->palette.setBrush(QPalette::Text, highlighted);
            option->palette.setBrush(QPalette::WindowText, highlighted);
            option->palette.setBrush(QPalette::HighlightedText, highlighted);
        }
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
    table->setItemDelegate(new SelectionDelegate(table));
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

QString italianMonth(const QDate &date)
{
    QString name = QLocale(QLocale::Italian).monthName(date.month(), QLocale::ShortFormat);
    name.remove(QLatin1Char('.'));
    return name.left(1).toUpper() + name.mid(1);
}

QString placeholderHash()
{
    return QString::fromLatin1(QCryptographicHash::hash(QByteArrayLiteral("expense-tracker-local-account"),
                                                       QCryptographicHash::Sha256).toHex());
}

// A vertical group of widgets used as popup content.
QWidget *column(const QList<QWidget *> &widgets)
{
    auto *box = new QWidget;
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);
    for (QWidget *w : widgets)
        layout->addWidget(w);
    return box;
}

QLabel *hint(const QString &text)
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("subTitle"));
    label->setWordWrap(true);
    return label;
}

QDoubleSpinBox *moneySpin(double minimum, double value = 0.0)
{
    auto *spin = new QDoubleSpinBox;
    spin->setRange(minimum, 999999999.99);
    spin->setDecimals(2);
    spin->setSingleStep(1.0);
    spin->setSuffix(QStringLiteral(" €"));
    spin->setValue(value);
    return spin;
}

QString nameEditError(const QLineEdit *edit)
{
    return edit->text().trimmed().isEmpty() ? QObject::tr("Inserisci un nome.") : QString();
}

const QList<QColor> &presetColors()
{
    static const QList<QColor> colors = {
        QColor("#6658d3"), QColor("#3b82f6"), QColor("#0ea5e9"), QColor("#14b8a6"), QColor("#22c55e"),
        QColor("#eab308"), QColor("#f97316"), QColor("#ef4444"), QColor("#ec4899"), QColor("#64748b")};
    return colors;
}
}

MainWindow::MainWindow(const QString &databasePath, QWidget *parent)
    : QMainWindow(parent), m_manager(databasePath), m_databasePath(databasePath)
{
    setWindowTitle(tr("Expense Tracker"));
    resize(1180, 780);
    setMinimumSize(980, 660);

    QSettings settings;
    m_darkTheme = settings.value(QStringLiteral("theme/dark"), false).toBool();
    m_accent = QColor(settings.value(QStringLiteral("theme/accent"), QStringLiteral("#6658d3")).toString());
    if (!m_accent.isValid())
        m_accent = QColor(QStringLiteral("#6658d3"));

    if (!initializeData()) {
        QTimer::singleShot(0, this, [this] { close(); });
        return;
    }
    buildUi();
    applyTheme();

    new QShortcut(QKeySequence::New, this, [this] { if (!Popup::isOpen()) addTransaction(); });
    new QShortcut(QKeySequence(Qt::Key_Delete), m_transactionsTable,
                  [this] { if (!Popup::isOpen()) deleteTransaction(); }, Qt::WidgetShortcut);
    new QShortcut(QKeySequence(Qt::Key_Return), m_transactionsTable,
                  [this] { if (!Popup::isOpen()) editTransaction(); }, Qt::WidgetShortcut);
    new QShortcut(QKeySequence(Qt::Key_Enter), m_transactionsTable,
                  [this] { if (!Popup::isOpen()) editTransaction(); }, Qt::WidgetShortcut);

    refreshAll();

    // Creates the recurring transactions that became due (also while the app was closed).
    m_recurringTimer = new QTimer(this);
    m_recurringTimer->setInterval(30 * 60 * 1000);
    connect(m_recurringTimer, &QTimer::timeout, this, [this] { if (!Popup::isOpen()) processRecurring(); });
    m_recurringTimer->start();
    QTimer::singleShot(300, this, [this] { processRecurring(); });
}

bool MainWindow::initializeData()
{
    if (!m_manager.open()) {
        showError(tr("Impossibile aprire il database: %1").arg(m_manager.lastError()));
        return false;
    }
    if (!m_manager.getAccounts(m_accounts)) {
        showError(tr("Impossibile leggere i conti: %1").arg(m_manager.lastError()));
        return false;
    }
    if (m_accounts.isEmpty()) {
        int id = 0;
        if (!m_manager.addAccount(tr("Conto principale"), placeholderHash(),
                                  QDate::currentDate().toString(Qt::ISODate), 0, &id) ||
            !m_manager.getAccounts(m_accounts)) {
            showError(tr("Impossibile creare il conto iniziale: %1").arg(m_manager.lastError()));
            return false;
        }
    }
    m_accountId = m_accounts.first().value(QStringLiteral("ID_Account")).toInt();
    const int saved = QSettings().value(QStringLiteral("account/active"), 0).toInt();
    for (const QVariantMap &account : m_accounts) {
        if (account.value(QStringLiteral("ID_Account")).toInt() == saved)
            m_accountId = saved;
    }

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

void MainWindow::applyTheme()
{
    setStyleSheet(buildStyleSheet(m_accent, m_darkTheme));
    setWindowIcon(appIcon(m_accent));
    if (m_logoLabel)
        m_logoLabel->setPixmap(logoPixmap(42, m_accent));
    const QColor accent = effectiveAccent(m_accent, m_darkTheme);
    if (m_donut) {
        m_donut->setDarkMode(m_darkTheme);
        m_donut->setAccent(accent);
    }
    if (m_bars)
        m_bars->setDarkMode(m_darkTheme);
    QSettings settings;
    settings.setValue(QStringLiteral("theme/dark"), m_darkTheme);
    settings.setValue(QStringLiteral("theme/accent"), m_accent.name());
    syncAppearanceControls();
    if (m_balanceValue)
        refreshDashboard();
}

void MainWindow::setAccent(const QColor &color)
{
    if (!color.isValid())
        return;
    m_accent = color;
    applyTheme();
}

void MainWindow::setDarkTheme(bool dark)
{
    m_darkTheme = dark;
    applyTheme();
}

void MainWindow::syncAppearanceControls()
{
    for (QPushButton *swatch : m_swatches) {
        const QColor c(swatch->property("color").toString());
        const bool selected = c.name() == m_accent.name();
        const QSignalBlocker blocker(swatch);
        swatch->setChecked(selected);
        swatch->setText(selected ? QStringLiteral("✓") : QString());
        swatch->setStyleSheet(
            QStringLiteral("QPushButton { background: %1; border: none; border-radius: 17px; "
                           "min-width: 34px; max-width: 34px; min-height: 34px; max-height: 34px; "
                           "color: %2; font-weight: 700; font-size: 13pt; padding: 0; } "
                           "QPushButton:hover { border: 3px solid rgba(255, 255, 255, 130); }")
                .arg(c.name(), readableOn(c).name()));
    }
    if (m_lightButton && m_darkButton) {
        const QSignalBlocker b1(m_lightButton), b2(m_darkButton);
        m_lightButton->setChecked(!m_darkTheme);
        m_darkButton->setChecked(m_darkTheme);
    }
    if (m_hexEdit && !m_hexEdit->hasFocus())
        m_hexEdit->setText(m_accent.name().toUpper());
}

void MainWindow::buildUi()
{
    auto *root = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *sidebar = new QFrame(root);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setFixedWidth(230);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(16, 22, 16, 20);
    sideLayout->setSpacing(6);
    auto *brandRow = new QWidget(sidebar);
    brandRow->setObjectName(QStringLiteral("transparentBox"));
    auto *brandLayout = new QHBoxLayout(brandRow);
    brandLayout->setContentsMargins(2, 8, 2, 18);
    brandLayout->setSpacing(12);
    m_logoLabel = new QLabel(brandRow);
    m_logoLabel->setFixedSize(42, 42);
    brandLayout->addWidget(m_logoLabel);
    auto *brandNames = new QVBoxLayout;
    brandNames->setContentsMargins(0, 0, 0, 0);
    brandNames->setSpacing(0);
    auto *brandTitle = new QLabel(tr("Expense"), brandRow);
    brandTitle->setObjectName(QStringLiteral("brandTitle"));
    auto *brandSubtitle = new QLabel(tr("TRACKER"), brandRow);
    brandSubtitle->setObjectName(QStringLiteral("brandSubtitle"));
    QFont spaced = brandSubtitle->font();
    spaced.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
    brandSubtitle->setFont(spaced);
    brandNames->addWidget(brandTitle);
    brandNames->addWidget(brandSubtitle);
    brandLayout->addLayout(brandNames, 1);
    sideLayout->addWidget(brandRow);

    auto *caption = new QLabel(tr("CONTO ATTIVO"), sidebar);
    caption->setObjectName(QStringLiteral("sidebarCaption"));
    sideLayout->addWidget(caption);
    m_accountCombo = new QComboBox(sidebar);
    sideLayout->addWidget(m_accountCombo);
    sideLayout->addSpacing(14);

    m_pages = new QStackedWidget(root);
    auto *group = new QButtonGroup(this);
    group->setExclusive(true);
    const QStringList navLabels = {tr("Panoramica"), tr("Transazioni"), tr("Categorie"), tr("Ricorrenti"), tr("Conti"), tr("Aspetto")};
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
    auto *footer = new QLabel(tr("Gestione personale\nDati salvati in locale"), sidebar);
    footer->setObjectName(QStringLiteral("sidebarFooter"));
    sideLayout->addWidget(footer);

    m_pages->addWidget(buildDashboardPage());
    m_pages->addWidget(buildTransactionsPage());
    m_pages->addWidget(buildCategoriesPage());
    m_pages->addWidget(buildRecurringPage());
    m_pages->addWidget(buildAccountsPage());
    m_pages->addWidget(buildAppearancePage());
    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(m_pages, 1);
    setCentralWidget(root);

    connect(m_accountCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0)
            switchAccount(m_accountCombo->itemData(index).toInt());
    });

    m_toast = new QLabel(this);
    m_toast->setObjectName(QStringLiteral("toast"));
    m_toast->hide();
    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, m_toast, &QWidget::hide);
}

void MainWindow::showPage(int index)
{
    m_pages->setCurrentIndex(index);
    if (index >= 0 && index < m_navButtons.size())
        m_navButtons.at(index)->setChecked(true);
    if (index == 1) refreshTransactions();
    if (index == 2) refreshCategories();
    if (index == 3) refreshRecurring();
    if (index == 4) refreshAccounts();
}

void MainWindow::showToast(const QString &text)
{
    m_toast->setText(text);
    m_toast->adjustSize();
    m_toast->move((width() - m_toast->width()) / 2, height() - m_toast->height() - 30);
    m_toast->show();
    m_toast->raise();
    m_toastTimer->start(2800);
}

void MainWindow::showError(const QString &message)
{
    if (centralWidget())
        Popup::message(this, tr("Qualcosa non ha funzionato"), message);
    else
        QMessageBox::critical(this, tr("Expense Tracker"), message);
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
    m_dashSubtitle = subTitle(QString(), page);
    titles->addWidget(m_dashSubtitle);
    header->addLayout(titles, 1);
    auto *initial = button(tr("Saldo iniziale…"), "secondaryButton", page,
                           tr("Imposta il saldo di partenza del conto attivo"));
    header->addWidget(initial, 0, Qt::AlignTop);
    layout->addLayout(header);
    connect(initial, &QPushButton::clicked, this, [this] { setInitialBalance(m_accountId); });

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
    m_filterMin = moneySpin(0);
    m_filterMax = moneySpin(0);
    m_filterMin->setParent(filters);
    m_filterMax->setParent(filters);
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
    auto *transfer = button(tr("⇄ Trasferisci"), "secondaryButton", page,
                            tr("Sposta denaro tra due conti"));
    auto *edit = button(tr("Modifica"), "secondaryButton", page, tr("Invio o doppio clic"));
    auto *remove = button(tr("Elimina"), "secondaryButton", page, tr("Canc"));
    auto *csv = button(tr("Esporta CSV"), "secondaryButton", page,
                       tr("Esporta le transazioni attualmente filtrate"));
    actions->addWidget(add);
    actions->addWidget(transfer);
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
    connect(transfer, &QPushButton::clicked, this, &MainWindow::addTransfer);
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
    auto *rename = button(tr("Rinomina"), "secondaryButton", page, tr("Doppio clic sulla riga"));
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

QWidget *MainWindow::buildAccountsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(16);
    layout->addWidget(pageTitle(tr("Conti"), page));
    layout->addWidget(subTitle(tr("Gestisci più conti e sposta denaro tra di loro. "
                                  "I trasferimenti non contano come entrate o spese."), page));

    auto *top = new QHBoxLayout;
    QLabel *total = nullptr;
    top->addWidget(metricCard(tr("Patrimonio totale"), &total, page));
    m_totalValue = total;
    top->addStretch(1);
    layout->addLayout(top);

    m_accountsTable = makeTable({tr("Conto"), tr("Saldo iniziale"), tr("Saldo attuale")}, page);
    layout->addWidget(m_accountsTable, 1);

    auto *actions = new QHBoxLayout;
    auto *add = button(tr("+ Nuovo conto"), "primaryButton", page);
    auto *transfer = button(tr("⇄ Trasferisci"), "secondaryButton", page, tr("Sposta denaro tra due conti"));
    auto *rename = button(tr("Rinomina"), "secondaryButton", page);
    auto *initial = button(tr("Saldo iniziale…"), "secondaryButton", page);
    auto *remove = button(tr("Elimina"), "secondaryButton", page,
                          tr("Si possono eliminare solo i conti senza movimenti"));
    actions->addWidget(add);
    actions->addWidget(transfer);
    actions->addWidget(rename);
    actions->addWidget(initial);
    actions->addWidget(remove);
    actions->addStretch();
    layout->addLayout(actions);
    connect(add, &QPushButton::clicked, this, &MainWindow::addAccount);
    connect(transfer, &QPushButton::clicked, this, &MainWindow::addTransfer);
    connect(rename, &QPushButton::clicked, this, &MainWindow::renameAccount);
    connect(initial, &QPushButton::clicked, this, [this] {
        const int id = selectedAccountId();
        if (id == 0)
            Popup::message(this, tr("Seleziona un conto"), tr("Seleziona prima un conto dalla tabella."));
        else
            setInitialBalance(id);
    });
    connect(remove, &QPushButton::clicked, this, &MainWindow::deleteAccount);
    connect(m_accountsTable, &QTableWidget::cellDoubleClicked, this, [this] {
        const int id = selectedAccountId();
        if (id != 0) {
            switchAccount(id);
            showPage(0);
        }
    });
    return page;
}

QWidget *MainWindow::buildAppearancePage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(16);
    layout->addWidget(pageTitle(tr("Aspetto"), page));
    layout->addWidget(subTitle(tr("Scegli il colore dell'applicazione: sfondi, pulsanti e grafici si adattano subito."), page));

    QVBoxLayout *colorLayout = nullptr;
    auto *colorPanel = makePanel(tr("Colore dell'applicazione"), page, &colorLayout);
    auto *swatchRow = new QHBoxLayout;
    swatchRow->setSpacing(10);
    for (const QColor &color : presetColors()) {
        auto *swatch = new QPushButton(colorPanel);
        swatch->setCheckable(true);
        swatch->setCursor(Qt::PointingHandCursor);
        swatch->setProperty("color", color.name());
        swatch->setToolTip(color.name().toUpper());
        m_swatches.append(swatch);
        swatchRow->addWidget(swatch);
        connect(swatch, &QPushButton::clicked, this, [this, color] { setAccent(color); });
    }
    swatchRow->addStretch();
    colorLayout->addLayout(swatchRow);

    auto *hexRow = new QHBoxLayout;
    hexRow->setSpacing(10);
    m_hexEdit = new QLineEdit(colorPanel);
    m_hexEdit->setMaxLength(7);
    m_hexEdit->setPlaceholderText(QStringLiteral("#RRGGBB"));
    m_hexEdit->setFixedWidth(160);
    auto *apply = button(tr("Applica"), "primaryButton", colorPanel);
    auto *restore = button(tr("Ripristina predefinito"), "secondaryButton", colorPanel);
    hexRow->addWidget(Popup::field(tr("Colore personalizzato"), m_hexEdit, colorPanel));
    hexRow->addWidget(apply, 0, Qt::AlignBottom);
    hexRow->addWidget(restore, 0, Qt::AlignBottom);
    hexRow->addStretch();
    colorLayout->addLayout(hexRow);
    m_hexHint = new QLabel(colorPanel);
    m_hexHint->setObjectName(QStringLiteral("warning"));
    m_hexHint->hide();
    colorLayout->addWidget(m_hexHint);
    layout->addWidget(colorPanel);

    auto applyHex = [this] {
        QString text = m_hexEdit->text().trimmed();
        if (!text.startsWith(QLatin1Char('#')))
            text.prepend(QLatin1Char('#'));
        const QColor c(text);
        if (!c.isValid() || text.size() != 7) {
            m_hexHint->setText(tr("Colore non valido: usa il formato #RRGGBB, per esempio #3B82F6."));
            m_hexHint->show();
            return;
        }
        m_hexHint->hide();
        setAccent(c);
        showToast(tr("Colore applicato"));
    };
    connect(apply, &QPushButton::clicked, this, applyHex);
    connect(m_hexEdit, &QLineEdit::returnPressed, this, applyHex);
    connect(restore, &QPushButton::clicked, this, [this] {
        m_hexHint->hide();
        setAccent(presetColors().first());
    });

    QVBoxLayout *modeLayout = nullptr;
    auto *modePanel = makePanel(tr("Modalità"), page, &modeLayout);
    auto *modeRow = new QHBoxLayout;
    modeRow->setSpacing(10);
    m_lightButton = button(tr("☀  Chiaro"), "segment", modePanel);
    m_darkButton = button(tr("☾  Scuro"), "segment", modePanel);
    m_lightButton->setCheckable(true);
    m_darkButton->setCheckable(true);
    auto *modeGroup = new QButtonGroup(this);
    modeGroup->setExclusive(true);
    modeGroup->addButton(m_lightButton);
    modeGroup->addButton(m_darkButton);
    modeRow->addWidget(m_lightButton);
    modeRow->addWidget(m_darkButton);
    modeRow->addStretch();
    modeLayout->addLayout(modeRow);
    layout->addWidget(modePanel);
    layout->addStretch();
    connect(m_lightButton, &QPushButton::clicked, this, [this] { setDarkTheme(false); });
    connect(m_darkButton, &QPushButton::clicked, this, [this] { setDarkTheme(true); });
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

QString MainWindow::accountName(int accountId) const
{
    for (const QVariantMap &account : m_accounts) {
        if (account.value(QStringLiteral("ID_Account")).toInt() == accountId)
            return account.value(QStringLiteral("Name")).toString();
    }
    return tr("Conto sconosciuto");
}

void MainWindow::refreshAll()
{
    if (!m_manager.getCategories(m_categories)) {
        showError(m_manager.lastError());
        return;
    }
    refreshAccounts();
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
    refreshRecurring();
}

void MainWindow::refreshAccounts()
{
    if (!m_manager.getAccounts(m_accounts)) {
        showError(m_manager.lastError());
        return;
    }
    bool activeExists = false;
    for (const QVariantMap &account : m_accounts)
        activeExists = activeExists || account.value(QStringLiteral("ID_Account")).toInt() == m_accountId;
    if (!activeExists && !m_accounts.isEmpty())
        m_accountId = m_accounts.first().value(QStringLiteral("ID_Account")).toInt();

    {
        const QSignalBlocker blocker(m_accountCombo);
        m_accountCombo->clear();
        for (const QVariantMap &account : m_accounts)
            m_accountCombo->addItem(account.value(QStringLiteral("Name")).toString(),
                                    account.value(QStringLiteral("ID_Account")));
        m_accountCombo->setCurrentIndex(m_accountCombo->findData(m_accountId));
    }

    qint64 total = 0;
    m_accountsTable->setRowCount(m_accounts.size());
    for (int row = 0; row < m_accounts.size(); ++row) {
        const QVariantMap &account = m_accounts.at(row);
        const int id = account.value(QStringLiteral("ID_Account")).toInt();
        const qint64 credit = account.value(QStringLiteral("Credit")).toLongLong();
        total += credit;
        auto *name = cell((id == m_accountId ? QStringLiteral("● ") : QString()) +
                          account.value(QStringLiteral("Name")).toString());
        name->setData(Qt::UserRole, id);
        m_accountsTable->setItem(row, 0, name);
        m_accountsTable->setItem(row, 1, cell(formatMoney(account.value(QStringLiteral("Initial_Credit")).toLongLong()),
                                              Qt::AlignVCenter | Qt::AlignRight));
        auto *balance = cell(formatMoney(credit), Qt::AlignVCenter | Qt::AlignRight);
        balance->setForeground(QColor(credit < 0 ? QStringLiteral("#d94d64") : QString()));
        m_accountsTable->setItem(row, 2, balance);
    }
    m_totalValue->setText(formatMoney(total));
}

void MainWindow::switchAccount(int accountId)
{
    if (accountId == m_accountId || accountId == 0)
        return;
    m_accountId = accountId;
    QSettings().setValue(QStringLiteral("account/active"), accountId);
    refreshAll();
}

void MainWindow::fillTransactionRow(QTableWidget *table, int row, const Transaction &t) const
{
    const QString isoKey = QStringLiteral("%1 %2").arg(t.getDate()).arg(t.getId(), 10, 10, QLatin1Char('0'));
    auto *date = cell(QDate::fromString(t.getDate(), Qt::ISODate).toString(QStringLiteral("dd/MM/yyyy")),
                      Qt::AlignVCenter | Qt::AlignLeft, isoKey);
    date->setData(Qt::UserRole, t.getId());
    date->setData(Qt::UserRole + 2, false);
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

void MainWindow::fillTransferRow(QTableWidget *table, int row, const Transfer &t) const
{
    const bool outgoing = t.fromAccountId == m_accountId;
    const QString isoKey = QStringLiteral("%1 %2").arg(t.date).arg(t.id, 10, 10, QLatin1Char('0'));
    auto *date = cell(QDate::fromString(t.date, Qt::ISODate).toString(QStringLiteral("dd/MM/yyyy")),
                      Qt::AlignVCenter | Qt::AlignLeft, isoKey);
    date->setData(Qt::UserRole, t.id);
    date->setData(Qt::UserRole + 2, true);
    table->setItem(row, 0, date);
    table->setItem(row, 1, cell(outgoing ? tr("→ %1").arg(accountName(t.toAccountId))
                                         : tr("← %1").arg(accountName(t.fromAccountId))));
    auto *note = cell(t.note);
    note->setToolTip(t.note);
    table->setItem(row, 2, note);
    table->setItem(row, 3, cell(tr("Trasferimento")));
    const qint64 signedAmount = outgoing ? -t.amount : t.amount;
    auto *amount = cell((outgoing ? QStringLiteral("−") : QStringLiteral("+")) + formatMoney(t.amount),
                        Qt::AlignVCenter | Qt::AlignRight, QVariant::fromValue<qint64>(signedAmount));
    amount->setForeground(QColor(QStringLiteral("#3b9ae1"))); // neutral: not income, not expense
    table->setItem(row, 4, amount);
}

void MainWindow::refreshDashboard()
{
    m_dashSubtitle->setText(tr("Conto «%1» · un riepilogo chiaro delle tue finanze.").arg(accountName(m_accountId)));
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
        QStringLiteral("color:%1;").arg(balance < 0 ? QStringLiteral("#d94d64")
                                                    : effectiveAccent(m_accent, m_darkTheme).name()));

    m_recentTable->setRowCount(recent.size());
    for (int row = 0; row < recent.size(); ++row)
        fillTransactionRow(m_recentTable, row, recent.at(row));
    m_recentStack->setCurrentIndex(recent.isEmpty() ? 1 : 0);

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

    const TransactionFilter filter = currentFilter();
    QList<Transaction> transactions;
    QList<Transfer> transfers;
    if (!m_manager.getTransactions(m_accountId, transactions, filter) ||
        (filter.categoryId == 0 && !m_manager.getTransfers(m_accountId, transfers, filter))) {
        showError(m_manager.lastError());
        return;
    }
    m_visibleTransactions = transactions;

    m_transactionsTable->setSortingEnabled(false);
    m_transactionsTable->setRowCount(transactions.size() + transfers.size());
    int row = 0;
    for (const Transaction &t : transactions)
        fillTransactionRow(m_transactionsTable, row++, t);
    for (const Transfer &t : transfers)
        fillTransferRow(m_transactionsTable, row++, t);
    m_transactionsTable->setSortingEnabled(true); // re-applies the current sort indicator

    const bool empty = transactions.isEmpty() && transfers.isEmpty();
    m_transactionsEmpty->setText(filterIsActive()
        ? tr("Nessun movimento corrisponde ai filtri.\nProva a modificarli o premi Reset.")
        : tr("Nessun movimento su questo conto.\nPremi «+ Aggiungi» o Ctrl+N per inserire il primo."));
    m_transactionsStack->setCurrentIndex(empty ? 1 : 0);

    QString summary = transactions.size() == 1 ? tr("1 transazione") : tr("%1 transazioni").arg(transactions.size());
    try {
        const Statistics stats(transactions);
        summary += tr("   ·   Entrate %1   ·   Uscite %2   ·   Saldo %3")
                       .arg(formatMoney(stats.getTotalIncome()), formatMoney(stats.getTotalExpenses()),
                            formatMoney(stats.getBalance()));
    } catch (const std::overflow_error &) {
    }
    if (!transfers.isEmpty())
        summary += tr("   ·   %n trasferimenti (esclusi dai totali)", nullptr, transfers.size());
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

MainWindow::RowRef MainWindow::selectedTransactionRow() const
{
    const int row = m_transactionsTable ? m_transactionsTable->currentRow() : -1;
    if (row < 0 || !m_transactionsTable->item(row, 0))
        return {};
    const QTableWidgetItem *item = m_transactionsTable->item(row, 0);
    return {item->data(Qt::UserRole).toInt(), item->data(Qt::UserRole + 2).toBool()};
}

int MainWindow::selectedCategoryId() const
{
    const int row = m_categoriesTable ? m_categoriesTable->currentRow() : -1;
    return (row < 0 || !m_categoriesTable->item(row, 0)) ? 0 : m_categoriesTable->item(row, 0)->data(Qt::UserRole).toInt();
}

int MainWindow::selectedAccountId() const
{
    const int row = m_accountsTable ? m_accountsTable->currentRow() : -1;
    return (row < 0 || !m_accountsTable->item(row, 0)) ? 0 : m_accountsTable->item(row, 0)->data(Qt::UserRole).toInt();
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
    if (Popup::isOpen())
        return;
    auto *form = new TransactionForm(m_categories, m_accountId);
    if (!Popup::run(this, tr("Nuova transazione"), form, tr("Salva"), [form] { return form->validate(); }))
        return;
    if (!m_manager.addTransaction(form->transaction())) {
        showError(tr("Impossibile salvare la transazione: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Transazione aggiunta"));
}

void MainWindow::editTransaction()
{
    const RowRef ref = selectedTransactionRow();
    if (ref.id == 0) {
        Popup::message(this, tr("Seleziona una transazione"), tr("Seleziona una riga da modificare."));
        return;
    }
    if (ref.transfer) {
        Popup::message(this, tr("Trasferimento"),
                       tr("I trasferimenti non si possono modificare: eliminalo e creane uno nuovo."));
        return;
    }
    const auto it = std::find_if(m_visibleTransactions.cbegin(), m_visibleTransactions.cend(),
                                 [&ref](const Transaction &t) { return t.getId() == ref.id; });
    if (it == m_visibleTransactions.cend()) {
        showError(tr("La transazione selezionata non è più disponibile."));
        return;
    }
    auto *form = new TransactionForm(m_categories, m_accountId);
    form->setTransaction(*it);
    if (!Popup::run(this, tr("Modifica transazione"), form, tr("Salva"), [form] { return form->validate(); }))
        return;
    if (!m_manager.updateTransaction(form->transaction())) {
        showError(tr("Impossibile modificare la transazione: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Transazione modificata"));
}

void MainWindow::deleteTransaction()
{
    const RowRef ref = selectedTransactionRow();
    if (ref.id == 0) {
        Popup::message(this, tr("Seleziona una transazione"), tr("Seleziona una riga da eliminare."));
        return;
    }
    const QString text = ref.transfer
        ? tr("Vuoi eliminare questo trasferimento? I saldi dei due conti verranno ripristinati.")
        : tr("Vuoi eliminare questa transazione? Il saldo verrà aggiornato.");
    if (!Popup::confirm(this, tr("Conferma eliminazione"), text, tr("Elimina"), true))
        return;
    const bool ok = ref.transfer ? m_manager.deleteTransfer(ref.id) : m_manager.deleteTransaction(ref.id);
    if (!ok) {
        showError(tr("Impossibile eliminare: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(ref.transfer ? tr("Trasferimento eliminato") : tr("Transazione eliminata"));
}

void MainWindow::exportCsv()
{
    const QString problem = filterProblem();
    if (!problem.isEmpty()) {
        Popup::message(this, tr("Filtri non validi"), problem);
        return;
    }
    auto *path = new QLineEdit(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                   .filePath(QStringLiteral("transazioni.csv")));
    auto *content = column({Popup::field(tr("Percorso del file CSV"), path),
                            hint(tr("Verranno esportate le transazioni attualmente filtrate del conto attivo "
                                    "(i trasferimenti sono esclusi)."))});
    if (!Popup::run(this, tr("Esporta in CSV"), content, tr("Esporta"), [path] {
            return path->text().trimmed().isEmpty() ? QObject::tr("Inserisci il percorso del file.") : QString();
        }))
        return;
    if (!m_manager.exportTransactionsToCsv(m_accountId, path->text().trimmed(), currentFilter())) {
        showError(tr("Esportazione non riuscita: %1").arg(m_manager.lastError()));
        return;
    }
    showToast(tr("CSV esportato correttamente"));
}

void MainWindow::addCategory()
{
    auto *name = new QLineEdit;
    name->setPlaceholderText(tr("Es. Abbonamenti"));
    name->setMaxLength(60);
    auto *type = new QComboBox;
    type->addItem(tr("Uscita"), QStringLiteral("out"));
    type->addItem(tr("Entrata"), QStringLiteral("in"));
    auto *content = column({Popup::field(tr("Nome categoria"), name), Popup::field(tr("Tipo"), type)});
    if (!Popup::run(this, tr("Nuova categoria"), content, tr("Crea"), [this, name, type] {
            if (name->text().trimmed().isEmpty())
                return tr("Inserisci un nome per la categoria.");
            if (categoryNameTaken(name->text(), type->currentData().toString(), 0))
                return tr("Esiste già una categoria «%1» di questo tipo.").arg(name->text().trimmed());
            return QString();
        }))
        return;
    if (!m_manager.addCategory(name->text().trimmed(), type->currentData().toString())) {
        showError(tr("Impossibile creare la categoria: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Categoria aggiunta"));
}

void MainWindow::renameCategory()
{
    const int id = selectedCategoryId();
    if (id == 0) {
        Popup::message(this, tr("Seleziona una categoria"), tr("Seleziona una categoria da rinominare."));
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
    auto *name = new QLineEdit(current);
    name->setMaxLength(60);
    if (!Popup::run(this, tr("Rinomina categoria"), Popup::field(tr("Nuovo nome"), name), tr("Salva"),
                    [this, name, type, id] {
                        if (name->text().trimmed().isEmpty())
                            return tr("Inserisci un nome per la categoria.");
                        if (categoryNameTaken(name->text(), type, id))
                            return tr("Esiste già una categoria «%1» di questo tipo.").arg(name->text().trimmed());
                        return QString();
                    }))
        return;
    if (name->text().trimmed() == current)
        return;
    if (!m_manager.updateCategory(id, name->text().trimmed())) {
        showError(tr("Impossibile rinominare la categoria: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Categoria rinominata"));
}

void MainWindow::deleteCategory()
{
    const int id = selectedCategoryId();
    if (id == 0) {
        Popup::message(this, tr("Seleziona una categoria"), tr("Seleziona una categoria da eliminare."));
        return;
    }
    const QString name = categoryName(id);
    int usage = 0;
    if (!m_manager.getCategoryUsage(id, usage)) {
        showError(m_manager.lastError());
        return;
    }
    if (usage > 0) {
        Popup::message(this, tr("Categoria in uso"),
                       tr("La categoria «%1» è usata da %2 movimenti o ricorrenze e non può essere eliminata.\n"
                          "Modifica o elimina prima quelle transazioni.").arg(name).arg(usage));
        return;
    }
    if (!Popup::confirm(this, tr("Conferma eliminazione"), tr("Vuoi eliminare la categoria «%1»?").arg(name),
                        tr("Elimina"), true))
        return;
    if (!m_manager.deleteCategory(id)) {
        showError(tr("Impossibile eliminare la categoria: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Categoria eliminata"));
}

void MainWindow::addAccount()
{
    auto *name = new QLineEdit;
    name->setPlaceholderText(tr("Es. Carta, Risparmi, Contanti"));
    name->setMaxLength(60);
    auto *initial = moneySpin(0);
    auto *content = column({Popup::field(tr("Nome del conto"), name),
                            Popup::field(tr("Saldo iniziale"), initial)});
    if (!Popup::run(this, tr("Nuovo conto"), content, tr("Crea"), [name] { return nameEditError(name); }))
        return;
    int id = 0;
    if (!m_manager.addAccount(name->text().trimmed(), placeholderHash(),
                              QDate::currentDate().toString(Qt::ISODate), qRound64(initial->value() * 100.0), &id)) {
        showError(tr("Impossibile creare il conto: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Conto creato"));
}

void MainWindow::renameAccount()
{
    const int id = selectedAccountId();
    if (id == 0) {
        Popup::message(this, tr("Seleziona un conto"), tr("Seleziona prima un conto dalla tabella."));
        return;
    }
    auto *name = new QLineEdit(accountName(id));
    name->setMaxLength(60);
    if (!Popup::run(this, tr("Rinomina conto"), Popup::field(tr("Nuovo nome"), name), tr("Salva"),
                    [name] { return nameEditError(name); }))
        return;
    if (!m_manager.renameAccount(id, name->text().trimmed())) {
        showError(tr("Impossibile rinominare il conto: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Conto rinominato"));
}

void MainWindow::deleteAccount()
{
    const int id = selectedAccountId();
    if (id == 0) {
        Popup::message(this, tr("Seleziona un conto"), tr("Seleziona prima un conto dalla tabella."));
        return;
    }
    const QString name = accountName(id);
    if (m_accounts.size() <= 1) {
        Popup::message(this, tr("Impossibile eliminare"), tr("Non puoi eliminare l'ultimo conto rimasto."));
        return;
    }
    QList<Transaction> any;
    QList<Transfer> transfers;
    QList<RecurringRule> rules;
    if (!m_manager.getRecentTransactions(id, 1, any) || !m_manager.getTransfers(id, transfers) ||
        !m_manager.getRecurring(id, rules)) {
        showError(m_manager.lastError());
        return;
    }
    if (!any.isEmpty() || !transfers.isEmpty() || !rules.isEmpty()) {
        Popup::message(this, tr("Conto con movimenti"),
                       tr("Il conto «%1» contiene transazioni, trasferimenti o ricorrenze e non può essere eliminato.").arg(name));
        return;
    }
    if (!Popup::confirm(this, tr("Conferma eliminazione"), tr("Vuoi eliminare il conto «%1»?").arg(name),
                        tr("Elimina"), true))
        return;
    if (!m_manager.deleteAccount(id)) {
        showError(tr("Impossibile eliminare il conto: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Conto eliminato"));
}

void MainWindow::setInitialBalance(int accountId)
{
    qint64 current = 0;
    if (!m_manager.getAccountInitialCredit(accountId, current)) {
        showError(m_manager.lastError());
        return;
    }
    auto *spin = moneySpin(0, current / 100.0);
    auto *content = column({Popup::field(tr("Saldo iniziale"), spin),
                            hint(tr("Il saldo attuale del conto «%1» verrà aggiornato di conseguenza.").arg(accountName(accountId)))});
    if (!Popup::run(this, tr("Saldo iniziale"), content, tr("Salva")))
        return;
    if (!m_manager.setAccountInitialCredit(accountId, qRound64(spin->value() * 100.0))) {
        showError(tr("Impossibile aggiornare il saldo iniziale: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Saldo iniziale aggiornato"));
}

void MainWindow::addTransfer()
{
    if (Popup::isOpen())
        return;
    if (m_accounts.size() < 2) {
        Popup::message(this, tr("Servono due conti"),
                       tr("Per trasferire denaro crea almeno due conti dalla pagina Conti."));
        return;
    }
    auto *from = new QComboBox;
    auto *to = new QComboBox;
    for (const QVariantMap &account : m_accounts) {
        const QString label = QStringLiteral("%1  ·  %2")
                                  .arg(account.value(QStringLiteral("Name")).toString(),
                                       formatMoney(account.value(QStringLiteral("Credit")).toLongLong()));
        from->addItem(label, account.value(QStringLiteral("ID_Account")));
        to->addItem(label, account.value(QStringLiteral("ID_Account")));
    }
    from->setCurrentIndex(from->findData(m_accountId));
    to->setCurrentIndex(from->currentIndex() == 0 ? 1 : 0);
    auto *amount = moneySpin(0.01, 1.0);
    auto *date = new QDateEdit(QDate::currentDate());
    date->setCalendarPopup(true);
    date->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    auto *note = new QLineEdit;
    note->setMaxLength(200);
    note->setPlaceholderText(tr("Facoltativa, es. Risparmio mensile"));

    auto *row = new QWidget;
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);
    rowLayout->addWidget(Popup::field(tr("Importo"), amount));
    rowLayout->addWidget(Popup::field(tr("Data"), date));
    auto *content = column({Popup::field(tr("Da"), from), Popup::field(tr("A"), to), row,
                            Popup::field(tr("Nota"), note)});
    if (!Popup::run(this, tr("Trasferisci denaro"), content, tr("Trasferisci"), [from, to] {
            return from->currentData().toInt() == to->currentData().toInt()
                       ? QObject::tr("Scegli due conti diversi.") : QString();
        }))
        return;
    if (!m_manager.addTransfer(from->currentData().toInt(), to->currentData().toInt(),
                               qRound64(amount->value() * 100.0), date->date().toString(Qt::ISODate),
                               note->text().trimmed())) {
        showError(tr("Impossibile eseguire il trasferimento: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Trasferimento eseguito"));
}

namespace
{
QString frequencyText(const RecurringRule &r)
{
    const int n = r.interval;
    if (r.frequency == QLatin1String("daily"))
        return n == 1 ? QObject::tr("Ogni giorno") : QObject::tr("Ogni %1 giorni").arg(n);
    if (r.frequency == QLatin1String("weekly"))
        return n == 1 ? QObject::tr("Ogni settimana") : QObject::tr("Ogni %1 settimane").arg(n);
    if (r.frequency == QLatin1String("monthly"))
        return n == 1 ? QObject::tr("Ogni mese") : QObject::tr("Ogni %1 mesi").arg(n);
    return n == 1 ? QObject::tr("Ogni anno") : QObject::tr("Ogni %1 anni").arg(n);
}

bool ruleFinished(const RecurringRule &r)
{
    return !r.endDate.isEmpty() && r.nextDate > r.endDate;
}
}

QWidget *MainWindow::buildRecurringPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(30, 28, 30, 26);
    layout->setSpacing(16);
    layout->addWidget(pageTitle(tr("Ricorrenti"), page));
    layout->addWidget(subTitle(tr("Entrate e uscite che si ripetono, come affitto, abbonamenti o stipendio: "
                                  "le transazioni vengono create da sole alla data prevista."), page));

    m_recurringTable = makeTable({tr("Descrizione"), tr("Categoria"), tr("Importo"), tr("Frequenza"),
                                  tr("Prossima"), tr("Stato")}, page);
    QLabel *empty = nullptr;
    m_recurringStack = withEmptyState(m_recurringTable, &empty, page);
    empty->setText(tr("Nessuna ricorrenza su questo conto.\nPremi «+ Nuova ricorrenza» per crearne una."));
    layout->addWidget(m_recurringStack, 1);

    auto *actions = new QHBoxLayout;
    auto *add = button(tr("+ Nuova ricorrenza"), "primaryButton", page);
    auto *edit = button(tr("Modifica"), "secondaryButton", page, tr("Doppio clic sulla riga"));
    auto *toggle = button(tr("Pausa / Riprendi"), "secondaryButton", page,
                          tr("Le esecuzioni saltate durante la pausa non vengono create"));
    auto *remove = button(tr("Elimina"), "secondaryButton", page,
                          tr("Le transazioni già create restano nel conto"));
    actions->addWidget(add);
    actions->addWidget(edit);
    actions->addWidget(toggle);
    actions->addWidget(remove);
    actions->addStretch();
    layout->addLayout(actions);
    connect(add, &QPushButton::clicked, this, &MainWindow::addRecurring);
    connect(edit, &QPushButton::clicked, this, &MainWindow::editRecurring);
    connect(toggle, &QPushButton::clicked, this, &MainWindow::toggleRecurring);
    connect(remove, &QPushButton::clicked, this, &MainWindow::deleteRecurring);
    connect(m_recurringTable, &QTableWidget::cellDoubleClicked, this, [this] { editRecurring(); });
    return page;
}

void MainWindow::refreshRecurring()
{
    if (!m_recurringTable)
        return;
    if (!m_manager.getRecurring(m_accountId, m_rules)) {
        showError(m_manager.lastError());
        return;
    }
    m_recurringTable->setRowCount(m_rules.size());
    for (int row = 0; row < m_rules.size(); ++row) {
        const RecurringRule &r = m_rules.at(row);
        const bool finished = ruleFinished(r);
        auto *name = cell(r.note.isEmpty() ? categoryName(r.categoryId) : r.note);
        name->setData(Qt::UserRole, r.id);
        m_recurringTable->setItem(row, 0, name);
        m_recurringTable->setItem(row, 1, cell(categoryName(r.categoryId)));
        auto *amount = cell((r.isIncome ? QStringLiteral("+") : QStringLiteral("−")) + formatMoney(r.amount),
                            Qt::AlignVCenter | Qt::AlignRight);
        amount->setForeground(QColor(r.isIncome ? QStringLiteral("#16845b") : QStringLiteral("#d94d64")));
        m_recurringTable->setItem(row, 2, amount);
        m_recurringTable->setItem(row, 3, cell(frequencyText(r)));
        m_recurringTable->setItem(row, 4, cell(finished ? QStringLiteral("—")
            : QDate::fromString(r.nextDate, Qt::ISODate).toString(QStringLiteral("dd/MM/yyyy"))));
        m_recurringTable->setItem(row, 5, cell(!r.active ? tr("In pausa") : finished ? tr("Conclusa") : tr("Attiva")));
    }
    m_recurringStack->setCurrentIndex(m_rules.isEmpty() ? 1 : 0);
}

int MainWindow::selectedRecurringId() const
{
    const int row = m_recurringTable ? m_recurringTable->currentRow() : -1;
    return (row < 0 || !m_recurringTable->item(row, 0)) ? 0 : m_recurringTable->item(row, 0)->data(Qt::UserRole).toInt();
}

int MainWindow::processRecurring(bool announce)
{
    int created = 0;
    if (!m_manager.processRecurring(QDate::currentDate().toString(Qt::ISODate), created)) {
        showError(tr("Impossibile creare le transazioni ricorrenti: %1").arg(m_manager.lastError()));
        return 0;
    }
    if (created > 0 && announce) {
        refreshAll();
        showToast(created == 1 ? tr("1 transazione ricorrente creata")
                               : tr("%1 transazioni ricorrenti create").arg(created));
    }
    return created;
}

void MainWindow::addRecurring()
{
    if (Popup::isOpen())
        return;
    auto *form = new RecurringForm(m_categories, m_accountId, false);
    if (!Popup::run(this, tr("Nuova ricorrenza"), form, tr("Salva"), [form] { return form->validate(); }))
        return;
    if (!m_manager.addRecurring(form->rule())) {
        showError(tr("Impossibile salvare la ricorrenza: %1").arg(m_manager.lastError()));
        return;
    }
    const int created = processRecurring(false); // back-dated start: create the missed ones now
    refreshAll();
    showToast(created > 0 ? tr("Ricorrenza creata · %1 transazioni aggiunte").arg(created)
                          : tr("Ricorrenza creata"));
}

void MainWindow::editRecurring()
{
    const int id = selectedRecurringId();
    if (id == 0) {
        Popup::message(this, tr("Seleziona una ricorrenza"), tr("Seleziona una riga da modificare."));
        return;
    }
    const auto it = std::find_if(m_rules.cbegin(), m_rules.cend(),
                                 [id](const RecurringRule &r) { return r.id == id; });
    if (it == m_rules.cend())
        return;
    auto *form = new RecurringForm(m_categories, m_accountId, true);
    form->setRule(*it);
    if (!Popup::run(this, tr("Modifica ricorrenza"), form, tr("Salva"), [form] { return form->validate(); }))
        return;
    RecurringRule rule = form->rule();
    rule.id = id;
    rule.accountId = m_accountId;
    if (!m_manager.updateRecurring(rule)) {
        showError(tr("Impossibile modificare la ricorrenza: %1").arg(m_manager.lastError()));
        return;
    }
    const int created = processRecurring(false);
    refreshAll();
    showToast(created > 0 ? tr("Ricorrenza aggiornata · %1 transazioni aggiunte").arg(created)
                          : tr("Ricorrenza aggiornata"));
}

void MainWindow::toggleRecurring()
{
    const int id = selectedRecurringId();
    if (id == 0) {
        Popup::message(this, tr("Seleziona una ricorrenza"), tr("Seleziona prima una riga dalla tabella."));
        return;
    }
    const auto it = std::find_if(m_rules.cbegin(), m_rules.cend(),
                                 [id](const RecurringRule &r) { return r.id == id; });
    if (it == m_rules.cend())
        return;
    const bool activate = !it->active;
    if (!m_manager.setRecurringActive(id, activate, QDate::currentDate().toString(Qt::ISODate))) {
        showError(tr("Impossibile aggiornare la ricorrenza: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(activate ? tr("Ricorrenza ripresa") : tr("Ricorrenza messa in pausa"));
}

void MainWindow::deleteRecurring()
{
    const int id = selectedRecurringId();
    if (id == 0) {
        Popup::message(this, tr("Seleziona una ricorrenza"), tr("Seleziona una riga da eliminare."));
        return;
    }
    const auto it = std::find_if(m_rules.cbegin(), m_rules.cend(),
                                 [id](const RecurringRule &r) { return r.id == id; });
    const QString name = it == m_rules.cend() ? QString()
        : (it->note.isEmpty() ? categoryName(it->categoryId) : it->note);
    if (!Popup::confirm(this, tr("Conferma eliminazione"),
                        tr("Vuoi eliminare la ricorrenza «%1»?\nLe transazioni già create restano nel conto.").arg(name),
                        tr("Elimina"), true))
        return;
    if (!m_manager.deleteRecurring(id)) {
        showError(tr("Impossibile eliminare la ricorrenza: %1").arg(m_manager.lastError()));
        return;
    }
    refreshAll();
    showToast(tr("Ricorrenza eliminata"));
}
