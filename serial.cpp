#ifdef _WIN32
#include "stdafx.h"
#endif
#include <string.h>
#include <time.h>

#include "serial.h"
#include "ut.h"


CSerial::CSerial()
: TSliceEnabled(false)
	{
	ClearUngetBuf();
	LastError = SERIAL_ERR_OK;
	SetDefTimeout(10000, SERIAL_OP_READ);
	SetDefTimeout(10000, SERIAL_OP_WRITE);
	SetDefTimeout(15000, SERIAL_OP_SENDEXPECT);
	SetDefTimeout(100000,SERIAL_OP_AWAITECALL);
	SetDefTimeout(20000, SERIAL_OP_CALL);
	IsTimeoutInMS = true;
	Connected = false;
	m_RecvTimeout = m_SendTimeout = 0;
}

CSerial::~CSerial(){

}

CSerialType CSerial::GetConnectorType(){
	return serialTypeNone;
}

void CSerial::SetDeviceName (const char* name){
	if (name)
		DeviceName=name;
	else
		DeviceName="";
}

const char* CSerial::GetDeviceName(){
	return DeviceName.c_str();
}

int CSerial::Open(const char* name, int timeout){
	ClearUngetBuf();
	SetDeviceName (name);
	OnConnect(this);
	return SERIAL_ERR_OK;
}

int CSerial::Close(){
	ClearUngetBuf();
	Connected = false;
	SetDeviceName(NULL);
	OnDisconnect(this);
	return SERIAL_ERR_OK;
}

int CSerial::SendChar(int c){
	return LastError=SERIAL_ERR_NOTSUPPORTED;
}

unsigned long CSerial::GetSysTimeMS(){
	return GetTickCount();
}

time_t CSerial::GetSysTimeS(){
	return time(NULL);
}

time_t CSerial::GetTOTime(){
	return IsTimeoutInMS ? GetSysTimeMS():GetSysTimeS();
}

void CSerial::SetTimeout(bool isSend, unsigned long timeout){
	switch (timeout){
	case SERIAL_DEFTIMEOUTREAD:
		timeout = DefReadTimeout;
		break;
	case SERIAL_DEFTIMEOUTWRITE:
		timeout = DefWriteTimeout;
		break;
	case SERIAL_DEFTIMEOUTSENDEXPECT:
		timeout = DefSendExpectTimeout;
		break;
	case SERIAL_DEFTIMEOUTAWAITECALL:
		timeout = DefAwaiteCallTimeout;
		break;

	case SERIAL_DEFTIMEOUTCALL:
		timeout = 10000;
		break;

	case SERIAL_TIMEOUTLEFT:
		return;
	}

	if ((timeout&0xFF000000L) == 0UL){ // timeout can not be too long
		if (isSend)
			m_SendTimeout = GetTOTime()+timeout;
		else
			m_RecvTimeout = GetTOTime()+timeout;
	}
}

void CSerial::Idle(){
#ifdef _WIN32
	#ifdef _CONSOLE
		::Sleep(5);
	#else
	
	  if (TSliceEnabled){
		  MSG msg;
		  while(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)){
	  		::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		  }
	  }
	  ::Sleep(1);
	
	#endif
#else
	::Sleep(1);
#endif

}

void CSerial::SetDefTimeout (unsigned long timeout, int operation){
	switch (operation){
	case SERIAL_OP_READ:
		DefReadTimeout = timeout;
		break;
	case SERIAL_OP_WRITE:
		DefWriteTimeout = timeout;
		break;
	case SERIAL_OP_SENDEXPECT:
		DefSendExpectTimeout = timeout;
		break;
	case SERIAL_OP_AWAITECALL:
		DefAwaiteCallTimeout = timeout;
		break;
	case SERIAL_OP_CALL:
		DefCallTimeout = timeout;
		break;
	}
}

bool CSerial::CheckTimeout(bool isSend){
	unsigned long curTime = GetTOTime();
//	TRACE ("CT/O %d %u %u\n", isSend, curTime isSend?m_SendTimeout:m_RecvTimeout );
	unsigned long dif =((curTime - (isSend?m_SendTimeout:m_RecvTimeout)));
	return (dif&0x80000000) == 0;
}

int CSerial::PutChar(int chr, unsigned long timeout){
	SetTimeout (true, timeout);
	int errorCode=0;
	while ((errorCode=SendChar (chr)) != 0){
		if (errorCode == SERIAL_ERR_SBFULL){
			if (CheckTimeout(true)){
				errorCode = SERIAL_ERR_TIMEOUT;
				break;
			}
			else
				Idle();
		}
		else
			break;
	}
	return LastError=errorCode;
}

