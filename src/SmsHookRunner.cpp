#include "SmsHookRunner.h"

#include <cerrno>
#include <cstring>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

#include "sms-daemon.h"
#include "ut.h"

namespace chdv::sms_daemon {
namespace {

int RunSmsHook(const std::string& hook, const ReceivedSMS& sms) {
    const std::string text = toUTF8(sms.m_sText);
    const std::string from = toUTF8(sms.m_From);
    const std::string sentTime(sms.m_TimeStamp);
    const std::string recvTime = toLocalTime(sms.m_RecvTime);
    const std::string smsc = toUTF8(sms.m_SMSC);

    pid_t pid = fork();
    if(pid < 0) {
        std::ostringstream message;
        message << "Can not fork SMS hook " << hook << ": " << std::strerror(errno);
        LogError(errno, message.str().c_str());
        return -1;
    }

    if(pid == 0) {
        std::vector<char*> argv;
        argv.reserve(9);
        argv.push_back(const_cast<char*>(hook.c_str()));
        argv.push_back(const_cast<char*>(text.c_str()));
        argv.push_back(const_cast<char*>(from.c_str()));
        argv.push_back(const_cast<char*>(sentTime.c_str()));
        argv.push_back(const_cast<char*>(recvTime.c_str()));
        argv.push_back(const_cast<char*>(smsc.c_str()));
        argv.push_back(const_cast<char*>(sms.m_IMSI.c_str()));
        argv.push_back(const_cast<char*>(sms.m_IMEI.c_str()));
        argv.push_back(nullptr);
        execvp(hook.c_str(), argv.data());
        _exit(127);
    }

    int status = 0;
    while(waitpid(pid, &status, 0) < 0) {
        if(errno == EINTR)
            continue;
        std::ostringstream message;
        message << "Can not wait SMS hook " << hook << ": " << std::strerror(errno);
        LogError(errno, message.str().c_str());
        return -1;
    }

    if(WIFEXITED(status)) {
        const int exitCode = WEXITSTATUS(status);
        if(exitCode != 0) {
            std::ostringstream message;
            message << "SMS hook " << hook << " exited with code " << exitCode;
            LogError(exitCode, message.str().c_str());
        }
        return exitCode;
    }
    if(WIFSIGNALED(status)) {
        const int signal = WTERMSIG(status);
        std::ostringstream message;
        message << "SMS hook " << hook << " was terminated by signal " << signal;
        LogError(signal, message.str().c_str());
        return 128 + signal;
    }

    return 0;
}

} // namespace

int RunSmsHooks(const std::vector<std::string>& hooks, const ReceivedSMS& sms) {
    int result = 0;
    for(const auto& hook : hooks) {
        const int hookResult = RunSmsHook(hook, sms);
        if(hookResult != 0 && result == 0)
            result = hookResult;
    }
    return result;
}

} // namespace chdv::sms_daemon
