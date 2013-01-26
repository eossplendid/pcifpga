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
#include <pthread.h>

#include "common.h"
#include "discManager.h"
#include "appReadPacket.h"
#include "ProcessLayerHandler.h"
#include "backStage.h"



static int AppQid;							/*驱动控制消息,也收存盘结束的消息*/
Flags AppFlag[nCHANNEL];					/*各通道状态标志*/
SvUploadCondition	SvCondition;				/*SV指定上传条件*/
static int drvFd;							/*pci设备文件描述符*/

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;			/*互斥锁*/

MsgCommand		*AppMsgCommand[nCHANNEL];					/*驱动消息数据结构*/
MsgChInterface	*AppChInterface[nCHANNEL];
MsgGpsInterface	*AppGpsInterface[nCHANNEL];
MsgFpgaTime		*AppFpgaTime[nCHANNEL];
MsgTriggerTime	*AppTriggerTime[nCHANNEL];
MsgAlarmOut		*AppAlarmOut[nCHANNEL];
MsgBeepAlarm	*AppBeepAlarm[nCHANNEL];
MsgWarchDog		*AppWarchDog[nCHANNEL];
MsgClearFifo		*AppClearFifo[nCHANNEL];
MsgChFilter		*AppChFilter[nCHANNEL];
MsgFilterMac		*AppFilterMac[nCHANNEL];
MsgChStatus		*AppChStatus[nCHANNEL];
MsgTriggerStatus	*AppTriggerStatus[nCHANNEL];
MsgKeyStatus		*AppKeyStatus[nCHANNEL];
MsgFrameCounters	*AppFrameCounters[nCHANNEL];
MsgReadBuffer	*AppReadBuffer[nCHANNEL];


/*驱动操作部分变量*/
static unsigned char MsgBuffer[nCHANNEL][MSG_BUF_LEN];
unsigned char		Ch1Buffer[CH_BUF_LEN];
unsigned char		Ch2Buffer[CH_BUF_LEN];

/*测试用变量*/
static unsigned long packetCnt = 0;
/*报文分析线程查看此标志,决定是否进行分析功能*/
int app_getNewFileFlag(chNo)
{
	return AppFlag[chNo].newFile;
}

int app_getStopParseFlag(int chNo)
{
	return AppFlag[chNo].stopParse;
}

unsigned char app_getSvConChNo()
{
	return SvCondition.chNo;
}

void app_setSvCondition(unsigned char *info)
{
	
	if (info == NULL)
	{
		/*传入NULL表示停止了smv上传,将匹配项清掉*/
		memset((unsigned char *)&SvCondition, 0, sizeof(SvUploadCondition));
		return;
	}

	memcpy((unsigned char *)&SvCondition, info, sizeof(SvUploadCondition));

	return;
}

/*由sv上传部分调用,如果关闭了sv上传,则停止上传,关闭线程回收资源*/
int app_getSvEnable()
{
	return SvCondition.svEnable;
}

/*由长连接管理调用,通过后台命令动态开关sv上传功能*/
void app_setSvEnable(unsigned char flag)
{
	SvCondition.svEnable = flag;
	return;
}


/*telnet调试查询app队列状态接口*/
int app_getMsgQueueInfo(struct msqid_ds *info)
{
	msgctl(AppQid, IPC_STAT, info);
	return 0;
}

/*telnet 调试用接口,暂不完善,先不改进*/
unsigned long app_getPacketCnt()
{
	return packetCnt;
}

