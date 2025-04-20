#pragma once
#include <string>
#include <vector>

using TOutPduBlock=std::vector<std::string> ;
class COutPduSms
{
enum PDU_EncodingScheme{ PDU_7=0, PDU_8, PDU_16 };
	std::string m_sPone;
	std::wstring m_sText;
public:
	COutPduSms(void);
	COutPduSms (const char* phone, const wchar_t* text);
	COutPduSms (const char* phone, const char* text, bool isUTF8=true);
	void SetPhoneNumber(const char* phoneNo) { m_sPone = phoneNo; }
	void AddText(const wchar_t* text) { m_sText += text; }
	void AddText(const char* text, bool isUTF8);
	~COutPduSms(void);
	TOutPduBlock ParseText(void);
private:
	std::string MakeSmsSubmitHeader(bool isHasUDH, PDU_EncodingScheme encodingScheme);
public:
	// flags can be set if need
	bool m_bForceUDH;
	bool m_bFlash;
	bool m_bRRq;
	int  m_nTTL;
	bool m_bForceUnicode;
	bool m_bDisable8Bit;
	unsigned char m_nReference;
};
