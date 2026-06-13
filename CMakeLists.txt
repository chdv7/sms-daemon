cmake_minimum_required(VERSION 3.10)

project(sms_project LANGUAGES C CXX)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -Wall
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wall>)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall>)
endif()

# По умолчанию Debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---- Общая библиотека ----
set(SMS_CORE_SOURCES
    ${SRC_DIR}/PduSMS.cpp
    ${SRC_DIR}/serial.cpp
    ${SRC_DIR}/rsserial.cpp
    ${SRC_DIR}/xmlParser.cpp
    ${SRC_DIR}/RecvSMS.cpp
    ${SRC_DIR}/ut.cpp
    ${SRC_DIR}/gen-xml.cpp
    ${SRC_DIR}/Ussd.cpp
    ${SRC_DIR}/Config.cpp
)

add_library(sms-core STATIC ${SMS_CORE_SOURCES})

# ---- Демон ----
set(DAEMON_SOURCES
    ${SRC_DIR}/sms-daemon.cpp
    ${SRC_DIR}/sms-daemon-mdm.cpp
)

add_executable(sms-daemon ${DAEMON_SOURCES})
target_link_libraries(sms-daemon PRIVATE sms-core)

# ---- Отправщик ----
set(SENDER_SOURCES
    ${SRC_DIR}/SendSMS.cpp
)

add_executable(sms-send ${SENDER_SOURCES})
target_link_libraries(sms-send PRIVATE sms-core)

install(TARGETS sms-daemon sms-send
        RUNTIME DESTINATION bin)

# ========================
#      ТЕСТЫ (gtest)
# ========================

option(BUILD_TESTING "Build unit tests" ON)

if (BUILD_TESTING)
    enable_testing()

    # Ищем установленный GoogleTest
    find_package(GTest REQUIRED)

    # Тестовый бинарник
    add_executable(sms-tests
        tests/test_pdu.cpp
        tests/test_ussd.cpp
        tests/test_config.cpp
        # сюда добавишь другие тестовые файлы по мере надобности
        # tests/test_serial.cpp
        # tests/test_xml.cpp
    )

    target_link_libraries(sms-tests
        PRIVATE
            sms-core
            GTest::GTest
            GTest::Main
    )

    # Регистрируем один тест в CTest – он внутри уже запустит все gtest-тесты
    add_test(NAME sms-tests COMMAND sms-tests)
endif()
#
#
#
# mkdir -p build
# cmake -S . -B build -DBUILD_TESTING=ON
# cmake --build build
# ctest --test-dir build -V