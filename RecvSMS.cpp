#include "RecvSMS.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ut.h"

#define ASSERT(a)

static bool const HexDecChars[128] = {
    //      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F
    /* 0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 1 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 2 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    /* 4 */ 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 5 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 6 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 7 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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

inline bool isHexDecChar(char chr) {
  return chr & 0x80 ? false : HexDecChars[(int)chr];
}
unsigned short lookup7_8Bit[] = {
    //	    0       1       2       3       4       5       6       7       8 9
    // A       B       C       D       E       F
    0x0040, 0x00A3, 0x0024, 0x00A5, 0x00E8, 0x00E9, 0x00F9, 0x00EC, 0x00F2,
    0x00C7, 0x000A, 0x00D8, 0x00F8, 0x000D, 0x00C5, 0x00E5,  // 0
    0x0394, 0x005F, 0x03A6, 0x0393, 0x039B, 0x038F, 0x03A0, 0x03A8, 0x03A3,
    0x0398, 0x039E, 0xFFFF, 0x00C6, 0x00E6, 0x00DF, 0x00C9,  // 1
    0x0020, 0x0021, 0x0022, 0x0023, 0x00A4, 0x0025, 0x0026, 0x0027, 0x0028,
    0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,  // 2
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038,
    0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,  // 3
    0x00A1, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048,
    0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,  // 4
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058,
    0x0059, 0x005A, 0x00C4, 0x00D6, 0x00D1, 0x00DC, 0x00A7,  // 5
    0x00BF, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068,
    0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,  // 6
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078,
    0x0079, 0x007A, 0x00E4, 0x00F6, 0x00F1, 0x00FC, 0x00E0,  // 7
    // Escaped characters
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0x000C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,  // 8
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x005E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,  // 9
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x007B,
    0x007D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x005C,  // A
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0x005B, 0x007E, 0x005D, 0xFFFF,  // B
    0x007C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,  // C
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,  // D
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x20AC, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,  // E
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,  // F
};

void AppendWstring(wstring& w, const char* s) {
  while (*s) w += *(unsigned char*)(s++);
}

struct T_UDH {
  unsigned char udhl;
  unsigned char iei;
  unsigned char iedl;
  unsigned char refNr;
  unsigned char totalParts;
  unsigned char thisPart;
};

static int TextToBin(const char* text, int maxlen, unsigned char* bin) {
  int bLen = strlen(text) / 2;
  if (bLen > maxlen) bLen = maxlen;

  for (int i = 0; i < bLen; ++i, text += 2) {
    int bin1 = -1;
    if (sscanf(text, "%02X", &bin1) != 1 || bin1 < 0) return -1;
    bin[i] = bin1;
  }
  return bLen;
}

static string decode7BitStep(const char* text, int len) {
  unsigned char bIn[7];
  memset(bIn, 0, sizeof bIn);
  int sz = TextToBin(text, len == 8 ? 7 : len, bIn);
  string out;
  for (int i = 0; i < len; ++i) {
    if ((i >= sz) && (i != 7))  // check for end of string
      break;
    int chr = 0;
    switch (i) {
      case 0:
        chr = bIn[0];
        break;
      case 1:
        chr = (bIn[0] >> 7) | (bIn[1] << 1);
        break;
      case 2:
        chr = (bIn[1] >> 6) | (bIn[2] << 2);
        break;
      case 3:
        chr = (bIn[2] >> 5) | (bIn[3] << 3);
        break;
      case 4:
        chr = (bIn[3] >> 4) | (bIn[4] << 4);
        break;
      case 5:
        chr = (bIn[4] >> 3) | (bIn[5] << 5);
        break;
      case 6:
        chr = (bIn[5] >> 2) | (bIn[6] << 6);
        break;
      case 7:
        chr = bIn[6] >> 1;
        break;
    }
    chr &= 0x7f;
    if (chr == 0) {  // @ character will be encoded as code 0x80 higest bit
                     // will be cutted at decode7bit
      chr = 0x80;
    }
    out += chr;
  }
  return out;
}

static wstring decode7bit(const char* text, int maxOutLen, int firstGSMChar) {
  string sgsmCode;
  sgsmCode.reserve(maxOutLen + 3);
  if (firstGSMChar >= 0) sgsmCode += firstGSMChar;

  while (maxOutLen > 0) {
    int len = min(maxOutLen, 8);  // 7 bytes = 56 bit group
    sgsmCode += decode7BitStep(text, len);
    if (strlen(text) <= 14) break;
    text += 14;
    maxOutLen -= len;
  }

  wstring sout;
  sout.reserve(sgsmCode.length() + 3);

  int escPreffix = 0;
  for (string::iterator it = sgsmCode.begin(); it != sgsmCode.end(); ++it) {
    unsigned char chr = *it & 0x7f;
    if (*it == 0x1b) {
      escPreffix = 0x80;
      continue;
    }
    wchar_t wchr = lookup7_8Bit[escPreffix | chr];
    if (wchr == 0xFFFF) {
      ASSERT(false);
      wchr = L'?';
    }
    sout += wchr;
    escPreffix = 0;
  }
  return sout;
}

CRecvSMSPart::CRecvSMSPart(void) : m_RecvTime(time(NULL)) {}

static int GetInt(const char* sInt) {
  if (!isHexDecChar(*sInt)) return -1;
  if (!isHexDecChar(*(sInt + 1))) return -1;
  int rtn = -1;
  sscanf(sInt, "%02X", &rtn);
  return rtn;
}

static int GetPhone(const char* s, std::wstring* phone, bool isSMSC) {
  std::wstring _x;
  if (!phone) phone = &_x;
  phone->clear();
  size_t phoneSZ = GetInt(s);

  if (phoneSZ < 0) return phoneSZ;

  if (phoneSZ == 0) return 2;

  if (isSMSC) {  // smsc provide number in octets including type octet
    phoneSZ = (phoneSZ - 1) * 2;  // convert it to phone length
  }
  size_t padding = phoneSZ & 1;
  size_t textSZ = 2;
  s += 2;
  int type = GetInt(s);
  if (type < 0) return type;
  textSZ += 2;
  s += 2;
  switch (type & 0x70) {
    case 0x50: {
      *phone = decode7bit(s, phoneSZ * 4 / 7, -1);
      //				printf ("}%s{", phone->c_str());
      textSZ += phoneSZ + padding;
    } break;
    case 0x10:
      *phone = L"+";
    default:
      phoneSZ += padding;
      if (strlen(s) < (phoneSZ + padding)) return -2;
      textSZ += phoneSZ;
      for (; phoneSZ > 0; phoneSZ -= 2, s += 2) {
        *phone += *(s + 1);
        if (*s != 'F') *phone += *s;
      }
  }
  return textSZ;
}

CRecvSMSPart::~CRecvSMSPart(void) {}

string GetTimeStamp(const char* s) {
  string out;
  if (strlen(s) < 14) return out;

  out += s[5];  // day
  out += s[4];
  out += '.';

  out += s[3];  // mon
  out += s[2];
  out += '.';

  out += s[1];  // year
  out += s[0];
  out += ' ';

  out += s[7];  // hour
  out += s[6];
  out += ':';

  out += s[9];  // min
  out += s[8];
  out += ':';

  out += s[11];  // sec
  out += s[10];

  char tmp3[3] = {s[13], s[12], 0};
  int i_tz = 1000;

  if ((sscanf(tmp3, "%d", &i_tz) == 1) && (i_tz != 1000)) {
    char tmp30[30];
    sprintf(tmp30, " UTC+%02d:%02d", i_tz >> 2, (i_tz & 3) * 15);
    out += tmp30;
  }

  return out;
}

int CRecvSMSPart::ProcessRecvPDU(std::string rawText) {
  m_RawText = rawText;
  const char* sms = rawText.c_str();

  // Get SMS centre
  int sz = GetPhone(sms, &m_SMSC, true);
  if (sz < 0) return sz;
  sms += sz;

  // Get FirstOctet of SMS_DELIVER
  int nSmsDeliver = GetInt(sms);
  if (nSmsDeliver < 0) return nSmsDeliver;
  sms += 2;

  // Get From
  sz = GetPhone(sms, &m_From, false);
  if (sz < 0) return sz;
  sms += sz;

  int pid = GetInt(sms);
  if (pid < 0) return pid;
  sms += 2;

  int dcs = GetInt(sms);
  if (dcs < 0) return dcs;
  sms += 2;

  if (strlen(sms) < 14) return -3;
  m_TimeStamp = ::GetTimeStamp(sms);
  sms += 14;

  int udl = ::GetInt(sms);
  if (udl < 0) return udl;

  sms += 2;
  // userData
  bool hasUDH = nSmsDeliver & 0x40;
  T_UDH udh;
  if (hasUDH) {
    if (TextToBin(sms, sizeof(udh), (unsigned char*)&udh) < 0) return -6;
    udl -= sizeof(udh);  // correct UDL with size of UDH in bytes for 7 bit
                         // coding it has to be corrected additionaly
    m_nParts = udh.totalParts;
    m_nPartNo = udh.thisPart;
    m_nRefNr = udh.refNr;
    sms += sizeof(udh) * 2;
  }
  switch (dcs & 0xEC) {
    case 0x00: {
      int first_chr_of_7bitSMS = -1;
      if (hasUDH) {  // manual correction of first character
        udl -= 1;    // correct UDL. Length of UDH in septets is 7. 6 of them
                     // is already corrected by UDH processing. UDH len in
                     // bytes(6) already counded during UDH procesing
        if (udl > 0) {
          first_chr_of_7bitSMS = GetInt(sms);
          if (first_chr_of_7bitSMS < 0) return -7;
          first_chr_of_7bitSMS >>= 1;
          udl--;
          sms += 2;
        }
      }
      m_sText += ::decode7bit(sms, udl, first_chr_of_7bitSMS);
    } break;
    case 0x04: {
      char buffer[512];
      int sz = TextToBin(sms, udl, (unsigned char*)buffer);
      buffer[sz] = 0;
      AppendWstring(m_sText, buffer);
    } break;
    case 0x08: {
      int udl16bit = udl / 2;
      for (int i = 0; i < udl16bit; ++i) {
        wchar_t wchr;
        int sz = TextToBin(sms, 2, (unsigned char*)&wchr);
        if (sz != 2) break;
        m_sText += ntohs(wchr);
        sms += 4;
      }
    } break;
    default:
      return -5;
  }
  return 0;
}

int CRecvSMSProcessor::Init(string cachePath, string interfaceID) {
  m_InterfaceID = interfaceID;
  m_LastProcessedItemID = 0;

  return 0;
}

int CRecvSMSProcessor::processPDU(const char* pdu) {
  auto sms = std::make_unique<CRecvSMSPart>();
  sms->ProcessRecvPDU(pdu);

  if (sms->m_nParts <= 1) {
    if (onSmsCallBack) {
      ReceivedSMS receivedSMS(std::move(sms), m_InterfaceID);
      onSmsCallBack(receivedSMS);
    }
  } 
  else
    ProcessPart(std::move(sms));
  return 0;
}

XMLNode CRecvSMSPart::GenXML(bool debugFlag) const {
  XMLNode xPart = XMLNode::createXMLTopNode("Part");

  char tmp1024[1024];

  xPart.addAttribute("From", toUTF8(m_From).c_str());

  sprintf(tmp1024, "%02X", m_nRefNr);
  xPart.addAttribute("ref", tmp1024);

  sprintf(tmp1024, "%d", m_nPartNo);
  xPart.addAttribute("partno", tmp1024);

  xPart.addAttribute("time", m_TimeStamp.c_str());
  xPart.addAttribute("smsc", toUTF8(m_SMSC).c_str());

  xPart.addAttribute("ReceiveTime", toLocalTime(m_RecvTime).c_str());

  sprintf(tmp1024, "%d", m_nParts);
  xPart.addAttribute("nparts", tmp1024);

  if (debugFlag) xPart.addAttribute("raw", m_RawText.c_str());

  xPart.addText(toUTF8(m_sText).c_str());
  return xPart;
}

ReceivedSMS::ReceivedSMS(
    std::vector<std::unique_ptr<CRecvSMSPart>>&& cachedParts,
    const std::string& m_Interface)
    : m_RecvTime(time(nullptr)), m_Interface(m_Interface) {
  parts = std::move(cachedParts);
  init();
}

void ReceivedSMS::init() {

  m_nParts = parts.size();

  for (const auto& part : parts){
    if (part){
      m_From = part->m_From;
      m_SMSC = part->m_SMSC;
      m_TimeStamp = part->m_TimeStamp;
      break;
    }
  }
  bool missed = false;
  for (const auto& part : parts){
    if (part){ 
      m_sText += part->m_sText;
      missed = false;
    }
    else if (!missed){
      isOk = false;
      m_sText += L"<...>";
      missed = true;
    }
  }
}

XMLNode ReceivedSMS::GenXML(bool includeParts, bool debugFlag) const {
  XMLNode xSms = XMLNode::createXMLTopNode("Sms");
  char tmp1024[1024];

  xSms.addAttribute("From", toUTF8(m_From).c_str());

  xSms.addAttribute("time", m_TimeStamp.data());
  xSms.addAttribute("smsc", toUTF8(m_SMSC).c_str());

  xSms.addAttribute("ReceiveTime", toLocalTime(m_RecvTime).c_str());

  xSms.addAttribute("status", isOk? "Received" : "Bad");
  xSms.addAttribute("Interface", m_Interface.c_str());
  sprintf(tmp1024, "%d", m_nParts);
  xSms.addAttribute("nParts", tmp1024);

  xSms.addText(toUTF8(m_sText).c_str());
  if (includeParts)
    for (const auto& part : parts) {
      if (!part)
        continue;
      auto xPart = part->GenXML(debugFlag);
      xSms.addChild(xPart);
    }
  return xSms;
}

int CRecvSMSProcessor::ProcessPart(std::unique_ptr<CRecvSMSPart> pSms) {
  auto key = pSms->getKey();
  auto itSmsStorage = cache.find(key);  //

  if (itSmsStorage == cache.end()) {
    auto [it_new, is_ok] = cache.emplace(key, pSms->m_nParts);
    if (!is_ok) return -1;
    itSmsStorage = it_new;
  }
  auto& smsStorage = itSmsStorage->second;
  if (smsStorage.nParts() != pSms->m_nParts) {
    fprintf(stderr,
            "Differet part indicator registered nParts:%u <> part#:%u has "
            "nParts:%u\n",
            (unsigned int)smsStorage.nParts(), pSms->m_nPartNo, pSms->m_nParts);
  }
  auto err = smsStorage.addPart(std::move(pSms));
  if (err) fprintf(stderr, "Part storage error %d\n", err);
  if (smsStorage.isReceived()) {
    printf("all parts \n");
    ReceivedSMS receivedSMS(std::move(smsStorage.parts), m_InterfaceID);
    cache.erase(itSmsStorage);
    if (onSmsCallBack) onSmsCallBack(receivedSMS);
  }
  return 0;
}

void CRecvSMSProcessor::clearCache() {
    if(cache.empty())
        return;
    time_t now = time(NULL);
    vector<smsCacheKey> expiredEntries;
    for(auto& entry : cache)
        if((now - entry.second.lastUpdate) > smsPartTimeout_){
            expiredEntries.push_back(entry.first);
            if (onSmsCallBack)
              onSmsCallBack(ReceivedSMS (std::move(entry.second.parts), m_InterfaceID));
        }

    for(auto& entryKey : expiredEntries){
        cache.erase(entryKey);
    }
}