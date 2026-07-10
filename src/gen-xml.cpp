#include "gen-xml.hpp"
#include "RecvSMS.h"
#include "Ussd.h"
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
    if(!sms.m_IMSI.empty())
        xSms.addAttribute("IMSI", sms.m_IMSI.c_str());
    if(!sms.m_IMEI.empty())
        xSms.addAttribute("IMEI", sms.m_IMEI.c_str());
    sprintf(tmp, "%zu", sms.m_nParts);
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

XMLNode GenXML(const ReceivedUssd& ussd, bool debugFlag) {
    XMLNode xUssd = XMLNode::createXMLTopNode("Ussd");
    char tmp[32];

    sprintf(tmp, "%d", ussd.mode);
    xUssd.addAttribute("mode", tmp);
    sprintf(tmp, "%d", ussd.dcs);
    xUssd.addAttribute("dcs", tmp);
    if(!ussd.request.empty())
        xUssd.addAttribute("Request", ussd.request.c_str());
    if(ussd.sendTime)
        xUssd.addAttribute("SendTime", toLocalTime(ussd.sendTime).c_str());
    xUssd.addAttribute("ReceiveTime", toLocalTime(ussd.receiveTime).c_str());
    xUssd.addAttribute("Interface", ussd.interface.c_str());
    if(!ussd.imsi.empty())
        xUssd.addAttribute("IMSI", ussd.imsi.c_str());
    if(!ussd.imei.empty())
        xUssd.addAttribute("IMEI", ussd.imei.c_str());
    if(debugFlag)
        xUssd.addAttribute("raw", ussd.raw.c_str());
    xUssd.addText(ussd.text.c_str());
    return xUssd;
}
} // namespace chdv::sms_daemon
