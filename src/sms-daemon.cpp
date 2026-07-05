#include "sms-daemon.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

#include <iostream>
#include "gen-xml.hpp"
#include "sms-daemon-mdm.h"
#include "SmsHookRunner.h"
#include "ut.h"
#include "version.h"

namespace {
std::string g_LogFile = SMS_LOG_FILE;

void CreateDir(const std::string& path) {
    std::filesystem::create_directories(path);
    chmod(path.c_str(), 0777);
}

std::string SanitizeFileNamePart(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for(unsigned char ch : value) {
        if(std::isalnum(ch) || ch == '+' || ch == '-' || ch == '_' || ch == '.')
            result += static_cast<char>(ch);
        else
            result += '_';
    }
    return result;
}

std::string MakeUniqueOutputXmlPath(const std::string& dir, time_t when, const std::string& service, const std::string& sender) {
    std::tm tm{};
    localtime_r(&when, &tm);

    std::ostringstream base;
    base << std::put_time(&tm, "%Y%m%d-%H%M%S")
         << '-' << SanitizeFileNamePart(service);

    const auto safeSender = SanitizeFileNamePart(sender);
    if(!safeSender.empty())
        base << '-' << safeSender;

    std::filesystem::path path = std::filesystem::path(dir) / (base.str() + ".xml");
    for(int i = 1; std::filesystem::exists(path); ++i)
        path = std::filesystem::path(dir) / (base.str() + "-" + std::to_string(i) + ".xml");
    return path.string();
}
} // namespace

void SetLogPath(const std::string& path) {
    g_LogFile = path;
    std::filesystem::path logPath(path);
    if(logPath.has_parent_path())
        std::filesystem::create_directories(logPath.parent_path());
}

void Log(const char* text) {
    FILE* f = fopen(g_LogFile.c_str(), "a");
    if(f) {
        fprintf(f, "SMS daemon: %s\n", text);
        fclose(f);
    }
}

void LogError(int code, const char* text) {
    FILE* f = fopen(g_LogFile.c_str(), "a");
    if(f) {
        fprintf(f, "%d SMS daemon: %s\n", code, text);
        fclose(f);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "sms-daemon version " << SMS_DAEMON_VERSION << std::endl;

    mkdir(SMS_TMP_DIR, 0777);
    chmod(SMS_TMP_DIR, 0777);

    int isdaemon = 0;
    std::string configPath = SMS_CONFIG_PATH;
    bool configRequired = true;
    for(int i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "--daemon")) {
            isdaemon = 1;
        }
        else if(!strcmp(argv[i], "--config") || !strcmp(argv[i], "-c")) {
            if(++i >= argc) {
                fprintf(stderr, "%s requires a file path\n", argv[i - 1]);
                return 1;
            }
            configPath = argv[i];
            configRequired = true;
        }
        else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            return 1;
        }
    }
    configPath = std::filesystem::absolute(configPath).string();

    if(isdaemon) {
        pid_t pid;

        /* Fork off the parent process */
        pid = fork();
        if(pid < 0) {
            LogError(-1, "Can not start SMS daemon");
            fprintf(stderr, "Can not start daemon");
            return 1;
        }
        /* If we got a good PID, then
        we can exit the parent process. */
        if(pid > 0) {
            FILE* f = fopen(DATA_PID_PATH, "w");
            if(f) {
                fprintf(f, "%d", pid);
                fclose(f);
            }
            return 0;
        }
        else { // daemon mode
            chdir("/");
            //			close(STDIN_FILENO);
            //			close(STDOUT_FILENO);
            //			close(STDERR_FILENO);
            Sleep(START_DELAY);
        }
    }
    try {
        chdv::sms_daemon::CSmsDaemon daemon;
        daemon.Setup(configPath, configRequired);
        CreateDir(daemon.JobDir());
        if(daemon.SmsXmlEnabled())
            CreateDir(daemon.SmsInDir());
        else
            std::cout << "Incoming SMS XML output disabled" << std::endl;
        if(daemon.UssdXmlEnabled())
            CreateDir(daemon.UssdInDir());
        else
            std::cout << "USSD XML output disabled" << std::endl;
        daemon.RegisterInSmsCallBack ([](const chdv::sms_daemon::ReceivedSMS& sms){
            std::cout << "SMS From:" << toUTF8(sms.m_From) << " Parts:" << sms.m_nParts << " Text:" << toUTF8(sms.m_sText) << std::endl;
            return 0;
        });
        if(daemon.SmsXmlEnabled()) {
            daemon.RegisterInSmsCallBack ([&daemon](const chdv::sms_daemon::ReceivedSMS& sms){
                auto xml = GenXML(sms, daemon.Debug(), daemon.Debug());
                const auto path = MakeUniqueOutputXmlPath(daemon.SmsInDir(), sms.m_RecvTime, "SMS", toUTF8(sms.m_From));
                xml.writeToFile(path.c_str(), nullptr, true);
                return 0;
            });
        }
        daemon.RegisterInSmsCallBack([&daemon](const chdv::sms_daemon::ReceivedSMS& sms) {
            return chdv::sms_daemon::RunSmsHooks(daemon.SmsHooks(), sms);
        });

        if(daemon.UssdXmlEnabled()) {
            daemon.RegisterInUssdCallBack([&daemon](const chdv::sms_daemon::ReceivedUssd& ussd) {
                auto xml = GenXML(ussd, daemon.Debug());
                const auto path = MakeUniqueOutputXmlPath(daemon.UssdInDir(), ussd.receiveTime, "USSD", "");
                xml.writeToFile(path.c_str(), nullptr, true);
                return 0;
            });
        }

        // daemon.RegisterInSmsCallBack([](const std::string number, const
        // std::string text, std::string& replay, void* userdata) { replay = "OK
        //" + text; return 0; });
        daemon.Go();
    }
    catch(chdv::sms_daemon::CSmsDaemon::SmsDaemonError& e) {
        std::cerr << "Critical error. Code: " << e.ErrorCode << " \"" << e.Verbose << "\"" << std::endl;
        return e.ErrorCode > 0 ? e.ErrorCode : 1;
    }

    return 0;
}
