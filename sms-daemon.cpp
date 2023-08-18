#include "rsserial.h"
#include "RecvSMS.h"
#include "ut.h"
#include <iostream>
#include "sms-daemon.h"


string GetUniqString (){
	static unsigned char cnt=0;
	static time_t last_time = 0;
	time_t t_now = time(NULL);
	if (last_time != t_now){
		last_time = t_now;
		cnt=0;
	}
	char buf[50];
	sprintf (buf, "%08lX%02X", t_now, cnt++);
	return buf;
}

static bool const HexDecChars [128]= {
//      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F
/* 0 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
/* 1 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
/* 2 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
/* 3 */	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 
/* 4 */	0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
/* 5 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
/* 6 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
/* 7 */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
/* 8 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   9 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   A 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   B   	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   C   	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   D   	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   E   	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   F   	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
*/
};

bool isHexDecChar (char chr){
	return chr&0x80 ? false : HexDecChars[(int)chr];
}


string makePrintableStr(const string& s) {
	string rtn;
	for (size_t i=0 ; i < s.length(); ++i) {
		char ch = s[i];
		if (isprint (ch) && ch != '\\')
			rtn += ch;
		else {
			char tmp[10];
			sprintf(tmp, "\\x%02X", ch&0xff);
			rtn += tmp;
		}
	}
	return rtn;
}

void ReportSendStatus (uint64_t messageID, int err){
	FILE* f = fopen (SMS_LOG_FILE, "a");
	if (f){
		fprintf (f, "$%d %016llX\n", err, messageID);
		fclose (f);
	}
}

void Log (const char* text){
	FILE* f = fopen (SMS_LOG_FILE, "a");
	if (f){
		fprintf (f, "SMS daemon: %s\n", text);
		fclose (f);
	}
}

void LogError (int code, const char* text){
	FILE* f = fopen (SMS_LOG_FILE, "a");
	if (f){
		fprintf (f, "%d SMS daemon: %s\n", code, text);
		fclose (f);
	}
}

int ProcessSmsBlock(TSmsBlock& block, CRecvSMSList& smsProcessor) {
	int rtn = 0;
	for (unsigned int i = 0; i < block.sms.size(); ++i) {
		TMdmRcvSms& sms = block.sms[i];
		XMLNode xPdu = smsProcessor.GetCache().getChildNodeWithAttribute("PDU", "Text",sms.pdu.c_str());
		if (xPdu.isEmpty()) {
			XMLNode xPdu = smsProcessor.GetCache().addChild("PDU");
			xPdu.addAttribute("Text", sms.pdu.c_str());
			char buf[30];
			sprintf(buf, "%lu", (unsigned long)time(NULL));
			xPdu.addAttribute("RcvTime", buf);			
			smsProcessor.ProcessPDU(sms.pdu.c_str());
			rtn++;
		}
		sms.isProcessed = true;
	}	
	return rtn;
}

// Process queue
// Returns 
// 0 - no change
// >0 - cache is updated
// <0 - error
int ProcessPDUQueue (XMLNode xCache){
	int rtn = 0;
	if (access (IN_SMS_PATH,  F_OK) >= 0 ){
		return 0;  // File for nvipbox already exist. wait for transmit
	}
	if (errno != ENOENT){
		return -1;  // No access to the file
	}

	XMLNode xPDU = xCache.getChildNode("PDU");
	if (!xPDU.isEmpty()){
		time_t pduTime = GetXMLInt (xPDU, "RcvTime", 0);
		const char* smsText = GetXMLStr( xPDU, "Text", "");
		if (*smsText && (time(NULL)-pduTime) < MAX_SMS_TIME){
			FILE* f = fopen (IN_SMS_PATH, "w");
			if (f){
				fprintf (f, "%s %s\n", GetUniqString().c_str(), smsText );
				fclose(f);
			}			        
		}
		else { // SMS is too old drop it
		}
		xPDU.deleteNodeContent();
		rtn = 1;
	}
	return rtn;
}

int main(int argc, char *argv[])
{

        mkdir (SMS_TMP_DIR, 0777);
        chmod (SMS_TMP_DIR, 0777);
        mkdir (OUT_SMS_DIR, 0777);
        chmod (OUT_SMS_DIR, 0777);
        mkdir (IN_SMS_XML_DIR, 0777);
        chmod (IN_SMS_XML_DIR, 0777);

	int isdaemon = 0;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--daemon")) {
			isdaemon = 1;
			printf("sms-daemon\n");
		}
		else 
			printf ("Test mode\n");
	}

	if (isdaemon) {
		pid_t pid;

		/* Fork off the parent process */
		pid = fork();
		if (pid < 0) {
			LogError (-1, "Can not start SMS daemon");
			fprintf(stderr, "Can not start daemon");
			return 1;
		}
		/* If we got a good PID, then
		we can exit the parent process. */
		if (pid > 0) {
			FILE* f = fopen (DATA_PID_PATH, "w");
			if (f) {
				fprintf(f, "%d", pid);
				fclose(f);
			}
			return 0;
		}
		else {  // daemon mode
			chdir("/");
//			close(STDIN_FILENO);
//			close(STDOUT_FILENO);
//			close(STDERR_FILENO);
			Sleep(START_DELAY);
		}
	}
	mdm_daemon_loop();
	return 0;
}


