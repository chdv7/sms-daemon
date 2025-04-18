CFLAGS   += -Wall -g
CPPFLAGS += -Wall -std=c++17 -g
LDFLAGS=

DAEMON_EXE  = sms-daemon
DAEMON_SOURCES_CPP = 	sms-daemon.cpp \
		sms-daemon-mdm.cpp \
		PduSMS.cpp \
		serial.cpp \
		rsserial.cpp \
		xmlParser.cpp \
		RecvSMS.cpp \
		ut.cpp       

SENDER_EXE =  sms-send  
SENDER_SOURCES_CPP = PduSMS.cpp \
		SendSMS.cpp

SOURCES_CPP = $(DAEMON_SOURCES_CPP)  $(SENDER_SOURCES_CPP) 

SOURCES_C   =
LDFLAGS += 


DAEMON_OBJECTS     = $(DAEMON_SOURCES_CPP:.cpp=.o)
SENDER_OBJECTS     = $(SENDER_SOURCES_CPP:.cpp=.o) 

all: $(DAEMON_EXE) $(SENDER_EXE)
 
$(DAEMON_EXE): $(DAEMON_OBJECTS)
	$(CXX) -o $(DAEMON_EXE) $(LDFLAGS) $(DAEMON_OBJECTS)

$(SENDER_EXE): $(SENDER_OBJECTS)
	$(CXX) -o $(SENDER_EXE) $(SENDER_OBJECTS)


%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $<

%.o: %.c
	$(CC) -c $(CFLAGS) $<

clean:
	-rm -f $(DAEMON_EXE) $(SENDER_EXE) $(DAEMON_OBJECTS) $(SENDER_OBJECTS)

install:

.PHONY: all install clean
