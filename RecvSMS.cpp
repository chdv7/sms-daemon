#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>

#include "ut.h"
#include "RecvSMS.h"

#define ASSERT(a)

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

inline bool isHexDecChar (char chr){
	return chr&0x80 ? false : HexDecChars[(int)chr];
}
unsigned short lookup7_8Bit[] = {
//	    0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F
	0x0040, 0x00A3, 0x0024, 0x00A5, 0x00E8, 0x00E9, 0x00F9, 0x00EC, 0x00F2, 0x00C7, 0x000A, 0x00D8, 0x00F8, 0x000D, 0x00C5, 0x00E5, // 0 
	0x0394, 0x005F, 0x03A6, 0x0393, 0x039B, 0x038F, 0x03A0, 0x03A8, 0x03A3, 0x0398, 0x039E, 0xFFFF, 0x00C6, 0x00E6, 0x00DF, 0x00C9, // 1
	0x0020, 0x0021, 0x0022, 0x0023, 0x00A4, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, // 2
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, // 3
	0x00A1, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, // 4
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x00C4, 0x00D6, 0x00D1, 0x00DC, 0x00A7, // 5
	0x00BF, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, // 6
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x00E4, 0x00F6, 0x00F1, 0x00FC, 0x00E0, // 7
// Escaped characters
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x000C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, // 8 
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x005E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, // 9
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x007B, 0x007D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x005C, // A
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x005B, 0x007E, 0x005D, 0xFFFF, // B
	0x007C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, // C
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, // D
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x20AC, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, // E
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, // F
};





void AppendWstring (wstring& w, const char* s){
	while (*s)
		w+= *(unsigned char*)(s++);
}


struct T_UDH{  
	unsigned char udhl;
	unsigned char iei;
	unsigned char iedl;
	unsigned char refNr;
	unsigned char totalParts;
	unsigned char thisPart;
};


static int TextToBin (const char* text, int maxlen, unsigned char* bin){
	int bLen = strlen (text)/2;
	if (bLen > maxlen)
		bLen = maxlen;

	for (int i=0; i < bLen; ++i, text+=2){
		int bin1=-1;
		if (sscanf (text, "%02X",&bin1 )!=1 || bin1<0)
			return -1;
		bin[i] = bin1;
	}
	return bLen;
} 


static string decode7BitStep (const char* text, int len){
 
	unsigned char bIn[7];
	memset (bIn, 0, sizeof bIn);
	int sz = TextToBin (text, len==8?7:len, bIn);
	string out;
	for (int i=0; i< len; ++i){
		if ((i>=sz)&&(i != 7))  // check for end of string
			break;  
		int chr = 0;
		switch (i){
			case 0:
				chr =  bIn[0];                    break;
			case 1:
				chr = (bIn[0]>>7) | (bIn[1] <<1); break;
			case 2:
				chr = (bIn[1]>>6) | (bIn[2] <<2); break;
			case 3:
				chr = (bIn[2]>>5) | (bIn[3] <<3); break;
			case 4:
				chr = (bIn[3]>>4) | (bIn[4] <<4); break;
			case 5:
				chr = (bIn[4]>>3) | (bIn[5] <<5); break;
			case 6:
				chr = (bIn[5]>>2) | (bIn[6] <<6); break;
			case 7:
				chr =  bIn[6]>>1;                 break;
		}
		chr &= 0x7f;
		if (chr ==0){  // @ character will be encoded as code 0x80 higest bit will be cutted at decode7bit
			chr= 0x80;
		}
		out += chr;
	}
	return out;
}

static wstring decode7bit (const char* text, int maxOutLen, int firstGSMChar ){
	string sgsmCode;
	sgsmCode.reserve (maxOutLen+3);
	if (firstGSMChar >=0)
		sgsmCode+= firstGSMChar;

	while (maxOutLen >0 ){
		int len = min (maxOutLen, 8); // 7 bytes = 56 bit group
		sgsmCode += decode7BitStep( text, len);
		if (strlen(text) <= 14)
			break;
		text+=14;
		maxOutLen -= len;
	}

	wstring sout;
	sout.reserve(sgsmCode.length()+3);

	int escPreffix = 0;
	for (string::iterator it = sgsmCode.begin(); it != sgsmCode.end(); ++it){
		unsigned char chr = *it&0x7f;
		if (*it == 0x1b){
			escPreffix = 0x80;
			continue;
		}
		wchar_t wchr = lookup7_8Bit[escPreffix|chr];
		if (wchr == 0xFFFF){
			ASSERT (false);
			wchr = L'?';
		}
		sout += wchr;
		escPreffix = 0;
	}
	return sout;
}






