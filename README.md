# Expense Tracker

A C++ application for managing personal income and expenses, tracking financial balances, and organizing transactions by category. Data is stored persistently using an SQLite database.

## Features

## Features To be implemented

- Add, edit, and delete transactions
- Track income and expenses
- Organize transactions by category
- Filter transactions by date, category, or amount
- Calculate total income, expenses, and current balance
- Store data persistently using SQLite
- Export financial data to CSV

## Technologies

- **C++** — Core application logic
- **SQLite** — Data persistence
- **CMake** — Build system
- **GoogleTest** — Unit testing

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
- CMake 3.20 or newer
- SQLite3

## Installation

```bash
git clone https://github.com/Alessandrossxs/ExpenseTracker.git
cd ExpenseTracker
cmake -S . -B build
cmake --build build
```

## Example

```text
================================
       EXPENSE TRACKER
================================

1. Add transaction
2. View transactions
3. Edit transaction
4. Delete transaction
5. View financial summary
6. Export to CSV
0. Exit

Select an option:
```

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
