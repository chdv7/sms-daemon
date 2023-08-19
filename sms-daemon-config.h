#pragma once

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