CRecvSMSPart::CRecvSMSPart(void)
: m_nPartNo(0)
, m_nParts(0)
, m_nRefNr(0)
//, m_Index(-1)

{
	
}

static int GetInt(const char* sInt)
{
	if (!isHexDecChar(*sInt) )
		return -1;
	if (!isHexDecChar(*(sInt+1)) )
		return -1;
	int rtn = -1;
	sscanf (sInt, "%02X", &rtn);
	return rtn;
}

static int GetPhone(const char* s, std::wstring* phone, bool isSMSC)
{
	std::wstring _x;
	if (!phone)
		phone = &_x;
	phone->empty();
	size_t phoneSZ = GetInt (s);

	if (phoneSZ < 0)
		return phoneSZ;

	if (phoneSZ == 0)
		return 2;

	if (isSMSC){ // smsc provide number in octets including type octet
		phoneSZ = (phoneSZ-1)*2; // convert it to phone length
	}
	size_t padding = phoneSZ &1;
	size_t textSZ = 2;	s+=2;
	int type = GetInt (s);
	if (type < 0)
		return type;
	textSZ+=2; s+=2;
	switch (type&0x70){
		case 0x50:
			{
				*phone = decode7bit (s, phoneSZ*4/7, -1 );
//				printf ("}%s{", phone->c_str());
				textSZ+= phoneSZ+padding;
			}	
			break;
		case 0x10:
			*phone = L"+";
		default:
			phoneSZ+= padding;
			if (strlen (s) < (phoneSZ+padding))
				return -2;
			textSZ+= phoneSZ;
			for (;phoneSZ>0;phoneSZ-=2, s+=2){
				*phone += *(s+1);
				if (*s != 'F')
					*phone += *s;
			}
	}
	return textSZ;
}



CRecvSMSPart::~CRecvSMSPart(void)
{
}

string GetTimeStamp (const char* s ){
	string out;
	if (strlen(s) < 14)
		return out;
	
	out += s[5];  //day
	out += s[4];
	out += '.';

	out += s[3];  // mon
	out += s[2];
	out += '.';

	out += s[1];  //year
	out += s[0];
	out += ' ';

	out += s[7]; //hour
	out += s[6];
	out += ':';

	out += s[9]; // min
	out += s[8];
	out += ':';

	out += s[11]; //sec
	out += s[10];

	char tmp3[3]= {s[13], s[12], 0};
	int i_tz = 1000;
	
	if ((sscanf (tmp3, "%d", &i_tz)==1) && (i_tz != 1000)){
		char tmp30[30];
		sprintf (tmp30, " UTC+%02d:%02d", i_tz>>2, (i_tz&3)*15);
		out +=tmp30;
	}
		
	return out;

}

