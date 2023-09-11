#pragma once
#include <string>
#include <list>
#include <map>
#include <functional>
#include "xmlParser.h"

using namespace std;

#define MAX_SMS_TIME		(15*24*3600) // 15 days


class CRecvSMSPart
{
public:
	CRecvSMSPart(void);
	~CRecvSMSPart(void);
	int ProcessRecvPDU(std::string rawText);
	std::string m_RawText;
//	int         m_Index;
	std::wstring m_SMSC;
	std::wstring m_sText;
	std::wstring m_From;
	std::string m_TimeStamp;
	unsigned char	m_nPartNo;
	unsigned char	m_nParts;
	unsigned char	m_nRefNr;
	XMLNode GenXML(bool debugFlag=false);
};

class CRecvSMSProcessor {
public:
	CRecvSMSProcessor();
	~CRecvSMSProcessor();
	string m_InterfaceID;
	std::string m_sDestPreffix;
	bool m_bAddDebugInfo;
	int m_nSmsProcessed;
	int m_nPartsProcessed;
	int Init(string cachePath, string interfaceID);
	int ProcessPDU(const char*  pdu);
	void SaveCache (void);
	XMLNode GetCache () {return m_xCache; }
	void SetSmsCallBack(std::function<int(XMLNode)> fn) { m_OnSmsCallBackPtr = fn; }
protected:
	std::function<int(XMLNode)> m_OnSmsCallBackPtr{nullptr}; // returns  rtn > 0 sms will be saved after the call, rtn <= 0 - SMS will not be saved
	XMLNode m_xCache;
	int SaveSMS(XMLNode sms);
	int ProcessPart(XMLNode sms);
	int64_t m_LastProcessedItemID;
	std::string m_sCachePath;
};