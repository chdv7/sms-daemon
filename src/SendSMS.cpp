#include <stdio.h>
#include <sys/timeb.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <filesystem>

#include "Config.h"
#include "PduSMS.h"

using namespace chdv::sms_daemon;
void Usage() {
    printf(
        "\
Usage: sms-send [-c <config>] <phoneNo|ussd> [<out folder>]\n\
");
}

std::string GenUniqueID() {
    static unsigned counter = 0;
    char buf[100] = "--";
    struct timeb tt;
    ftime(&tt);
    sprintf(buf, "%08X-%08lX-%08X", getpid(), tt.time * 1000 + tt.millitm, counter++);
    return buf;
}

int SaveSMS(const char* pdu, const char* dir) {
    std::string uniqueID = "Local-" + GenUniqueID();
    std::string tmp_name = std::string(dir) + "/.tmp_" + uniqueID;
    // printf ("%s\n", tmp_name.c_str());
    FILE* f = fopen(tmp_name.c_str(), "w");
    if(!f)
        return -1;
    fputs(pdu, f);
    fclose(f);
    return rename(tmp_name.c_str(), (std::string(dir) + "/" + uniqueID).c_str()) == 0 ? 0 : -2;
    return 0;
}

int GenSMS(const char* phoneNo, const char* dir) {
    std::string sms_text;
    sms_text.reserve(256);
    for(;;) {
        int ch = getchar();
        if(ch == EOF)
            break;
        //		if (ch == '\n')
        //			break;
        sms_text += (char)ch;
    }

    COutPduSms sms_processor(phoneNo, sms_text.c_str());
    TOutPduBlock pdu_list = sms_processor.ParseText();

    for(auto& it : pdu_list) {
        int err = SaveSMS(it.c_str(), dir);
        if(err < 0)
            printf("Saving error %d\n", err);
    }
    return 0;
}

int GenUSSD(const char* ussd, const char* dir) {
 
    COutPduSms sms_processor("", ussd);
    TUssdRequest ussdRq = sms_processor.ParseUssd();
    std::stringstream out;
    out << "U\"" << ussdRq.payload << "\","<< ussdRq.dcs;
    int err = SaveSMS(out.str().c_str(), dir);
    if(err < 0)
         printf("Saving error %d\n", err);

    return err;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        Usage();
        return 1;
    }
    const char* phone_No = "";
    const char* out_dir = nullptr;
    std::string configPath = SMS_CONFIG_PATH;
    bool configRequired = true;

    for(int i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--config")) {
            if(++i >= argc) {
                fprintf(stderr, "%s requires a file path\n", argv[i - 1]);
                return 1;
            }
            configPath = argv[i];
            configRequired = true;
        }
        else if(!*phone_No) {
            phone_No = argv[i];
        }
        else if(!out_dir) {
            out_dir = argv[i];
        }
        else {
            Usage();
            return 1;
        }
    }

    if(!*phone_No) {
        Usage();
        return 1;
    }

    SmsDaemonConfig config;
    std::string error;
    if(!LoadSmsDaemonConfig(std::filesystem::absolute(configPath).string(), config, error, configRequired)) {
        fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    if(!out_dir)
        out_dir = config.jobDir.c_str();

    if (*phone_No == '*' || *phone_No == '#')
        return GenUSSD(phone_No, out_dir);
    else
        return GenSMS(phone_No, out_dir);
}