int CRecvSMSPart::ProcessRecvPDU(std::string rawText){
	m_RawText = rawText;
	const char* sms = rawText.c_str();

// Get SMS centre
	int sz = GetPhone (sms, &m_SMSC, true);
	if (sz <0)
		return sz;
	sms += sz;

// Get FirstOctet of SMS_DELIVER
	int nSmsDeliver = GetInt(sms);
	if (nSmsDeliver < 0)
		return nSmsDeliver;
	sms +=2;

// Get From
	sz = GetPhone (sms, &m_From, false);
	if (sz <0)
		return sz;
	sms += sz;


	int pid = GetInt(sms );
	if (pid<0)
		return pid;
	sms+=2;

	int dcs = GetInt (sms);
	if (dcs <0)
		return dcs;
	sms+=2;

	if (strlen (sms) < 14)
		return -3;
	m_TimeStamp = ::GetTimeStamp(sms);
	sms+=14;

	int udl = ::GetInt (sms);
	if (udl<0)
		return udl;

	sms+=2;
	// userData
	bool hasUDH = nSmsDeliver&0x40;
	T_UDH udh;
	if (hasUDH){
		if (TextToBin( sms, sizeof (udh), (unsigned char*)&udh)<0)
			return -6;
		udl -= sizeof (udh); // correct UDL with size of UDH in bytes for 7 bit coding it has to be corrected additionaly
		m_nParts = udh.totalParts;
		m_nPartNo= udh.thisPart;
		m_nRefNr = udh.refNr;
		sms+= sizeof (udh)*2;
	}
	switch (dcs&0xEC){
		case 0x00:
			{
				int first_chr_of_7bitSMS=-1;
				if (hasUDH){ // manual correction of first character
					udl -=1; // correct UDL. Length of UDH in septets is 7. 6 of them is already corrected by UDH processing.
							 // UDH len in bytes(6) already counded during UDH procesing
						if (udl>0){
							first_chr_of_7bitSMS = GetInt(sms);
							if (first_chr_of_7bitSMS < 0)
								return -7;
							first_chr_of_7bitSMS >>=1;
							udl--;
							sms+=2;
						}
				}
				m_sText += ::decode7bit(sms, udl, first_chr_of_7bitSMS);
			}
			break;
		case 0x04:
			{
				char buffer[512];
				int sz = TextToBin( sms, udl, (unsigned char*)buffer);
				buffer[sz] = 0;
				AppendWstring( m_sText, buffer);
			}
			break;
		case 0x08:
			{
				int udl16bit = udl/2;
				for (int i=0; i < udl16bit; ++i){
					wchar_t wchr;
					int sz = TextToBin (sms, 2, (unsigned char*)&wchr);
					if (sz !=2)
						break;
					m_sText += ntohs(wchr);
					sms+=4;
				}
			}				
			break;
		default:
			return -5;
	}
	return 0;
}


CRecvSMSProcessor::CRecvSMSProcessor()
: m_sDestPreffix ("")
, m_bAddDebugInfo(false)
, m_nSmsProcessed(0)
, m_nPartsProcessed(0)
, m_LastProcessedItemID(0)
, m_sCachePath("SMSCache.xml")
{
}


CRecvSMSProcessor::~CRecvSMSProcessor(){
	SaveCache();
}

int CRecvSMSProcessor::Init(string cachePath, string interfaceID)
{
	m_InterfaceID = interfaceID;
	if (!cachePath.empty())
		m_sCachePath = cachePath;
	m_xCache = XMLNode::parseFile(m_sCachePath.c_str());
	if (m_xCache.isDeclaration())
		m_xCache = m_xCache.getChildNode();
	if (m_xCache.isEmpty())
		m_xCache = XMLNode::createXMLTopNode("SmsCache");
	m_LastProcessedItemID = 0;
	const char*  psID = m_xCache.getAttribute("LastProcessedID");
	if (psID) {
		sscanf(psID, "%lX", &m_LastProcessedItemID);
	}
// Cleanup cache
	time_t t = time (NULL);
	for (int i=0;;i++){
		XMLNode xSMS = m_xCache.getChildNode ("SMS", i);
		if (xSMS.isEmpty())
			break;
		int smsTime = GetXMLInt (xSMS, "RcvTime", 0);
		if ((smsTime + MAX_SMS_TIME) < t)
			xSMS.deleteNodeContent();
	
	}

	return 0;
}

int CRecvSMSProcessor::ProcessPDU(const char*  pdu)
{
	CRecvSMSPart sms;
	sms.ProcessRecvPDU(pdu);
	XMLNode xSMS = sms.GenXML(m_bAddDebugInfo);
	m_nPartsProcessed++;

	if (::GetXMLInt(xSMS, "nparts", 0) <= 1) {
		if (!m_OnSmsCallBackPtr || (m_OnSmsCallBackPtr(xSMS) > 0))
			SaveSMS(xSMS);
	}
	else {
		ProcessPart(xSMS);
	}
	return 0;
}