/*存储缓冲数据
flag:0,超时存盘; 1, 正常存盘
position:缓冲逻辑分区,0,1,2
chNo:通道号
*/
static unsigned long app_saveBufferData(unsigned int flag, unsigned int chNo, unsigned int position)
{
	Flags *appFlag;
	int no;
	unsigned long len, offsetLen;
	if ((flag > 1) || (chNo > nCHANNEL) || (chNo == 0))
	{
		APP_DEBUG("app_saveBufferData param error ,flag:%d, chNo:%d---lineNo:%d\n",
												flag, chNo, __LINE__);
		return -1;
	}

	printf("app_saveBufferData flag:%d, chNo:%d\n", flag, chNo);
	no = chNo - 1;
	appFlag = &AppFlag[no];
	
	if (flag == APP_SAVE_TIMEOUT)
	{
		if ((len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.buf) > 0)
		{
			if (appFlag->bufferStatus.currentBuffer == 0)
			{
				if ((len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.buf) > 0)
				{
					appFlag->bufferStatus.bufBStart = appFlag->bufferStatus.buf + MBYTES_10;
					appFlag->bufferStatus.analyz = appFlag->bufferStatus.rear = appFlag->bufferStatus.bufBStart;
				}
			}
			else if (appFlag->bufferStatus.currentBuffer == 1)
			{
				if ((len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.bufBStart) > 0)
				{
					appFlag->bufferStatus.bufCStart = appFlag->bufferStatus.buf + MBYTES_20;
					appFlag->bufferStatus.analyz = appFlag->bufferStatus.rear = appFlag->bufferStatus.bufCStart;
				}
			}
			else if (appFlag->bufferStatus.currentBuffer == 2)
			{
				if ((len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.bufCStart) > 0)
				{
					appFlag->bufferStatus.analyz = appFlag->bufferStatus.rear = appFlag->bufferStatus.buf;
				}
			}

			/*有数据再发消息*/
			if (len > 0)
			{
				dm_msgDiscMngSend(MSGQ_RQST_SAVE, NULL, chNo, len, appFlag->bufferStatus.currentBuffer);
				appFlag->totalLength += len;
			}
			}
	}
	else if (flag == APP_SAVE_NORMAL)
	{
		//if (appFlag->bufferStatus.currentBuffer == 0)
		if (position == 0)
		{
			len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.buf;			/*AStart就是buf*/
			dm_msgDiscMngSend(MSGQ_RQST_SAVE, NULL, chNo, len, 0);
			appFlag->totalLength += len;		
			appFlag->bufferStatus.bufStatus[0] = 1;
			appFlag->bufferStatus.bufBStart = appFlag->bufferStatus.analyz;
			appFlag->bufferStatus.currentBuffer = 1;
			APP_DEBUG("chNo:%d, bufA save data  ---lineNo:%d\n", chNo, __LINE__);
		}
		//else if (appFlag->bufferStatus.currentBuffer == 1)
		else if (position == 1)
		{
			len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.bufBStart;
			dm_msgDiscMngSend(MSGQ_RQST_SAVE, NULL, chNo, len, 1);
			appFlag->totalLength += len;		
			appFlag->bufferStatus.bufStatus[1] = 1;
			appFlag->bufferStatus.bufCStart = appFlag->bufferStatus.analyz;
			appFlag->bufferStatus.currentBuffer = 2;
			APP_DEBUG("chNo:%d, bufB save data  ---lineNo:%d\n", chNo, __LINE__);
		}	
		//else if (appFlag->bufferStatus.currentBuffer == 2)
		else if (position == 2)
		{
			len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.bufCStart;
			dm_msgDiscMngSend(MSGQ_RQST_SAVE, NULL, chNo, len, 2);
			appFlag->totalLength += len;		
			appFlag->bufferStatus.bufStatus[2] = 1;

			/*尾巴的半包拷到1区里面去*/
			memcpy(appFlag->bufferStatus.buf, appFlag->bufferStatus.analyz, (appFlag->bufferStatus.rear - appFlag->bufferStatus.analyz));
			offsetLen = appFlag->bufferStatus.rear - appFlag->bufferStatus.analyz;
			
			appFlag->bufferStatus.analyz = appFlag->bufferStatus.buf;
			appFlag->bufferStatus.rear = appFlag->bufferStatus.buf + offsetLen;
			appFlag->bufferStatus.currentBuffer = 0;
			APP_DEBUG("chNo:%d, bufC save data  ---lineNo:%d\n", chNo, __LINE__);
		}
	}

	return len;
}

/*设定sv上传消息,攒够10个发消息进行上传*/
int svUploadInfoSet(unsigned char **packet, unsigned long len, int reset)
{
	static MsgSvInfo svInfo;
	
	if (reset == 1)
	{
		svInfo.count = 0;
	}
	
	svInfo.packet[svInfo.count] = *packet;
	svInfo.length[svInfo.count] = len;

	svInfo.count++;

	if (svInfo.count == MAX_UPLOAD_NUM)
	{
		svInfo.request = MSG_RQST_SVUPLOAD;
		bs_svMsgSend(svInfo);
		svInfo.count = 0;
	}

	return 0;
}

