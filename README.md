# Expense Tracker

A C++ application for managing personal income and expenses, tracking financial balances, and organizing transactions by category. Data is stored persistently using an SQLite database.

## Features

- Add, edit, and delete transactions
- Track income and expenses
- Organize transactions by category
- Filter transactions by date range, category, and amount range
- Calculate total income, expenses, and current balance
- Switch between light and dark themes and customize the accent palette
- Use the interface in Italian or English
- Store accounts, categories, transactions, and balances in SQLite
- Track multiple checking, debit-card, credit-card, savings, and cash accounts
- Transfer money between accounts without counting transfers as income or expenses
- Export filtered transaction data to CSV

## Technologies

- **C++** — Core application logic
- **Qt 6 Widgets and SQL** — Application framework and SQLite access
- **CMake** — Build system

## Project Structure

```text
ExpenseTracker/
├── src/
├── include/
├── tests/
├── translations/
├── data/
├── CMakeLists.txt
├── README.md
└── .gitignore
```

## Requirements

- A C++17-compatible compiler or newer
- CMake 3.21 or newer
- Qt 6 with the Widgets, Sql, and LinguistTools modules
- The Qt SQLite SQL driver (`QSQLITE`)

## Installation

```bash
git clone https://github.com/Alessandrossxs/ExpenseTracker.git
cd ExpenseTracker
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

### Windows installer

On Windows with Qt 6, MinGW, CMake, and IExpress installed, build a self-extracting
per-user installer from PowerShell:

```powershell
.\packaging\Build-Installer.ps1
```

The script builds a separate Release configuration with `BUILD_TESTING=OFF`,
deploys the Qt and MinGW runtime files, and writes
`dist\ExpenseTracker-Setup.exe`. Run the installer to choose a per-user install
folder and optionally create a desktop shortcut. The uninstaller removes the
installed application files and shortcuts but preserves account and transaction
databases. The setup and uninstaller run under the current PowerShell execution
policy; neither script overrides it.

The SQLite database is stored as a normal local file and is not encrypted by
the application. Expense Tracker is a local ledger, not a multi-user security
boundary: selecting an account does not authenticate a user or restrict access
to that account's data. The legacy `Password_Hash` database column is retained
for compatibility and is not used for authentication. Anyone who can read the
database file can read or modify its contents. Protect it with your operating
system account and device security (including full-disk encryption where
available), and keep backups in a secure location. CSV exports are also
unencrypted. The installer is not code-signed, so obtain it only from a trusted
release source.

## Amounts, accounts, and filters

Transaction amounts and account balances are stored as integer cents to avoid
floating-point rounding. Pass amounts to the C++ API in cents (for example,
`1234` represents 12.34 in the user's currency). An account's balance starts
at its initial credit and is adjusted atomically whenever a transaction is
added, edited, or deleted.

Transaction filters are optional. Empty dates, category ID `0`, minimum amount
`0`, and maximum amount `0` mean that the corresponding filter is disabled.
Dates use the ISO format `YYYY-MM-DD`; amount bounds are inclusive.

CSV exports use UTF-8 and contain the columns
`transaction_id,account_id,category_id,date,type,amount_cents,notes`. Values
are written in cents, not formatted currency units. CSV files are unencrypted;
the application warns before creating one. Store exports only in a location
protected by your Windows account and device encryption.

## Application entry point

The Qt interface is implemented in `src/MainWindow.cpp` and launched by
`src/main.cpp`. The application stores its database in the per-user application data
directory. On first launch after this change, it attempts to copy an existing
legacy database from the previous workspace/current-directory locations; the
source file is kept as a backup. If migration fails, the app continues using the
legacy file rather than silently creating an empty database. Appearance
settings, including language, are saved locally.
Choose Italian or English in the Appearance page; the new language takes effect
the next time the application starts.

Use `Ctrl+1` through `Ctrl+5` to switch between the sidebar pages, `Ctrl+N` to
add a transaction from anywhere, and `Ctrl+F` to open transaction search. In
the search field, `Enter` selects the first result and `Esc` clears the query.
The Tutorial button at the bottom of the sidebar starts a step-by-step tour
through the actual dashboard, transactions, categories, analysis, and appearance
pages. The tour stays open and non-modal as it moves between sections.

Use the account selector in the sidebar to switch between account ledgers. New
accounts have an independent opening balance and type. Internal transfers are
recorded separately, update both account balances atomically, and are excluded
from income/expense analysis.

The account manager shows each account's balance, transaction count, and
transfer count. Account names and types can be edited without changing history.
Removal is limited to accounts with no transactions, no transfers, a zero
balance, and at least one remaining account.

The dashboard shows the selected account's current balance, this month's
income, expenses, and net balance, alongside the balance of every account and
the combined balance. Its six-month trend and latest six transactions follow
the selected account. Use Analysis for historical periods, comparisons, and
expense-category breakdowns.

Transactions can include optional notes. Notes are shown in the transaction
list, included in CSV exports, and searchable from the transaction search box.

The application opens maximized in a standard window. Its Analysis page includes monthly
income/expense trends, category breakdowns, and period insights.
Checking, debit-card, credit-card, and other accounts can be viewed over the
last 1, 3, or 6 months, navigable one month at a time with the previous/next
controls. Cash and savings accounts keep their all-time analysis.

Translation catalogs are maintained in `translations/ExpenseTracker_it.ts` and
`translations/ExpenseTracker_en.ts`. CMake compiles them into `.qm` files in
`build/translations`, alongside the application runtime files.

## Example menu design

```text
================================
       EXPENSE TRACKER
================================

1. Add transaction
2. View transactions
3. Edit transaction
4. Delete transaction
5. View financial summary
6. Filter transactions
7. Export to CSV
0. Exit

Select an option:
```

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
