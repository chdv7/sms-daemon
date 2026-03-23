#pragma once
#include "xmlParser.h"
namespace chdv::sms_daemon {
class ReceivedSMS;
class CRecvSMSPart;
XMLNode GenXML(const CRecvSMSPart&, bool debugFlag = false);
XMLNode GenXML(const ReceivedSMS&, bool includeParts = false, bool debugFlag = false);
} // namespace chdv::sms_daemon