/*数据分析,识别每帧数据的包类型,发送分析消息*/
static int parseFifoFrameData(int chNo)
{

	unsigned long qLength;
	unsigned char   *pDataTime, *pPayload;
	unsigned short *pLength, *pFrameState;
	unsigned int *pHead, *pFCS, *pTail;
	unsigned char style, abnormal;
	static unsigned int gooseSvPacketCnt[2] = {0, 0};
	int ret, no;
	int jumpLen;
	
	MsgProcessLayer msg;
	MsgSvInfo svMsg;
	Flags *appFlag;
	
	unsigned char srcMac[6], dstMac[6];
	unsigned short appId;

	unsigned char svCondition[14];			/*源mac和目的mac各12字节,appId两字节*/

	unsigned long frameLen = 0;
	int errorFrame = 0;

	no = chNo - 1;
	appFlag = &AppFlag[no];
	msg.chNo = chNo;

	while(1)
	{
		qLength = (appFlag->bufferStatus.rear - appFlag->bufferStatus.analyz) * 4;
		#if 1
		APP_DEBUG("qLenth:%ld\ndatas:", qLength);
		APP_DEBUG("%08x %08x %08x %08x %08x %08x\n", *appFlag->bufferStatus.analyz,
										*(appFlag->bufferStatus.analyz + 1),
										*(appFlag->bufferStatus.analyz + 2),
										*(appFlag->bufferStatus.analyz + 3),
										*(appFlag->bufferStatus.analyz + 4),
										*(appFlag->bufferStatus.analyz + 5));
		#endif
		if (qLength < 96)			//最短帧为64字节+16字节时间+16字节包封
		{
			//APP_DEBUG("qLength is too short , return to read\n");
			break;
		}

		pHead = appFlag->bufferStatus.analyz;
		if (Ntohs(*pHead) == FRAME_HEADE)		/*找到了帧头1a2b3c4d*/
		{
			pLength = (unsigned short*)(pHead + 1);
			pFrameState = pLength + 1;

			pPayload = pFrameState + 9;
			if (errorFrame == 1)
			{
				printf(" errorFrame , datatime:%08x  %08x\n", *(unsigned int *)(pFrameState+1), *(unsigned int *)(pFrameState+3));
				errorFrame = 0;
			}
			
			APP_DEBUG("pLength:%04x, ---lineNo:%d\n", *pLength, __LINE__);
			if ((Ntohs(*pLength)+14) > qLength)		/*当前缓冲区数据长度不够完整帧长度,跳出等待下一次读包再解*/
			{
				break;
			}

			packetCnt++;
			
			appFlag->totalPackets++;
			if (appFlag->newFile == 0)												/*新文件*/
			{
				APP_DEBUG("chNo:%d,new file name request\n", chNo);
				//strncpy(appFlag->time, "1234567", 14);
				//appFlag->time[14] = '\0';
				dm_msgDiscMngSend(MSGQ_RQST_NEWFILE, NULL, chNo, 0, 0);
				appFlag->newFile = 1;
			}
						
			#if 1
			/*检验各指针位置是否正确*/
			APP_DEBUG("pHead:%08x, pLength:%08x\n", *pHead, *pLength);
			//APP_DEBUG("pPayload:%x %x %x %x\n",*pPayload, *(pPayload+1), *(pPayload+2), *(pPayload+3));
			APP_DEBUG("pFrameState:%08x\n",*pFrameState);			
			#endif

			#if 1
			/*确认帧类型和异常状态,挑出goose和smv两种报文和异常类型根据需要进行上报*/
			style = (*pFrameState & 0xff00) >> 8;
			//abnormal = (*pFrameState & 0xff);
			//APP_DEBUG("style:%x, abnormal:%x, ---lineNo:%d\n", style, abnormal, __LINE__);

			if (((style & 0x10) == 0) && ((style & 0x20) == 0))			/*既不是smv,也不是goose，分析下一帧,1<<4,1<<5*/
			{
				APP_DEBUG("common packet\n");
				//appFlag->bufferStatus.analyz = (unsigned int *)(pFrameState + 5);		/*下一帧,state后10字节*/

				/*从frameState一直到帧尾结束6 = + 8 - 2,减掉frame,从time开始,不然会错开两字节*/
				jumpLen = Ntohs(*pLength) + 6;		
				
				if ((jumpLen % 4) != 0)
				{
					jumpLen = jumpLen + (4 - (jumpLen % 4));
				}

				//printf("jumplen is :%d\n", jumpLen);
				/*测试用*/
				if ((jumpLen % 4) != 0)
				{
					printf("jumpLen calculate error,val:%d ---lineNo:%d\n",(jumpLen % 4),  __LINE__);
				}
				/************/
				
				/*为减少指针移动的次数,这里进行类型转换,先按8字节跳转,
					再对8取余,将尾巴几个字节补上,jumpLen%8的结果只有0或4两种*/
				appFlag->bufferStatus.analyz = ((unsigned int *)((long long *)(pFrameState+1) + (jumpLen / 8))) + (jumpLen % 8) / 4;

				continue;
			}

			/*异常先不分析,以后再加*/
			#if 0
			if((abnormal & (1 << 0)) != 0)			/*CRC错误*/
			{}
			else if((abnormal & (1 << 1)) != 0)			/*接收错误*/
			{}
			else if ((abnormal & (1 << 2)) != 0)			/*无效符号*/
			{}
			else if ((abnormal & (1 << 3)) != 0)			/*非整数字节*/
			{}
			else if ((abnormal & (1 << 4)) != 0)			/*超长帧*/
			{}
			#endif

			if (appFlag->stopParse == 0)
			{
				/*先看分析的消息队列满标志是否被置过,如果满了要等队列空了再发*/
				if (appFlag->queueFull == 1)
				{
					if (plh_getMsgQueueStatus(no) > 0)
					{
						appFlag->queueFull = 0;
						gooseSvPacketCnt[no] = 0;
						/*给分析指针赋值*/
						msg.packets[gooseSvPacketCnt[no]] = (unsigned char *)appFlag->bufferStatus.analyz;
						gooseSvPacketCnt[no]++;
						/*sv和goose 报文个数统计*/
						#if 1
						if ((style & 0x10) != 0)
						{
							appFlag->svPackets++;
							msg.type[gooseSvPacketCnt[no]] = MSG_PROCESS_SVTYPE;
						#if 0
							memcpy(dstMac, pPayload, 6);
							memcpy(srcMac, pPayload+6, 6);
							memcpy(appId, pPayload+14, 2);
						#endif

							/*这里还得判有没有VLAN ID,有四个字节*/
							memcpy(svCondition, pPayload, 12);
							//memcpy(svCondition+12, pPayload+14+4,2);

						#if 0
							if ((strcmp(dstMac, appFlag->svCondition.dstMac) == 0) && 
									(strcmp(srcMac, appFlag->svCondition.srcMAC) == 0) && 
									(appFlag->svCondition.appID == appId))
						#else
						//	if (memcmp(svCondition, SvCondition.dstMac, 12) == 0)			/*这样比较效率高*/
						#endif
							{
								if (SvCondition.svEnable > 0)
								{
									if (SvCondition.chNo == chNo)
									{
										frameLen = Ntohs(*pLength) + 14;					/*整帧长*/
										if ((frameLen % 4) != 0)
										{
											frameLen = frameLen + 4- (frameLen % 4);			/*补齐字节数*/
										}
										svUploadInfoSet((unsigned char **)&appFlag->bufferStatus.analyz, frameLen, 1);
									}
								}
							}
						}
						else if ((style & 0x20)  != 0)
						{
							printf("goose type packet ---lineNo:%d\n", __LINE__);
							printf("datatime:%08x  %08x\n", *(unsigned int *)(pFrameState+1), *(unsigned int *)(pFrameState+3));
							appFlag->goosePackets++;
							msg.type[gooseSvPacketCnt[no]] = MSG_PROCESS_GOOSETYPE;
						}
						#endif

						#if 1
						/*发送告警重置*/
						msg.mtype = TYPE_PROCESSLAYER;
						msg.request = MSG_RQST_RESET_WARNING;
						ret = plh_msgSend(&msg);
						#endif
					}
				}
				else
				{
					/*给分析指针赋值*/
					msg.packets[gooseSvPacketCnt[no]] = (unsigned char *)appFlag->bufferStatus.analyz;

					/*sv和goose 报文个数统计*/
					#if 1
					if ((style & 0x20) != 0)				/*1<<5*/
					{
						static unsigned long count = 0;
						count++;
						
						appFlag->svPackets++;
						msg.type[gooseSvPacketCnt[no]] = MSG_PROCESS_SVTYPE;
						#if 1
						#if 0
						memcpy(dstMac, pPayload, 6);
						memcpy(srcMac, pPayload+6, 6);
						memcpy((unsigned char *)&appId, pPayload+14, 2);

						if ((strncmp(dstMac, appFlag->svCondition.dstMac, 6) == 0) && 
							(strncmp(srcMac, appFlag->svCondition.srcMAC, 6) == 0) && 
							(appFlag->svCondition.appID == appId))
						{
							svUploadInfoSet(&appFlag->bufferStatus.analyz, (Ntohs(*pLength) + 8 + 6), 1);
						}
						#endif
							memcpy(svCondition, pPayload, 12);
	//						memcpy(svCondition+12, pPayload+14+4,2);

						
						#if 0
							if ((strcmp(dstMac, appFlag->svCondition.dstMac) == 0) && 
									(strcmp(srcMac, appFlag->svCondition.srcMAC) == 0) && 
									(appFlag->svCondition.appID == appId))
						#else
					//		if (memcmp(svCondition, SvCondition.dstMac, 12) == 0)			/*这样比较效率高*/
						#endif

						{
								if (SvCondition.svEnable > 0)
								{
									frameLen = Ntohs(*pLength) + 14;					/*整帧长*/
									if ((frameLen % 4) != 0)
									{
										frameLen = frameLen + 4- (frameLen % 4);			/*补齐字节数*/
									}
									svUploadInfoSet((unsigned char **)&appFlag->bufferStatus.analyz, frameLen, 0);
								}
							}
						#endif
					}
					else if ((style & 0x10)  != 0)				/*1<<4*/
					{
						APP_DEBUG("goose type packets, ---lineNo:%d\n", __LINE__);
						printf("datatime:%08x  %08x\n", *(unsigned int *)(pFrameState+1), *(unsigned int *)(pFrameState+3));
						appFlag->goosePackets++;
						msg.type[gooseSvPacketCnt[no]] = MSG_PROCESS_GOOSETYPE;
					}
					#endif
					gooseSvPacketCnt[no]++;
					
					if (gooseSvPacketCnt[no] == MAX_PROCESS_NUM)
					{
						/*10个包给分析线程发一次消息*/
						msg.mtype = TYPE_PROCESSLAYER;
						msg.request = MSG_RQST_PROCESS;
						msg.count = MAX_PROCESS_NUM;

						#if 1
						ret = plh_msgSend(&msg);
						
						/*消息发送失败,队列已满*/
						if (ret == PROCESS_MSGQUEUE_FULL)
						{
							APP_DEBUG("PROCESS_MSGQUEUE_FULL appFlag->queueFull = 1 ---lineNo:%d\n", __LINE__);
							appFlag->queueFull = 1;
						}
						#endif
						gooseSvPacketCnt[no] = 0;
					}
				}
			}
			else
			{
				/*清空记录的指针,等关闭停止分析标志后再重新记录*/				
				gooseSvPacketCnt[no] = 0;
			}
			#endif
		//	appFlag->bufferStatus.analyz = (unsigned int *)(pFrameState + 5); 				/*下一帧*/
		//	pDataTime = (unsigned char *)(pLength + 1);										/*时间戳*/

			/*从frameState一直到帧尾结束6 = + 8 - 2,减掉frame,从time开始,不然会错开两字节*/
			jumpLen = Ntohs(*pLength) + 6;		

			if ((jumpLen % 4) != 0)
			{
				jumpLen = jumpLen + (4 - jumpLen % 4);
			}
			
			/*为减少指针移动的次数,这里进行类型转换,先按8字节跳转,
				再对8取余,将尾巴几个字节补上,jumpLen%8的结果只有0或4两种*/
			appFlag->bufferStatus.analyz = ((unsigned int *)((long long *)(pFrameState+1) + (jumpLen / 8))) + (jumpLen % 8) / 4;
		}
		else										/*没找到帧头就往后挪1个字节,继续找*/
		{
			#if 1
//			static count = 0;
//			count++;
//			if ((count % 2000) == 0)
			{		
				errorFrame = 1;
				//printf("wrong frame head pointer +1\n");
				//printf("time is :%s\n", dm_getCurrentTimeStr());
			}
			#endif
			appFlag->bufferStatus.analyz += 1;
		}
	}

	return 0;

}

