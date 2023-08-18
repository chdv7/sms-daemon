#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>



#define IN_SMS_PATH             "/var/spool/sms/insms"
#define IN_SMS_CACHE_NAME       "/var/spool/sms/sms-daemon-cacheSMS.xml"
#define DEVICE 					"/dev/ttyUSB0"
#define DEVICE_MDM			DEVICE

#define DATA_PID_PATH       	"/var/run/sms-daemon.pid"
#define OUT_SMS_DIR         	"/var/spool/sms/outsms"
#define START_DELAY				2000
//#define DEVICE 			"ttyMdm0"
#define SMS_TMP_DIR		"/tmp/sms-daemon"
#define SMS_LOG_FILE	        SMS_TMP_DIR"/Log"
#define IN_SMS_XML_DIR          SMS_TMP_DIR"/ReceivedSMS"
#define SMS_LIST_RECEIVED	"/tmp/sms-list-received"
#define LOOP_SLEEP_MS		3000

// #define _DEBUG__

using namespace std;

struct TMdmRcvSms {
	string pdu;
	int index {-1};
	bool isProcessed{false};
};

struct TSmsBlock {
	int err{ -1 };
	vector <TMdmRcvSms> sms;
	bool IsError() { return err < 0; }
};

bool isHexDecChar (char chr);

std::string GetUniqString ();
std::string makePrintableStr(const std::string& s);

void ReportSendStatus (uint64_t messageID, int err);
void Log (const char* text);
void LogError (int code, const char* text);
int ProcessSmsBlock(TSmsBlock& block, CRecvSMSList& smsProcessor);
int ProcessPDUQueue (XMLNode xCache);
int mdm_daemon_loop ();
