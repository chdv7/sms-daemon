#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>

#include "ut.h"
#include "PduSMS.h"
#include "sms-daemon-mdm.h"
#include "sms-daemon.h"

using namespace std;


//#define __DO_NOT_DEL_SMS__

static const char* answers[] = {"OK\r", "ERROR\r",  NULL};
static const char* answers_go[] = {">", "OK\r", "ERROR\r",  NULL };


static bool const HexDecChars[128] = {
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

bool isHexDecChar(char chr) {
	return chr & 0x80 ? false : HexDecChars[(int)chr];
}

void CSmsDaemon::Setup() {
	return;
}
int CSmsDaemon::Go() {
	Init();
	return Do();
}
void CSmsDaemon::Init() {
	m_RecvSMSProcessor.Init(m_CachePath, m_DeviceName);
	m_RecvSMSProcessor.m_sDestPreffix = m_InSmsXmlDir + "/";
	m_RecvSMSProcessor.SetSmsCallBack([this](XMLNode sms) { return OnCompleteSmsDecodeCB(sms); });
	m_RecvSMSProcessor.SaveCache();
	int err = m_Connector.Open(m_DeviceName.c_str());
	if (err < 0)
		throw SmsDaemonError(err, (string("Can not open device ") + m_DeviceName).c_str());
	m_Connector.SetComParams(115200);

	err = m_Connector.SendExpect("\r\rAT&F\r", answers);
	if (err)
		printf("AT&F error %d\n", err);

	err = m_Connector.SendExpect("ATE0\r", answers);
	if (err)
		printf("ATE error %d\n", err);

	err = m_Connector.SendExpect("AT+CMGF=0\r", answers);
	if (err)
		printf("AT+CMGF error %d\n", err);

	err = m_Connector.SendExpect("AT+CPMS=\"SM\",\"SM\",\"SM\"\r", answers);
	if (err)
		printf("AT+CPMS error %d\n", err);

	return;

}


int CSmsDaemon::OnCompleteSmsDecodeCB(XMLNode sms) {
	int rtn = 1;

	string number = GetXMLStr(sms, "from", "");
	int r = 0;
	for (auto& it : m_SmsInXMLCallback) {
		if (it.fn) {
			string replay;
			int r = it.fn(sms, replay, it.userdata);
			if (!replay.empty()) {
				SendSms(number, replay);
			}
			if (r < 0)
				rtn = r;
			if (r)
				break;
		}
	}
	if (!r && !m_SmsInCallback.empty()){
		string text = sms.getText();
		for (auto& it : m_SmsInCallback) {
			if (it.fn) {
				string replay;
				int r = it.fn(number, text, replay, it.userdata);
				if (!replay.empty())
					SendSms(number, replay);
				if (r < 0)
					rtn = 0;
				if (r)
					break;
			}
		}
	}
	return rtn;
}

int CSmsDaemon::Do() {
	for (;;) {
		DoProcessInSmsBlock();
		DoProcessOutSmsFolder();
		Sleep(m_LoopDelay);
	}
	return 0;
}

void CSmsDaemon::DoProcessInSmsBlock() {
	TSmsBlock smsBlock = GetSmsBlockFromModem();
	if (smsBlock.err < 0) {
		LogError(smsBlock.err, "Error processing incoming SMS");
		printf("Error processing incoming SMS: %d\n", smsBlock.err);
	}

	if (!smsBlock.sms.empty()) {
		ProcessSmsBlock(smsBlock);
		DelSmsBlock(smsBlock.sms);
		m_RecvSMSProcessor.SaveCache();
	}
	
}

CSmsDaemon::TSmsBlock CSmsDaemon::GetSmsBlockByCMGR() {
	TSmsBlock rtn;
	for (int i = 0; i < 255; ++i) {
		char command[20];
		sprintf(command, "AT+CMGR=%d\r", i);
		char log[1000];
		rtn.err = m_Connector.SendExpect(command, answers, 1000, log, sizeof(log));
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
				if (*(++ptr) == '\n')
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

CSmsDaemon::TSmsBlock CSmsDaemon::GetSmsBlockByCMGL() {
	TSmsBlock rtn;
	char log[50 * 1024];
	rtn.err = m_Connector.SendExpect("AT+CMGL=4\r", answers, 3000, log, sizeof log);
	if (rtn.err) {
		fprintf(stderr, "+CMGL Error %d\n", rtn.err);
		if (rtn.err > 0)
			rtn.err = -1;
	}
	else {
		for (char* ptr = log; ptr && ((ptr = strstr(ptr, "CMGL:")) != NULL); ) {
			ptr += 5; // skip CMGL:
			TMdmRcvSms sms;
			if (sscanf(ptr, "%d", &sms.index) > 0 && sms.index >= 0) {
				if ((ptr = strchr(ptr, '\r')) == NULL)
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

CSmsDaemon::TSmsBlock CSmsDaemon::GetSmsBlockFromModem() {
	TSmsBlock rtn = GetSmsBlockByCMGL();
	if (rtn.IsError())
		rtn = GetSmsBlockByCMGR();
	return rtn;
}

int CSmsDaemon::ProcessSmsBlock(TSmsBlock& smsBlock) {
	int rtn = 0;
	for (TMdmRcvSms& sms: smsBlock.sms) {
		if (!m_DirtySimSlots[sms.index]) {
			m_RecvSMSProcessor.ProcessPDU(sms.pdu.c_str());
			m_DirtySimSlots[sms.index] = true;
		}
	}
	return rtn;
}

bool CSmsDaemon::DelSms(int smsIndex) {
#ifdef __DO_NOT_DEL_SMS__
	m_DirtySimSlots[smsIndex] = false;
	return true;
#else
	char command[100];
	sprintf(command, "at+cmgd=%d,0\r", smsIndex);

	int err = m_Connector.SendExpect(command, answers, 2000);

	if (err)
		fprintf(stderr, "+CMGD error %d\n", err);
	else
		m_DirtySimSlots[smsIndex] = false;
	return err;
#endif
}


void CSmsDaemon::DelSmsBlock(vector <TMdmRcvSms> block) {
	for (auto& sms : block) {
		if (m_DirtySimSlots[sms.index])
			DelSms(sms.index);
	}
}

int CSmsDaemon::SendSmsPart(std::string pdu) {
	int err = 0;
	if (pdu.size() > 4) {
		for (auto it = pdu.begin(); it != pdu.end(); ++it) {
			if (!isHexDecChar(*it)) {
				pdu.erase(it, pdu.end());
				break;
			}
		}


		char cmd[200];
		char log[1024];
		memset(log, 0, sizeof(log));
		sprintf(cmd, "AT+CMGS=%u\r", pdu.length() / 2 - 1);
		err = m_Connector.SendExpect(cmd, answers_go, 1000, log, sizeof(log));
		if (!err) {
			pdu += "\x1a";
			memset(log, 0, sizeof(log));
			err = m_Connector.SendExpect(pdu.c_str(), answers, 20000, log, sizeof(log));
		}
		if (!err)
			m_Connector.Puts("\r\r", 10);
	}
	else {
		err = -1;
	}
	return err;
}

void CSmsDaemon::DoProcessOutSmsFolder() {
	DIR* dir = opendir(m_OutSmsMailDir.c_str());
	if (!dir) {
		fprintf(stderr, "can not open %s\n", m_OutSmsMailDir.c_str());
		mkdir(m_OutSmsMailDir.c_str(), 0777);
		chmod(m_OutSmsMailDir.c_str(), 0777);
		return;
	};

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (*entry->d_name == '.')
			continue;
		//        printf("%s\n", entry->d_name);
		string fname(m_OutSmsMailDir);
		fname += "/";
		fname += entry->d_name;
		FILE* f = fopen(fname.c_str(), "r");
		if (f) {
			char buf[1024] = "";
			fgets(buf, sizeof(buf), f);
			fclose(f);
			int err = strlen(buf) ? SendSmsPart(buf) : -100;
			if (err) {
				string outStr(string("Can not send SMS ") + entry->d_name);
				fprintf(stderr, "%s\n", outStr.c_str());
				fprintf(stderr, "Error code:%d\n", err);
			}
		}
		unlink(fname.c_str());
	};

	closedir(dir);

}

int  CSmsDaemon::SendSms(std::string number, std::string text, int flags) {

	COutPduSms sms_processor(number.c_str(), text.c_str());
	TOutPduBlock pdu_block = sms_processor.ParseText();

	for (auto& it : pdu_block) {
		SendSmsPart(it);
	}
	return 0;
}