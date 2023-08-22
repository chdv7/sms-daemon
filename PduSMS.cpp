#include "PduSMS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <arpa/inet.h>
#include <time.h>

typedef unsigned short Uchar;
#pragma pack(push, 1)

struct T_UDH{
	unsigned char udhl;
	unsigned char iei;
	unsigned char iedl;
	unsigned char refNr;
	unsigned char totalParts;
	unsigned char thisPart;
};
#pragma pack(pop)


static int MAX_TEXT_LEN[3]= {160, 140, 70};
static const int UdhLenInChars[3]= {7, 6, 3};

static unsigned char gReference((unsigned char)time(NULL));


unsigned short lookup7Bit[256] = { // lookup table to check possibility convert 8 bit character to 7 bit
// values
// 0x00-0x7f code
// 0x80-0xfe esc code flag + code;
// 0xFF - not possible 
// mask 0xff00 high byte does not used for encoding, but makes possible to store character @ with GSM code 0
//	  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0xFF, 0x8A, 0x0D, 0xFF, 0xFF,  // 0
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 1
	0x20, 0x21, 0x22, 0x23, 0x02, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,  // 2
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,  // 3
      0xff00, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,  // 4
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0xBC, 0xAF, 0xBE, 0x94, 0x11,  // 5
	0xFF, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,  // 6
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0xA8, 0xC0, 0xA9, 0xBD, 0xFF,  // 7
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 8
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 9
	0xFF, 0x40, 0xFF, 0x01, 0x24, 0x03, 0xFF, 0x5F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // A
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x60,  // B
	0xFF, 0xFF, 0xFF, 0xFF, 0x5B, 0x0E, 0x1C, 0x09, 0xFF, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // C
	0xFF, 0x5D, 0xFF, 0xFF, 0xFF, 0xFF, 0x5C, 0xFF, 0x0B, 0xFF, 0xFF, 0xFF, 0x5E, 0xFF, 0xFF, 0x1E,  // D
	0x7F, 0xFF, 0xFF, 0xFF, 0x7B, 0x0F, 0x1D, 0xFF, 0x04, 0x05, 0xFF, 0xFF, 0x07, 0xFF, 0xFF, 0xFF,  // E
	0xFF, 0x7D, 0x08, 0xFF, 0xFF, 0xFF, 0x7C, 0xFF, 0x0C, 0x06, 0xFF, 0xFF, 0x7E, 0xFF, 0xFF, 0xFF,  // F

};


wstring UTF8ToUnicode (const char* src){
static const char XML_utf8ByteTable[256] =
{
//  0 1 2 3 4 5 6 7 8 9 a b c d e f
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x00
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x10
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x20
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x30
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x40
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x50
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x60
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x70End of ASCII range
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,// 0x80 0x80 to 0xbf invalid
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,// 0x90
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,// 0xa0
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,// 0xb0
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,// 0xc0 0xc2 to 0xdf 2 byte
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,// 0xd0
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,// 0xe0 0xe0 to 0xef 3 byte
    4,4,4,4,4,4,4,4,5,5,5,5,6,6,7,8 // 0xf0 0xf0 to 0xf7 4 byte, 0xf8- and higher invalid
};


	wstring sout;
	if (!src){
		return sout;
	}


	while (*src){
		switch(XML_utf8ByteTable[*(const unsigned char*)src]){


		case 1: // 00h-7fh (7 bit)
			sout += *src++;
			break;

		case 2: // c0-df h11 bit 
			{
				wchar_t tmp =  *src++ &0x1f;   // 110yyyyy
				if (!*src) continue;
				tmp <<=6;
				tmp |= *src++ &0x3f;   // 10xxxxxx
				sout += tmp;
			}
			break;

		case 3: // e0-efh 16 bit
			{
				wchar_t tmp =  *src++ &0x0f;   // 1110xxxx
				if (!*src) continue;
				tmp <<=6;
				tmp |= *src++ &0x3f;   // 10yyyyyy
				if (!*src) continue;
				tmp <<=6;
				tmp |= *src++ &0x3f;   // 10xxxxxx
				sout += tmp;
			}
			break;
/*
		case 4:  // we do not support 21 bit characters   
			{
				Uchar tmp =  *src++ &0x07;	// 11110www
				if (!*src) continue;
				tmp <<=6;
				tmp |= *src++ &0x3f;			// 10xxxxxx
				if (!*src) continue;
				tmp <<=6;
				tmp |= *src++ &0x3f;			// 10yyyyyy
				if (!*src) continue;
				tmp <<=6;
				tmp |= *src++ &0x3f;			// 10xxxxxx
				sout+=tmp;
			}
			break;
*/
		default:
			src++;
		}
	}
	return sout;
}; 




