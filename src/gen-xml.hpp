#pragma once
#include "xmlParser.h"
namespace chdv::sms_daemon {
class ReceivedSMS;
class CRecvSMSPart;
struct ReceivedUssd;
XMLNode GenXML(const CRecvSMSPart&, bool debugFlag = false);
XMLNode GenXML(const ReceivedSMS&, bool includeParts = false, bool debugFlag = false);
XMLNode GenXML(const ReceivedUssd&, bool debugFlag = false);
} // namespace chdv::sms_daemon