XMLNode CRecvSMSPart::GenXML(bool debugFlag)
{
	XMLNode xSMS = XMLNode::createXMLTopNode("SMS");
	XMLNode xPart = xSMS;

	char from[200];
	char tmp1024[1024];
	UnicodeToUTF8 (from, m_From.c_str(), sizeof(from));

	char smsc[200];
	UnicodeToUTF8 (smsc, m_SMSC.c_str(), sizeof(smsc));

	time_t t = time(NULL);
	struct tm* p_tm = localtime(&t);
	char s_time[100];
	sprintf (s_time, "%lu", t);
	xSMS.addAttribute ("RcvTime", s_time); 
	strftime(s_time, sizeof(s_time), "%d.%m.%y %H:%M:%S", p_tm);

	if (m_nParts>1){
		sprintf (tmp1024, "%s-%02X", from, m_nRefNr);
		string id (tmp1024);
		xPart.addAttribute ("id", id.c_str()); 
		xPart = xSMS.addChild("part");

		sprintf (tmp1024, "%d", m_nPartNo);
		xPart.addAttribute ("partno", tmp1024);

		xPart.addAttribute ("time", m_TimeStamp.c_str());
		xPart.addAttribute ("smsc", smsc);

		xPart.addAttribute("ReceiveTime", s_time);

		sprintf(tmp1024, "%d", m_nParts);
		xSMS.addAttribute("nparts", tmp1024);
	}

	xSMS.addAttribute("from", from);
	xSMS.addAttribute ("time", m_TimeStamp.c_str());

	xSMS.addAttribute ("ReceiveTime", s_time);
	xSMS.addAttribute ("smsc", smsc);
	if (debugFlag)
		xPart.addAttribute ("raw", m_RawText.c_str());

	::UnicodeToUTF8(tmp1024, m_sText.c_str(), sizeof (tmp1024));
	xPart.addText(tmp1024);
	return xSMS;
}

int CRecvSMSProcessor::SaveSMS(XMLNode sms)
{
	static unsigned short cnt=0;
	static time_t last_t = 0;
	char fname[512];
	string preffix;
	for ( const char* from = GetXMLStr (sms, "From", ""); *from; from++){
		if (::isalnum(*from) || *from == '+')
		       preffix += *from;
	}
	if (preffix.empty())
		preffix = "Unknown";
	time_t t = time (NULL);
	if (last_t != t ){
		last_t = t;
		cnt = 0;
	}
	sprintf (fname, "%s-%08X%04X", preffix.c_str(),(int)time(NULL), cnt++);

	sms.writeToFile((m_sDestPreffix+fname+".xml").c_str());
	m_nSmsProcessed++;

	return 0;
}

int CRecvSMSProcessor::ProcessPart(XMLNode xNewSMS)
{
	const char* sId = xNewSMS.getAttribute ("id");
	if (!sId){
		ASSERT (0);
		return -101;
	}

	XMLNode xMainSMS = m_xCache.getChildNodeWithAttribute ("sms", "id", sId );
	if (xMainSMS.isEmpty()){ // first part of new SMS
		m_xCache.addChild (xNewSMS);
	}
	else { // Next part
		{
			XMLNode xNewPart = xNewSMS.getChildNode ("part");
			xMainSMS.updateAttribute(GetXMLStr(xNewPart, "ReceiveTime", ""), "ReceiveTime", "ReceiveTime");
			xMainSMS.addChild(xNewPart);
		}

		int nParts = ::GetXMLInt (xMainSMS, "nparts", 1);
		bool isReady = true;
		string sText;
		for (int i =0; i < nParts && isReady; ++i){
			char tmp20[20];
			sprintf (tmp20, "%d",i+1);
			XMLNode xPart = xMainSMS.getChildNodeWithAttribute("part", "partno", tmp20);
			if (xPart.isEmpty())
				isReady = false;
			else{
				const char* txt = xPart.getText();
				if (txt)
					sText += txt;
			}
		}
		if (isReady){
			xMainSMS.addText(sText.c_str());
			if (!m_bAddDebugInfo){
				for (;;){
					XMLNode x = xMainSMS.getChildNode("part");
					if (x.isEmpty())
						break;
					else
						x.deleteNodeContent();
				}
			}
			if ( !m_OnSmsCallBackPtr || (m_OnSmsCallBackPtr(xMainSMS) > 0))
				SaveSMS(xMainSMS);
			xMainSMS.deleteNodeContent();
		}
	}
	return 0;
}


void CRecvSMSProcessor::SaveCache(void){
	m_xCache.writeToFile(m_sCachePath.c_str());
}