COutPduSms::COutPduSms(void)
: m_bForceUDH(false)
, m_bFlash(false)
, m_bRRq(false)
, m_nTTL(-1)
, m_bForceUnicode(false)
, m_bDisable8Bit(false)
, m_nReference(++gReference)
{
}

COutPduSms::COutPduSms (const char* phone, const wchar_t* text)
: m_sPone(phone)
, m_sText(text)
, m_bForceUDH(false)
, m_bFlash(false)
, m_bRRq(false)
, m_nTTL(-1)
, m_bForceUnicode(false)
, m_bDisable8Bit(false)
, m_nReference(++gReference)
{

}

COutPduSms::COutPduSms (const char* phone, const char* text, bool isUTF8)
: m_sPone(phone)
, m_bForceUDH(false)
, m_bFlash(false)
, m_bRRq(false)                         
, m_nTTL(-1)
, m_bForceUnicode(false)
, m_bDisable8Bit(false)
, m_nReference(++gReference)
{
	AddText(text, isUTF8);
}


COutPduSms::~COutPduSms(void)
{
}

void COutPduSms::AddText(const char* text, bool isUTF8) {
	if (isUTF8) {
		m_sText = ::UTF8ToUnicode(text);
	}
	else {  // 8 bit plain text
		int len = strlen(text);
		m_sText.reserve(len + 5);
		while (*text) {
			m_sText += *((unsigned char*)text++);
		}
	}

}


string ClearPhoneNo (string text){
	if (text[0] == '+')
		text.erase(text.begin());

	for (string::iterator it = text.begin(); it != text.end(); ++it){
		if (!isdigit( *it)){
			text.erase( it, text.end());
			break;
		}
	}
	return text;
}

string BinToText ( unsigned char* ptr, int len ){
	string sout;
	sout.reserve(len*2+2);
	for (int i=0;i < len;i++){
		char tmp[100];
		sprintf (tmp, "%02X", *ptr++);
		sout += tmp;
	}
	return sout;
}

string BinToText ( unsigned char chr){
	char tmp[100];
	sprintf (tmp, "%02X", chr);
	return tmp;
}

static string EncodeHexDec(string text){
	string rtn;
	if (text.length() &1)
		text+= 'F';

	int len = text.length();
	string out;
	for (int i = 0; i < len; ++i){
		out += text[i^1];
	}

	return out;
}


string COutPduSms::MakeSmsSubmitHeader(bool isHasUDH, PDU_EncodingScheme encodingScheme)
{
	string phoneNo = ClearPhoneNo (m_sPone);
	string hdr;
	hdr.reserve(60);
	hdr += "00";                    // SMSC
	unsigned char    nSmsSubmit  = 0x01; // TP_MTI
	if (isHasUDH) nSmsSubmit |= 0x40;
	if (m_bRRq)      nSmsSubmit |= 0x20;
	if (m_nTTL>0)    nSmsSubmit |= 0x10;  // TP-VPF b10 - Relative format
	hdr += BinToText(nSmsSubmit); // FirstOctet of SMS Submit
	hdr += "00";                  // TP-Message-Reference
	hdr += BinToText(phoneNo.length());	// Address Length (phone len)

										// Type-Of-Address
	if (!m_sPone.empty() && m_sPone[0] == '+')
		hdr += "91";					// Type international / ISDN
	else
		hdr += "81";					// Type unknown/ISDN - to be used for national numbers

	hdr += EncodeHexDec(phoneNo);	// Phone No
	hdr += "00";					// TP-PID
	unsigned char tp_dcs=0;
	switch (encodingScheme){
		case PDU_7 :/*tp_dcs=0*/;  break; 
		case PDU_8 : tp_dcs |= 4;  break; 
		case PDU_16: tp_dcs |= 8;  break;
//		default: ASSERT(false);

	}
	if (m_bFlash)
		tp_dcs |= 0x10;
	hdr += BinToText(tp_dcs); // TP-DCS

	if (m_nTTL>0)
		hdr += BinToText(m_nTTL);			// TTL if need

	return hdr;
}

static string encode7bitStep8to7 (const wchar_t* text, int len){
//	const unsigned char mask[8] = {0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f}
	unsigned char bout[8];
	memset (bout, 0, sizeof bout);

	for (int i=0; i< len; ++i){
		unsigned char chr = (unsigned char) text[i]&0x7f;
		bout[i] = chr>>i;
		if (i>0)
			bout[i-1] |= chr<< (8-i);
	}
	if (len==8)
		len=7;

	return BinToText(bout, len);
}

