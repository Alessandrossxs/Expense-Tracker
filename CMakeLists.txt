cmake_minimum_required(VERSION 3.21)

project(ExpenseTracker VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql LinguistTools)

qt_standard_project_setup(I18N_SOURCE_LANGUAGE it_IT)

qt_add_executable(ExpenseTracker
    src/main.cpp
    src/MainWindow.cpp
    src/TransactionDialog.cpp
    src/Database.cpp
    src/ExpenseManager.cpp
    src/Transaction.cpp
    src/Statistics.cpp
    include/MainWindow.h
    include/ComboPopupDelegate.h
    include/CategoryTranslations.h
    include/TransactionDialog.h
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

qt_add_translations(ExpenseTracker
    IMMEDIATE_CALL
    TS_FILES
        translations/ExpenseTracker_en.ts
        translations/ExpenseTracker_it.ts
    QM_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/translations"
    QM_FILES_OUTPUT_VARIABLE EXPENSE_TRACKER_TRANSLATION_FILES
)
add_dependencies(ExpenseTracker ExpenseTracker_lrelease)

add_custom_command(TARGET ExpenseTracker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:ExpenseTracker>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${EXPENSE_TRACKER_TRANSLATION_FILES}
        "$<TARGET_FILE_DIR:ExpenseTracker>/translations"
    VERBATIM
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

    add_executable(TranslationTests tests/TranslationTests.cpp)
    set_target_properties(TranslationTests PROPERTIES AUTOMOC OFF)
    target_link_libraries(TranslationTests PRIVATE Qt6::Core)
    add_dependencies(TranslationTests ExpenseTracker_lrelease)

    add_test(
        NAME TranslationTests
        COMMAND TranslationTests
            "${CMAKE_CURRENT_BINARY_DIR}/translations/ExpenseTracker_en.qm"
            "${CMAKE_CURRENT_BINARY_DIR}/translations/ExpenseTracker_it.qm"
    )
endif()
