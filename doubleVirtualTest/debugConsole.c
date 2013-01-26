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
#include "common.h"

static int DcQid;			/*telnet调试用消息队列ID号*/

	

/*给调试程序回消息*/
int dc_responseConsoleCmd(MsgDebugConsole *msg)
{
	msg->mtype = TYPE_DEBUGCONSOLE_RSP;
	msgsnd(DcQid, msg, (sizeof(MsgDebugConsole)-sizeof(long)), 0);
	return 0;
}

/*消息队列处理*/
static void *debugConsoleThread(void *arg)
{
	struct msqid_ds info;
	MsgDebugConsole msg;

	#if 1
	while(1)
	{
		system("date >> /root/hp/log");
		//system("free >> /root/hp/log");
		sleep(60);
	}
	#endif

	#if 0
	if ((DcQid = msgget(600, 0666 | IPC_CREAT)) == -1)
	{
		CONSOLE_DEBUG("msgget failed, line:%d\n", __LINE__);
		return ;
	}

	printf("debug console DcQid:%d\n", DcQid);
	while(1)
	{
		if (msgrcv(DcQid, &msg, (sizeof(MsgDebugConsole)-sizeof(long)), TYPE_DEBUGCONSOLE_CMD, 0) == -1)
		{
			CONSOLE_DEBUG("msgrcv failed , line:%d\n", __LINE__);
			continue;
		}

		CONSOLE_DEBUG("console rcv msg, request:%d\n", msg.request);
		switch(msg.request)
		{
			case DEBUG_CMD_QUERY_DMMSG:
			{
				dm_getMsgQueueInfo(&msg.msgInfo);
				dc_responseConsoleCmd(&msg);
			}
				break;

			case DEBUG_CMD_QUERY_APPMSG:
			{
				app_getMsgQueueInfo(&msg.msgInfo);
				dc_responseConsoleCmd(&msg);
			}
				break;

			case DEBUG_CMD_QUERY_PLHMSG:
			{
				plh_getMsgQueueInfo(&msg.msgInfo);
				dc_responseConsoleCmd(&msg);
			}
				break;

			case DEBUG_CMD_PACKETCNT:
			{
				msg.cnt = app_getPacketCnt();
				dc_responseConsoleCmd(&msg);
			}
				break;
			default:
				break;
		}
		


	}
	#endif
	pthread_exit((void *)0);
}

/*供主流程调用,初始化,创建线程*/
int dc_debugConsoleInit(pthread_t *id)
{
	int ret = 0;
	
	if((ret = pthread_create(id, NULL, debugConsoleThread, NULL)) != 0 ) 
	{
		CONSOLE_DEBUG("\nTY-3012S creats debugConsoleThread thread failed!\n");
		return ret;
	}

	return ret;
}