/*app消息队列发消息接口*/
int app_msgSend(unsigned char quest, int param2, int chNo)
{
	MsgAppBuf msg;

	memset(&msg, 0, sizeof(MsgAppBuf));

	if ((quest == MSGQ_RQST_SAVE_FILE) && (chNo == 0))
	{
		int i;
		for (i = 1; i <= nCHANNEL; i++)
		{
			app_msgSend(MSGQ_RQST_SAVE_FILE, 0, i);
		}
		
		return 0;
	}
	
	msg.msgRequest = quest;
	msg.mtype = chNo;

	msg.msgParam2 = param2;
	
	if (msgsnd(AppQid, &msg, (sizeof(MsgAppBuf)-sizeof(long)), IPC_NOWAIT) < 0)
	{
		APP_DEBUG("chNo:%d,msgsnd(AppQid failed, error:%s ---lineNo:%d\n", chNo, strerror(errno), __LINE__);
	}
	
	return 0;
}


/*磁盘管理存盘时根据收到的虚拟分区号,获取缓冲存盘数据段首地址*/
unsigned char * app_getBufferStartPtr(int no, int chNo)
{
	if (no == 0)						/*bufferA*/
	{
		return (unsigned char *)AppFlag[chNo].bufferStatus.buf;
	}
	else if (no == 1)					/*bufferB*/
	{
		return (unsigned char *)AppFlag[chNo].bufferStatus.bufBStart;
	}
	else if (no == 2)					/*bufferC*/
	{
		return (unsigned char *)AppFlag[chNo].bufferStatus.bufCStart;
	}
	
	return NULL;
}

