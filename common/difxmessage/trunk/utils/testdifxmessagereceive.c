#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "difxmessage.h"

int main(int argc, char **argv)
{
	int sock;
	int l;
	char message[256], from[32];
	time_t t;
	char timestr[32];

	difxMessageInit(argv[0]);
	difxMessagePrint();

	sock = difxMessageReceiveOpen();

	for(;;)
	{
		from[0] = 0;
		l = difxMessageReceive(sock, message, 255, from);
		if(l < 0)
		{
			usleep(100000);
			continue;
		}
		message[l] = 0;
		if(strncmp(message, "exit", 4) == 0)
		{
			break;
		}
		time(&t);
		strcpy(timestr, ctime(&t));
		timestr[strlen(timestr)-1] = 0;
		printf("[%s %s] %s\n", timestr, from, message);
	}

	difxMessageReceiveClose(sock);

	return 0;
}
