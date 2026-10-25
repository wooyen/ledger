cmake_minimum_required(VERSION 3.24)
project(accounting-core
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Modern C++20 double-entry accounting engine"
)

# ── 全局设置 ─────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── 库类型 ───────────────────────────────────────────────
# 遵循 CMake 约定：该变量由用户通过 -DBUILD_SHARED_LIBS=ON/OFF 控制
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" ON)

# ── 可选后端 ─────────────────────────────────────────────
option(WITH_SQLITE   "Build with SQLite backend"         OFF)
option(WITH_PGSQL    "Build with PostgreSQL backend"      OFF)
option(WITH_MARIADB  "Build with MariaDB backend"         OFF)
option(WITH_EMBEDDED "Build with Embedded MariaDB"        OFF)
option(WITH_REDIS    "Build with Redis backend"           OFF)
option(WITH_XML      "Build with XML backend"             OFF)

option(BUILD_TESTS   "Build unit tests"                   ON)
option(BUILD_EXAMPLES "Build example programs"            OFF)

# ── 依赖查找 ─────────────────────────────────────────────
# TODO: re-enable when domain model stabilizes
# find_package(nlohmann_json REQUIRED)
# find_package(tl-expected   REQUIRED)

# ── 核心库源文件（始终编译） ──────────────────────────────
set(ledger_core_sources
    # Core types
    src/core/uuid.cpp
    src/core/rational.cpp
    src/core/allocator.cpp
    # TODO: re-enable when domain model stabilizes
    # src/repository/factory.cpp
    # src/service/balance.cpp
    # src/service/accounting.cpp
    # src/service/import_gnucash.cpp
    # src/ledger.cpp
    # # Memory backend — always compiled
    # src/repository/memory/memory_store.hpp
    # src/repository/memory/account_repo.cpp
    # src/repository/memory/transaction_repo.cpp
    # src/repository/memory/split_repo.cpp
    # src/repository/memory/price_repo.cpp
    # src/repository/memory/commodity_repo.cpp
    # src/repository/memory/snapshot_repo.cpp
)

add_library(ledger_core ${ledger_core_sources})

set_target_properties(ledger_core PROPERTIES
    # 静态库也开启 PIC，便于被其他共享库/可执行文件链接
    POSITION_INDEPENDENT_CODE ON
    # 仅对共享库生效，生成 libledger_core.so.0.1.0 及其符号链接
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(ledger_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ledger_core
    # TODO: re-enable when domain model stabilizes
    # PUBLIC
    #     nlohmann_json::nlohmann_json
    #     tl::expected
)

target_compile_features(ledger_core PUBLIC cxx_std_20)

# TODO: re-enable backends when repo implementations are compiled
# ── SQLite 后端 ──────────────────────────────────────────
if(WITH_SQLITE)
    find_package(SQLite3 REQUIRED)
    target_compile_definitions(ledger_core PUBLIC LEDGER_HAS_SQLITE)
    target_link_libraries(ledger_core PRIVATE SQLite::SQLite3)
    target_sources(ledger_core PRIVATE
        src/repository/sqlite/sqlite_store.hpp
        src/repository/sqlite/account_repo.cpp
        src/repository/sqlite/transaction_repo.cpp
        src/repository/sqlite/split_repo.cpp
        src/repository/sqlite/price_repo.cpp
        src/repository/sqlite/commodity_repo.cpp
        src/repository/sqlite/snapshot_repo.cpp
    )
endif()

# ── PostgreSQL 后端 ──────────────────────────────────────
if(WITH_PGSQL)
    find_package(PostgreSQL REQUIRED)
    target_compile_definitions(ledger_core PUBLIC LEDGER_HAS_PGSQL)
    target_link_libraries(ledger_core PRIVATE PostgreSQL::PostgreSQL)
    target_sources(ledger_core PRIVATE
        src/repository/postgresql/pgsql_store.hpp
        src/repository/postgresql/account_repo.cpp
        src/repository/postgresql/transaction_repo.cpp
        src/repository/postgresql/split_repo.cpp
        src/repository/postgresql/price_repo.cpp
        src/repository/postgresql/commodity_repo.cpp
        src/repository/postgresql/snapshot_repo.cpp
    )
endif()

# ── MariaDB 后端 ─────────────────────────────────────────
if(WITH_MARIADB)
    find_package(MariaDB REQUIRED)
    target_compile_definitions(ledger_core PUBLIC LEDGER_HAS_MARIADB)
    target_link_libraries(ledger_core PRIVATE MariaDB::MariaDB)
    target_sources(ledger_core PRIVATE
        src/repository/mariadb/mariadb_store.hpp
        src/repository/mariadb/account_repo.cpp
        src/repository/mariadb/transaction_repo.cpp
        src/repository/mariadb/split_repo.cpp
        src/repository/mariadb/price_repo.cpp
        src/repository/mariadb/commodity_repo.cpp
        src/repository/mariadb/snapshot_repo.cpp
    )
endif()

# ── Embedded MariaDB 后端 ────────────────────────────────
if(WITH_EMBEDDED)
    find_package(MariaDBEmbedded REQUIRED)
    target_compile_definitions(ledger_core PUBLIC LEDGER_HAS_EMBEDDED_MARIADB)
    target_link_libraries(ledger_core PRIVATE MariaDBEmbedded::MariaDBEmbedded)
    target_sources(ledger_core PRIVATE
        src/repository/mariadb_embedded/embedded_store.hpp
        src/repository/mariadb_embedded/account_repo.cpp
        src/repository/mariadb_embedded/transaction_repo.cpp
        src/repository/mariadb_embedded/split_repo.cpp
        src/repository/mariadb_embedded/price_repo.cpp
        src/repository/mariadb_embedded/commodity_repo.cpp
        src/repository/mariadb_embedded/snapshot_repo.cpp
    )
endif()

# ── Redis 后端 ───────────────────────────────────────────
if(WITH_REDIS)
    find_package(hiredis REQUIRED)
    target_compile_definitions(ledger_core PUBLIC LEDGER_HAS_REDIS)
    target_link_libraries(ledger_core PRIVATE hiredis::hiredis)
    target_sources(ledger_core PRIVATE
        src/repository/redis/redis_store.hpp
        src/repository/redis/account_repo.cpp
        src/repository/redis/transaction_repo.cpp
        src/repository/redis/split_repo.cpp
        src/repository/redis/price_repo.cpp
        src/repository/redis/commodity_repo.cpp
        src/repository/redis/snapshot_repo.cpp
    )
endif()

# ── XML 后端 ─────────────────────────────────────────────
if(WITH_XML)
    find_package(pugixml REQUIRED)
    target_compile_definitions(ledger_core PUBLIC LEDGER_HAS_XML)
    target_link_libraries(ledger_core PRIVATE pugixml::pugixml)
    target_sources(ledger_core PRIVATE
        src/repository/xml/xml_store.hpp
        src/repository/xml/account_repo.cpp
        src/repository/xml/transaction_repo.cpp
        src/repository/xml/split_repo.cpp
        src/repository/xml/price_repo.cpp
        src/repository/xml/commodity_repo.cpp
        src/repository/xml/snapshot_repo.cpp
    )
endif()

# ── 测试 ─────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── 示例 ─────────────────────────────────────────────────
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
