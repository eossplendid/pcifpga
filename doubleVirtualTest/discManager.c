#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#include "common.h"
#include "xmlHandler.h"
#include "discManager.h"
#include "backStage.h"
#include "appReadPacket.h"
#include "ProcessLayerHandler.h"

#ifndef OK
#define OK 	1
#endif

extern int errno;
static int DmQid;			/*消息队列ID号*/

static int WriteDiskFd[nCHANNEL];		/*存盘描述符,双通道的时候就改为[2]*/
static char FileName[nCHANNEL][FILENAME_LEN];			/*各通道文件名*/

static void *dm_formatDisk(void *arg)
{
	char disk;
	char rmCommand[32];
	int chNo;

	chNo = *((int *)arg);
	disk = xml_getCurrentDiskInfo(chNo);
	
	if (disk >= 'A' && disk < 'F')
	{
		disk += 1;
	}
	else if ( disk == 'F') 
	{
		disk = 'A';
	}
	else
	{
		DM_DEBUG("newDiskPosition error\n");
		pthread_exit((void *)1);
	}
	/*双通道的时候这里需要对通道号进行判断*/
	/*以后通道再多,就改用下面的写法,这就需要对
	chNo进行值的检查
	sprintf(rmCommand, "rm /sd%c/disk%c/* -rf", 'a'+chNo, disk);*/
	
	if (chNo == 0)
	{
		sprintf(rmCommand, "rm /sda/disk%c/* -rf", disk);
	}
	else if (chNo == 1)
	{
		sprintf(rmCommand, "rm /sdb/disk%c/* -rf", disk);
	}
	else
	{
		DM_DEBUG("formate Disk error, chNo val wrong :%d ---lineNo:%d\n", chNo, __LINE__);
	}
	
	DM_DEBUG("rmCommand is :%s, ---lineNo:%d\n", rmCommand, __LINE__);
	system(rmCommand);

	pthread_exit((void *)1);

}

void dm_msgDiscMngSend(int quest, char *param1, int param2, long param3, int param4)
{
	MsgQueueBuf msg;

	memset(&msg, 0, sizeof(MsgQueueBuf));

	#if 0
	/*对于立即存盘通道号为0要求所有通道都存盘,此时递归调用向每个通道的磁盘管理都发送
	一个立即存盘结束消息*/
	if ((param2 == 0) && (quest == MSGQ_RQST_SAVE_END))
	{
		int i;
		for (i = 1; i <= nCHANNEL; i++)
		{
			dm_msgDiscMngSend(MSGQ_RQST_SAVE_END, NULL, i, 0, 0);
		}
	}
	#endif	

	if (param2 == 1)
	{
		msg.mtype = TYPE_DSCMANAGER_A;
	}
	else if (param2 == 2)
	{
		msg.mtype = TYPE_DSCMANAGER_B;
	}
	else
	{
		DM_DEBUG("mtype error, val:%d ---lineNo:%d\n", param2, __LINE__);
	}

	msg.mtype = TYPE_DSCMANAGER_A;

	msg.msgRequest = quest;

	//DM_DEBUG("dm_msgDiscMngSend, quest:%d\n", msg.msgRequest);
	if (param1 != NULL)
	{
		strcpy(msg.msgParam, param1);
	}
	
	msg.msgParam2 = param2;
	msg.msgParam3 = param3;
	msg.msgParam4 = param4;

	if (msgsnd(DmQid, &msg, (sizeof(MsgQueueBuf)-sizeof(long)), IPC_NOWAIT) < 0)
	{
		printf("(msgsnd(DmQid, &msg, (sizeof(MsgQueueBuf)-sizeof(long)), IPC_NOWAIT) < 0) ---lineNo:%d\n", __LINE__);
	}

	return ;

}


static int dm_isCustomFile()
{
	if (xml_getConfigDevFileF())
	{
		//DM_DEBUG("dm_isCustomFile TRUE\n");
		return 1;
	}
	else
	{
		//DM_DEBUG("dm_isCustomFile FALSE\n");
		return 0;
	}

}

