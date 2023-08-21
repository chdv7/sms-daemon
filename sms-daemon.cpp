#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include "sms-daemon.h"
#include "sms-daemon-mdm.h"
#include "ut.h"


void Log(const char* text) {
	FILE* f = fopen(SMS_LOG_FILE, "a");
	if (f) {
		fprintf(f, "SMS daemon: %s\n", text);
		fclose(f);
	}
}

void LogError(int code, const char* text) {
	FILE* f = fopen(SMS_LOG_FILE, "a");
	if (f) {
		fprintf(f, "%d SMS daemon: %s\n", code, text);
		fclose(f);
	}
}


int main(int argc, char *argv[])
{
        mkdir (SMS_TMP_DIR, 0777);
        chmod (SMS_TMP_DIR, 0777);
        mkdir (OUT_SMS_DIR, 0777);
        chmod (OUT_SMS_DIR, 0777);
        mkdir (IN_SMS_XML_DIR, 0777);
        chmod (IN_SMS_XML_DIR, 0777);

	int isdaemon = 0;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--daemon")) {
			isdaemon = 1;
			printf("sms-daemon\n");
		}
		else 
			printf ("Test mode\n");
	}

	if (isdaemon) {
		pid_t pid;

		/* Fork off the parent process */
		pid = fork();
		if (pid < 0) {
			LogError (-1, "Can not start SMS daemon");
			fprintf(stderr, "Can not start daemon");
			return 1;
		}
		/* If we got a good PID, then
		we can exit the parent process. */
		if (pid > 0) {
			FILE* f = fopen (DATA_PID_PATH, "w");
			if (f) {
				fprintf(f, "%d", pid);
				fclose(f);
			}
			return 0;
		}
		else {  // daemon mode
			chdir("/");
//			close(STDIN_FILENO);
//			close(STDOUT_FILENO);
//			close(STDERR_FILENO);
			Sleep(START_DELAY);
		}
	}
	try {
		CSmsDaemon daemon;
		daemon.Setup();
		int err = daemon.Go();
	}
	catch (CSmsDaemon::SmsDaemonError& e) {
		std::cerr << "Critical error. Code: " << e.ErrorCode << " \"" << e.Verbose << "\"" << std::endl;
	}

	return 0;
}
