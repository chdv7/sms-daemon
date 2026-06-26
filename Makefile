# Компиляторы и флаги
CXX       = g++
CC        = gcc
CFLAGS    += -Wall -g
CPPFLAGS  += -Wall -std=c++17 -g -MMD -MP
LDFLAGS   +=

# Папки
SRCDIR    = src
BUILD_DIR = build

# Установка
PREFIX     ?= /usr/local
BINDIR     ?= $(PREFIX)/bin
SYSCONFDIR ?= /etc
CONFIGDIR  ?= $(SYSCONFDIR)/sms-daemon
CONFIGFILE ?= $(CONFIGDIR)/config.cfg
CONFIG_SRC ?= sms-daemon.cfg
INSTALL    ?= install
INSTALL_PROGRAM ?= $(INSTALL) -m 0755
INSTALL_DATA    ?= $(INSTALL) -m 0644
INSTALL_DIR     ?= $(INSTALL) -d -m 0755
INSTALL_RUNTIME_DIR ?= $(INSTALL) -d -m 0777

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
    Config.cpp \
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

install: all
	$(INSTALL_DIR) "$(DESTDIR)$(BINDIR)"
	$(INSTALL_PROGRAM) "$(DAEMON_EXE)" "$(DESTDIR)$(BINDIR)/$(DAEMON_EXE)"
	$(INSTALL_PROGRAM) "$(SENDER_EXE)" "$(DESTDIR)$(BINDIR)/$(SENDER_EXE)"
	$(INSTALL_DIR) "$(DESTDIR)$(CONFIGDIR)"
	@if [ ! -f "$(DESTDIR)$(CONFIGFILE)" ]; then \
		$(INSTALL_DATA) "$(CONFIG_SRC)" "$(DESTDIR)$(CONFIGFILE)"; \
	else \
		echo "keep existing $(DESTDIR)$(CONFIGFILE)"; \
	fi
	$(INSTALL_RUNTIME_DIR) "$(DESTDIR)/tmp/outsms"
	$(INSTALL_RUNTIME_DIR) "$(DESTDIR)/tmp/sms-daemon"
	$(INSTALL_RUNTIME_DIR) "$(DESTDIR)/tmp/sms-daemon/ReceivedSMS"

# Включение зависимостей
-include $(DEPENDS_CPP)

.PHONY: all clean install
