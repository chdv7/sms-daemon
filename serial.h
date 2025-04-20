#ifndef _SERIAL_H_
#define _SERIAL_H_
#include <string>


#ifdef _WIN32
#pragma warning(disable : 4996)
#endif

// Seraial device
enum CSerialType{
	serialTypeNone=0,
	serialTypeRS,
	serialTypeIPClient
};


#define SERIAL_TIMEOUTLEFT          0x80000000UL
#define SERIAL_DEFTIMEOUTREAD       0x80000001UL
#define SERIAL_DEFTIMEOUTWRITE      0x80000002UL
#define SERIAL_DEFTIMEOUTSENDEXPECT 0x80000003UL
#define SERIAL_DEFTIMEOUTCALL	    0x80000004UL
#define SERIAL_DEFTIMEOUTAWAITECALL 0x80000005UL


#define SERIAL_IOCTL_SET_DEF_RD_TIMEOUT           1UL
#define SERIAL_IOCTL_SET_DEF_WR_TIMEOUT           2UL
#define SERIAL_IOCTL_SET_DEF_SE_TIMEOUT           3UL
#define SERIAL_IOCTL_SET_DEF_CL_TIMEOUT           4UL
#define SERIAL_IOCTL_SET_DEF_AC_TIMEOUT           5UL
#define SERIAL_IOCTL_SET_BAUDRATE                20UL
#define SERIAL_IOCTL_SET_DATABITS                21UL
#define SERIAL_IOCTL_SET_STOPBITS                22UL
#define SERIAL_IOCTL_SET_FLOWCNTRL               23UL
#define SERIAL_IOCTL_SET_INITSTRING              24UL
#define SERIAL_IOCTL_SET_ANSWERSTRING            25UL
#define SERIAL_IOCTL_SET_LISTENPORT              26UL
#define SERIAL_IOCTL_SET_ANSWERCONNECTORNAME     27UL
#define SERIAL_IOCTL_GET_SOCKET_ADDRESS          28UL
#define SERIAL_IOCTL_SET_ORIGINATOR_PORT         29UL
#define SERIAL_IOCTL_SET_PARITY                  30UL
#define SERIAL_IOCTL_GET_MODEM_MASK              31UL
#define SERIAL_IOCTL_SET_MODEM_READY_MASK        32UL
#define SERIAL_IOCTL_SET_ORIGINATOR_IP           33UL
#define SERIAL_IOCTL_SET_LISTEN_IP               34UL
#define SERIAL_IOCTL_SET_DEFAULT_PORT			 36UL



#define SERIAL_OP_READ              1
#define SERIAL_OP_WRITE             2
#define SERIAL_OP_SENDEXPECT        3
#define SERIAL_OP_CALL              4
#define SERIAL_OP_AWAITECALL        5


#define SERIAL_ERR_OK               0
#define SERIAL_ERR_GENERAL      -1000
#define SERIAL_ERR_REMPTY       -1001
#define SERIAL_ERR_SBFULL       -1002
#define SERIAL_ERR_TIMEOUT      -1003
#define SERIAL_ERR_NOTOPENED    -1004
#define SERIAL_ERR_WRONGADDRESS -1005
#define SERIAL_ERR_WRONGDEVNAME -1005
#define SERIAL_ERR_RERROR       -1006
#define SERIAL_ERR_WERROR       -1007
#define SERIAL_ERR_LOCKEDOUT    -1008
#define SERIAL_ERR_SETPARAMS    -1009
#define SERIAL_ERR_WRONGPARAMS  -1010
#define SERIAL_ERR_CANNOTINIT   -1011
#define SERIAL_ERR_UNGET_EMPTY  -1012
#define SERIAL_ERR_UNGET_OVER   -1013
#define SERIAL_ERR_ENCRYPTION   -1014
#define SERIAL_ERR_NOTSUPPORTED -1100

#define UNGET_BUF_SIZE          64*1024
//#define UNGET_BUF_SIZE			 200

class CSerialHandler{
public:
	CSerialHandler* GetParentHandler();
	void SetParentHandler (CSerialHandler* serialHandler);
	virtual ~CSerialHandler();
	CSerialHandler();
	virtual void OnSendChar(CSerialHandler* handler);
	virtual void OnReceiveChar(CSerialHandler* handler);
	virtual void OnDisconnect(CSerialHandler* handler);
	virtual void OnConnect(CSerialHandler* handler);
	virtual void OnIncomingCall(CSerialHandler* handler);
protected:
	CSerialHandler* ParentHandler;
};