//添加文件尾
static void dm_add_fileTail(unsigned long packets, int chNo)
{
	FileTail tail;
	tail.tailTag = FILE_TAIL_TAG;
	tail.packetCount = packets;
	write(WriteDiskFd[chNo], (const unsigned char *)&tail, sizeof(FileTail));
	
	return;
}

//函数名称：设置包头，用于MPC8377嵌入式
static int dm_getFileHeader(void **fileHeader , int *headLen)	//共计24byte
{
	FileHeader *head;
	//少个自定义的文件头结构体
	CustomFHeader *cHead;
	//if (dm_isCustomFile())
	if (xml_getConfigDevFileF())
	{
		cHead = (CustomFHeader *)malloc(sizeof(CustomFHeader));
		*fileHeader = cHead;
		cHead->header.dwMagic = FILE_HEADER_MAGIC_SELF;
		cHead->header.wMajor = FILE_HEADER_MAJOR;
		cHead->header.wMinor = FILE_HEADER_MINOR;
		cHead->header.dwThisZone = FILE_HEADER_THISZONE;
		cHead->header.dwSigFigs = FILE_HEADER_SIGFIGS;
		cHead->header.dwSnapLen = FILE_HEADER_SNAPLEN;		//应该64K字节，0x0000FFFF
		cHead->header.dwLinkType = FILE_HEADER_LINKTYPE_ETH;
		cHead->cfginfolen = 0;

		/*配置解析时将整个文件长度和内容拷入内存,这里直接拿过来用*/

		if (cHead->cfginfolen == 0)
		{
			*headLen = sizeof(CustomFHeader) - 4;
		}
		else
		{
			*headLen = sizeof(CustomFHeader) - 4 + cHead->cfginfolen;
		}
	}
	else
	{
		head = (FileHeader *)malloc(sizeof(FileHeader));
		*fileHeader = head;
		head->dwMagic    = FILE_HEADER_MAGIC;
		head->wMajor     = FILE_HEADER_MAJOR;
		head->wMinor     = FILE_HEADER_MINOR;
		head->dwThisZone = FILE_HEADER_THISZONE;
		head->dwSigFigs  = FILE_HEADER_SIGFIGS;
		head->dwSnapLen  = FILE_HEADER_SNAPLEN;		//应该64K字节，0x0000FFFF
		head->dwLinkType = FILE_HEADER_LINKTYPE_ETH;

		*headLen = sizeof(FileHeader);
	}
	return 0;
}


unsigned char *dm_getCurrentTimeStr()
{
	unsigned char timeStr[16];
	TimeStr tStr;

	struct tm *tm_loc, *tm_gm;
	time_t t;

	t = time(NULL);
	tm_loc = localtime(&t);

	#if 0
	DM_DEBUG("loc, year:%d, mon:%0d, day:%d, hour:%d, minute:%d, sec:%d, time:%s\n",
															tm_loc->tm_year, (unsigned short)tm_loc->tm_mon, tm_loc->tm_mday,
															tm_loc->tm_hour, tm_loc->tm_min, tm_loc->tm_sec, asctime(tm_loc));
	#endif
	sprintf(tStr.year, "%d", (unsigned short)(tm_loc->tm_year + 1900));

	if ((tm_loc->tm_mon + 1) < 10)
	{
		sprintf(tStr.mon, "0%d", tm_loc->tm_mon + 1);
	}
	else
	{
		sprintf(tStr.mon, "%d", tm_loc->tm_mon + 1);
	}

	if (tm_loc->tm_mday < 10)
	{
		sprintf(tStr.day, "0%d", tm_loc->tm_mday);
	}
	else
	{
		sprintf(tStr.day, "%d", tm_loc->tm_mday);
	}

	if (tm_loc->tm_hour < 10)
	{
		sprintf(tStr.hour, "0%d", tm_loc->tm_hour);
	}
	else
	{
		sprintf(tStr.hour, "%d", tm_loc->tm_hour);
	}

	if (tm_loc->tm_min < 10)
	{
		sprintf(tStr.min, "0%d", tm_loc->tm_min);
	}
	else
	{
		sprintf(tStr.min, "%d", tm_loc->tm_min);
	}

	if (tm_loc->tm_sec < 10)
	{
		sprintf(tStr.sec, "0%d", tm_loc->tm_sec);
	}
	else
	{
		sprintf(tStr.sec, "%d", tm_loc->tm_sec);
	}
	
	strncpy(timeStr, tStr.year, 4);
	strncpy(&timeStr[4], tStr.mon, 2);
	strncpy(&timeStr[6], tStr.day, 2);	
	strncpy(&timeStr[8], tStr.hour, 2);
	strncpy(&timeStr[10], tStr.min, 2);
	strncpy(&timeStr[12], tStr.sec, 2);
	timeStr[14] = '\0';

	return timeStr;

}
//XXX-D-C-YYYYMMDDHHmmSS

