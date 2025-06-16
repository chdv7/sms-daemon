#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "RecvSMS.h"
#include "rsserial.h"
#include "sms-daemon-config.h"

typedef int (*SmsCallBack)(const std::string number, const std::string text,
                           std::string& replay, void* userdata);
typedef int (*SmsCallBackXml)(XMLNode sms, std::string& replay, void* userdata);
class CSmsDaemon {
  struct TMdmRcvSms {
    std::string pdu;
    int index{-1};
    bool isProcessed{false};
  };
  struct TSmsBlock {
    int err{-1};
    std::vector<TMdmRcvSms> sms;
    bool IsError() { return err < 0; }
  };
  struct TCallBackDescriptor {
    TCallBackDescriptor(SmsCallBack fn_, void* userdata_)
        : fn(fn_), userdata(userdata_) {}
    SmsCallBack fn;
    void* userdata;
  };
  std::vector<TCallBackDescriptor> m_SmsInCallback;

  struct TCallBackXMLDescriptor {
    TCallBackXMLDescriptor(SmsCallBackXml fn_, void* userdata_)
        : fn(fn_), userdata(userdata_) {}
    SmsCallBackXml fn;
    void* userdata;
  };
  std::vector<TCallBackXMLDescriptor> m_SmsInXMLCallback;

  std::unordered_map<int, bool> m_DirtySimSlots;
  std::string m_DeviceName{DEVICE};
  std::string m_CachePath{IN_SMS_CACHE_NAME};
  std::string m_OutSmsMailDir{OUT_SMS_DIR};
  CRSSerial m_Connector;
  CRecvSMSProcessor m_RecvSMSProcessor;
  int m_LoopDelay{LOOP_SLEEP_MS};

  void Init();
  int Do();
  void DoProcessInSmsBlock();
  TSmsBlock GetSmsBlockFromModem();
  TSmsBlock GetSmsBlockByCMGR();
  TSmsBlock GetSmsBlockByCMGL();
  int ProcessSmsBlock(TSmsBlock& smsBlock);
  void DoProcessOutSmsFolder();
  int SendSmsPart(std::string pdu);
  int SendSms(std::string number, std::string text, int flags = 0);
  bool DelSms(int index);
  void DelSmsBlock(vector<TMdmRcvSms> sms);
  int OnCompleteSmsDecodeCB(XMLNode sms);
  void onReceivedSMS(const ReceivedSMS&);

 public:
  CSmsDaemon() {}
  void Setup();
  int Go();
  void RegisterInSmsCallBack(SmsCallBack fn, void* userdata = nullptr) {
    m_SmsInCallback.emplace_back(fn, userdata);
  }
  void RegisterInSmsCallBack(SmsCallBackXml fn, void* userdata = nullptr) {
    m_SmsInXMLCallback.emplace_back(fn, userdata);
  }
  struct SmsDaemonError {
    SmsDaemonError(int code = 0, std::string explanation = std::string())
        : Verbose(explanation), ErrorCode(code) {}
    std::string Verbose;
    int ErrorCode;
  };
};
