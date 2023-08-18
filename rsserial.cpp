// Port from Windows
// Not Completed
#ifdef _WIN32
	#include "stdafx.h"
#else
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <termios.h>
       	#include <unistd.h>
	#include <iostream>
#endif
#include <memory.h>
#include "rsserial.h"

using namespace std;

CRSSerial::CRSSerial()
: Parity (parityNone)
, FlowControl (flowControlNone)
, StopBits (1)
, DataBits (8)
, BaudRate (115200UL)
, ModemReadyMask (0)
, EchoIndex (0)
, IsEchoCancellation(false)
#ifdef _WIN32
, ComHandler (NULL)
#else
, ComHandler (-1)
#endif	
{
#ifdef _WIN32
	TimeOuts.ReadIntervalTimeout =0;
	TimeOuts.ReadTotalTimeoutMultiplier =0;
	TimeOuts.ReadTotalTimeoutConstant = 1;
	TimeOuts.WriteTotalTimeoutMultiplier=0;
	TimeOuts.WriteTotalTimeoutConstant=1;
#endif
	SetDefTimeout(10000, SERIAL_OP_READ);
	SetDefTimeout(10000, SERIAL_OP_WRITE);
	SetDefTimeout(15000, SERIAL_OP_SENDEXPECT);

}

CRSSerial::~CRSSerial(){
	Close();
}