int CSerial::Puts(const char* s, unsigned long timeout){
	int errorCode = SERIAL_ERR_OK;
	if (s){
		errorCode = PutBlock (s, strlen(s), timeout);
	}
	return LastError=errorCode;
}

int CSerial::PutBlock(const char* buf, size_t size, unsigned long timeout){
	int errorCode = SERIAL_ERR_OK;
	if (buf){
		SetTimeout (true, timeout);
		while ((size--) && !errorCode)
			errorCode = PutChar(*(buf++), SERIAL_TIMEOUTLEFT);
	}
	return LastError=errorCode;
}

int  CSerial::PutBlockEx(const char* buf, unsigned long size, unsigned long timeout){
	if (!buf){
		return LastError = ( size > 0  ?  SERIAL_ERR_GENERAL : SERIAL_ERR_OK) ;
	}
	const char* buf_=buf;
	int errCode = LastError = SERIAL_ERR_OK;
	SetTimeout (false, timeout);
	while (size>0){
		errCode = PutChar(*buf, SERIAL_TIMEOUTLEFT);
		if (errCode<0){
			LastError=errCode;
			break;
		}
		buf++;
		size--;
	}
	int sent_size = buf-buf_;
	return sent_size ? sent_size: LastError;

}


int CSerial::ReceiveChar(){
	return LastError=SERIAL_ERR_NOTSUPPORTED;
}

int CSerial::GetChar(unsigned long timeout){

	int chr = GetFromUnget ();
	if (chr >=0)
		return chr;
	SetTimeout (false, timeout);
	while ((chr=ReceiveChar ()) < 0){
		if (chr == SERIAL_ERR_REMPTY){
			if (CheckTimeout(false)){
				chr = SERIAL_ERR_TIMEOUT;
				break;
			}
			else
				Idle();
		}
		else
			break;
	}
	LastError = (chr >=0?0:chr);
	return chr;
}

int CSerial::Gets (char* buf, unsigned long size, int separator, unsigned long timeout){
	int chr = SERIAL_ERR_OK;
	if (buf && size){
		SetTimeout (false, timeout);
		while (--size>0){
			chr = GetChar(SERIAL_TIMEOUTLEFT);
			if (chr<0)
				break;
			if (chr == separator)
				break;
			*(buf++) = chr;
		}
	}
	*buf = '\0';
	return LastError=(chr>=0?SERIAL_ERR_OK:chr);
}

int CSerial::Gets (char* buf, unsigned long size, const char* separators, unsigned long timeout){
	int chr = 0;
	if (buf && size && separators){
		SetTimeout (false, timeout);
		while (--size>0){
			chr = GetChar(SERIAL_TIMEOUTLEFT);
			if (chr<0){
				break;
			}
			const char* sPtr = separators;
			for (; *sPtr; ++sPtr)
				if (chr == *sPtr)
					break;
			if (*sPtr)
				break;
			*(buf++) = chr;
		}
	}
	*buf = '\0';
	return LastError=(chr>=0?SERIAL_ERR_OK:chr);
}

int CSerial::GetBlock (char* buf, unsigned long size, unsigned long timeout){
	int chr = SERIAL_ERR_OK;
	if (buf && size){
		SetTimeout (false, timeout);
		while (size){
			chr = GetChar(SERIAL_TIMEOUTLEFT);
			if (chr<0){
				break;
			}
			*buf++ = chr;
			size--;
		}
	}
	return LastError=(chr>=0?SERIAL_ERR_OK:chr);
}

int  CSerial::GetBlockEx (char* buf, unsigned long size, unsigned long timeout){
	if (!buf){
		return LastError = ( size > 0  ?  SERIAL_ERR_GENERAL : SERIAL_ERR_OK) ;
	}
	char* buf_=buf;
	int chr = LastError = SERIAL_ERR_OK;
	SetTimeout (false, timeout);
	while (size>0){
		chr = GetChar(SERIAL_TIMEOUTLEFT);
		if (chr<0){
			LastError=chr;
			break;
		}
		*buf++ = chr;
		size--;
	}
	int recvd_size = buf-buf_;
	return recvd_size ? recvd_size: LastError;
}


int CSerial::EnableTimeSlice(bool enable){
	TSliceEnabled =enable;
	return LastError = SERIAL_ERR_OK;
}

int CSerial::GetLastError(){
	return LastError;
}

