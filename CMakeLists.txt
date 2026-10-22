cmake_minimum_required(VERSION 3.21)

project(ExpenseTracker VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

qt_standard_project_setup()

qt_add_executable(ExpenseTracker
    src/main.cpp
    src/MainWindow.cpp
    src/ChartWidgets.cpp
    src/TransactionForm.cpp
    src/RecurringForm.cpp
    src/Popup.cpp
    src/Theme.cpp
    src/Database.cpp
    src/ExpenseManager.cpp
    src/Transaction.cpp
    src/Statistics.cpp
    include/MainWindow.h
    include/ChartWidgets.h
    include/Format.h
    include/TransactionForm.h
    include/RecurringForm.h
    include/Recurring.h
    include/Popup.h
    include/Theme.h
    include/Transfer.h
    include/Database.h
    include/ExpenseManager.h
    include/Statistics.h
    include/Transaction.h
)

target_include_directories(ExpenseTracker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ExpenseTracker PRIVATE
    Qt6::Widgets
    Qt6::Sql
)

include(CTest)

if(BUILD_TESTING)
    add_executable(DatabaseTests
        tests/DatabaseTests.cpp
        src/Database.cpp
        src/ExpenseManager.cpp
        src/Transaction.cpp
        src/Statistics.cpp
        include/Database.h
        include/ExpenseManager.h
        include/Statistics.h
        include/Transaction.h
    )

    target_include_directories(DatabaseTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(DatabaseTests PRIVATE
        Qt6::Sql
    )

    add_test(NAME DatabaseTests COMMAND DatabaseTests)

    add_executable(FeatureTests
        tests/FeatureTests.cpp
        src/Database.cpp
        src/ExpenseManager.cpp
        src/Transaction.cpp
        src/Statistics.cpp
    )
    target_include_directories(FeatureTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(FeatureTests PRIVATE Qt6::Sql)
    add_test(NAME FeatureTests COMMAND FeatureTests)
endif()
