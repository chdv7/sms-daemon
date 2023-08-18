#include "ut.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/timeb.h>


static int strnicmp (const char* s1, const char* s2, int maxlen){
	for (;maxlen>0;--maxlen, ++s1, ++s2){
	    if  (*s1 != *s2)
		return *s1 >*s2 ? 1:-1;
	    if (!*s1)
		break;
	}
	return 0;
}

const char* GetXMLStr (XMLNode& n, const char* name, const char*def){
	const char* v = n.getAttribute(name);
	return v?v:def;
}

int GetXMLInt (XMLNode& n, const char* name, int def){
	const char* v = n.getAttribute(name);
	if (!v)
		return def;
	const char* format = "%d";
	if (strnicmp(v, "0x", 2)==0){
		v+=2;
		format = "%x";
	}
	size_t len = strlen(v);
	if (len<=0)
		return def;
	if (tolower(v[len-1]) == 'h' )
		format = "%x";

	int r=def;
	sscanf (v, format, &r);
	return r;
}


int UnicodeToUTF8 (char* d, const wchar_t* src, size_t sz){
	char* bufPtr = d;
	if (!d)
		return 0;
	if (sz<=0)
		return 0;
	if (!src){
		*d = 0;
		return 0;
	}
	--sz;
	while (*src ){
		size_t code_length = 1;
		if (*src& ~0x0000007F)code_length++;  // *src at least 11 bit
		if (*src& ~0x000007FF)code_length++;  // *src 16 bit

		if (sz < code_length )
			break;

		switch (code_length){  
		case 1:         // *src ( 7 bit) 000000000zzzzzzz
			*d = (char)*src;				  // 0zzzzzzz
			break;

		case 2:			// *src (11 bit) 00000yyyyyzzzzzz
			*d     = (*src>>6)         | 0xc0; // 110yyyyy
			*(d+1) = (*src     &0x003f)| 0x80; // 10zzzzzz
			break;

		case 3:			// *src (16 bit) xxxxyyyyyyzzzzzz
			*d     =  (*src>>12)        | 0xe0; // 1110xxxx
			*(d+1) = ((*src>>6) &0x003f)| 0x80; // 10yyyyyy
			*(d+2) =  (*src     &0x003f)| 0x80; // 10zzzzzz
			break;

		}
		sz-= code_length;
		d+= code_length;
		src++;
	}
	*d=0;
	return d-bufPtr;
}

int UTF8ToUnicode (wchar_t* d, const char* src, int sz){
static const char XML_utf8ByteTable[256] =
{
    //  0 1 2 3 4 5 6 7 8 9 a b c d e f
    0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x00
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x10
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x20
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x30
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x40
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x50
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x60
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x70End of ASCII range
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x80 0x80 to 0xc1 invalid
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0x90
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0xa0
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,// 0xb0
    1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,// 0xc0 0xc2 to 0xdf 2 byte
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,// 0xd0
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,// 0xe0 0xe0 to 0xef 3 byte
    4,4,4,4,4,1,1,1,1,1,1,1,1,1,1,1 // 0xf0 0xf0 to 0xf4 4 byte, 0xf5 and higher invalid
};
	if (!d)
		return 0;
	if (sz<=0)
		return 0;
	if (!src){
		*d = 0;
		return 0;
	}
	wchar_t* bufPtr = d;

	while (*src && --sz > 0){
		switch(XML_utf8ByteTable[*(const unsigned char*)src]){

		case 4:
			*d =  *src++ &0x07;   // 11110www
			if (!*src) continue;
			*d <<=6;
			*d |= *src++ &0x3f;   // 10xxxxxx
			if (!*src) continue;
			*d <<=6;
			*d |= *src++ &0x3f;   // 10yyyyyy
			if (!*src) continue;
			*d <<=6;
			*d |= *src++ &0x3f;   // 10xxxxxx
			break;

		case 3:
			*d =  *src++ &0x0f;   // 1110xxxx
			if (!*src) continue;
			*d <<=6;
			*d |= *src++ &0x3f;   // 10yyyyyy
			if (!*src) continue;
			*d <<=6;
			*d |= *src++ &0x3f;   // 10xxxxxx
			break;

		case 2:
			*d =  *src++ &0x1f;   // 110yyyyy
			if (!*src) continue;
			*d <<=6;
			*d |= *src++ &0x3f;   // 10xxxxxx
			break;

		case 1:
		default:
			*d = *src++;
			break;
		}
		d++;
	}
	*d = 0;
	return d - bufPtr;
}; 


int64_t GetTickCnt64 (){
    	struct timeb  tt;
	ftime (&tt);
	return ((int64_t)tt.time)*1000+tt.millitm;
}

int32_t GetTickCnt32 (){
    	struct timeb  tt;
	ftime (&tt);
	return ((int32_t)tt.time)*1000+tt.millitm;
}