struct CBinRecord {
	unsigned char* Buf;
	unsigned long Len;
	CBinRecord(void* buf=NULL, unsigned long length=0UL);
};

typedef CBinRecord* LPBINRECORD;

class CSerial: public CSerialHandler{
protected:
	unsigned char UngetBuf[UNGET_BUF_SIZE];
	unsigned long UngetBufPtr;
	unsigned long DefAwaiteCallTimeout;
	std::string  DeviceName;              // name of device
	unsigned long DefWriteTimeout;        // default timeout for write operations
	unsigned long DefReadTimeout;         // default timeout for read operations
	unsigned long DefSendExpectTimeout;   // default timeout for send/expect operations
	unsigned long DefCallTimeout;
	bool         TSliceEnabled;          // flag if timeslice is enabled (has no effect in console mode)
protected:
	bool         Connected;              // flag if connection is established
	bool         IsTimeoutInMS;          // flag if timeout in miliseconds(true). Timeout in seconds if it is false
	unsigned long m_SendTimeout;
	unsigned long m_RecvTimeout;
	int          LastError;              // Last error
	             CSerial();
	virtual void Idle();                 // function calls until class is awaiting end operation
	                                     // during timeout
	virtual bool CheckTimeout(bool isSend);     // If timeout
	void         SetDeviceName (const char* name); // Set (change device name)
	virtual void SetTimeout(bool isSend, unsigned long timeout);
	int          GetFromUnget();
public:
	virtual      CSerial* AwaiteCall (unsigned long timeout=SERIAL_DEFTIMEOUTAWAITECALL);
	virtual int  IoCtl(unsigned long code, unsigned long lParam=0, void* bparam=NULL );
	bool		 IsConnected ();

	virtual void ClearRX();
	int          SendExpect (const char* send, const char** samples, unsigned long timeout=SERIAL_DEFTIMEOUTSENDEXPECT, char* log=NULL, long logSize=0);
	int          SendExpect (LPBINRECORD send, LPBINRECORD* samples, unsigned long timeout=SERIAL_DEFTIMEOUTSENDEXPECT, LPBINRECORD log=NULL);
	virtual void OnConnect(CSerialHandler* handler);		// if connection is established
	virtual void OnDisconnect(CSerialHandler* handler);	// if connection is lost
	bool         IsTimeSliceEnabled();
	virtual unsigned long GetDefTimeout(int op);
	int          GetLastError();
	virtual int  EnableTimeSlice(bool enable=true);	// enable meassges pump during awaiting of operations end
	virtual int  GetBlock   (char* buf, size_t size, unsigned long timeout=SERIAL_DEFTIMEOUTREAD);
	virtual int  GetBlockEx (char* buf, unsigned long size, unsigned long timeout=SERIAL_DEFTIMEOUTREAD);
	virtual int  Gets (char* buf, unsigned long size, int separator='\n', unsigned long timeout=SERIAL_DEFTIMEOUTREAD);
	virtual int  Gets (char* buf, unsigned long size, const char* separators, unsigned long timeout=SERIAL_DEFTIMEOUTREAD);
	virtual int  ReceiveChar();
	virtual int  PutBlock  (const char* buf, size_t size, unsigned long timeout= SERIAL_DEFTIMEOUTWRITE);
	virtual int  PutBlockEx(const char* buf, unsigned long size, unsigned long timeout= SERIAL_DEFTIMEOUTWRITE);
	virtual int  Puts(const char* s, unsigned long timeout= SERIAL_DEFTIMEOUTWRITE);
	virtual int  PutChar(int chr, unsigned long timeout = SERIAL_DEFTIMEOUTWRITE);
	virtual void SetDefTimeout (unsigned long timeout, int operation);
	time_t GetSysTimeS();
	unsigned long GetSysTimeMS();
	time_t GetTOTime();
	virtual int  SendChar(int c);
	virtual int  GetChar(unsigned long timeout= SERIAL_DEFTIMEOUTREAD);
	virtual int  Close();
	virtual int  Open(const char* name, int timeout=SERIAL_DEFTIMEOUTCALL);
	const char*  GetDeviceName();
	virtual      CSerialType GetConnectorType();
	int          Ungetc (int chr);
	int          Ungets (char* str);
	int          UngetBlock (char* str, unsigned long size);
	void         ClearUngetBuf();
	int          GetUngetBufSize () {return UngetBufPtr;}
	virtual     ~CSerial();
};

#endif