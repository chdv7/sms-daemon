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
    std::string m_LastUssdRequest;
    time_t m_LastUssdSendTime{};

    std::array<bool, maxSmsIndex> m_DirtySimSlots;
    std::string m_DeviceName{DEVICE};
    std::string m_CachePath{IN_SMS_CACHE_NAME};
    std::string m_OutSmsMailDir{OUT_SMS_DIR};
    std::string m_SmsInDir{IN_SMS_XML_DIR};
    std::string m_UssdInDir{IN_SMS_XML_DIR};
    std::string m_LogFile{SMS_LOG_FILE};
    std::vector<std::string> m_SmsHooks{};
    bool m_Debug{false};
    CRSSerial m_Connector;
    CRecvSMSProcessor m_RecvSMSProcessor;
    int m_LoopDelay{LOOP_SLEEP_MS};

    void Init();
    int Do();
    void DoProcessInSmsBlock();
    void DoProcessModemInput();
    bool ProcessModemInput(const std::string& input);
    void EmitUssdResponse(ReceivedUssd ussd);
    TSmsBlock GetSmsBlockFromModem();
    TSmsBlock GetSmsBlockByCMGR();
    TSmsBlock GetSmsBlockByCMGL();
    int ProcessSmsBlock(TSmsBlock& smsBlock);
    void DoProcessOutSmsFolder();
    int SendSmsPart(std::string pdu);
    int SendUssd(std::string_view ussd, std::string request = std::string());
    int SendMmi(std::string_view code);
    int QueryCallForwarding(std::string_view code);
    int SendSms(std::string number, std::string text, int flags = 0);
    bool DelSms(int index);
    void DelSmsBlock(vector<TMdmRcvSms> sms);

public:
    const std::string& SmsInDir() const { return m_SmsInDir; }
    const std::string& UssdInDir() const { return m_UssdInDir; }
    const std::string& JobDir() const { return m_OutSmsMailDir; }
    const std::string& LogFile() const { return m_LogFile; }
    const std::vector<std::string>& SmsHooks() const { return m_SmsHooks; }
    bool Debug() const { return m_Debug; }
    CSmsDaemon() {
    }
    void Setup(const std::string& configPath = SMS_CONFIG_PATH, bool configRequired = true);
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