#pragma once
#include "xmlParser.h"
class ReceivedSMS;
class CRecvSMSPart;
XMLNode GenXML(const CRecvSMSPart&, bool debugFlag = false);
XMLNode GenXML(const ReceivedSMS&, bool includeParts = false, bool debugFlag = false);