/*驱动消息设置接口*/
static int SetMsgBuffer(int no)
{
	AppMsgCommand[no] = (MsgCommand *)MsgBuffer[no];
	memset(MsgBuffer[no], 0x0, MSG_BUF_LEN);
	AppMsgCommand[no]->Head = MSGHEAD;
	AppMsgCommand[no]->VersionSid	= MSGSESSIONID;
	AppMsgCommand[no]->Id	= MSGID;
	AppMsgCommand[no]->Tail = MSGTAIL;
	return 0;
}


/*读驱动,获取LED灯状态*/
int app_getLedStatus(int *ledH, int *ledL)
{
	MsgLedStatus AppLedStatus;
	int ret;
	/*设置*/
	AppLedStatus.Head = MSGHEAD;
	AppLedStatus.VersionSid = MSGSESSIONID;
	AppLedStatus.Id = CHECK_LED_STATUS;
	AppLedStatus.Tail = MSGTAIL;

 	pthread_mutex_lock(&mutex);
	ret = read(drvFd, AppLedStatus, MSG_BUF_LEN);			/*将完整的消息内容送入Driver*/
	pthread_mutex_unlock(&mutex);

	if (ret == -1)
	{
		/*未获取正确*/
		return -1;
	}
	else
	{
		*ledH = AppLedStatus.LedH;
		*ledL = AppLedStatus.LedL;

		return 0;
	}
	
	return 0;
}

