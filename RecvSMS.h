#pragma once
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "xmlParser.h"

using namespace std;

#define MAX_SMS_TIME (15 * 24 * 3600)  // 15 days

using smsCacheKey = std::pair<uint8_t, std::wstring>;

struct hashSmsKey {
  size_t operator()(const smsCacheKey& p) const noexcept {
    return (p.first + (p.first << 6)) ^ (p.first << 16) ^
           hash<wstring>{}(p.second);
  }
};
class CRecvSMSPart {
 public:
  CRecvSMSPart(void);
  ~CRecvSMSPart(void);
  int ProcessRecvPDU(std::string rawText);
  std::string m_RawText{};
  std::wstring m_SMSC{};
  std::wstring m_sText{};
  std::wstring m_From{};
  std::string m_TimeStamp{};
  uint8_t m_nPartNo{};
  uint8_t m_nParts{};
  uint8_t m_nRefNr{};
  time_t m_RecvTime{};
  XMLNode GenXML(bool debugFlag = false) const;
  smsCacheKey getKey() const { return {m_nRefNr, m_From}; }
  bool operator<(const CRecvSMSPart& other) const {
    return getKey() < other.getKey();
  }
};
struct SmsCacheEntry {
  SmsCacheEntry(size_t nParts) : parts(nParts) {}
  int addPart(std::unique_ptr<CRecvSMSPart> part) {
    if (part->m_nPartNo == 0) return -111;
    size_t index = part->m_nPartNo - 1;
    if (index >= parts.size()) return -112;
    int ret = 0;
    if (parts[index] == nullptr){
      nPartsReceived++;
      lastUpdate = time(NULL);
    }
    else
      ret = 113;
    parts[index] = std::move(part);
    return ret;
  }
  size_t nParts() { return parts.size(); }
  size_t nRecvdParts() { return nPartsReceived; }

  bool isReceived() { return nRecvdParts() == nParts(); }
  time_t lastUpdate {};
  size_t nPartsReceived{};
  std::vector<std::unique_ptr<CRecvSMSPart>> parts;
};

using SmsCache = std::unordered_map<smsCacheKey, SmsCacheEntry, hashSmsKey>;
class ReceivedSMS {
 public:
  ReceivedSMS(std::unique_ptr<CRecvSMSPart> onePartSms,
              const std::string& m_Interface)
      : m_RecvTime(time(nullptr)), m_Interface(m_Interface) {
    parts.push_back(std::move(onePartSms));
    init();
  }
  ReceivedSMS(std::vector<std::unique_ptr<CRecvSMSPart>>&& parts,
              const std::string& m_Interface);
  XMLNode GenXML(bool includeParts = false, bool debugFlag = false) const;
  std::wstring m_sText{};
  std::wstring_view m_From{};
  std::wstring_view m_SMSC{};
  std::string_view m_TimeStamp{};
  size_t m_nParts{};
  time_t m_RecvTime{};
  std::vector<std::unique_ptr<CRecvSMSPart>> parts;
  std::string m_Interface;
  bool isOk {true};
 private:
  void init();
};

class CRecvSMSProcessor {
 public:
  CRecvSMSProcessor() = default;
  ~CRecvSMSProcessor() = default;
//  int m_nSmsProcessed{};
//  int m_nPartsProcessed{};
  time_t smsPartTimeout_ {600};
  int Init(string cachePath, string interfaceID);
  void SetSmsCallBack(std::function<void(const ReceivedSMS&)> fn) {
    onSmsCallBack = fn;
  }
  int processPDU(const char* pdu);
  void clearCache();
 protected:
  int ProcessPart(std::unique_ptr<CRecvSMSPart> sms);
  string m_InterfaceID{};
  std::function<void(const ReceivedSMS&)> onSmsCallBack{};
  SmsCache cache;
  int64_t m_LastProcessedItemID;
  std::string m_sCachePath;
};