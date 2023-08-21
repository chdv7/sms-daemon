#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include "sms-daemon-config.h"
#include "rsserial.h"
#include "RecvSMS.h"

typedef int (*SmsCallBack) (const std::string number, const std::string text, std::string& replay, void* userdata);
class CSmsDaemon {
	struct TMdmRcvSms {
		std::string pdu;
		int index{ -1 };
		bool isProcessed{ false };
	};
	struct TSmsBlock {
		int err{ -1 };
		std::vector <TMdmRcvSms> sms;
		bool IsError() { return err < 0; }
	};
	struct TCallBackDescriptor {
		TCallBackDescriptor(SmsCallBack fn_, void* userdata_) : fn(fn_), userdata(userdata_){}
		SmsCallBack fn;
		void*       userdata;
	};
	std::vector <TCallBackDescriptor> m_SmsInCallback;
	std::unordered_map <int, bool> m_DirtySimSlots;
	std::string  m_DeviceName{ DEVICE };
	std::string  m_CachePath { IN_SMS_CACHE_NAME };
	std::string  m_InSmsXmlDir{ IN_SMS_XML_DIR };
	std::string  m_OutSmsMailDir{ OUT_SMS_DIR };
	CRSSerial    m_Connector;
	CRecvSMSProcessor m_RecvSMSProcessor;
	int m_LoopDelay{ LOOP_SLEEP_MS };

	void Init();
	int Do();
	void DoProcessInSMS(); 
	TSmsBlock GetSmsFromModem();
	TSmsBlock GetSmsFromModemByCMGR();
	TSmsBlock GetSmsFromModemByCMGL();
	int ProcessSmsBlock(TSmsBlock& smsBlock);
	void DoProcessOutSMS();
	bool DelSMS(int index);
	void DelSMSBlock(vector <TMdmRcvSms> sms);
	int  OnCompleteSmsDecodeCB(XMLNode sms);
public:
	CSmsDaemon() {}
	void Setup();
	int Go();
	void RegisterInSmsCallBack(SmsCallBack fn, void* userdata = nullptr) { m_SmsInCallback.emplace_back(fn, userdata); }
	struct SmsDaemonError {
		SmsDaemonError(int code = 0, std::string explanation = std::string()) : ErrorCode(code), Verbose(explanation) {}
		std::string Verbose;
		int			ErrorCode;
	};
};
