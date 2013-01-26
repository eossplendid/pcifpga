#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "debugConsole.h"

static int msgQid;

#define	DEBUG_CMD_QUERY			1

#define	DEBUG_CMD_QUIT			0
static void *rcvMsgThread(void *arg)
{
	MsgDebugConsole msg;
	
	while(1)
	{
		if (msgrcv(msgQid, &msg, (sizeof(MsgDebugConsole)-sizeof(long)), TYPE_DEBUGCONSOLE_RSP, 0) == -1)
		{
			printf("msgrcv failed , line:%d\n", __LINE__);
			continue;
		}

		printf("rcv msg back\n");
		switch(msg.request)
		{
			case DEBUG_CMD_QUERY_DMMSG:
			{
				printf("DiscManager msgQueue:max bytes:%ld, msgs:%d, cbytes:%ld\n,", msg.msgInfo.msg_qbytes, msg.msgInfo.msg_qnum, msg.msgInfo.msg_cbytes);
			}
				break;
			case DEBUG_CMD_QUERY_APPMSG:
			{
				printf("AppRead msgQueue:max bytes:%ld, msgs:%d, cbytes:%ld\n,", msg.msgInfo.msg_qbytes, msg.msgInfo.msg_qnum, msg.msgInfo.msg_cbytes);
			}
				break;

			case DEBUG_CMD_QUERY_PLHMSG:
			{
				printf("ProcessHandler msgQueue:max bytes:%ld, msgs:%d, cbytes:%ld\n,", msg.msgInfo.msg_qbytes, msg.msgInfo.msg_qnum, msg.msgInfo.msg_cbytes);
			}
				break;

			case DEBUG_CMD_PACKETCNT:
			{
				printf("receive packets in total:%ld\n", msg.cnt);
			}
				break;
			case DEBUG_CMD_QUIT:
			{
				printf("exit\n");
				pthread_exit((void *)0);
			}
				break;
				
			default:
				break;
		}

	}

	pthread_exit((void *)0);
}

int sendMsgToApp(int cmd)
{
	MsgDebugConsole msg;
	msg.mtype = TYPE_DEBUGCONSOLE_CMD;
	msg.request = cmd;
	printf("snd msg\n");
	msgsnd(msgQid, &msg, (sizeof(MsgDebugConsole)-sizeof(long)), 0);
	return 0;
}

int main()
{
	char cmd[16];
	pthread_t tid;
	int arg;
	
	if ((msgQid =  msgget(600, 0666 | IPC_CREAT)) == -1)
	{
		printf("msgget failed, line:%d\n", __LINE__);
		return ;
	}

	//printf("msgQid:%d\n", msgQid);
	printf("-------------------------------------------------------------------------------\n");
	printf("msg arg1      :check msgQueue status, arg1 value=1,2 or 3\n");
	printf("               (1,discManager; 2,appRead; 3,processHandler)\n");
	printf("packets	  :check total pakcets\n");
	printf("quit	:quit debug\n");
	printf("-------------------------------------------------------------------------------\n");

	if((pthread_create(&tid, NULL, rcvMsgThread, NULL)) != 0 ) 
	{
		printf("\nTY-3012S creats rcvMsgThread failed!\n");
		return -1;
	}

	while(fgets(cmd, 32, stdin) != NULL)
	{
		if (strncmp(cmd, "help", 4) == 0)
		{
			printf("-------------------------------------------------------------------------------\n");
			printf("msg arg1      :check msgQueue status, arg1 value=1,2 or 3\n");
			printf("               (1,discManager; 2,appRead; 3,processHandler)\n");
			printf("packets	  :check total pakcets\n");
			printf("quit	:quit debug\n");
			printf("-------------------------------------------------------------------------------\n");
		}
		else if (strncmp(cmd, "msg", 3) == 0)
		{
			if ((cmd[4] >= '1') && (cmd[4] <= '3'))
			{
				arg = cmd[4] - '0';
				sendMsgToApp(arg);
			}
			else
			{
				printf("cmd msg arg error, please input your command again.\n");
			}
		}
		else if (strncmp(cmd, "packets", 7) == 0)
		{
			sendMsgToApp(DEBUG_CMD_PACKETCNT);
		}
		else if (strncmp(cmd, "quit", 4) == 0)
		{
			MsgDebugConsole msg;
			msg.mtype = TYPE_DEBUGCONSOLE_RSP;
			msg.request = DEBUG_CMD_QUIT;
			msgsnd(msgQid, &msg, (sizeof(MsgDebugConsole)-sizeof(long)), 0);
			break;
		}


	}

	pthread_join(tid, NULL);
	exit(0);
}