unsigned long CSerial::GetDefTimeout(int operation)
{
	switch (operation){
	case SERIAL_OP_READ:
		return DefReadTimeout;
	case SERIAL_OP_WRITE:
		return DefWriteTimeout;
	case SERIAL_OP_SENDEXPECT:
		return DefSendExpectTimeout;
	case SERIAL_OP_AWAITECALL:
		return DefAwaiteCallTimeout;
	default:
		return 0UL;
	}

}

bool CSerial::IsTimeSliceEnabled(){
	return TSliceEnabled;
}

void CSerial::OnConnect(CSerialHandler* handler){
	ClearUngetBuf();
	Connected = true;
	CSerialHandler::OnConnect(handler);
}

void CSerial::OnDisconnect(CSerialHandler* handler){
	ClearUngetBuf();
	Connected = false;
	CSerialHandler::OnDisconnect(handler);
}

int CSerial::SendExpect (const char* send, const char** expect, unsigned long timeout, char* log, long logSize){
	if (log && (logSize > 0)){
		*log = '\0';
		logSize--;	// reserve 1 character for terminating '\0'
	}
	int errorCode= SERIAL_ERR_OK;
	if (send){
		SetTimeout(true, timeout);
		ClearRX();
		errorCode = Puts (send, SERIAL_TIMEOUTLEFT);
	}
	if (errorCode <0 )
		return LastError=errorCode;
	int rtn = 0;

	if (expect){
		if (send){
			m_RecvTimeout = m_SendTimeout;
		}
		else{
			SetTimeout(false, timeout);
		}

		rtn = -1;
		const char** s = expect;
		int nsamples = 0;
		for (;*s!= NULL; ++s, ++nsamples);	// counting expect samples
		int* cnttable = new int[nsamples+1]; // initialize cnttable
		int i = 0;
		for (; i < nsamples; ++i)
			cnttable[i] = 0;

		while (rtn < 0){
			int chr=GetChar(SERIAL_TIMEOUTLEFT);
//			printf ("%c",chr);

			if (chr < 0){
				errorCode = chr;
				break;
			}

			if (log && (logSize > 0)){
				*(log++) = chr;
				*log = '\0';
				logSize--;
			}
			for (i = 0; i < nsamples;++i){
				if (chr == 0){		// '\0' can not be a part of string
					cnttable[i] = 0;
					continue;
				}
				const char* sample = expect[i];
				int ptr = cnttable[i];
				if (sample[ptr] == chr){
					ptr = ++(cnttable[i]);
				}
				else{
					ptr = cnttable[i] = 0;
				}
				if (sample[ptr] == '\0'){
					rtn = i;
					break;
				} // it it's already done
			} // samples check loop
		} // main loop
		delete[] cnttable;
	}	// if (samples)
	return LastError = (errorCode<0) ? errorCode : rtn;
}

int CSerial::SendExpect (LPBINRECORD send, LPBINRECORD* expect, unsigned long timeout, LPBINRECORD logRec){
	unsigned char* log = NULL;
	long logSize = 0;
	if (log){
		logSize = logRec->Len;
		log = logRec->Buf;
		logRec->Len = 0;
	}

	int errorCode = SERIAL_ERR_OK;
	if (send){
		SetTimeout(true, timeout);
		ClearRX();
		errorCode = PutBlock ((const char*)send->Buf, send->Len, SERIAL_TIMEOUTLEFT);
	}
	if (errorCode<0)
		return LastError=errorCode;
	int rtn = 0;

	if (expect){
		if (send){
			m_RecvTimeout = m_SendTimeout;
		}
		else{
			SetTimeout(false, timeout);
		}

		rtn = -1;
		LPBINRECORD* s = expect;
		int nsamples = 0;
		for (;*s!= NULL; ++s, ++nsamples);	// counting expect samples
		unsigned long* cnttable = new unsigned long[nsamples+1]; // initialize cnttable
		int i = 0; 	
		for (;i < nsamples; ++i)
			cnttable[i] = 0;

		while (rtn < 0){
//			TRACE ("%c",chr);
			int chr=GetChar(SERIAL_TIMEOUTLEFT);
			if (chr < 0){
				errorCode= chr;
				break;
			}
			if (log && logSize > 0){
				*(log++) = chr;
				logRec->Len++;
				logSize--;
			}
			for (i = 0; i < nsamples;++i){
				LPBINRECORD sample = expect[i];
				unsigned long ptr = cnttable[i];
				if (sample->Buf[ptr] == chr){
					ptr = ++(cnttable[i]);
				}
				else{
					ptr = cnttable[i] = 0;
				}
				if (sample->Len <= ptr){
					rtn = i;
					break;
				} // it it's already done
			} // samples check loop
		} // main loop
		delete[] cnttable;
	}	// if (samples)
	return LastError = (errorCode<0) ? errorCode : rtn;
}