/*告警上报需要文件名*/
unsigned char *dm_getFileNameForWarning(int chNo)
{
	if ((strlen(FileName[chNo]) <= 0) || (chNo > 1) || (chNo < 0))
	{
		return NULL;
	}
	return FileName[chNo];
}

static int dm_newFileName(int chNo, char *startTime)
{
	static int devNo = 0;
	int headLen = 0;
//	FileHeader head;
	void * head;			//文件头分自定义和通用两种
	char curPath;
	curFileInfo *info;
	//char fileName[FILENAME_LEN];		//文件名

	memset(FileName[chNo], 0, FILENAME_LEN);

	info = (curFileInfo *)xml_getCurFileInfoPtr(chNo);
	curPath = xml_getCurrentDiskInfo(chNo);

	info->attrNo++;
	info->packCnt = 0;
	memset(info->attrEndTm, 0, sizeof(info->attrEndTm));
	info->status = 0;
	sprintf(info->position,"disk%c", curPath);
	
	if (chNo == 0)
	{
		sprintf(info->path, "/sda/%s/", info->position);
	}
	else if(chNo == 1)
	{
		sprintf(info->path, "/sdb/%s/", info->position);
	}

	strcpy(info->attrStartTm,dm_getCurrentTimeStr());
	
	sprintf(info->filename, "XXX-%d-%d-%s", devNo, chNo, info->attrStartTm);

	#if 0
	DM_DEBUG("fileInfo----\nattrNo:%d, packCnt:%ld, status:%d, position:%s, path:%s, startTm:%s, filename:%s\n", 
				info->attrNo, info->packCnt, info->status, info->position, info->path, info->attrStartTm, info->filename);
	#endif
	
	sprintf(FileName[chNo], "%s%s", info->path, info->filename);
	DM_DEBUG("filename is :%s, linNo:%d\n", FileName[chNo], __LINE__);
	if ((WriteDiskFd[chNo] = open(FileName[chNo], O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1)
	{
		DM_DEBUG("open new file failed\n");
		return -1;
	}
	
	dm_getFileHeader(&head, &headLen);
	DM_DEBUG("getFileheader, headlen:%d\n", headLen);
	if (write(WriteDiskFd[chNo], (unsigned char *)head, headLen) < 0)
	{
		DM_DEBUG("write filehead error, ---lineNo:%d\n", __LINE__);
	}
	
	/*释放添加文件头时申请的内存*/
	free(head);
	head = NULL;
	return 0;
}


static int dm_refreshFileRecord(int cmd, int chNo, char *param, long param2)
{
	curFileInfo *info = NULL;

	/*测试用,直接返回,不做记录更新操作*/
	//return 0;
	/***************************************************/

	
	info = (curFileInfo *)xml_getCurFileInfoPtr(chNo);

	DM_DEBUG("dm_refreshFileRecord, cmd:%d, chNo:%d, param:%s, param2:%d,  ---lineNo:%d\n", cmd, chNo+1, param, param2, __LINE__);
	
	if (REFRESH_CMD_NEWFILE == cmd)
	{
		//新文件，创建file域，填写相关内容
		xml_addNewNodeFile(chNo, info);		
	}
	else if (REFRESH_CMD_ENDFILE == cmd)
	{
		//存完一个文件，修改结束时间，存盘状态，缓冲同步
		#if 0
		if (param == NULL)
		{
			DM_DEBUG("dm_refreshFileRecord in ENDFILE , param is NULL, check error ---lineNo:%d\n", __LINE__);
		}
		#endif
		strcpy(info->attrEndTm, dm_getCurrentTimeStr());				/*取系统rtc时间*/
		info->packCnt = param2;
		info->status = 1;
		xml_refreshNodeContent(chNo);
	}
	else if(REFRESH_CMD_FORMATE == cmd)
	{
		//有磁盘格式化了，删除相应disk域内的文件，清缓冲数据
		xml_delDiskRecord(chNo);
		//xml_recreateDiskRecord(chNo, info);
	}
	#if 0
	/*此功能包含在重启初始化中了,没必要这里做*/
	else if(REFRESH_CMD_CREATE == cmd)
	{
		//换硬盘
		xml_createNewRecord(chNo);
	}
	#endif
	else
	{
		DM_DEBUG("RefreshFileRecord cmd error,lineNo:%d\n", __LINE__);
		return -1;
	}

	xml_saveRecordFile(chNo);

	return 0;
}

#if 1
static int dm_isCurrentDiskOk(int chNo)
{
	struct statfs diskStat;
	char path[32];
	char tmp;
	long space =0;
	pthread_t fmtThread;
	static unsigned char flag = 0;
	int formatArg;

	tmp = xml_getCurrentDiskInfo(chNo);

	if (chNo == 0)
	{
		sprintf(path,"/sda/disk%c", tmp);
	}
	else if (chNo == 1)
	{
		sprintf(path, "/sdb/disk%c", tmp);
	}
	else
	{
		DM_DEBUG("dm_isCurrentDiskOk chNo is error\n");
		return FALSE;
	}
	
	if (statfs(path, &diskStat) == -1)
	{
		DM_DEBUG("error stat\n");
		return FALSE;
	}
	//板子文件系统大小单位为1k-blocks，换算成MB	
	space = (diskStat.f_bavail / 1024) * 4;	
	DM_DEBUG("path is :%s, space is %ld\n", path, space);
	if (space < 10240)					//空间小于10G，格式化下一个分区
	{
		if (flag == 0)
		{
			flag = 1;
			formatArg = chNo;
			if(pthread_create(&fmtThread, NULL, dm_formatDisk, (void *)&formatArg) != 0 ) 
			{
				DM_DEBUG("\nTY-3012S creats dm_formatDisk thread failed!\n");
				return ;
			}
			dm_refreshFileRecord(REFRESH_CMD_FORMATE, chNo, NULL, 0);
			DM_DEBUG("format msg sent flag ->1, lineNo:%d\n", __LINE__);
		}
	}
	
	if (space > 100)					/*大于100MB就继续存*/
	{
		//DM_DEBUG("current disk is :%c, space is enough, ---lineNo:%d\n", tmp, __LINE__);
		return 1;
	}
	else
	{
		//DM_DEBUG("current disk is :%c, space is full, ---lineNo:%d\n", tmp, __LINE__);
		flag = 0;
		return -1;
	}
}
#endif

//新分区，当前暂定6个分区，ABCDEF，如果到了F，就选A
static int dm_newDiskPosition(int chNo)
{
	char *currentPos;
	char disk;
	curFileInfo *info;

	info = (curFileInfo *)xml_getCurFileInfoPtr(chNo);

	info->attrNo = 0;
	
	disk = xml_getCurrentDiskInfo(chNo);
	DM_DEBUG("curDisk is %c, lineNo:%d\n", disk, __LINE__);
	if (disk >= 'A' && disk < 'F')
	{
		disk += 1;
	}
	else if ( disk == 'F') 
	{
		disk = 'A';
	}
	else
	{
		DM_DEBUG("newDiskPosition error\n");
		return -1;
	}
	DM_DEBUG("newDiskPosition curDisk is %c, lineNo:%d\n", disk, __LINE__);
	
	xml_setCurrentDiskInfo(chNo,disk);
	return 0;
}


static void *dm_findRecordFile(void *arg)
{
	int rFd;
	int len;
	//int sockfd;
	//unsigned long session;
	QueryParam *param;
	
	struct stat fileStat;

	param = (QueryParam *)arg;
	//session = param->session;
	//sockfd = param->sockfd;
	DM_DEBUG("dm_findRecordFile thread arg:%d, ---lineNo:%d\n", param->chNo, __LINE__);
	if (param->chNo == 1)
	{
		if ((rFd = open(XML_RECORD_FILE1A, O_RDONLY)) == -1)
		{
			if ((rFd = open(XML_RECORD_FILE1C, O_RDONLY)) == -1)
			{
				//告诉后台没记录
				bs_transSocketSentData(-1, 0, MS_FILETRANS_FILE_LIST_RSP, param->session, param->sockfd);
				pthread_exit((void *)0);
			}
			
			//读文件发送
			if (stat(XML_RECORD_FILE1C, &fileStat) == -1)
			{
				DM_DEBUG("error stat\n");
				pthread_exit((void *)-1);
			}
			DM_DEBUG("filesize is %ld\n", fileStat.st_size);
			//bs_transSocketSentRecInfo(1, fileStat.st_size, param->session, param->sockfd);

			bs_transSocketSentData(rFd, fileStat.st_size, MS_FILETRANS_FILE_LIST_RSP,param->session, param->sockfd);
		}

		if (stat(XML_RECORD_FILE1A, &fileStat) == -1)
		{
			DM_DEBUG("error stat\n");
			pthread_exit((void *)-1);
		}
		//bs_transSocketSentRecInfo(1, fileStat.st_size, param->session, param->sockfd);

		DM_DEBUG("filesize is %ld\n", fileStat.st_size);
		bs_transSocketSentData(rFd, fileStat.st_size, MS_FILETRANS_FILE_LIST_RSP, param->session, param->sockfd);

	}
	else if (param->chNo  == 2)
	{
		if ((rFd = open(XML_RECORD_FILE2A, O_RDONLY)) == -1)
		{
			if ((rFd = open(XML_RECORD_FILE2C, O_RDONLY)) == -1)
			{
				//告诉后台没记录
				bs_transSocketSentData(-1, 0, MS_FILETRANS_FILE_LIST_RSP, param->session, param->sockfd);
				pthread_exit((void *)0);
			}
			
			//读文件发送
			if (stat(XML_RECORD_FILE2C, &fileStat) == -1)
			{
				DM_DEBUG("error stat\n");
				pthread_exit((void *)-1);
			}
			DM_DEBUG("filesize is %ld\n", fileStat.st_size);
			//bs_transSocketSentRecInfo(1, fileStat.st_size, param->session, param->sockfd);

			bs_transSocketSentData(rFd, fileStat.st_size, MS_FILETRANS_FILE_LIST_RSP,param->session, param->sockfd);
		}

		if (stat(XML_RECORD_FILE2A, &fileStat) == -1)
		{
			DM_DEBUG("error stat\n");
			pthread_exit((void *)-1);
		}
		//bs_transSocketSentRecInfo(1, fileStat.st_size, param->session, param->sockfd);

		DM_DEBUG("filesize is %ld\n", fileStat.st_size);
		bs_transSocketSentData(rFd, fileStat.st_size, MS_FILETRANS_FILE_LIST_RSP, param->session, param->sockfd);

	}	

	close(rFd);	//注意回收资源
	pthread_exit((void *)0);
}

static void *dm_uploadPacketFile(void *arg)
{
	int fd;
	FileInfoParam *param;
	struct stat fileStat;

	param = (FileInfoParam *)arg;

	if ((fd = open(param->filename, O_RDONLY)) == -1)
	{
		//发送文件不存在的消息,然后返回
		bs_transSocketSentFileInfo(0, 0,param->session, param->sockfd);
		pthread_exit((void *)-1);
	}
	//读文件发送
	if (stat(param->filename, &fileStat) == -1)
	{
		bs_transSocketSentFileInfo(0, 0, param->session, param->sockfd);
		pthread_exit((void *)-1);
	}
	
	bs_transSocketSentFileInfo(1, fileStat.st_size, param->session, param->sockfd);
	bs_transSocketSentData(fd, fileStat.st_size, MS_FILETRANS_FILE_UPLOAD, param->session+1, param->sockfd);

	close(fd);
	
	pthread_exit((void *)0);
	
}

int dm_getMsgQueueInfo(struct msqid_ds *info)
{
	msgctl(DmQid, IPC_STAT, info);
	return 0;
}


static void *discManagerThread(void *arg)
{
	MsgQueueBuf msg;
	int temp_pthreadA, temp_pthreadB;
	pthread_t fmtThread;
	pthread_t upldThread;
	key_t key;		//创建队列所用关键字
	int ret;
	int len, ch;
	struct msqid_ds info;
	long type;
//	int count = 0;

	ret = -1;
	type = *(long *)arg;

	printf("msgQueue is running, DmQid:%d, arg:%d\n",DmQid, type);	

	while(1)
	{
		/*TYPE_DSCMANAGER*/
		
		if (msgrcv(DmQid, &msg, (sizeof(MsgQueueBuf)-sizeof(long)), 1, 0) == -1)
		{
			DM_DEBUG("msgrcv failed , line:%d\n", __LINE__);
			continue;
		}		
		
		DM_DEBUG("recv a msg  param2:%d, param:%s, paramPck:%ld, p4:%d,rqst:%d\n", msg.msgParam2, msg.msgParam, 
																			msg.msgParam3, msg.msgParam4, msg.msgRequest);
		
		switch (msg.msgRequest)
		{
			case MSGQ_RQST_SAVE:
			{
				//DM_DEBUG("MSGQ_RQST_SAVE, msg2:%d,msg.msgParam4:%d\n", msg.msgParam2 ,msg.msgParam4);
				ch = msg.msgParam2-1;
				#if 1
				/*驱动传上来的为双字数(4字节的倍数),所以长度部分要乘4还原成字节数*/
				if ((len = write(WriteDiskFd[ch], (const unsigned char *)app_getBufferStartPtr(msg.msgParam4, ch), msg.msgParam3)) < 0)
				{
					DM_DEBUG("write disk error:%s, ---lineNo:%d\n", strerror(errno), __LINE__);

				}
				#endif
				/*	发消息告诉主进程存完了*/
				app_msgSend(MSGQ_RQST_WRITE_END, msg.msgParam4, msg.msgParam2);
			}
				break;
					
			case MSGQ_RQST_SAVE_END:
			{
				ch = msg.msgParam2 - 1;
				#if 1
				//DM_DEBUG("rcv msg from type %d, Request Id:%d\n", msg.mtype, MSGQ_RQST_SAVE_END);
				if (dm_isCustomFile())		//自定义
				{
					/*写文件尾*/
					dm_add_fileTail((unsigned long)msg.msgParam3, ch);
				}

				/*同步数据,保证写入磁盘*/
				//fdatasync(WriteDiskFd[ch]);			
				
				close(WriteDiskFd[ch]);
				#endif
				dm_refreshFileRecord(REFRESH_CMD_ENDFILE, ch, msg.msgParam, msg.msgParam3);
				/*尝试此函数是否可以清cache,目前有效,未带来其他副作用*/
				//system("echo 3 > /proc/sys/vm/drop_caches");
			}
				break;

			case MSGQ_RQST_NEWFILE:
			{
				ch = msg.msgParam2 - 1;
				//DM_DEBUG("rcv msg from type %d, Request Id:%d\n", msg.mtype, MSGQ_RQST_NEWFILE);
				#if 1
				if(dm_isCurrentDiskOk(ch) < 0)						//判断当前分区大小,空间够就分配新文件名，不够就换分区		
				{
					dm_newDiskPosition(ch);
				}
				
				dm_newFileName(ch, msg.msgParam);		/*不再对分区进一步确认,直接申请新文件名*/
				dm_refreshFileRecord(REFRESH_CMD_NEWFILE, ch, msg.msgParam, 0);	//更新记录文件内容
				#endif
			}
				break;

			case MSGQ_RQST_FORMAT:
			{
				//DM_DEBUG("rcv msg from type %d, Request Id:%d\n", msg.mtype, MSGQ_RQST_FORMAT);
			}
				break;


			#if 0
			/*由后台直接发给APP队列处理*/
			/*后台发起,将缓冲中为存入数据存完*/
			case MSGQ_RQST_QSAVE:
			{
				//DM_DEBUG("rcv msg from type %d, Request Id:%d\n", msg.mtype, MSGQ_RQST_QSAVE);
				ch = msg.msgParam2 - 1;
				/*先判断当前是否有正在存写的文件,若没有就不存了*/
				if (app_getNewFileFlag(ch) > 0)
				{
					/*立即存盘*/
				}				
			}
				break;
			#endif
			
			case MSGQ_RQST_CLEARREC:			//更换硬盘，该通道内所有数据全部消失，记录文件重新创建
			{
				DM_DEBUG("rcv msg from type %d, Request Id:%d\n", msg.mtype, MSGQ_RQST_CLEARREC);
				dm_refreshFileRecord(REFRESH_CMD_CREATE, msg.msgParam2, NULL, 0);
			}
				break;

			case MSGQ_RQST_DISKCHECK:			//预留，暂时没想好
			{
				DM_DEBUG("rcv msg from type %d, Request Id:%d\n", msg.mtype, MSGQ_RQST_DISKCHECK);
			}
				break;

			case MSGQ_RQST_QUERY:
			{
				pthread_t Thread;
				static QueryParam param;

				param.chNo = msg.msgParam2;
				param.session = (unsigned long)msg.msgParam3;
				param.sockfd = msg.msgParam4;
				DM_DEBUG("rcv msg from type %d, Request Id:%d\n", msg.mtype, MSGQ_RQST_QUERY);
				if((ret = pthread_create(&Thread, NULL, dm_findRecordFile, (void *)&param)) != 0 ) 
				{
					DM_DEBUG("\nTY-3012S creats dm_findRecordFile thread failed!\n");
					return ;
				}
				
			}
				break;
				
			case MSGQ_RQST_UPLOAD:
			{
				DM_DEBUG("rcv msg from type %d, Request Id:%d\n", msg.mtype, MSGQ_RQST_UPLOAD);
				pthread_t Thread;
				static FileInfoParam param;
				
				if (strlen(msg.msgParam) > 0)
				{
					strcpy(param.filename, msg.msgParam);
				}
				else
				{
					//文件名参数未传过来,回个文件不存在再退,
					break;
				}

				param.session = (unsigned long)msg.msgParam3;
				param.sockfd = msg.msgParam4;
				
				if((ret = pthread_create(&Thread, NULL, dm_uploadPacketFile, (void *)&param)) != 0 ) 
				{
					DM_DEBUG("\nTY-3012S creats dm_uploadPacketFile thread failed!\n");
					return ;
				}
					
			}
				break;

			case MSGQ_RQST_STOP_UPLOAD:
			{
				DM_DEBUG("rcv msg from type %d, Request Id:%d\n", msg.mtype, MSGQ_RQST_STOP_UPLOAD);
				bs_setUploadCancelled();	
			}
				break;
				
			default:
				break;
		}
		
		#if 0
		msgctl(DmQid, IPC_STAT, &info);
		DM_DEBUG("msgInfo break:msg No:%d, max No:%d cur No:%d\n", info.msg_qnum, info.msg_qbytes, info.msg_cbytes);
		#endif
	}

	return ;
}



int dm_discManagerInit(pthread_t *id1, pthread_t *id2)
{
	int ret = 0;
	int type[2];
	
	/*先创建队列,后起的两个线程直接使用即可*/
	if ((DmQid = msgget(300, 0666 | IPC_CREAT)) == -1)
	{
		DM_DEBUG("msgget failed, line:%d\n", __LINE__);
		return ;
	}

	#if 1
	type[0] = TYPE_DSCMANAGER_A;
	if((ret = pthread_create(id1, NULL, discManagerThread, (void *)&type[0])) != 0 ) 
	{
		DM_DEBUG("\nTY-3012S creats discManagerThread thread failed!\n");
		return ret;
	}
	#endif
	
	#if 0
	type[1] = TYPE_DSCMANAGER_B;
	if((ret = pthread_create(id2, NULL, discManagerThread, (void *)&type[1])) != 0 ) 
	{
		DM_DEBUG("\nTY-3012S creats discManagerThread thread failed!\n");
		return ret;
	}
	#endif
	return ret;
}
