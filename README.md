# Expense Tracker

A C++ application for managing personal income and expenses, tracking financial balances, and organizing transactions by category. Data is stored persistently using an SQLite database.

## Features

- Add, edit, and delete transactions
- Track income and expenses
- Organize transactions by category
- Filter transactions by date range, category, and amount range
- Calculate total income, expenses, and current balance
- Store accounts, categories, transactions, and balances in SQLite
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
├── data/
├── CMakeLists.txt
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
git clone https://github.com/Alessandrossxs/ExpenseTracker.git
cd ExpenseTracker
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

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

## Application entry point

The current entry point is intentionally a minimal Qt window. Database access,
transaction management, statistics, filtering, and CSV export are available
through the C++ classes and can be connected to a UI in `src/main.cpp`.

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