void CSerial::ClearRX(){
	while(1){
		ClearUngetBuf();
		if (ReceiveChar() < 0)
				return;
	}
}
/*
void CSerial::Sleep(unsigned long timeout){
	SetTimeout (timeout);
	while(!CheckTimeout()){
		Idle();
	}
}
*/
bool CSerial::IsConnected (){
	return Connected;
}

CSerialHandler::CSerialHandler(){
	ParentHandler=NULL;
}

CSerialHandler::~CSerialHandler(){

}

void CSerialHandler::OnConnect(CSerialHandler* handler){
	if (ParentHandler)
		ParentHandler->OnConnect(handler?handler:this);
}

void CSerialHandler::OnDisconnect(CSerialHandler* handler){
	if(ParentHandler)
		ParentHandler->OnDisconnect(handler?handler:this);
}

void CSerialHandler::OnReceiveChar(CSerialHandler* handler){
	if(ParentHandler)
		ParentHandler->OnReceiveChar(handler?handler:this);

}

void CSerialHandler::OnSendChar(CSerialHandler* handler){
	if(ParentHandler)
		ParentHandler->OnSendChar(handler?handler:this);

}

void CSerialHandler::OnIncomingCall(CSerialHandler* handler){
	if(ParentHandler)
		ParentHandler->OnIncomingCall(handler?handler:this);

}

void CSerialHandler::SetParentHandler (CSerialHandler* serialHandler){
	ParentHandler = serialHandler;
}

CSerialHandler* CSerialHandler::GetParentHandler(){
	return ParentHandler;
}

int CSerial::IoCtl(unsigned long code, unsigned long lParam, void* bparam)
{
	int rtn = SERIAL_ERR_NOTSUPPORTED;
	switch (code){
		case SERIAL_IOCTL_SET_DEF_RD_TIMEOUT:
			SetDefTimeout (lParam, SERIAL_OP_READ);
			rtn = SERIAL_ERR_OK;
			break;
		case SERIAL_IOCTL_SET_DEF_WR_TIMEOUT:
			SetDefTimeout (lParam, SERIAL_OP_WRITE);
			rtn = SERIAL_ERR_OK;
			break;
		case SERIAL_IOCTL_SET_DEF_SE_TIMEOUT:
			SetDefTimeout (lParam, SERIAL_OP_SENDEXPECT); 
			rtn = SERIAL_ERR_OK;
			break;
		case SERIAL_IOCTL_SET_DEF_CL_TIMEOUT:
			SetDefTimeout (lParam, SERIAL_OP_CALL);		
			rtn = SERIAL_ERR_OK;
			break;
		case SERIAL_IOCTL_SET_DEF_AC_TIMEOUT:
			SetDefTimeout (lParam, SERIAL_OP_AWAITECALL);		
			rtn = SERIAL_ERR_OK;
			break;
		default:
			rtn = SERIAL_ERR_NOTSUPPORTED;
	}		
	return LastError=rtn;
}

CSerial* CSerial::AwaiteCall (unsigned long timeout){
	LastError = SERIAL_ERR_NOTSUPPORTED;
	return NULL;
}

int CSerial::GetFromUnget(){
	if (UngetBufPtr >= UNGET_BUF_SIZE)
		return LastError=SERIAL_ERR_UNGET_EMPTY;
	return UngetBuf[UngetBufPtr++];
}

int CSerial::Ungetc (int chr){
	if (!UngetBufPtr)
		return LastError=SERIAL_ERR_UNGET_OVER;

	UngetBuf[--UngetBufPtr] = chr;
	return SERIAL_ERR_OK;
}

void CSerial::ClearUngetBuf(){
	UngetBufPtr = UNGET_BUF_SIZE;
}

CBinRecord::CBinRecord(void* buf, unsigned long length){
	Buf = (unsigned char*)buf;
	Len = length;
}

int CSerial::UngetBlock (char* buf, unsigned long size){
	if (size > UngetBufPtr)
		return SERIAL_ERR_UNGET_OVER;
	
	UngetBufPtr-=size;
	memcpy( UngetBuf+UngetBufPtr, buf, size);
	return SERIAL_ERR_OK;
}


int CSerial::Ungets(char* str){
	if (str)
		return UngetBlock (str, strlen(str));
	else
		return SERIAL_ERR_OK;
}
