#include "gen-xml.hpp"
#include "RecvSMS.h"
#include "ut.h"
namespace chdv::sms_daemon {
XMLNode GenXML(const CRecvSMSPart& part, bool debugFlag) {
    XMLNode xPart = XMLNode::createXMLTopNode("Part");

    char tmp[100];

    xPart.addAttribute("From", toUTF8(part.m_From).c_str());

    sprintf(tmp, "%02X", part.m_nRefNr);
    xPart.addAttribute("ref", tmp);

    sprintf(tmp, "%d", part.m_nPartNo);
    xPart.addAttribute("partno", tmp);

    xPart.addAttribute("time", part.m_TimeStamp.c_str());
    xPart.addAttribute("smsc", toUTF8(part.m_SMSC).c_str());

    xPart.addAttribute("ReceiveTime", toLocalTime(part.m_RecvTime).c_str());

    sprintf(tmp, "%d", part.m_nParts);
    xPart.addAttribute("nparts", tmp);

    if(debugFlag)
        xPart.addAttribute("raw", part.m_RawText.c_str());

    xPart.addText(toUTF8(part.m_sText).c_str());
    return xPart;
}

XMLNode GenXML(const ReceivedSMS& sms, bool includeParts, bool debugFlag) {
    XMLNode xSms = XMLNode::createXMLTopNode("Sms");
    char tmp[100];

    xSms.addAttribute("From", toUTF8(sms.m_From).c_str());

    xSms.addAttribute("time", sms.m_TimeStamp.data());
    xSms.addAttribute("smsc", toUTF8(sms.m_SMSC).c_str());

    xSms.addAttribute("ReceiveTime", toLocalTime(sms.m_RecvTime).c_str());

    xSms.addAttribute("status", sms.isOk ? "Received" : "Bad");
    xSms.addAttribute("Interface", sms.m_Interface.c_str());
    sprintf(tmp, "%d", sms.m_nParts);
    xSms.addAttribute("nParts", tmp);

    xSms.addText(toUTF8(sms.m_sText).c_str());
    if(includeParts)
        for(const auto& part : sms.parts) {
            if(!part)
                continue;
            auto xPart = GenXML(*part, debugFlag);
            xSms.addChild(xPart);
        }
    return xSms;
}
} // namespace chdv::sms_daemon