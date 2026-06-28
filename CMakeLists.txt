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
    ${SRC_DIR}/SmsHookRunner.cpp
)

add_executable(sms-daemon ${DAEMON_SOURCES})
target_link_libraries(sms-daemon PRIVATE sms-core)

# ---- Отправщик ----
set(SENDER_SOURCES
    ${SRC_DIR}/SendSMS.cpp
)

add_executable(sms-send ${SENDER_SOURCES})
target_link_libraries(sms-send PRIVATE sms-core)

include(GNUInstallDirs)

set(SMS_DAEMON_SYSCONFDIR "/etc" CACHE PATH "System configuration directory")
set(SMS_DAEMON_CONFIG_DIR "${SMS_DAEMON_SYSCONFDIR}/sms-daemon" CACHE PATH "sms-daemon configuration directory")
set(SMS_DAEMON_CONFIG_FILE "${SMS_DAEMON_CONFIG_DIR}/config.cfg" CACHE FILEPATH "sms-daemon configuration file")
set(SMS_DAEMON_CONFIG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/sms-daemon.cfg" CACHE FILEPATH "Default sms-daemon configuration source")

install(TARGETS sms-daemon sms-send
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(SMS_DAEMON_SYSTEMD_UNIT_DIR "/etc/systemd/system" CACHE PATH "systemd unit directory")
set(SMS_DAEMON_SERVICE_FILE "${CMAKE_CURRENT_BINARY_DIR}/sms-daemon.service")
set(BINDIR "${CMAKE_INSTALL_FULL_BINDIR}")
set(CONFIGFILE "${SMS_DAEMON_CONFIG_FILE}")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/systemd/sms-daemon.service.in
    ${SMS_DAEMON_SERVICE_FILE}
    @ONLY
)
install(FILES ${SMS_DAEMON_SERVICE_FILE}
        DESTINATION ${SMS_DAEMON_SYSTEMD_UNIT_DIR})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_sms_daemon.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/install_sms_daemon.cmake
    @ONLY
)
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/install_sms_daemon.cmake)

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