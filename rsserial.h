#ifndef __SERIALRS_H__
#define __SERIALRS_H__
#include <vector>

#include "serial.h"

enum CFlowControl{
	flowControlNone=0,
	flowControlRTSCTS,
	flowControlDSRDTR,
	flowControlHALFDUPLEX,
	flowControlXonXoff,
	flowControlDown
};

enum CParity{
	parityNone = 0,
	parityOdd,
	parityEven,
	parityMark,
	paritySpace 
};

class CRSSerial: public CSerial{
private:
protected:
	virtual int	AdjustRSParams();

	CParity		Parity;
	CFlowControl	FlowControl;
	unsigned long	StopBits;
	unsigned long	DataBits;
	unsigned long	BaudRate;
	unsigned long	ModemReadyMask;

#ifdef _WIN32
	HANDLE		ComHandler;
	COMMTIMEOUTS	TimeOuts;
#else
	int		ComHandler;
#endif
	CRSSerial& operator = (const CRSSerial&) = delete;
	CRSSerial(const CRSSerial&) = delete;
public:
	int SetComParams (unsigned long baudRate=115200, unsigned long dataBits=8, int parity=0, unsigned long stopBits=1);
	int CheckModemReadyMask();
	virtual int GetControlLinesStatus();
	virtual int IoCtl(unsigned long code, unsigned long lParam=0, void* bParam=NULL);
	virtual CSerialType GetConnectorType();
	static bool IsValidAddress(const char* address);
	virtual int SendChar(int c);
	virtual int PutBlock(const char* buf, size_t size, unsigned long timeout=SERIAL_DEFTIMEOUTREAD);
	virtual int ReceiveChar();
	virtual int Close();
	virtual int Open (const char* name, int timeout=SERIAL_DEFTIMEOUTCALL);
	virtual ~CRSSerial();
	CRSSerial();
};

#endif