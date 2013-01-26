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

#include "common.h"
#include "xmlHandler.h"
#include "discManager.h"
#include "main.h"

extern int errno;
#define 	MAIN_DEBUG		printf

int main(int argc,char **argv)
{
	pthread_t diskTid[nCHANNEL];					/*磁盘管理线程,一个通道对应一个线程*/
	pthread_t bsTid;								/*后台线程,一个*/
	pthread_t plhTid[nCHANNEL];					/*分析线程,一个通道对应一个线程*/
	pthread_t debugTid;							/*telnet调试线程*/
	pthread_t appTid[nCHANNEL];
	
	xml_deviceConfigInit();		
#if 1
	xml_fileRecordInit();

	dm_discManagerInit(&diskTid[0], &diskTid[1]);
	sleep(1);
	dc_debugConsoleInit(&debugTid);
	sleep(1);
	
	bs_backStageInit(&bsTid);
	plh_init(&plhTid[0], &plhTid[1]);
	printf("111111111111111111111111111\n\n");
	app_Init(&appTid[0], &appTid[1]);

#endif	

	while(1)
	{
		sleep(3);
	};
	return 0;
}
