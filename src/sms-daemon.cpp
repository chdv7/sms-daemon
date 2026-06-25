#include "sms-daemon.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <filesystem>

#include <iostream>
#include "gen-xml.hpp"
#include "sms-daemon-mdm.h"
#include "ut.h"

void Log(const char* text) {
    FILE* f = fopen(SMS_LOG_FILE, "a");
    if(f) {
        fprintf(f, "SMS daemon: %s\n", text);
        fclose(f);
    }
}

void LogError(int code, const char* text) {
    FILE* f = fopen(SMS_LOG_FILE, "a");
    if(f) {
        fprintf(f, "%d SMS daemon: %s\n", code, text);
        fclose(f);
    }
}

int main(int argc, char* argv[]) {
    mkdir(SMS_TMP_DIR, 0777);
    chmod(SMS_TMP_DIR, 0777);
    mkdir(OUT_SMS_DIR, 0777);
    chmod(OUT_SMS_DIR, 0777);
    mkdir(IN_SMS_XML_DIR, 0777);
    chmod(IN_SMS_XML_DIR, 0777);

    int isdaemon = 0;
    std::string configPath = SMS_CONFIG_PATH;
    for(int i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "--daemon")) {
            isdaemon = 1;
            printf("sms-daemon\n");
        }
        else if(!strcmp(argv[i], "--config")) {
            if(++i >= argc) {
                fprintf(stderr, "--config requires a file path\n");
                return 1;
            }
            configPath = argv[i];
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
        daemon.Setup(configPath);
        daemon.RegisterInSmsCallBack ([](const chdv::sms_daemon::ReceivedSMS& sms){
            std::cout << "SMS From:" << toUTF8(sms.m_From) << " Parts:" << sms.m_nParts << " Text:" << toUTF8(sms.m_sText) << std::endl;
            return 0;
        });
        daemon.RegisterInSmsCallBack ([](const chdv::sms_daemon::ReceivedSMS& sms){
            auto xml = GenXML(sms, true, true);
            char buf[100];
            sprintf(buf, IN_SMS_XML_DIR "/SMS-%016llX.xml", static_cast<unsigned long long>(std::chrono::system_clock::now().time_since_epoch().count()));
            xml.writeToFile(buf, nullptr, true);
            return 0;
        });

        daemon.RegisterInUssdCallBack([](const chdv::sms_daemon::ReceivedUssd& ussd) {
            auto xml = GenXML(ussd, true);
            char buf[160];
            sprintf(buf, IN_SMS_XML_DIR "/USSD-%016llX.xml", static_cast<unsigned long long>(std::chrono::system_clock::now().time_since_epoch().count()));
            xml.writeToFile(buf, nullptr, true);
            return 0;
        });

        // daemon.RegisterInSmsCallBack([](const std::string number, const
        // std::string text, std::string& replay, void* userdata) { replay = "OK
        //" + text; return 0; });
        daemon.Go();
    }
    catch(chdv::sms_daemon::CSmsDaemon::SmsDaemonError& e) {
        std::cerr << "Critical error. Code: " << e.ErrorCode << " \"" << e.Verbose << "\"" << std::endl;
    }

    return 0;
}
