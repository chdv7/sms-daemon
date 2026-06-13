# Компиляторы и флаги
CXX       = g++
CC        = gcc
CFLAGS    += -Wall -g
CPPFLAGS  += -Wall -std=c++17 -g -MMD -MP
LDFLAGS   +=

# Папки
SRCDIR    = src
BUILD_DIR = build

# Основные цели
DAEMON_EXE = sms-daemon
SENDER_EXE = sms-send

# Исходники (имена файлов без каталога)
DAEMON_SOURCES_CPP = \
    sms-daemon.cpp \
    sms-daemon-mdm.cpp \
    PduSMS.cpp \
    serial.cpp \
    rsserial.cpp \
    xmlParser.cpp \
    RecvSMS.cpp \
    gen-xml.cpp \
    Ussd.cpp \
    Config.cpp \
    ut.cpp

SENDER_SOURCES_CPP = \
    PduSMS.cpp \
    Ussd.cpp \
    SendSMS.cpp

SOURCES_CPP = $(DAEMON_SOURCES_CPP) $(SENDER_SOURCES_CPP)

# Объектные файлы и зависимости (в build/)
OBJECTS_CPP   = $(addprefix $(BUILD_DIR)/,$(SOURCES_CPP:.cpp=.o))
DEPENDS_CPP   = $(addprefix $(BUILD_DIR)/,$(SOURCES_CPP:.cpp=.d))

DAEMON_OBJECTS = $(addprefix $(BUILD_DIR)/,$(DAEMON_SOURCES_CPP:.cpp=.o))
SENDER_OBJECTS = $(addprefix $(BUILD_DIR)/,$(SENDER_SOURCES_CPP:.cpp=.o))

# Цель по умолчанию
all: $(DAEMON_EXE) $(SENDER_EXE)

# Сборка исполняемых файлов
$(DAEMON_EXE): $(DAEMON_OBJECTS)
	$(CXX) -o $@ $(LDFLAGS) $^

$(SENDER_EXE): $(SENDER_OBJECTS)
	$(CXX) -o $@ $(LDFLAGS) $^

# Создание директории и компиляция с зависимостями
# C++
$(BUILD_DIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(CPPFLAGS) -o $@ $<

# C (если вдруг появятся .c файлы в src/)
$(BUILD_DIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -o $@ $<

# Очистка
clean:
	-rm -f $(DAEMON_EXE) $(SENDER_EXE)
	-rm -rf $(BUILD_DIR)

install:

# Включение зависимостей
-include $(DEPENDS_CPP)

.PHONY: all clean install
