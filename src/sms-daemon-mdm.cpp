#include "sms-daemon-mdm.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <iostream>
#include <sstream>

#include "Config.h"
#include "PduSMS.h"
#include "sms-daemon-config.h"
#include "sms-daemon.h"
#include "ut.h"


using namespace std;
namespace chdv::sms_daemon {
//#define __DO_NOT_DEL_SMS__
//#define SMS_DAEMON_DEBUG_SMS_POLLING

static const char* answers[] = {"OK\r", "ERROR\r", NULL};
static const char* answers_go[] = {">", "OK\r", "ERROR\r", NULL};
static const char* answers_mmi[] = {"OK\r", "NO CARRIER\r", "ERROR\r", NULL};
static const char* answers_ccfc[] = {"OK\r", "ERROR\r", NULL};

std::string CleanModemResponse(const char* log) {
    std::string text = log ? log : "";
    for(char& ch : text) {
        if(ch == '\r' || ch == '\n')
            ch = ' ';
    }
    const auto first = text.find_first_not_of(" \t");
    if(first == std::string::npos)
        return std::string();
    const auto last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

int CallForwardReason(const std::string& serviceCode) {
    if(serviceCode == "21")
        return 0; // unconditional
    if(serviceCode == "67")
        return 1; // mobile busy
    if(serviceCode == "61")
        return 2; // no reply
    if(serviceCode == "62")
        return 3; // not reachable
    if(serviceCode == "002")
        return 4; // all forwarding
    if(serviceCode == "004")
        return 5; // all conditional forwarding
    return -1;
}

std::string BuildCcfcCommand(int reason, int mode) {
    char command[64];
    snprintf(command, sizeof(command), "AT+CCFC=%d,%d,,,1\r", reason, mode);
    return command;
}

std::string BuildCcfcRegisterCommand(int reason, const std::string& number, const std::string& timeout) {
    const int type = !number.empty() && number.front() == '+' ? 145 : 129;
    char command[320];
    if(timeout.empty())
        snprintf(command, sizeof(command), "AT+CCFC=%d,3,\"%s\",%d,1\r", reason, number.c_str(), type);
    else
        snprintf(command, sizeof(command), "AT+CCFC=%d,3,\"%s\",%d,1,,,%s\r", reason, number.c_str(), type, timeout.c_str());
    return command;
}

bool BuildCallForwardCommand(std::string_view mmiCode, std::string& command) {
    const std::string code(mmiCode);
    const size_t len = code.size();
    if(len < 4 || code.back() != '#')
        return false;

    std::string serviceCode;
    int mode = -1;
    if(code.rfind("*#", 0) == 0) {
        serviceCode = code.substr(2, len - 3);
        mode = 2;
    }
    else if(code.rfind("##", 0) == 0) {
        serviceCode = code.substr(2, len - 3);
        mode = 4;
    }
    else if(code[0] == '#') {
        serviceCode = code.substr(1, len - 2);
        mode = 0;
    }
    else if(code.rfind("**", 0) == 0 || code[0] == '*') {
        const size_t serviceStart = code.rfind("**", 0) == 0 ? 2 : 1;
        const size_t separator = code.find('*', serviceStart);
        if(separator == std::string::npos) {
            serviceCode = code.substr(serviceStart, len - serviceStart - 1);
            mode = 1;
        }
        else {
            serviceCode = code.substr(serviceStart, separator - serviceStart);
            const size_t numberStart = separator + 1;
            size_t numberEnd = code.find('*', numberStart);
            if(numberEnd == std::string::npos)
                numberEnd = len - 1;
            const std::string number = code.substr(numberStart, numberEnd - numberStart);
            if(number.empty())
                return false;

            std::string timeout;
            if(serviceCode == "61" && numberEnd + 2 < len && code.compare(numberEnd, 2, "**") == 0)
                timeout = code.substr(numberEnd + 2, len - numberEnd - 3);

            const int reason = CallForwardReason(serviceCode);
            if(reason < 0)
                return false;
            command = BuildCcfcRegisterCommand(reason, number, timeout);
            return true;
        }
    }
    else {
        return false;
    }

    const int reason = CallForwardReason(serviceCode);
    if(reason < 0 || mode < 0)
        return false;
    command = BuildCcfcCommand(reason, mode);
    return true;
}

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

bool isHexDecChar(char chr) {
    return chr & 0x80 ? false : HexDecChars[(int)chr];
}

std::string ExtractNumericIdentifier(const char* log) {
    std::string best;
    std::string current;
    const char* ptr = log ? log : "";
    for(; *ptr; ++ptr) {
        if(std::isdigit(static_cast<unsigned char>(*ptr))) {
            current += *ptr;
        }
        else {
            if(current.size() > best.size())
                best = current;
            current.clear();
        }
    }
    if(current.size() > best.size())
        best = current;
    return best;
}

void CSmsDaemon::Setup(const std::string& configPath, bool configRequired) {
    SmsDaemonConfig config;
    std::string error;

    std::cout << "Config: " << configPath << std::endl;

    if(!LoadSmsDaemonConfig(configPath, config, error, configRequired))
        throw SmsDaemonError(-1, error);
    std::cout << config << std::endl;
    m_DeviceName = config.device;
    m_OutSmsMailDir = config.jobDir;
    m_SmsXmlEnabled = config.smsDirConfigured;
    m_UssdXmlEnabled = config.ussdDirConfigured;
    if(m_SmsXmlEnabled)
        m_SmsInDir = config.smsDir;
    if(m_UssdXmlEnabled)
        m_UssdInDir = config.ussdDir;
    m_LogFile = config.logFile;
    m_SmsHooks = config.smsHooks;
    m_Debug = config.debug;
    SetLogPath(m_LogFile);
}
int CSmsDaemon::Go() {
    Init();
    return Do();
}

std::string CSmsDaemon::QueryModemIdentifier(const char* command) {
    char log[1024] = "";
    const int err = m_Connector.SendExpect(command, answers, 3000, log, sizeof(log));
    ProcessModemInput(log);
    if(err)
        return std::string();
    return ExtractNumericIdentifier(log);
}

void CSmsDaemon::Init() {
    m_RecvSMSProcessor.Init(m_CachePath, m_DeviceName);
    m_RecvSMSProcessor.SetSmsCallBack([this](ReceivedSMS& sms) {
        sms.m_IMSI = m_Imsi;
        sms.m_IMEI = m_Imei;
        for(auto& cb : m_SmsInCallback)
            cb(sms);
    });

    int err = m_Connector.Open(m_DeviceName.c_str());
    if(err < 0)
        throw SmsDaemonError(err, (string("Can not open device ") + m_DeviceName).c_str());
    m_Connector.SetComParams(115200);

    err = m_Connector.SendExpect("AT\r", answers);
    if(err)
        printf("AT error %d\n", err);

    err = m_Connector.SendExpect("AT&F\r", answers);
    if(err)
        printf("AT&F error %d\n", err);

    err = m_Connector.SendExpect("ATE0\r", answers);
    if(err)
        printf("ATE error %d\n", err);

    err = m_Connector.SendExpect("AT+CMGF=0\r", answers);
    if(err)
        printf("AT+CMGF error %d\n", err);

    m_Imsi = QueryModemIdentifier("AT+CIMI\r");
    m_Imei = QueryModemIdentifier("AT+CGSN\r");
    std::cout << "IMSI: " << (m_Imsi.empty() ? "unknown" : m_Imsi) << std::endl;
    std::cout << "IMEI: " << (m_Imei.empty() ? "unknown" : m_Imei) << std::endl;

    err = m_Connector.SendExpect("AT+CPMS=\"SM\",\"SM\",\"SM\"\r", answers);
    if(err)
        printf("AT+CPMS error %d\n", err);

    return;
}


int CSmsDaemon::Do() {
    for(;;) {
        DoProcessModemInput();
        DoProcessInSmsBlock();
        DoProcessOutSmsFolder();
        Sleep(m_LoopDelay);
    }
    return 0;
}

void CSmsDaemon::DoProcessModemInput() {
    for(;;) {
        int chr = m_Connector.ReceiveChar();
        if(chr < 0)
            break;
        m_ModemInputBuffer += static_cast<char>(chr);
    }

    size_t end = 0;
    while((end = m_ModemInputBuffer.find('\n')) != std::string::npos) {
        std::string line = m_ModemInputBuffer.substr(0, end);
        m_ModemInputBuffer.erase(0, end + 1);
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        ProcessModemInput(line);
    }
}

void CSmsDaemon::EmitUssdResponse(ReceivedUssd ussd) {
    ussd.interface = m_DeviceName;
    ussd.imsi = m_Imsi;
    ussd.imei = m_Imei;
    ussd.request = m_LastUssdRequest;
    ussd.sendTime = m_LastUssdSendTime;
    for(auto& cb : m_UssdInCallback)
        cb(ussd);
}

bool CSmsDaemon::ProcessModemInput(const std::string& input) {
    ReceivedUssd ussd;
    if(!ParseUssdResponse(input, ussd))
        return false;
    EmitUssdResponse(std::move(ussd));
    return true;
}

void CSmsDaemon::DoProcessInSmsBlock() {
    TSmsBlock smsBlock = GetSmsBlockFromModem();
    if(smsBlock.err < 0) {
        LogError(smsBlock.err, "Error processing incoming SMS");
        printf("Error processing incoming SMS: %d\n", smsBlock.err);
    }

    ProcessSmsBlock(smsBlock);
    DelSmsBlock(smsBlock.sms);
}

CSmsDaemon::TSmsBlock CSmsDaemon::GetSmsBlockByCMGR() {
    TSmsBlock rtn;
    for(int i = 0; i < 255; ++i) {
        char command[20];
        sprintf(command, "AT+CMGR=%d\r", i);
        char log[1000];
        rtn.err = m_Connector.SendExpect(command, answers, 1000, log, sizeof(log));
        ProcessModemInput(log);
        if(rtn.err) {
            if(!rtn.sms.empty())
                rtn.err = 0;
            else if(rtn.err > 0)
                rtn.err = -2;
            break;
        }
        else {
            char* ptr = strstr(log, "CMGR:");
            if(ptr) {
                if((ptr = strchr(ptr, '\r')) == NULL)
                    continue;
                if(*(++ptr) == '\n')
                    ptr++;
                TMdmRcvSms sms;
                sms.index = i;
                while(isHexDecChar(*ptr)) {
                    sms.pdu += *(ptr++);
                }
                rtn.sms.push_back(sms);
            }
        }
    }
    return rtn;
}

CSmsDaemon::TSmsBlock CSmsDaemon::GetSmsBlockByCMGL() {
    TSmsBlock rtn;
    char log[50 * 1024];
    rtn.err = m_Connector.SendExpect("AT+CMGL=4\r", answers, 3000, log, sizeof log);
    ProcessModemInput(log);
#ifdef SMS_DAEMON_DEBUG_SMS_POLLING
    cout << "AT+CMGL=4: " << log << endl << endl;
#endif
    if(rtn.err) {
        fprintf(stderr, "+CMGL Error %d\n", rtn.err);
        if(rtn.err > 0)
            rtn.err = -1;
    }
    else {
        for(char* ptr = log; ptr && ((ptr = strstr(ptr, "CMGL:")) != NULL);) {
            ptr += 5; // skip CMGL:
            TMdmRcvSms sms;
            int index = -1;
            if(sscanf(ptr, "%d", &index) > 0 && index >= 0 && index < maxSmsIndex) {
                sms.index = index;
                if((ptr = strchr(ptr, '\r')) == NULL)
                    break;
                if(*(++ptr) == '\n')
                    ptr++;
                while(isHexDecChar(*ptr)) {
                    sms.pdu += *(ptr++);
                }
                rtn.sms.push_back(sms);
            }
        }
    }
    return rtn;
}

CSmsDaemon::TSmsBlock CSmsDaemon::GetSmsBlockFromModem() {
    TSmsBlock rtn = GetSmsBlockByCMGL();
    if(rtn.IsError())
        rtn = GetSmsBlockByCMGR();
    return rtn;
}

int CSmsDaemon::ProcessSmsBlock(TSmsBlock& smsBlock) {
    int rtn = 0;
    for(TMdmRcvSms& sms : smsBlock.sms) {
        if(!m_DirtySimSlots[sms.index]) {
            m_RecvSMSProcessor.processPDU(sms.pdu.c_str());
            m_DirtySimSlots[sms.index] = true;
            rtn++;
        }
    }
    m_RecvSMSProcessor.clearCache();
    return rtn;
}

bool CSmsDaemon::DelSms(int smsIndex) {
#ifdef __DO_NOT_DEL_SMS__
    m_DirtySimSlots[smsIndex] = false;
    return true;
#else
    char command[100];
    sprintf(command, "at+cmgd=%d,0\r", smsIndex);

    char log[1024] = "";
    int err = m_Connector.SendExpect(command, answers, 2000, log, sizeof(log));
    ProcessModemInput(log);

    if(err)
        fprintf(stderr, "+CMGD error %d\n", err);
    else
        m_DirtySimSlots[smsIndex] = false;
    return err;
#endif
}

void CSmsDaemon::DelSmsBlock(vector<TMdmRcvSms> block) {
    for(auto& sms : block) {
        cout << sms.index << " " << sms.pdu << endl;
        if(m_DirtySimSlots[sms.index])
            DelSms(sms.index);
    }
}

int CSmsDaemon::SendUssd(std::string_view ussd, std::string request) {
    std::string cmd("AT+CUSD=1,");
    cmd += ussd;
    cmd += '\r';
    m_LastUssdRequest = request.empty() ? DecodeUssdRequest(ussd) : std::move(request);
    if(m_LastUssdRequest.empty())
        m_LastUssdRequest = std::string(ussd);
    m_LastUssdSendTime = time(nullptr);
    char log[4096] = "";
    auto err = m_Connector.SendExpect(cmd.c_str(), answers, 5000, log, sizeof(log));
    ProcessModemInput(log);
    if (err){
        m_Connector.Puts("\r\r", 10);
        std::cout << "Error in : " << cmd << " code:" << err << std::endl;
    }
    return err;
}


int CSmsDaemon::QueryCallForwarding(std::string_view code) {
    std::string cmd;
    if(!BuildCallForwardCommand(code, cmd))
        return -1;

    m_LastUssdRequest = std::string(code);
    m_LastUssdSendTime = time(nullptr);
    char log[4096] = "";
    auto err = m_Connector.SendExpect(cmd.c_str(), answers_ccfc, 15000, log, sizeof(log));
    if(ProcessModemInput(log))
        return err;

    ReceivedUssd ussd;
    ussd.mode = err ? 2 : 0;
    ussd.raw = log;
    ussd.text = CleanModemResponse(log);
    if(ussd.text.empty())
        ussd.text = err ? "ERROR" : "OK";
    EmitUssdResponse(std::move(ussd));
    return err;
}

int CSmsDaemon::SendMmi(std::string_view code) {
    const int ccfcErr = QueryCallForwarding(code);
    if(ccfcErr >= 0)
        return ccfcErr;
    std::string cmd("ATD");
    cmd += code;
    cmd += ";\r";
    m_LastUssdRequest = std::string(code);
    m_LastUssdSendTime = time(nullptr);
    char log[4096] = "";
    auto err = m_Connector.SendExpect(cmd.c_str(), answers_mmi, 15000, log, sizeof(log));
    const bool hasUssdResponse = ProcessModemInput(log);
    const int modemResponse = err;
    if(err == 1)
        err = 0;
    if(!hasUssdResponse) {
        ReceivedUssd ussd;
        ussd.mode = err ? 2 : 0;
        ussd.raw = log;
        ussd.text = CleanModemResponse(log);
        if(ussd.text.empty()) {
            if(modemResponse == 0)
                ussd.text = "OK";
            else if(modemResponse == 1)
                ussd.text = "NO CARRIER";
            else if(modemResponse == 2)
                ussd.text = "ERROR";
            else
                ussd.text = "No modem response";
        }
        EmitUssdResponse(std::move(ussd));
    }
    if(err) {
        m_Connector.Puts("\r\r", 10);
        std::cout << "Error in : " << cmd << " code:" << err << std::endl;
    }
    return err;
}


int CSmsDaemon::SendSmsPart(std::string pdu) {
    int err = 0;
    if(pdu.size() > 4) {
        for(auto it = pdu.begin(); it != pdu.end(); ++it) {
            if(!isHexDecChar(*it)) {
                pdu.erase(it, pdu.end());
                break;
            }
        }

        char cmd[200];
        char log[1024];
        memset(log, 0, sizeof(log));
        sprintf(cmd, "AT+CMGS=%zu\r", pdu.length() / 2 - 1);
        err = m_Connector.SendExpect(cmd, answers_go, 1000, log, sizeof(log));
        ProcessModemInput(log);
        if(!err) {
            pdu += "\x1a";
            memset(log, 0, sizeof(log));
            err = m_Connector.SendExpect(pdu.c_str(), answers, 20000, log, sizeof(log));
            ProcessModemInput(log);
        }
        if(!err)
            m_Connector.Puts("\r\r", 10);
    }
    else {
        err = -1;
    }
    return err;
}

void CSmsDaemon::DoProcessOutSmsFolder() {
    DIR* dir = opendir(m_OutSmsMailDir.c_str());
    if(!dir) {
        fprintf(stderr, "can not open %s\n", m_OutSmsMailDir.c_str());
        mkdir(m_OutSmsMailDir.c_str(), 0777);
        chmod(m_OutSmsMailDir.c_str(), 0777);
        return;
    };

    struct dirent* entry;
    while((entry = readdir(dir)) != NULL) {
        if(*entry->d_name == '.')
            continue;
        //        printf("%s\n", entry->d_name);
        string fname(m_OutSmsMailDir);
        fname += "/";
        fname += entry->d_name;
        cout << fname << " ";
        FILE* f = fopen(fname.c_str(), "r");
        if(f) {
            char buf[1024] = "";
            char ussdRequest[1024] = "";
            fgets(buf, sizeof(buf), f);
            fgets(ussdRequest, sizeof(ussdRequest), f);
            fclose(f);
            for(char* ptr = buf; *ptr; ++ptr)
                if(*ptr == '\r' || *ptr == '\n') {
                    *ptr = '\0';
                    break;
                }
            for(char* ptr = ussdRequest; *ptr; ++ptr)
                if(*ptr == '\r' || *ptr == '\n') {
                    *ptr = '\0';
                    break;
                }
            int err = 0;
            if (!strlen(buf))
                err = -100;
            else if (*buf == 'U')
                err = SendUssd(buf + 1, ussdRequest);
            else if (*buf == 'M')
                err = SendMmi(buf + 1);
            else
                err = SendSmsPart(buf);
            cout << err << " ";
            if(err) {
                string outStr(string("Can not process job ") + entry->d_name);
                fprintf(stderr, "%s\n", outStr.c_str());
                fprintf(stderr, "Error code:%d\n", err);
            }
        }
        unlink(fname.c_str());
        cout << endl;
    };

    closedir(dir);
}

int CSmsDaemon::SendSms(std::string number, std::string text, int flags) {
    COutPduSms sms_processor(number.c_str(), text.c_str());
    TOutPduBlock pdu_block = sms_processor.ParseText();

    for(auto& it : pdu_block) {
        SendSmsPart(it);
    }
    return 0;
}
} // namespace chdv::sms_daemon {