/*主流程 从驱动读包,分析,发送存盘消息,动态调整分析功能开关*/
#if 1
static void*app_PacketRead(void *args)
#else
void	app_PacketRead(int chNo)
#endif
{
	MsgAppBuf msg;
	unsigned long qLength = 0, len =0, offsetLen = 0;			//记录大小,count每10MB存一次,total每300M换一个文件,qLenth > 25MB就停分析
	int packets = 0;						//记录包数,单个文件存够时传入磁盘管理

	Flags *appFlag;
	MsgProcessLayer msgProcess;			/*分析线程消息队列消息结构*/
	unsigned long singleFileLength, singleFileLengthMin;		/*接近配置的长度就存成一个文件*/
	int chNo, no;

	unsigned int idleTimes = 0;

	
	chNo = *((int *)args);
	no = chNo -1;
	printf("chNo:%d\n",chNo);

	appFlag = &AppFlag[no];
	msgProcess.chNo = chNo;

	/*从配置中获取单个文件长度*/
	singleFileLength = xml_getConfigDevFileS()*MBYTES;
	singleFileLengthMin = singleFileLength - MBYTES;	    			/*避免60M文件出现*/
	APP_DEBUG("singleFileLengthMin = %d\n", singleFileLengthMin / MBYTES);
	
	AppReadBuffer[no] = (MsgReadBuffer *)MsgBuffer[no];
	SetMsgBuffer(no);
	AppReadBuffer[no]->Id   = READ_CH_BUFFER;			//检查接收到的报文

	printf("app_read start, chNo:%d\n", chNo);
	while(1)
	{
		//msgctl(AppQid, IPC_STAT, &info);
		//printf("msgInfo :msg No:%d, max No:%d cur No:%d\n", info.msg_qnum, info.msg_qbytes, info.msg_cbytes);
		#if 1
		if (msgrcv(AppQid, &msg, (sizeof(MsgAppBuf)-sizeof(long)), chNo, IPC_NOWAIT) > 0)		
		{
			APP_DEBUG("AppReadPacket msgrcv packet read,chNo:%d,request:%d,buf No:%ld line:%d\n", chNo, msg.msgRequest, msg.msgParam2, __LINE__);
			if (msg.msgRequest == MSGQ_RQST_WRITE_END)
			{
				/*存盘结束,缓冲状态置闲*/
				if ((msg.msgParam2 >= 0) && (msg.msgParam2 < 3))
				{
					appFlag->bufferStatus.bufStatus[msg.msgParam2] = 0;
				}
				
				if (appFlag->stopParse == 1)									/*设置过关闭分析的标志*/
				{
					msgProcess.mtype = TYPE_PROCESSLAYER;
					msgProcess.request = MSG_RQST_RESET_WARNING;
					//plh_msgSend(&msgProcess);								/*发送重置告警分析消息*/
					appFlag->stopParse = 0;    									/*关停止报文分析标志*/
				}
			}
			else if (msg.msgRequest == MSGQ_RQST_SAVE_FILE)
			{
				/*快速存盘只是将当前文件做收尾操作,缓冲中未存入的数据不做写盘操作,写盘
				动作留给超时触发*/
				if (appFlag->newFile != 0)
				{
					APP_FILE_SAVE_END_SET(appFlag, chNo);
				}
			}
			else if (msg.msgRequest == MSGQ_RQST_SAVE_DATA)
			{
				#if 1
				/*只存数据,文件尾通过后台的立即存盘按钮实现*/
				if (appFlag->newFile != 0)
				{
					if (appFlag->bufferStatus.currentBuffer == 0)
					{
						if ((len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.buf) > 0)
						{
							appFlag->bufferStatus.bufBStart = appFlag->bufferStatus.buf + MBYTES_10;
							appFlag->bufferStatus.analyz = appFlag->bufferStatus.rear = appFlag->bufferStatus.bufBStart;
						}
					}
					else if (appFlag->bufferStatus.currentBuffer == 1)
					{
						if ((len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.bufBStart) > 0)
						{
							appFlag->bufferStatus.bufCStart = appFlag->bufferStatus.buf + MBYTES_20;
							appFlag->bufferStatus.analyz = appFlag->bufferStatus.rear = appFlag->bufferStatus.bufCStart;
						}
					}
					else if (appFlag->bufferStatus.currentBuffer == 2)
					{
						if ((len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.bufCStart) > 0)
						{
							appFlag->bufferStatus.analyz = appFlag->bufferStatus.rear = appFlag->bufferStatus.buf;
						}
					}


					if (len > 0)
					{
						dm_msgDiscMngSend(MSGQ_RQST_SAVE, NULL, chNo, len, appFlag->bufferStatus.currentBuffer);
						appFlag->bufferStatus.currentBuffer = (appFlag->bufferStatus.currentBuffer + 1) % 3;
					}
				}
				#endif				/*只存数据,文件尾通过后台的立即存盘按钮实现*/
				#if 0
				if (appFlag->newFile != 0)
				{
					app_saveBufferData(APP_SAVE_TIMEOUT, chNo, appFlag->bufferStatus.currentBuffer);
				}
				#endif
			}
			else if (msg.msgRequest == MSGQ_RQST_DRV_IOCTRL)
			{
				/*按照原来代码loop1内容处理UDP控制驱动消息,目前无此UDP端口,后续添加时只需将
				main.c里if(UdpFlag == 1)后面代码拷贝过来即可,但要考虑处理对读取报文和分析效率的影响*/
				/*ioctl(drvFd,xxx,xxx)*/
				/*write(drvFd,xxx,xxx)*/
				/*这里要加锁*/
			}
		}

	#if 0
		if (appFlag->queueFull == 0)
		{
			/*缓冲已满，告警*/
		}
	#endif
	
		#endif

#if 1		
		//printf("APP_MSGBUFF_READ_SET, chNo:%d, no:%d\n", chNo, no);

		#if 1
		/*多通道同时申请fpga资源读数据,用锁互斥*/
	 	if(pthread_mutex_lock(&mutex) != 0)
     		{                      
      			printf("pthread_mutex_lock error\n", strerror(errno));           
     		}                                                       
		else
		{
			APP_MSGBUFF_READ_SET(appFlag->bufferStatus.rear, no, chNo, MsgBuffer[no]);
			len = read(drvFd, appFlag->bufferStatus.rear, CH_BUF_LEN);			/*将完整的消息内容送入Driver*/
		}

		 if(pthread_mutex_unlock(&mutex) != 0)
		 {                    
		 	printf("pthread_mutex_unlock error:%s\n", strerror(errno));                         
   		 }                
		#endif

		/*测试用*/
		//continue;

		/*返回的len是四字节的倍数*/
		if (len == 0)
		{
			#if 1
			//没读到数据,原代码处理UDP消息，此功能挪至上边处理,这里不需要再做动作
			idleTimes++;
			/*读了500次都没数据,存盘*/
			if ((idleTimes % 5000) == 0)
			{
			//	printf("chNo:%d,idleTimes:%d\n", chNo, idleTimes);
			}
			
			if ((idleTimes > 5000) && (idleTimes <= 15000))
			{
				if (appFlag->networkStatus == 1)
				{
					appFlag->networkStatus = 0;	
			//		printf("chNo:%d,send msg to APP , 100 times no packets\n", chNo);
					app_msgSend(MSGQ_RQST_SAVE_DATA, 0, chNo);
				}
			}
			else if (idleTimes > 15000)				/*等了15秒还是没数据过来,结束当前文件存储,加文件尾*/
			{
				if (appFlag->newFile != 0)
				{
			//		printf("chNo:%d,1500 times no packets, end file\n", chNo);
					app_msgSend(MSGQ_RQST_SAVE_FILE, 0, chNo);
				}
			}
			#endif
			usleep(200);						/*没读到数据就延迟10ms,释放cpu资源*/
			continue;
		}
		else if (len >0)
		{
			#if 0
			/*测试读上来的数据*/
			int i;
			for(i = 0; i < len; i++) 
			{
				if((i % 0x04) == 0) printf("\n%04x:", i);	
					printf(" %08x", appFlag->bufferStatus.rear[i]);	
			}
			//fflush(stdout);		
			printf("\n");
			#endif
			#if 1
			/*网络恢复,又读到数据了,清状态*/
			if (idleTimes > 0)
			{
				idleTimes = 0;

				if (appFlag->networkStatus == 0)
				{
					appFlag->networkStatus = 1;	
				}
			}
			#endif
			
			appFlag->bufferStatus.rear += len;
			parseFifoFrameData(chNo);					//分析包数,看大小
		}
		else
		{
			#if 1
			/*读驱动错误，关闭重新打开*/
			APP_DEBUG("driver error ,chNo:%d, reopen ---lineNo:%d\n", chNo, __LINE__);
			/*处理异常同样先上锁*/
		 	if(pthread_mutex_lock(&mutex) != 0)
	     		{                      
	      			APP_DEBUG("pthread_mutex_lock chNo:%d,error:%s\n", chNo, strerror(errno));                           
	     		}                                                       
	
			close(drvFd);
			
			if ((drvFd = open(READ_DRIVER, O_RDONLY)) == -1)
			{
				APP_DEBUG("pci driver reopen failed,chNo:%d, check error:%s\n",chNo, strerror(errno));
			}		

			 if(pthread_mutex_unlock(&mutex) != 0)
			 {                    
			 	APP_DEBUG("pthread_mutex_unlock chNo:%d, error:%s\n", chNo, strerror(errno));                         
	   		 } 
			 
			#endif
		}
#endif	

#if 1
		/*此时肯定在第三区,并需要存盘了,缓冲区尾,如果此时1区还没存完,跳过读驱动操作*/
		if((appFlag->bufferStatus.rear - appFlag->bufferStatus.buf) > (MBYTES_30))
		{
			//APP_DEBUG("((appFlag->bufferStatus.buf + MAXQSIZE - appFlag->bufferStatus.rear) < (MSGMAXLEN*1024))\n");
			if (appFlag->bufferStatus.bufStatus[0] == 1)
			{
				/*缓冲已满,1区没存完,告警跳过读*/
				APP_DEBUG("chNo:%d, bufA overflow , warning ---lineNo:%d\n", chNo, __LINE__);
				printf("chNo:%d, bufA overflow , warning ---lineNo:%d\n", chNo, __LINE__);
				/*尾巴的半包拷到1区里面去*/
				memcpy(appFlag->bufferStatus.buf, appFlag->bufferStatus.analyz, (appFlag->bufferStatus.rear - appFlag->bufferStatus.analyz)*4);
				offsetLen = appFlag->bufferStatus.rear - appFlag->bufferStatus.analyz;
				
				appFlag->bufferStatus.analyz = appFlag->bufferStatus.buf;
				appFlag->bufferStatus.rear = appFlag->bufferStatus.buf + offsetLen;
				//appFlag->bufferStatus.rear = appFlag->bufferStatus.buf;
				appFlag->bufferStatus.currentBuffer = 0;
			}
			else
			{
	//			app_saveBufferData(APP_SAVE_NORMAL, chNo, appFlag->bufferStatus.currentBuffer);
				len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.bufCStart;
				dm_msgDiscMngSend(MSGQ_RQST_SAVE, NULL, chNo, len, 2);
				appFlag->totalLength += len;		
				appFlag->bufferStatus.bufStatus[2] = 1;

				/*尾巴的半包拷到1区里面去*/
				memcpy(appFlag->bufferStatus.buf, appFlag->bufferStatus.analyz, (appFlag->bufferStatus.rear - appFlag->bufferStatus.analyz)*4);
				offsetLen = appFlag->bufferStatus.rear - appFlag->bufferStatus.analyz;
				
				appFlag->bufferStatus.analyz = appFlag->bufferStatus.buf;
				appFlag->bufferStatus.rear = appFlag->bufferStatus.buf + offsetLen;
				//appFlag->bufferStatus.rear = appFlag->bufferStatus.buf;
				appFlag->bufferStatus.currentBuffer = 0;
				APP_DEBUG("chNo:%d, bufC save data  ---lineNo:%d\n", chNo, __LINE__);			
			}
		}
		/*此时数据在第三区,满足第二区数据的存储条件,判断2区数据是否已存,没存就发消息存储*/
		else if((appFlag->bufferStatus.rear - appFlag->bufferStatus.buf) > (MBYTES_20))
		{
			//APP_DEBUG("(appFlag->bufferStatus.rear - appFlag->bufferStatus.buf) >= MBYTES_20\n");
			if (appFlag->bufferStatus.bufStatus[2] == 1)
			{
				/*缓冲已满,3区没存完,告警跳过读*/
				APP_DEBUG("chNo:%d, bufC overflow , warning ---lineNo:%d\n", chNo,  __LINE__);
				printf("chNo:%d,bufC overflow , warning ---lineNo:%d\n", chNo, __LINE__);

				appFlag->bufferStatus.bufStatus[1] = 1;
				appFlag->bufferStatus.bufCStart = appFlag->bufferStatus.analyz;
				appFlag->bufferStatus.currentBuffer = 2;
			}
			else
			{
				/*如果2区数据还没存,先存了2区数据*/
			//	app_saveBufferData(APP_SAVE_NORMAL, chNo, appFlag->bufferStatus.currentBuffer);
				if (appFlag->bufferStatus.currentBuffer == 1)
				{
					len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.bufBStart;
					dm_msgDiscMngSend(MSGQ_RQST_SAVE, NULL, chNo, len, 1);
					appFlag->totalLength += len;		
					appFlag->bufferStatus.bufStatus[1] = 1;
					appFlag->bufferStatus.bufCStart = appFlag->bufferStatus.analyz;
					appFlag->bufferStatus.currentBuffer = 2;
					APP_DEBUG("chNo:%d, bufB save data  ---lineNo:%d\n", chNo, __LINE__);
				}
			}
		}
		/*此时数据在第二区,满足第一区数据的存储条件,判断1区数据是否已存,没存就发消息存储*/
		else if ((appFlag->bufferStatus.rear - appFlag->bufferStatus.buf) > (MBYTES_10))
		{
			if (appFlag->bufferStatus.bufStatus[1] == 1)
			{
				/*缓冲已满,2区没存完,告警跳过读*/
				APP_DEBUG("chNo:%d, bufB overflow , warning ---lineNo:%d\n", chNo, __LINE__);
				printf("chNo:%d, bufB overflow , warning ---lineNo:%d\n", chNo, __LINE__);
				appFlag->bufferStatus.bufStatus[0] = 1;
				appFlag->bufferStatus.bufBStart = appFlag->bufferStatus.analyz;
				appFlag->bufferStatus.currentBuffer = 1;
			}
			else
			{
				/*如果1区数据还没存,先存了一区数据*/
			//	app_saveBufferData(APP_SAVE_NORMAL, chNo, appFlag->bufferStatus.currentBuffer);
				if (appFlag->bufferStatus.currentBuffer == 0)
				{
					len = appFlag->bufferStatus.analyz - appFlag->bufferStatus.buf;	/*AStart就是buf*/
					dm_msgDiscMngSend(MSGQ_RQST_SAVE, NULL, chNo, len, 0);
					appFlag->totalLength += len;		
					appFlag->bufferStatus.bufStatus[0] = 1;
					appFlag->bufferStatus.bufBStart = appFlag->bufferStatus.analyz;
					appFlag->bufferStatus.currentBuffer = 1;
					APP_DEBUG("chNo:%d, bufA save data  ---lineNo:%d\n", chNo, __LINE__);
				}			
			}
		}
		/*看缓冲繁忙情况,判断是否打开停止分析开关,任意两个缓冲在忙就关分析*/
		if (((appFlag->bufferStatus.bufStatus[0] == 1) &&(appFlag->bufferStatus.bufStatus[1] == 1)) ||
			((appFlag->bufferStatus.bufStatus[0] == 1) && (appFlag->bufferStatus.bufStatus[2] == 1)) ||
			((appFlag->bufferStatus.bufStatus[1] == 1) && (appFlag->bufferStatus.bufStatus[2] == 1)))
		{
			appFlag->stopParse = 1;
			printf("chNo:%d,buf busy , stop parse ---lineNo:%d\n", chNo, __LINE__);		
			#if 0
			printf("buf 0:%d, 1:%d, 2:%d , stop parse ---lineNo:%d\n", appFlag->bufferStatus.bufStatus[0], 
															appFlag->bufferStatus.bufStatus[1],
															appFlag->bufferStatus.bufStatus[2],
															__LINE__);			
			#endif
		}

		if (appFlag->totalLength >= singleFileLengthMin)
		{
			APP_FILE_SAVE_END_SET(appFlag, chNo);			
			/*APP_DEBUG("SAVE END, xml_getConfigDevFileS:%d\n",xml_getConfigDevFileS());*/
		}
	}
#endif
	return;
}

