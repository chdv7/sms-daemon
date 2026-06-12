#pragma once
#include <array>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

#include "RecvSMS.h"
#include "Ussd.h"
#include "rsserial.h"
#include "sms-daemon-config.h"

namespace chdv::sms_daemon {
using SmsCallBack = std::function <int (const ReceivedSMS&)>;
using UssdCallBack = std::function<int(const ReceivedUssd&)>;

constexpr int maxSmsIndex = 128;
class CSmsDaemon {
    struct TMdmRcvSms {
        std::string pdu;
        uint8_t index{};
        bool isProcessed{false};
    };
    struct TSmsBlock {
        int err{-1};
        std::vector<TMdmRcvSms> sms;
        bool IsError() {
            return err < 0;
        }
    };

    std::vector<SmsCallBack> m_SmsInCallback;
    std::vector<UssdCallBack> m_UssdInCallback;
    std::string m_ModemInputBuffer;

    std::array<bool, maxSmsIndex> m_DirtySimSlots;
    std::string m_DeviceName{DEVICE};
    std::string m_CachePath{IN_SMS_CACHE_NAME};
    std::string m_OutSmsMailDir{OUT_SMS_DIR};
    CRSSerial m_Connector;
    CRecvSMSProcessor m_RecvSMSProcessor;
    int m_LoopDelay{LOOP_SLEEP_MS};

    void Init();
    int Do();
    void DoProcessInSmsBlock();
    void DoProcessModemInput();
    void ProcessModemInput(const std::string& input);
    int SendUssd(const std::string& request);
    TSmsBlock GetSmsBlockFromModem();
    TSmsBlock GetSmsBlockByCMGR();
    TSmsBlock GetSmsBlockByCMGL();
    int ProcessSmsBlock(TSmsBlock& smsBlock);
    void DoProcessOutSmsFolder();
    int SendSmsPart(std::string pdu);
    int SendSms(std::string number, std::string text, int flags = 0);
    bool DelSms(int index);
    void DelSmsBlock(vector<TMdmRcvSms> sms);

public:
    CSmsDaemon() {
    }
    void Setup();
    int Go();
    void RegisterInSmsCallBack(SmsCallBack fn) {
        m_SmsInCallback.emplace_back(fn);
    }
    void RegisterInUssdCallBack(UssdCallBack fn) {
        m_UssdInCallback.emplace_back(fn);
    }

    struct SmsDaemonError {
        SmsDaemonError(int code = 0, std::string explanation = std::string()) : Verbose(explanation), ErrorCode(code) {
        }
        std::string Verbose;
        int ErrorCode;
    };
};
} //namespace chdv::sms_daemon