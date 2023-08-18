#include "PduSMS.h"
#include <stdio.h>
#include <chrono>
#include <sys/timeb.h>
#include <unistd.h>



#define OUT_SMS_DIR      "/var/spool/sms/outsms"

void Usage (){
	printf ("\
Usage: nv-send-sms <phoneNo> [<out folder>]\n\
");
}

string GenUniqueID (){
	static unsigned  counter=0;
	char buf[100]="--";
	struct timeb  tt;
	ftime(&tt);
	sprintf (buf, "%08X-%08lX-%08X", getpid(), tt.time*1000+tt.millitm, counter++);
	return buf;
}

int SaveSMS (const char* pdu, const char* dir){
	string uniqueID = "Local-"+ GenUniqueID();
	string tmp_name = string(dir) + "/.tmp_" + uniqueID;
//printf ("%s\n", tmp_name.c_str());	
	FILE* f = fopen (tmp_name.c_str(), "w");
	if (!f)
		return -1;
	fputs (pdu, f);
	fclose(f);
	return rename (tmp_name.c_str(), (string(dir) + "/" + uniqueID).c_str())==0 ? 0: -2;
	return 0;
}

int GenSMS (const char* phoneNo, const char* dir){
	string sms_text;
	for (;;){
		int ch = getchar ();
		if (ch == EOF )
			break;
		if (ch == '\n')
			break;
		sms_text += ch;
	}


	CPduSMS sms_processor ( phoneNo, sms_text.c_str());
	TSmsList pdu_list = sms_processor.ParseText();
	
	for (auto it = pdu_list.begin(); it != pdu_list.end(); it++){
		int err = SaveSMS (it->c_str(), dir);
		if (err < 0)
			printf ("Saving error %d\n", err);
	}

	return 0;
}


int main(int argc, char *argv[])
{
	if (argc < 2){
		Usage();
		return 1;
	}
	const char* phone_No = "";
	const char* out_dir = OUT_SMS_DIR;
	
	for (int i=1; i < argc; ++i){
		switch (i){
			case 1:
				phone_No = argv[i];
			break;
			case 2:
				out_dir = argv[i];
			break;
		}
	}

	

	return GenSMS (phone_No, out_dir);
}