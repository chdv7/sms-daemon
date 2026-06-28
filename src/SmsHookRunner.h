#pragma once

#include <string>
#include <vector>

#include "RecvSMS.h"

namespace chdv::sms_daemon {

int RunSmsHooks(const std::vector<std::string>& hooks, const ReceivedSMS& sms);

} // namespace chdv::sms_daemon
