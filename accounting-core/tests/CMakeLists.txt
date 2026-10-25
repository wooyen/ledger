find_package(GTest REQUIRED)
include(GoogleTest)

# ── 测试可执行文件 ──────────────────────────────────────────

add_executable(ledger_tests
    test_rational.cpp
    test_uuid.cpp
    # TODO: re-enable when domain model stabilizes
    # test_account_repo.cpp
    # test_balance_service.cpp
    # test_accounting_service.cpp
)

target_link_libraries(ledger_tests
    PRIVATE
        ledger_core
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(ledger_tests)