static string encode7bit (const wchar_t* text, int maxtextlen ){
	string sout;
	sout.reserve (maxtextlen+10);

	while (maxtextlen >0){
		int len = min (maxtextlen, 8);
		sout += encode7bitStep8to7( text, len);
		text+=len;
		maxtextlen -= len;
	}
	return sout;
}


TOutPduBlock COutPduSms::ParseText(void)
{
	// Define encoding mode 

	wstring sText = m_sText;
	if (!sText.length())
		sText += ' ';


// Choice encoding scheme
	PDU_EncodingScheme encodingScheme = PDU_7;  // default is 7 bit	
	wstring sms7BitText;
	if (m_bForceUnicode){
		encodingScheme = PDU_16;
	}
	else{  // Scan message for unicode characters
		bool _8bitFound = false;
		bool _16bitFound = false;

		for (wstring::iterator it = sText.begin() ;it != sText.end(); ++it){
			if (*it&0xff00){
				_16bitFound = true;
				break;
			}
			unsigned char code = lookup7Bit[*it];
			if (code==0xff){
				_8bitFound = true;
			}
			if (!_8bitFound){
				if (code >=0x80){
					sms7BitText += 0x1b; // Add ESC
					code &=0x7f;     // Clear esc flag
				}
				sms7BitText += code; // adding clear code, code after ESC
			}
		}

		if (!_8bitFound && !_16bitFound && sms7BitText.length()*7/8 >= sText.length())
			_8bitFound = true; // force 8 bit if too meny esc characters found

		if (_16bitFound)
			encodingScheme = PDU_16;
		else if (_8bitFound)
			encodingScheme = m_bDisable8Bit ? PDU_16 : PDU_8;
		else{ 
			; // well 7 bit coding is OK for the message. 160(153) characters per SMS will be used
			sText = sms7BitText;
		}
	}

	int textLen = sText.length();
	int maxPartSize = MAX_TEXT_LEN[encodingScheme]; // max characters of sText can be sent in 1 part
	bool hasUDH = m_bForceUDH || m_bRRq || (textLen > maxPartSize);

	if (hasUDH)           // multiprt SMS has to be used
		maxPartSize -= UdhLenInChars[encodingScheme]; // Reserve space for header

	int nParts = (textLen-1)/maxPartSize+1;

	string smsPartHdr = MakeSmsSubmitHeader (hasUDH, encodingScheme); // Header common for all parts of SMS (next byte is TP-USER-DATA-LENGTH
//	TRACE ("%s\n", smsPartHdr.c_str());

	const wchar_t* txt = sText.c_str();


	T_UDH udh = {5, 0, 3, 
		m_nReference,
		(unsigned char)nParts, 0
	};

	TOutPduBlock outList;

	for (int iPart=0; iPart<nParts; ++iPart){
		string smsText = smsPartHdr;
		int partSZ = min(textLen, maxPartSize);
		int dataSZ = (hasUDH) ? partSZ + UdhLenInChars[encodingScheme] : partSZ;
		if (encodingScheme == PDU_16)
			dataSZ<<=1;
		smsText += BinToText(dataSZ);  // UDL in bytes including UDH for 7 bit encoding UDL is	in septets 
		if (hasUDH){
			udh.thisPart = iPart+1;
			smsText+= BinToText ((unsigned char*)&udh, sizeof udh);
			if (encodingScheme == PDU_7){ // manual boudary correction
				if (textLen>0){
					unsigned char chr = (*txt++)&0x7f;
					smsText += BinToText(chr<<1);	// insert 1 8-bit character as is.
													// it result inserting 1 more zero bit 
													// to close first 8->7 byte step 
													// 6 bytes UDH + 1 text byte = 7 bytes is emited 
												
				    partSZ--;
					textLen--;
				}
				else {
					smsText +='0';  // no data - insert empty
				}
			}
		}
		switch (encodingScheme){
			case PDU_7:
				smsText += encode7bit(txt, partSZ);
				break;

			case PDU_8:
				for (int i=0; i < partSZ; ++i )
					smsText += BinToText(txt[i]&0xff);
				break;

			case PDU_16:
				for (int i=0; i < partSZ; ++i ){
                                        smsText += BinToText ((txt[i] >>8)&0xff);
                                        smsText += BinToText ( txt[i]     &0xff);
				}
				break;

//			default: 	ASSERT(false);
		}
//		TRACE ("%s\n", smsText.c_str());

		
		outList.insert(outList.end(), smsText);

		txt += partSZ;
		textLen -= partSZ;
	}
	return outList;

}
