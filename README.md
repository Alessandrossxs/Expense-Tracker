# Expense Tracker

A C++ application for managing personal income and expenses, tracking financial balances, and organizing transactions by category. Data is stored persistently using an SQLite database.

## Features

- Qt desktop interface with three pages: **Panoramica** (dashboard), **Transazioni**, and **Categorie**
- Dashboard with current balance, total income, total expenses, this month's expenses, and recent transactions
- Add, edit, and delete transactions
- Organize transactions by category (default categories are created on first launch; new ones can be added from the UI)
- Filter transactions by date range, category, and amount range
- Store accounts, categories, transactions, and balances in SQLite
- Export filtered transaction data to CSV

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

On first launch the app creates a SQLite database named `expense_tracker.db`
in the **current working directory**, together with a default account and a set
of default categories. Launching it from a different directory creates or opens
a different database. The `.db` file is ignored by Git.

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
