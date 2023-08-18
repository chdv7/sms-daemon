#include "rsserial.h"
#include "RecvSMS.h"
#include "ut.h"
#include <iostream>
#include "sms-daemon.h"


static const char* answers[] = {"OK\r", "ERROR\r",  NULL};
static const char* answers_go[] = {">", "OK\r", "ERROR\r",  NULL };



bool DelSMS (CSerial* connector, int smsNo){
#ifdef _DEBUG__
	return true;
#else
	char command[100];
	sprintf (command, "at+cmgd=%d,0\r", smsNo);

	if(!connector)
		return false;
	int err =  connector->SendExpect(command, answers, 2000);

	if (err)
		fprintf (stderr,"+CMGD error %d\n", err);

	return err;
#endif
}


void DelMdmSmsByBlock(CSerial* connector, TSmsBlock& block) { // csv list
	for (unsigned int i = 0; i < block.sms.size(); ++i) {
		if (block.sms[i].isProcessed)
			DelSMS(connector, block.sms[i].index);
	}
}

TSmsBlock GetSmsFromModemByCMGR(CSerial* connector) {
	TSmsBlock rtn;
	for (int i = 0; i < 255; ++i) {
		char command[20];
		sprintf(command, "AT+CMGR=%d\r", i);
		char log[1000];
		rtn.err = connector->SendExpect(command, answers, 1000, log, sizeof(log));
		if (rtn.err) {
			if (!rtn.sms.empty())
				rtn.err = 0;
			else if (rtn.err > 0)
				rtn.err = -2;
			break;
		}
		else {
			char* ptr = strstr(log, "CMGR:");
			if (ptr) {
				if ((ptr = strchr(ptr, '\r')) == NULL)
					continue;
				if (*(++ptr)=='\n')
					ptr++;
				TMdmRcvSms sms;
				sms.index = i;
				while (isHexDecChar(*ptr)) {
					sms.pdu += *(ptr++);
				}
				rtn.sms.push_back(sms);
			}
		}
	}
	return rtn; 
}

TSmsBlock GetSmsFromModemByCMGL(CSerial* connector) {
	TSmsBlock rtn;
	char log[50 * 1024];
	rtn.err = connector->SendExpect("AT+CMGL=4\r", answers, 3000, log, sizeof log);
	if (rtn.err) {
		fprintf(stderr, "+CMGL Error %d\n", rtn.err);
		if (rtn.err > 0)
			rtn.err = -1;
	}
	else {
		for (char* ptr = log; ptr && ((ptr = strstr(ptr, "CMGL:")) != NULL); ) {
			ptr += 5; // skip CMGL:
			TMdmRcvSms sms;
			if (sscanf(ptr, "%d", &sms.index) > 0 && sms.index >=0) {
				if ((ptr = strchr (ptr, '\r')) == NULL)
					break;
				if (*(++ptr) == '\n')
					ptr++;
				while (isHexDecChar(*ptr)) {
					sms.pdu += *(ptr++);
				}
				rtn.sms.push_back(sms);
			}
		}

	}
	return rtn;
}

TSmsBlock GetSmsFromModem(CSerial* connector) {
	TSmsBlock rtn = GetSmsFromModemByCMGL (connector);
	if (rtn.IsError())
		rtn = GetSmsFromModemByCMGR(connector);
	return rtn;
}


int ProcessInSMS (CSerial* connector, CRecvSMSList& recvSMSProcessor){
	TSmsBlock smsBlock = GetSmsFromModem(connector);
	ProcessSmsBlock(smsBlock, recvSMSProcessor);
	DelMdmSmsByBlock(connector, smsBlock);
	int err = ProcessPDUQueue (recvSMSProcessor.GetCache());
	if (!smsBlock.sms.empty() || err >0 ){
		recvSMSProcessor.SaveCache();	
	}
	return 0;
}


void ProcessOutSMS (CSerial* connector){
    DIR*   dir = opendir(OUT_SMS_DIR);
    if (!dir) {
		fprintf (stderr, "can not open %s\n", OUT_SMS_DIR);
        mkdir (OUT_SMS_DIR, 0777);
        chmod (OUT_SMS_DIR, 0777);
		return;
    };

    struct dirent *entry;
    while ( (entry = readdir(dir)) != NULL) {
	if (*entry->d_name =='.')
		continue;
//        printf("%s\n", entry->d_name);
	string fname(OUT_SMS_DIR);
	fname += "/";
	fname += entry->d_name;
	FILE* f = fopen (fname.c_str(), "r");
	if (f){
		char buf [1024]="";
		int len=0;
		if (fgets(buf, sizeof (buf), f))
			len = strlen (buf);
		fclose(f);

		if (len > 4) {
			if (buf[len - 1] == '\n')
				buf[--len] = 0;
			uint64_t llMsgID = 0;
			char* sMsgID = strchr(buf, ':');
			if (sMsgID) {
				len = sMsgID - buf;
				*sMsgID++ = 0;
				sscanf(sMsgID, "%llX", &llMsgID);
			}

			//			printf ("%d <%s>\n", len, buf);

			char cmd[200];
			char log[1024];
			memset(log, 0, sizeof(log));
			sprintf(cmd, "AT+CMGS=%u\r", len / 2 - 1);
			int err = connector->SendExpect(cmd, answers_go, 1000, log, sizeof(log));
			if (!err){
				string full_cmd(buf);
	
				full_cmd += "\x1a";
				memset(log, 0, sizeof(log));
				err = connector->SendExpect(full_cmd.c_str(), answers, 20000, log, sizeof(log));
			}
			if (llMsgID){
				ReportSendStatus (llMsgID, err);
			}

			if (err) {
				string outStr(string("Can not send SMS ") + entry->d_name);
				if (!llMsgID)
					LogError (err, outStr.c_str());
				fprintf (stderr, "%s\n", outStr.c_str());
				fprintf (stderr, "Error code:%d\n", err );
			}
		}
	}
	unlink (fname.c_str());
    };

    closedir(dir);
}

int mdm_daemon_loop (){
//printf ("loop\n");
	CRSSerial connector;
	CRecvSMSList recvSMSProcessor;
        recvSMSProcessor.Init (IN_SMS_CACHE_NAME, DEVICE);
	recvSMSProcessor.m_sDestPreffix = IN_SMS_XML_DIR "/";
        recvSMSProcessor.SaveCache();
//	int err = connector.Open ("ttyUSB3");
	connector.SetComParams (115200);

	int err = connector.Open (DEVICE);
	if (err <0){
		LogError (err, "Unable to open modem");
		fprintf (stderr, "Unable to open modem error: %d\n", err);
		return 0;
	}

	err = connector.SendExpect("\r\rAT&F\r", answers);
	if (err)
		printf("AT&F error %d\n", err);

	err = connector.SendExpect("ATE0\r", answers);
	if (err)
		printf("ATE error %d\n", err);

	err = connector.SendExpect("AT+CMGF=0\r", answers);
	if (err)
		printf("AT+CMGF error %d\n", err);

	err = connector.SendExpect("AT+CPMS=\"SM\",\"SM\",\"SM\"\r", answers);
	if (err)
		printf("AT+CPMS error %d\n", err);

	for (;;){
		int err = ProcessInSMS(&connector, recvSMSProcessor);
		if (err < 0){
			LogError (err, "Error processing incoming SMS");
			printf ("Error processing incoming SMS: %d\n", err);
		}
		ProcessOutSMS (&connector);
		Sleep(LOOP_SLEEP_MS);
	}
}

bool test_8207(){
	return !access (DEVICE_MDM, 0);
}
