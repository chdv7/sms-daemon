cmake_minimum_required(VERSION 3.10)

# Проект на C и C++
project(sms_project LANGUAGES C CXX)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Общие флаги компиляции (аналог -Wall и -g для Debug)
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wall>)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall>)
endif()

# По умолчанию тип сборки Debug (чтобы был -g)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Каталог с исходниками
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Источники демона
set(DAEMON_SOURCES
    ${SRC_DIR}/sms-daemon.cpp
    ${SRC_DIR}/sms-daemon-mdm.cpp
    ${SRC_DIR}/PduSMS.cpp
    ${SRC_DIR}/serial.cpp
    ${SRC_DIR}/rsserial.cpp
    ${SRC_DIR}/xmlParser.cpp
    ${SRC_DIR}/RecvSMS.cpp
    ${SRC_DIR}/ut.cpp
)

# Источники отправщика
set(SENDER_SOURCES
    ${SRC_DIR}/PduSMS.cpp
    ${SRC_DIR}/SendSMS.cpp
)

# Исполняемый файл демона
add_executable(sms-daemon
    ${DAEMON_SOURCES}
)

# Исполняемый файл отправщика
add_executable(sms-send
    ${SENDER_SOURCES}
)

# Если понадобятся дополнительные библиотеки, можно добавить так:
# target_link_libraries(sms-daemon PRIVATE pthread ...)
# target_link_libraries(sms-send   PRIVATE pthread ...)

# Простейшая установка (по желанию)
install(TARGETS sms-daemon sms-send
        RUNTIME DESTINATION bin)
#
#
#
#
#
#
#
# rm -rf build          # (Linux/macOS; на Windows см. ниже)
# mkdir build
# cmake -S . -B build   # конфигурация (создаёт CMakeCache.txt в build/)
# cmake --build build   # сборка