#ifdef _WIN32
int CRSSerial::Open (const char* name, int /*timeout*/){
	Close();
	if (name){
		CString comName;
		if (!strnicmp(name, "com", 3))
			comName = "\\\\.\\";
		comName += name;
		ComHandler = CreateFile(comName, GENERIC_READ|GENERIC_WRITE, 0, NULL,/*CREATE_ALWAYS*/OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (ComHandler  == INVALID_HANDLE_VALUE){
			ComHandler= NULL;
			LastError = SERIAL_ERR_NOTOPENED;
		}
		else if(!SetCommTimeouts(ComHandler, &TimeOuts)){
			Close();
			LastError = SERIAL_ERR_NOTOPENED;
		}
		else if (AdjustRSParams()){
			Close();
			LastError = SERIAL_ERR_NOTOPENED;
		}   
		else{
			LastError = CSerial::Open(name);
		}
	}
	return LastError;
}

int CRSSerial::Close(){
	if (ComHandler){
		CFlowControl fc = FlowControl;
		FlowControl = flowControlDown;
		AdjustRSParams();
		FlowControl = fc;
		CloseHandle(ComHandler);
		ComHandler = NULL;
	}
	SentEchoSample.clear();
	EchoIndex = 0;
	return CSerial::Close();
}

int CRSSerial::AdjustRSParams(){
	if (!ComHandler)
		return LastError = SERIAL_ERR_NOTOPENED;
	DCB dcb;
	memset (&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof (dcb);
	int err= GetCommState (ComHandler,&dcb);
	if (!err)
		return LastError = SERIAL_ERR_SETPARAMS;
	dcb.fBinary=1;
	dcb.BaudRate = BaudRate;
	dcb.fTXContinueOnXoff = 1;
	dcb.fDsrSensitivity = 0;
	dcb.fErrorChar = 0;
	dcb.fNull =0;
	dcb.fAbortOnError=0;
	dcb.ByteSize=(BYTE)DataBits;
	dcb.Parity=Parity;
	
	switch (StopBits){
	case 1:
		dcb.StopBits = 0;
		break;
	case 2:
		dcb.StopBits = 2;
		break;
	default:
		StopBits =    1;
		dcb.StopBits = 0;
	}
	
	switch (FlowControl){
	case  flowControlDown:
		dcb.fOutX = 0;
		dcb.fInX = 0;
		dcb.fOutxCtsFlow = 0;
		dcb.fOutxDsrFlow = 0;
		dcb.fRtsControl =RTS_CONTROL_DISABLE;
		dcb.fDtrControl =DTR_CONTROL_DISABLE;
		break;
	case flowControlNone:
		dcb.fOutX = 0;
		dcb.fInX = 0;
		dcb.fOutxCtsFlow = 0;
		dcb.fOutxDsrFlow = 0;
		dcb.fRtsControl =RTS_CONTROL_ENABLE;
		dcb.fDtrControl =DTR_CONTROL_ENABLE;
		break;
		
	case flowControlXonXoff:
		dcb.fOutX = 1;
		dcb.fInX = 1;
		dcb.fOutxCtsFlow = 0;
		dcb.fOutxDsrFlow = 0;
		dcb.fRtsControl =RTS_CONTROL_ENABLE;
		dcb.fDtrControl =DTR_CONTROL_ENABLE;
		break;
		
	case flowControlRTSCTS:
		dcb.fOutX = 0;
		dcb.fInX = 0;
		dcb.fOutxCtsFlow = 1;
		dcb.fOutxDsrFlow = 0;
		dcb.fRtsControl =RTS_CONTROL_HANDSHAKE;
		dcb.fDtrControl =DTR_CONTROL_ENABLE;
		break;
		
	case flowControlHALFDUPLEX:
		dcb.fOutX = 0;
		dcb.fInX = 0;
		dcb.fOutxCtsFlow = 0;
		dcb.fOutxDsrFlow = 0;
		dcb.fRtsControl =RTS_CONTROL_TOGGLE;
		dcb.fDtrControl =DTR_CONTROL_DISABLE;
		break;
		
		
	case flowControlDSRDTR:
		dcb.fOutX = 0;
		dcb.fInX = 0;
		dcb.fOutxCtsFlow = 0;
		dcb.fOutxDsrFlow = 1;
		dcb.fRtsControl =RTS_CONTROL_ENABLE;
		dcb.fDtrControl =DTR_CONTROL_HANDSHAKE;
		break;
		
	default:
		return LastError = SERIAL_ERR_SETPARAMS;
	}
	err= SetCommState(ComHandler,&dcb);
	return LastError = err ? SERIAL_ERR_OK : SERIAL_ERR_SETPARAMS;
}

int CRSSerial::ReceiveChar(){
	if (CheckModemReadyMask())
		return LastError;

	int chr = GetFromUnget ();
	if (chr >=0)
		return chr;

	unsigned char x ='\0';
	unsigned long readedBytes=0L;
	BOOL r = ReadFile (ComHandler, &x, 1L, &readedBytes, NULL);
	if( !r || (readedBytes<=0L)){ // No character received
		int err = ::GetLastError ();
		if (r)
			return LastError = SERIAL_ERR_REMPTY;
		else
			return LastError = SERIAL_ERR_RERROR;
	}
//	TRACE ("mask:%08X\r", GetControlLinesStatus());

	if  (IsEchoCancellation && SentEchoSample.size() > EchoIndex ){ // If echo cancellation and sample is valid
		if (SentEchoSample[EchoIndex] == x){  // Echo treceived
			if (++EchoIndex == SentEchoSample.size()){ // Is last echo character ? -> Reset echo buffer
				SentEchoSample.clear();
				EchoIndex = 0;
			}
			return LastError = SERIAL_ERR_REMPTY; // Receive nothing
		}
		else{ // Responce missmatches echo
// Recover all received characters 
			Ungetc(x);
			for (int i = EchoIndex-1; i >=0; i--)
				Ungetc(SentEchoSample[i]);
			x = GetFromUnget();
// Reset echo sample
			SentEchoSample.clear();
			EchoIndex = 0;
		}
	}
	return x;
}

int CRSSerial::SendChar(int c){

	if (CheckModemReadyMask())
		return LastError;
	// TRACE ("%c", c);
	unsigned long writenBytes=0L;
	BOOL r = WriteFile (ComHandler, &c, 1L, &writenBytes, NULL);
	if( !r || (writenBytes<=0L)){
		int err = ::GetLastError ();
		return LastError = err ? SERIAL_ERR_WERROR:SERIAL_ERR_SBFULL;
	}

	if  (IsEchoCancellation) // If echo cancellation add sample
		SentEchoSample.push_back(c);

	return LastError = SERIAL_ERR_OK;
}

int CRSSerial::PutBlock(const char* buf, unsigned long size, unsigned long timeout){
	if (CheckModemReadyMask())
		return LastError;
	int errorCode = SERIAL_ERR_OK;
	unsigned long BytesSent=0;
	if (buf){
		SetTimeout (TRUE, timeout);
		while ((size-BytesSent) && !errorCode){
			unsigned long writenBytes=0L;
			const unsigned char* writePtr = (const unsigned char*)buf+BytesSent;
			BOOL r = WriteFile (ComHandler, writePtr, size-BytesSent, &writenBytes, NULL);
			BytesSent += writenBytes;
			
			if( !r || (writenBytes==0L)){ // No bytes sent or error
				if( ::GetLastError ())
					return LastError = SERIAL_ERR_WERROR;
				if (CheckTimeout(TRUE))
					return LastError = SERIAL_ERR_TIMEOUT;
			}
			else { // at least part of buffer has been sent
				if  (IsEchoCancellation){ // If echo cancellation add sample
					for (;writenBytes;--writenBytes){
						SentEchoSample.push_back(*(writePtr++)); // save sample for echo cancellation
					}
				}
			}
		}
	}

	return LastError = SERIAL_ERR_OK;

}

bool CRSSerial::IsValidAddress(const char* address){
	if (!address)
		return FALSE;
	char buf[4];
	strncpy (buf, address, 3);
	buf[3] = '\0';
	for (int i = 0; i < 3; ++i)
		buf[i] = toupper (buf[i]);
	
	if       (!stricmp (buf,"COM")) 
		return TRUE;
	else if  (!stricmp (buf,"AUX")) 
		return TRUE;
	else
		return FALSE;
	
	
}

int CRSSerial::GetControlLinesStatus(){
	if (!ComHandler)
		return LastError=SERIAL_ERR_NOTOPENED;
	unsigned long mask = 0;
	if (GetCommModemStatus( ComHandler, &mask)){
		return mask&0x7fffffff; // should be positive
	}
	else
		return LastError=SERIAL_ERR_GENERAL;
}


#else

static int SetNonBlock (int fd) {
	int flags = fcntl(fd, F_GETFL, 0); 
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	return 0;
}

int CRSSerial::Open (const char* name, int /*timeout*/){
	Close();
	if (name){
		string comName;
		if (*name != '/') // short name
			comName = "/dev/";
		comName += name;
		ComHandler = open ( comName.c_str(), O_RDWR);
		if (ComHandler  < 0){
			LastError = SERIAL_ERR_NOTOPENED;
		}
		else if(SetNonBlock(ComHandler)){
			Close();
			LastError = SERIAL_ERR_NOTOPENED;
		}
		else if (AdjustRSParams()){
			Close();
			LastError = SERIAL_ERR_NOTOPENED;
		}   
		else{
			LastError = CSerial::Open(name);
		}
	}
	return LastError;
}

int CRSSerial::Close(){
	if (ComHandler>=0){
		CFlowControl fc = FlowControl;
		FlowControl = flowControlDown;
		AdjustRSParams();
		FlowControl = fc;
		close(ComHandler);
		ComHandler = -1;
	}
	SentEchoSample.clear();
	EchoIndex = 0;
	return CSerial::Close();
}

int CRSSerial::SendChar(int c){
//printf ("SendChar %c\n", c);

	if (ComHandler >=0 ){
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 5000;

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(ComHandler, &fds);

		select(ComHandler + 1, NULL, &fds, NULL, &tv);

		if (FD_ISSET(ComHandler, &fds)) {


			int err = write( ComHandler, &c, 1);
			if (err < 0) {
				return LastError = SERIAL_ERR_GENERAL;
			}
			if (err == 0)
				return LastError = SERIAL_ERR_SBFULL;
		}
		else
			return LastError = SERIAL_ERR_SBFULL;
	}
	else
		return LastError = SERIAL_ERR_NOTOPENED;
	return SERIAL_ERR_OK;
}

int CRSSerial::ReceiveChar(){
	unsigned char ch=0;

	if (ComHandler >=0 ){
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 5000;

		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(ComHandler, &fds);

		select(ComHandler + 1, &fds, NULL, NULL, &tv);

		if (FD_ISSET(ComHandler, &fds)) {
			int err = read( ComHandler, &ch, 1);
				if (err < 0)
					return LastError = SERIAL_ERR_GENERAL;
				if (err > 0)
					return ch;
		}

	}
	else{
		return LastError = SERIAL_ERR_NOTOPENED;
	}
	return LastError = SERIAL_ERR_REMPTY;
}

int CRSSerial::AdjustRSParams(){
 	struct termios options; /*структура для установки порта*/
//    	tcgetattr(ComHandler, &options); /*читает пораметры порта*/
	memset (&options, 0, sizeof (options));
#if 0
cout  
	<< "c_iflag   " << options.c_iflag  <<  "\n"
	<< "c_oflag   " << options.c_oflag  <<  "\n"
	<< "c_cflag   " << options.c_cflag  <<  "\n"
	<< "c_lflag   " << options.c_lflag  <<  "\n"
	<< "c_line    " << (int)options.c_line <<  "\n"
	<< "c_ispeed  " << options.c_ispeed <<  "\n"
	<< "c_ospeed  " << options.c_ospeed <<  "\n"
"\n" ;

	for (int i = 0; i < NCCS; ++i)
		cout << (int) options.c_cc[i] <<" ";
	cout << "\n";
#endif
	cfmakeraw (&options);
	options.c_cflag &= ~(CSIZE | CSTOPB | PARENB | CBAUD);
	options.c_cflag  |=  CREAD | HUPCL | CLOCAL ;

//	options.c_cflag = 2237;
	options.c_iflag = IGNBRK;
//	options.c_oflag =0;
//	options.c_lflag =0;

	switch (DataBits){
		case 5:  options.c_cflag |= CS5; break;
		case 6:  options.c_cflag |= CS6; break;
		case 7:  options.c_cflag |= CS7; break;
		case 8: 
		default: options.c_cflag |= CS8; break;			
	}
	if (StopBits==2){
		options.c_cflag |= CSTOPB;
	}
	int br = B9600;
	switch (BaudRate){
		case 300:    br = B300;   break;
	        case 600:    br = B600;   break;
	        case 1200:   br = B1200;  break;
	        case 2400:   br = B2400;  break;
	        case 4800:   br = B4800;  break;
	        case 19200:  br = B19200; break;
	        case 38400:  br = B38400; break;
	        case 57600:  br = B57600; break;
	        case 115200: br = B115200;break;
		case 230400: br = B230400;break;
	        case 9600:
		default:     br = B9600;
	}

	options.c_cflag |= br;

	cfsetospeed(&options, br);
	cfsetispeed(&options, br);

//  tcflag_t c;		/* input mode flags */
 //   tcflag_t c_oflag;		/* output mode flags */
 //   tcflag_t c_cflag;		/* control mode flags */
 //   tcflag_t c_lflag;		/* local mode flags */
 //   cc_t c_line;			/* line discipline */
 //   cc_t c_cc[NCCS];		/* control characters */
 //   speed_t c_ispeed;		/* input speed */
 //   speed_t c_ospeed;		/* output speed */
	tcflush(ComHandler, TCIOFLUSH); // Flush buffers		 	
 	tcsetattr(ComHandler, TCSANOW, &options); /*пишет пораметры порта*/

	return 0;
}

bool CRSSerial::IsValidAddress(const char* address){
	return true;
}

int CRSSerial::GetControlLinesStatus(){
	return 0;
}

#endif

int CRSSerial::SetComParams (unsigned long baudRate, unsigned long dataBits, int parity, unsigned long stopBits){
	Parity = (CParity)parity;
	StopBits = stopBits;
	DataBits = dataBits;
	BaudRate = baudRate;	
	if (Connected){
		return AdjustRSParams();
	}
	else
		return 0;
}


CSerialType CRSSerial::GetConnectorType(){
	return serialTypeRS;
}

int CRSSerial::IoCtl(unsigned long code, unsigned long lParam, void* bParam){
	int rtn = SERIAL_ERR_OK;
	switch (code){
	case SERIAL_IOCTL_SET_BAUDRATE:
		BaudRate = lParam;
		rtn = AdjustRSParams();
		break;
	case SERIAL_IOCTL_SET_DATABITS:
		DataBits = lParam;
		rtn = AdjustRSParams();
		break;
	case SERIAL_IOCTL_SET_STOPBITS:
		StopBits = lParam;
		rtn = AdjustRSParams();
		break;
	case SERIAL_IOCTL_SET_FLOWCNTRL:
		FlowControl = (CFlowControl)lParam;
		rtn = AdjustRSParams();
		break;
	case SERIAL_IOCTL_SET_PARITY:
		Parity = (CParity)lParam;
		rtn = AdjustRSParams();
		break;

	case SERIAL_IOCTL_GET_MODEM_MASK:
		rtn = GetControlLinesStatus();
		if (rtn > 0){
			if (bParam)
				*(unsigned long*)bParam = lParam ? rtn&lParam : rtn;
			rtn = 0;
		}
		break;

	case SERIAL_IOCTL_SET_MODEM_READY_MASK:
		ModemReadyMask = lParam;
		break;
	
	case SERIAL_IOCTL_SET_ECHOCANCELLATION:
		IsEchoCancellation = lParam;
		break;

	default:
		rtn = CSerial::IoCtl(code, lParam, bParam);
	}
	return LastError=rtn;
}

int CRSSerial::CheckModemReadyMask(){

	if (ModemReadyMask){ // Just for optimization
		int err = GetControlLinesStatus();
		if (err >= 0)
			LastError = ((ModemReadyMask == (ModemReadyMask&(unsigned long)err)) ? SERIAL_ERR_OK: SERIAL_ERR_NOTOPENED);
		//else
		//	Lasterror=err; // Already done
	}
	else
		LastError = (ComHandler?SERIAL_ERR_OK:SERIAL_ERR_NOTOPENED);

	return LastError;
}
void CRSSerial::ClearRX(){
	CSerial::ClearRX();
	SentEchoSample.clear();
	EchoIndex = 0;
}