/*初始化,创建消息队列,分配内存*/
int app_Init(pthread_t *id1, pthread_t *id2)
{
	key_t key;
	int chNo[2];
	int ret;
	memset(&AppFlag, 0, sizeof(AppFlag));

	#if 1
	chNo[0] = 1;
	AppFlag[0].bufferStatus.buf = (unsigned int *)malloc(sizeof(unsigned int) * MAXQSIZE);

	if (AppFlag[0].bufferStatus.buf == NULL)
	{
		APP_DEBUG("malloc failed, exit system\n");
		exit(-1);
	}

	AppFlag[0].bufferStatus.analyz = AppFlag[0].bufferStatus.buf;
	AppFlag[0].bufferStatus.rear = AppFlag[0].bufferStatus.buf;
	#endif

	#if 1
	chNo[1] = 2;
	AppFlag[1].bufferStatus.buf = (unsigned int *)malloc(sizeof(unsigned int) * MAXQSIZE);

	if (AppFlag[1].bufferStatus.buf == NULL)
	{
		APP_DEBUG("malloc failed, exit system\n");
		exit(-1);
	}

	AppFlag[1].bufferStatus.analyz = AppFlag[1].bufferStatus.buf;
	AppFlag[1].bufferStatus.rear = AppFlag[1].bufferStatus.buf;
	#endif

	printf("mutext init\n");
	if (pthread_mutex_init(&mutex,NULL) != 0)
	{
		printf("pthread_mutex_init failed errno:%s\n", strerror(errno));
		exit(-1);
	}
	
	drvFd = open(READ_DRIVER, O_RDONLY);				//打开驱动程序
	if (drvFd == -1)
	{
		printf("TY-3012S App open /dev/pcifpga error!\n");	//主程序使用App作为标识
		exit(-1);					//严重问题,驱动程序丢失
	}

	{
		int len;
		MsgFpgaTime buf;

		buf.Head = MSGHEAD;
		buf.VersionSid = MSGSESSIONID;
		buf.Id = CHECK_FPGA_TIMER;
		buf.Tail = MSGTAIL;
	
		len = read(drvFd, (unsigned char *)&buf, CH_BUF_LEN);			/*将完整的消息内容送入Driver*/
	}
	
	if ((AppQid = msgget(500, 0666 | IPC_CREAT)) == -1)
	{
		APP_DEBUG("msgget failed, line:%d\n", __LINE__);
		exit(-1);
	}
	printf("AppQid:%d\n", AppQid);
	
	sleep(1);
	#if 1
	if((ret = pthread_create(id1, NULL, app_PacketRead, (void *)&chNo[0])) != 0 ) 
	{
		DM_DEBUG("\nTY-3012S creats app_Init 1 thread failed!\n");
		return ret;
	}
	#endif
	
	sleep(1);
	#if 1
	if((ret = pthread_create(id2, NULL, app_PacketRead, (void *)&chNo[1])) != 0 ) 
	{
		DM_DEBUG("\nTY-3012S creats app_Init 2 thread failed!\n");
		return ret;
	}
	#endif
	return;
}



