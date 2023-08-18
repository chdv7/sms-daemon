#pragma once
#include <string>
#include <list>
#include <map>
#include "xmlParser.h"

using namespace std;

#define MAX_SMS_TIME		(15*24*3600) // 15 days

class CRecvSMS
{
public:
	CRecvSMS(void);
	~CRecvSMS(void);
	int ProcessRawText(std::string rawText);
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

class CRecvSMSList {
public:
	CRecvSMSList();
	~CRecvSMSList();
	string m_InterfaceID;
	std::string m_sDestPreffix;
	bool m_bAddDebugInfo;
	int m_nSmsProcessed;
	int m_nPartsProcessed;

	int Init(string cachePath, string interfaceID);
	int ProcessPDU(const char*  pdu);
	void SaveCache (void);
	XMLNode GetCache () {return m_xCache; }
protected:
	XMLNode m_xCache;
	int SaveSMS(XMLNode sms);
	int ProcessPart(XMLNode sms);
	int64_t m_LastProcessedItemID;
	std::string m_sCachePath;
};