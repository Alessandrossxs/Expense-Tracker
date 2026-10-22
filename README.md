# Expense Tracker

A C++ application for managing personal income and expenses, tracking financial balances, and organizing transactions by category. Data is stored persistently using an SQLite database.

## Features

- Qt desktop interface with six pages: **Panoramica**, **Transazioni**, **Categorie**, **Ricorrenti**, **Conti**, and **Aspetto**
- Dashboard with current balance, total income, total expenses, this month's expenses, a spending-by-category chart, a 6-month income/expense chart, and the latest transactions
- Multiple accounts, switchable from the sidebar; create, rename, delete (empty accounts only) and set each account's initial balance
- Transfers between accounts, kept apart from income and expenses so they never count as earnings or spending
- Recurring incomes and expenses (daily, weekly, monthly, yearly, every N periods, optional end date): due transactions are created automatically, including those missed while the app was closed; rules can be paused, edited and deleted
- Add, edit, and delete transactions, each with an optional note
- Sortable transaction table (click a column header) with a totals line for the filtered list
- Filter by period (all, this month, last 30 days, this year, custom range), category, and amount range; filters apply instantly
- Add, rename, and delete categories (a category in use cannot be deleted; duplicate names are rejected)
- **Aspetto** page: pick the application color (presets or any `#RRGGBB`) and light/dark mode; backgrounds, buttons, selections and charts follow, and the choice is remembered
- Forms and confirmations open as in-app popups, with short notifications instead of extra windows
- Keyboard shortcuts: `Ctrl+N` new transaction, `Enter` edit, `Del` delete
- Store accounts, categories, transactions, transfers, and balances in SQLite
- Export filtered transaction data of the active account to CSV (`transaction_id,account_id,category_id,date,type,amount_cents,note`; transfers are not included)

## Technologies

- **C++** — Core application logic
- **Qt 6 Widgets and SQL** — Application framework and SQLite access
- **CMake** — Build system

## Project Structure

```text
Expense-Tracker/
├── .github/workflows/   # CI: build and run tests
├── src/                 # Implementation (UI, database, business logic)
├── include/             # Headers
├── tests/               # Database and statistics tests
├── CMakeLists.txt
├── LICENSE
├── README.md
└── .gitignore
```

## Requirements

- A C++17-compatible compiler or newer
- CMake 3.21 or newer
- Qt 6 with the Widgets and Sql modules
- The Qt SQLite SQL driver (`QSQLITE`)

## Installation

```bash
git clone https://github.com/Alessandrossxs/Expense-Tracker.git
cd Expense-Tracker
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage

Run the application from the directory where you want the data to live:

```bash
./build/ExpenseTracker
```

On first launch the app creates a SQLite database named `expense_tracker.db` in
the per-user application data folder (for example
`%APPDATA%\Alessandro\Expense Tracker` on Windows), together with a default
account and default categories. If a
database named `expense_tracker.db` exists in the directory you launch the app
from (older versions stored it there), it is copied to the new location on the
first run. Databases created by earlier versions are upgraded automatically.

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
`transaction_id,account_id,category_id,date,type,amount_cents`. Values are
written in cents, not formatted currency units.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
