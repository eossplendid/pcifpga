/* 1、装置的Xml配置文件下载、上载、版本查询程序
   2、装置的IDPROM配置文件的下载程序
   3、装置扫描程序
   4、装置Xml配置文件的解码程序
   5、装置的状态查询程序
*/

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <malloc.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "backStage.h"
#include "common.h"
#include "discManager.h"
#include "appReadPacket.h"


static int SvUploadQid;			/*sv上传消息队列ID号*/
static int SvQueueFull = 0;			/*sv上传队列满标志,0为可用,1为满*/

extern int errno;
static int longLinkSockFd;		/*长连接socket描述符*/

unsigned char	XmlFileBuf[MSG_MAX_LEN];	//设置Xml文件缓冲区，一直保留Xml配置文件
unsigned char	XmlMsgBuff[MSG_MAX_LEN];	//设置请求消息缓冲区，用于存储消息命令和下载的配置文件
unsigned char	XmlResBuff[MSG_RES_LEN];	//设置消息应答缓冲区，设置消息的应答区

unsigned int	XmlFlieTime = 0;		//设置Xml文件的配置时间，应从Xml配置文件中解码出来

/*add by haopeng 2012.11.6*/
static int UploadCancelled = 0;			/*停止上传标志*/
//static UploadStatus TransferStatus;		/*所有上传线程的状态,记录是否正在上传*/
static unsigned int	links[MAX_LINK_TYPES];				//记录三类tcp链接个数,0为长连接,1为配置连接,2为sv上传,3为录播上传
/**************************************/

//static pthread_t	xml_tcp_thread;

//建立一个TPC通信线程，为了下载或上载系统的配置文件
#if 0
short int Htons(short int value)
{
	return ((value & 0xff) << 8) + ((value & 0xffff) >> 8); 
}
#endif

/*取消录播文件上传,置标记位为1*/
void bs_setUploadCancelled()
{	
	UploadCancelled = 1;
	return;
}

/*录播文件和文件记录数据上传*/
int bs_transSocketSentData(int fd, unsigned long size, unsigned short cmd, unsigned int session, int sockfd)
{
	unsigned long len;
	unsigned char buf[MSG_MAX_LEN];
	unsigned short tail1,tail2;
	RecFileTransRsp msg;

	UploadCancelled = 0;
	#if 1
	
	msg.header.head= Htons(0xF6F62828);
	msg.header.msgId = Htons(cmd);
	session++;
	msg.header.session1 = Htons(session & 0x0ffff);
	msg.header.session2 = Htons(session / 0x10000);	

	msg.tail1= Htons(0x0909);
	msg.tail2 = Htons(0xD7D7);

	#endif

	msg.packets_count = size / (32*1024) + 1;

//	msg.packets_count = 0x01;
	msg.current_packet = 0;

	if (fd == -1)
	{
		msg.packets_count = 0;
		msg.data_len = 0;
		if (write(sockfd, (unsigned char *)&msg, sizeof(RecFileTransRsp)) <= 0)
		{
			close(sockfd);
		}
		return -1;
	}
	
	BS_DEBUG("bs_transSocketSentRecData, count:%d\n", msg.packets_count);
	//TransferStatus.recStatus = 1;
	while((len = read(fd, buf, 0x8000)) > 0)	/*32k*/
	{
		msg.data_len = len;
		msg.header.length = Htons(len+16);
		if (UploadCancelled == 1)
		{
			msg.header.msgId = Htons(MS_FILETRANS_FILE_UPLOAD_CANCEL_ACK);
			msg.header.length = Htons(0x0006);
			write(sockfd, (unsigned char *)&msg, sizeof(MsgTransHeader));
			write(sockfd, (unsigned char *)&msg.tail1, 4);
			break;
		}
		write(sockfd, (unsigned char *)&msg, sizeof(MsgTransHeader)+10);
		//write(sockfd, (unsigned char *)&msg.current_packet, 6);
		write(sockfd, buf, len);
		write(sockfd, (unsigned char *)&msg.tail1, 4);
		msg.current_packet++;
		session++;
		msg.header.session1 = Htons(session & 0x0ffff);
		msg.header.session2 = Htons(session / 0x10000);	
	}

	//TransferStatus.recStatus = 0;

	BS_DEBUG("bs_transSocketSentRecData done\n");
	
	return 0;	
}


/*文件信息响应,传录播文件之前先回文件信息响应给后台*/
int bs_transSocketSentFileInfo(int status, unsigned long size, unsigned int session, int sockfd)
{
	RecFileInfoResponse * rsp;
	unsigned char RecResBuff[MSG_MAX_LEN];
	
	rsp = (RecFileInfoResponse *)RecResBuff;

	rsp->header.head = Htons(0xF6F62828);
	rsp->header.length = Htons(0x000e);
	rsp->header.msgId = Htons(MS_FILETRANS_FILE_INFORMATION);
	
	session++;
	rsp->header.session1 = Htons(session & 0x0ffff);
	rsp->header.session2 = Htons(session / 0x10000);
	
	if (status > 0)
	{
		rsp->file_size = Htons(size);
		rsp->fault_reason = 0;
	}
	else
	{
		rsp->file_size = 0;
		rsp->fault_reason = 1;
	}
	rsp->tail1 = Htons(0x0909);
	rsp->tail2 = Htons(0xD7D7);
	
	if (write(sockfd, RecResBuff, sizeof(RecFileInfoResponse)) <= 0) 
	{
		close(sockfd);
		printf("TY-3012S RecFileInfoResponse write ClientXmlSockfd error: %d!\n", sockfd);
		return -1;
	}
	
	return 0;
}

#if 0
static void *recordFileTcp()
{
	struct sockaddr_in my_tcpserver_addr;
	int addr_len = sizeof(struct sockaddr_in);
	int XmlSockfd; 
	unsigned char RecMsgBuff[MSG_MAX_LEN];
	unsigned char RecResBuff[MSG_RES_LEN];
	RecMsgCommand		*ServerRecMsgCmd;
	RecConnResponse		*ServerRecConnResponse;
	RecListCommand		*ServerRecListCmd;
	CommonMsg 			*cmd, *rsp;

	#if 0
	XmlVerResponse		*ServerXmlVerResponse;
	XmlDownloadResponse	*ServerXmlDownloadResponse;
	XmlDownloadFinish	*ServerXmlDownloadFinish;
	XmlUploadResponse	*ServerXmlUploadResponse;

	int		XmlFlieLen;
	#endif
	unsigned char	MsgTail0, MsgTail1, MsgTail2, MsgTail3;	//下载Xml配置文件时，解码针的尾部，第一字节到第四字节
	unsigned short	MsgLength, MsgReserve, MsgId;
	unsigned int 	MsgHead, MsgSession, MsgTail;
	int		i, j, ret;

	ServerRecMsgCmd = (RecMsgCommand *)RecMsgBuff;
	ServerRecListCmd = (RecListCommand *)RecMsgBuff;
	cmd = (CommonMsg *)RecMsgBuff;
	rsp = (CommonMsg *)RecResBuff;
	ServerRecConnResponse = (RecConnResponse *)RecResBuff;
//	ServerXmlVerResponse		= (XmlVerResponse *)XmlResBuff;
//	ServerXmlDownloadResponse	= (XmlDownloadResponse *)XmlResBuff;
//	ServerXmlDownloadFinish		= (XmlDownloadFinish *)XmlResBuff;
//	ServerXmlUploadResponse		= (XmlUploadResponse *)XmlResBuff;
	memset(RecMsgBuff, 0, MSG_MAX_LEN);
	memset(RecResBuff, 0, MSG_RES_LEN);
	ret = 0;
	XmlSockfd = socket(AF_INET, SOCK_STREAM, 0);	//建立一个TCP服务器
	if (XmlSockfd < 0) {
		printf("TY-3012S ConfigXmlFile creates XmlSockfd error!\n");
		exit(1);	//系统严重错误，需要复位处理!
		ret = -1;
	}
	printf("TY-3012S ConfigXmlFile creates XmlSocket successful: %d!\n", XmlSockfd);
	bzero(&my_tcpserver_addr, sizeof(my_tcpserver_addr));
	my_tcpserver_addr.sin_family = AF_INET;
	my_tcpserver_addr.sin_port = htons(recTcpPort);
	my_tcpserver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(XmlSockfd, (struct sockaddr *)&my_tcpserver_addr, sizeof(my_tcpserver_addr)) < 0) {
		printf("TY-3012S ConfigXmlFile binds XmlSocket error!\n");
		exit(1);	//系统严重错误，需要复位处理!
		ret = -2;
	}
	printf("TY-3012S ConfigXmlFile binds XmlSocket successful: %d, %d!\n", recTcpPort, my_tcpserver_addr.sin_port);
	if(listen(XmlSockfd, 3) < 0) {	//侦听队列的长度=3!
		printf("TY-3012S ConfigXmlFile listens XmlSocket error!\n");
		exit(1);	//系统严重错误，需要复位处理!
		ret = -3;
	}
	printf("TY-3012S ConfigXmlFile listens XmlSocket successful: %d!\n", 3);
	//等待客户端的连接请求
	while(1) {
		if((ClientRecSockfd = accept(XmlSockfd, (struct sockaddr *)&my_tcpserver_addr, &addr_len)) < 0) {
			printf("TY-3012S ConfigXmlFile accepts ClientXmlSockfd error!\n");
			exit(EXIT_FAILURE);	//系统严重错误，需要复位处理!
			ret = -4;
		} else {
			//使用阻塞型的数据接收，节省系统资源
			//fcntl(ClientXmlSockfd, F_SETFL, O_NONBLOCK);
			printf("TY-3012S ConfigXmlFile accepts ClientXmlSockfd successful: %d!\n", ClientRecSockfd);
		}
		
		while(1) {
			//先读取消息的头部，共计12字节
			if (read(ClientRecSockfd, RecMsgBuff, sizeof(MsgTransHeader)) <= 0) {
				close(ClientRecSockfd);	//读取消息头部14字节出现错误，关闭连接，并重新建立连接!
				printf("TY-3012S ConfigXmlFile read ClientXmlSockfd error1: %d!\n", ClientRecSockfd);
				break;
			}

			#if 0
			printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgHead:     %08x!\n", ServerXmlMsgCommand->MsgHead);
			printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgLength:   %08x!\n", ServerXmlMsgCommand->MsgLength);
			printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgSession1:  %08x!\n", ServerXmlMsgCommand->MsgSession1);
			printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgSession2:  %08x!\n", ServerXmlMsgCommand->MsgSession2);
			printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgId:       %08x!\n", ServerXmlMsgCommand->MsgId);
			#endif

			MsgHead = Ntohs(ServerRecMsgCmd->header.head);
						
			MsgLength= Ntohs(ServerRecMsgCmd->header.length);
			MsgId = Ntohs(ServerRecMsgCmd->header.msgId);
			MsgSession = Ntohs(ServerRecMsgCmd->header.session2 * 0x10000) + Ntohs(ServerRecMsgCmd->header.session1);

			#if 0
			printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgHead:     %08x!\n", ServerXmlMsgCommand->MsgHead);
			printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgLength:   %08x!\n", ServerXmlMsgCommand->MsgLength);
			printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgId:       %08x!\n", ServerXmlMsgCommand->MsgId);
			#endif
			#if 1
			printf("TY-3012S recordFileTcp ServerXmlMsgCommand->MsgHead:     %08x!\n", MsgHead);
			printf("TY-3012S recordFileTcp ServerXmlMsgCommand->MsgLength:   %08x!\n", MsgLength);
			printf("TY-3012S recordFileTcp ServerXmlMsgCommand->MsgSession:  %08x!\n", MsgSession);
			printf("TY-3012S recordFileTcp ServerXmlMsgCommand->MsgId:       %08x!\n", MsgId);
			#endif
			//对消息的头部进行解码
			//消息头定位字节错误，关闭连接
			if(MsgHead != 0xF6F62828) {
				close(ClientRecSockfd);	//消息的定位帧头4字节出现错误，关闭连接，并重新建立连接!
				printf("TY-3012S recordFileTcp decodes head error1: %d!\n", ClientRecSockfd);
				break;
			}
			//消息保留位字节错误，关闭连接
			#if 0
			if(MsgReserve != 0x0000) {
				close(ClientXmlSockfd);	//消息的保留位2字节出现错误，关闭连接，并重新建立连接!
				printf("TY-3012S ConfigXmlFile decodes reserve error1: %d!\n", ClientXmlSockfd);
				break;
			}
			#endif
			
			//根据消息执行相关命令
			//1、MS_CFG_CONN_REQ	=0x08，配置文件连接请求；MS_CFG_CONN_RESP	=0x09，配置文件连接响应
			//2、MS_CFG_VER_QRY	=0x10，配置文件版本查询；MS_CFG_VER_RESP	=0x11，配置文件查询响应；
			//3、MS_CFG_DOWN_REQ	=0x12，配置文件下载请求；MS_CFG_DOWN_RESP	=0x13，配置文件下载响应；
			//4、MS_CFG_DOWNLOAD	=0x14，配置文件下载开始；MS_CFG_DOWNLOAD_ACK	=0x15，配置文件下载完成；
			//5、MS_CFG_UPLOAD_REQ	=0x16，配置文件上载请求；MS_CFG_UPLOAD		=0x17，配置文件上载开始；
			switch(MsgId) {
				case FILETRANS_CONNECT_REQ:
				{
					if (read(ClientRecSockfd, (RecMsgBuff + HEADER_LEN), 8) <= 0) {
						close(ClientRecSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
						printf("TY-3012S recordFileTcp read ClientXmlSockfd error2: %d!\n", ClientRecSockfd);
						break;
					}

					if (ServerRecMsgCmd->conn_type == 1)
					{
						printf("TY-3012S conn_type:1!\n");
					}
					else if(ServerRecMsgCmd->conn_type == 2)
					{
						printf("TY-3012S conn_type:2!\n");
					}
					else if (ServerRecMsgCmd->conn_type == 3)
					{
						printf("TY-3012S conn_type:3!\n");
					}
					else
					{
						printf("TY-3012S conn_type:unkown!\n");
					}
					
					MsgTail = Ntohs(ServerRecMsgCmd->tail1 * 0x10000) + Ntohs(ServerRecMsgCmd->tail2);
					//消息尾定位字节错误，关闭连接
					printf("TY-3012S recordFileTcp ServerRecMsgCmd->MsgTail:     %08x!\n\n", MsgTail);
					if(MsgTail != 0x0909D7D7) {
						close(ClientRecSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
						printf("TY-3012S recordFileTcp decodes tail error1: %d!\n", ClientRecSockfd);
						break;
					}
					
					ServerRecConnResponse->header.head = Htons(0xF6F62828);
					ServerRecConnResponse->header.length = Htons(0x000a);
					ServerRecConnResponse->header.msgId = Htons(FILETRANS_CONNECT_RSP);
					MsgSession++;
					ServerRecConnResponse->header.session1 = Htons(MsgSession & 0x0ffff);
					ServerRecConnResponse->header.session2 = Htons(MsgSession / 0x10000);
					ServerRecConnResponse->result = 1;
					ServerRecConnResponse->tail1 = Htons(0x0909);
					ServerRecConnResponse->tail2 = Htons(0xD7D7);
					//将响应消息发送到ClientXmlSockfd
					if (write(ClientRecSockfd, RecResBuff, sizeof(RecConnResponse)) <= 0) {
						close(ClientRecSockfd);
						printf("TY-3012S ConfigXmlFile write ClientXmlSockfd error: %d!\n", ClientRecSockfd);
						break;
					}

				}
					break;

				case FILETRANS_FILE_LIST_QRY:
				{
					if (read(ClientRecSockfd, (RecMsgBuff + HEADER_LEN), 24) <= 0) {
						close(ClientRecSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
						printf("TY-3012S recordFileTcp read ClientXmlSockfd error2: %d!\n", ClientRecSockfd);
						break;
					}

					MsgTail = Ntohs(ServerRecListCmd->tail1 * 0x10000) + Ntohs(ServerRecListCmd->tail2);
					printf("TY-3012S recordFileTcp ServerRecListCmd->MsgTail:     %08x! chNo:%d\n", MsgTail, ServerRecListCmd->channelNo);
					if(MsgTail != 0x0909D7D7) {
						close(ClientRecSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
						printf("TY-3012S recordFileTcp decodes tail error1: %d!\n", ClientRecSockfd);
						break;
					}
					
					dm_msgDiscMngSend(MSGQ_RQST_QUERY, NULL, ServerRecListCmd->channelNo, MsgSession, ClientRecSockfd);
					
				}
					break;


				case FILETRANS_FILE_UPLOAD_REQ:
				{
					FILETRANS_FILE_INFORMATION;
					FILETRANS_FILE_UPLOAD;
				}
					break;

				case FILETRANS_FILE_UPLOAD_CANCEL:
				{
					if (read(ClientRecSockfd, (RecMsgBuff + HEADER_LEN), 4) <= 0) {
						close(ClientRecSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
						printf("TY-3012S recordFileTcp read ClientXmlSockfd error2: %d!\n", ClientRecSockfd);
						break;
					}

					
					MsgTail = Ntohs(cmd->tail1 * 0x10000) + Ntohs(cmd->tail2);
					//消息尾定位字节错误，关闭连接
					printf("TY-3012S recordFileTcp ServerRecMsgCmd->MsgTail:     %08x!\n\n", MsgTail);
					if(MsgTail != 0x0909D7D7) {
						close(ClientRecSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
						printf("TY-3012S recordFileTcp decodes tail error1: %d!\n", ClientRecSockfd);
						break;
					}
					
					rsp->header.head = Htons(0xF6F62828);
					rsp->header.length = Htons(0x000a);
					rsp->header.msgId = Htons(FILETRANS_FILE_UPLOAD_CANCEL_ACK);
					MsgSession++;
					rsp->header.session1 = Htons(MsgSession & 0x0ffff);
					rsp->header.session2 = Htons(MsgSession / 0x10000);
					rsp->tail1 = Htons(0x0909);
					rsp->tail2 = Htons(0xD7D7);
					//将响应消息发送到ClientXmlSockfd
					if (write(ClientRecSockfd, RecResBuff, sizeof(RecConnResponse)) <= 0) 
					{
						close(ClientRecSockfd);
						printf("TY-3012S ConfigXmlFile write ClientXmlSockfd error: %d!\n", ClientRecSockfd);
						break;
					}
				}
					break;

				default:
					close(ClientRecSockfd);
					printf("TY-3012S ConfigXmlFile decodes command error1: %d!\n", ClientRecSockfd);
					sleep(1);
					break;
			}
		}
	}
}
#endif


int bs_svMsgSend(MsgSvInfo msg)
{
	struct msqid_ds info;
	msg.mtype = TYPE_SVUPLOAD;
	//msg.request = MSG_RQST_SVUPLOAD;

	if (SvQueueFull == 1)
	{
		/*速率不匹配,sv队列已满*/
		if (msgctl(SvUploadQid, IPC_STAT, &info) < 0) 
		{
			BS_DEBUG("SvUploadQid msgctl failed, errno:%d, %s, ---lineNo:%d\n", errno, strerror(errno), __LINE__); 
		}

		if (info.msg_cbytes == 0)		/*SV队列读空了*/
		{
			SvQueueFull = 0;
		}
		
		return -1;
	}


	msg.chNo = app_getSvConChNo();
	
	if (msgsnd(SvUploadQid, &msg, (sizeof(MsgSvInfo)-sizeof(long)), IPC_NOWAIT) < 0)
	{
		SvQueueFull = 1;
		BS_DEBUG("msgsnd(SvUploadQid) failed errno:%s  ---lineNo:%d\n", strerror(errno), __LINE__);
		if (msgctl(SvUploadQid, IPC_STAT, &info) < 0) 
		{
			BS_DEBUG("SvUploadQid msgctl failed, errno:%d, %s, ---lineNo:%d\n", errno, strerror(errno), __LINE__); 
		}

		BS_DEBUG("queue bytes:%d, maxBytes:%d, msgNum:%d\n", info.msg_cbytes, info.msg_qbytes, info.msg_qnum);
	}
	
	return 0;
}


/*sv数据实时上传*/
int svPacketDataUpload(int sockfd)
{
	/*根据消息队列收到的消息进行数据上传*/
	MsgSvInfo msg;
	int i;
	struct msqid_ds info;

	/*这里再使能上传,不然队列会直接溢出*/
	app_setSvEnable(1);

	#if 0
	/*测试速度用*/
	{
		int fd, cnt;
		unsigned char *buf;
		unsigned long len, total = 0;
		struct timeval time_start, time_end; 
		long start,end,diff;

		buf = (unsigned char *)malloc(3*1024*1024);
		fd = open("/sda/diskF/file", O_RDONLY);
		if (fd == -1)
		{
			printf("test:open file error ---lineNo:%d\n", __LINE__);
			return -1;
		}

		cnt = 0;
		while (1)
		{
			lseek(fd, 0, SEEK_SET);
			read(fd, buf, 28);
			buf[28] = '\0';
			printf("28 buf:%s\n", buf);
			
			gettimeofday(&time_start, NULL);
			start = time_start.tv_sec * 1000 + time_start.tv_usec / 1000;
			
			while ((len = read(fd, buf, 3*1024*1024)) > 0)
			{
				total += len;
				cnt++;
				write(sockfd, buf, len);
				printf("cnt:%d\n", cnt);
				sleep(1);
			}

			gettimeofday(&time_end, NULL);
			end = time_end.tv_sec * 1000 + time_end.tv_usec / 1000;
			diff = end - start;

			printf("write socket bytes:%ld bytes, time:%dms\n", total, diff);
			total = 0;
			diff = 0;
		}
	}

	/******************************************************/
	#endif
	
	while(1)
	{
		if (msgrcv(SvUploadQid, &msg, (sizeof(MsgSvInfo)-sizeof(long)), TYPE_SVUPLOAD, 0) == -1)
		{
			BS_DEBUG("SvUploadQid msgrcv failed , line:%d\n", __LINE__);
			continue;
		}		

		switch(msg.request)
		{
			case MSG_RQST_SVUPLOAD:
			{
				if (app_getSvEnable() == 0)
				{
					BS_DEBUG("app_getSvEnable() == 0 ---lineNo:%d\n", __LINE__);
					BS_SV_NEED_TO_RETURN(SvUploadQid, info);
					break;
				}
				
				for(i = 0; i < msg.count; i++)
				{					
					if (msg.packet[i] != NULL)
					{
						if (write(sockfd, msg.packet[i], msg.length[i]) < 0)
						{
							BS_DEBUG("SvUpload write socket failed, errno:%s ---lineNo:%d\n", strerror(errno), __LINE__);
							app_setSvEnable(0);
							close(sockfd);

							BS_SV_NEED_TO_RETURN(SvUploadQid, info);				
							break;
							/*SV的SOCKET异常后关闭socket,后台重发连接,这里继续运行,把没读完的读完*/
						}
					}		
				}
			}
				break;

			case MSG_RQST_SVUPLOAD_STOP:
			{
				app_setSvEnable(0);
				close(sockfd);
				return 0;
			}
				break;

			default:
				break;
		}
	}

	return 0;
}

/*告警上报消息发送*/
void bs_WarningMsgSend(unsigned long errWd, WarningMsg *info)
{
	static unsigned long session = 0;
	AlarmReportReq warning;
	
	session++;
	warning.header.head = Htons(0xF6F62828); 
	warning.header.length = Htons(0x0026); 
	warning.header.msgId = Htons(MS_DEVI_ALARM_REQ); 
	warning.header.session1 = Htons(session & 0x0ffff); 
	warning.header.session2 = Htons(session / 0x10000); 
	warning.chNo = info->chNo;
	warning.appId = info->appId;

	warning.dataTime1 = info->dataTime1;
	warning.dataTime2 = info->dataTime2;

	//warning.dataTime1 = 3600*24*30;
	//warning.dataTime2 = 0;

	memcpy(warning.dstMac, info->dstMac, MAX_MAC_LEN);
	memcpy(warning.srcMac, info->srcMac, MAX_MAC_LEN);

	//printf("time1:%08x, time2:%08x\n", warning.dataTime1, warning.dataTime2);
	//memset(warning.dstMac, 0x12, 6);
	//memset(warning.srcMac, 0xa0, 6);
	warning.almCatg = info->almCatg;
	warning.msgAckTag = 0;							/*应答标志可能以后是从配置取的,现在暂定全不需要应答*/
	warning.MsgAlarmCode = errWd;
	warning.tail1 = Htons(0x0909); 
	warning.tail2 = Htons(0xD7D7); 
	
	BS_DEBUG("send warnings\n");
	if (write(longLinkSockFd, (unsigned char *)&warning, sizeof(AlarmReportReq)) < 0) 
	{ 
//		close(Sockfd); 
		printf("TY-3012S bs_svWarningMsgSend  error: %d! %s\n", longLinkSockFd, strerror(errno)); 
	} 

	return;
}

/*各种TCP连接总处理流程,包含配置,录播文件管理,长连接三种*/
static void *managerServerTcp(void *arg)
{
	int ClientSockfd;
	unsigned char *RecMsgBuff, *RecResBuff;
	unsigned char *XmlMsgBuff, * XmlResBuff;
	RecMsgCommand		*ServerRecMsgCmd;
	RecConnResponse		*ServerRecConnResponse;
	RecListCommand		*ServerRecListCmd;
	CommonMsg 			*cmd, *rsp;
	RecFileCommand		*ServerFileCmd;
		
	unsigned char	MsgTail0, MsgTail1, MsgTail2, MsgTail3;	
	unsigned short	MsgLength, MsgReserve, MsgId;
	unsigned int 	MsgHead, MsgSession, MsgTail;
	int		i, j, ret, conntype;

/*********/
	int XmlSockfd, XmlFilefd;
	XmlMsgCommand		*ServerXmlMsgCommand;
	XmlConnResponse		*ServerXmlConnResponse;
	XmlVerResponse		*ServerXmlVerResponse;
	XmlDownloadResponse	*ServerXmlDownloadResponse;
	XmlDownloadFinish	*ServerXmlDownloadFinish;
	XmlUploadResponse	*ServerXmlUploadResponse;
	int		XmlFlieLen;
	unsigned int	XmlFileLength = 0;
	int chNoTmp;

	conntype = -1;
	RecMsgBuff = (unsigned char *)malloc(MSG_MAX_LEN * sizeof(unsigned char ));
	if (NULL == RecMsgBuff)
	{
		BS_DEBUG("managerServerTcp fail to malloc for RecMsgBuff\n");
		pthread_exit((void *)-1);
	}

	RecResBuff = (unsigned char *)malloc(MSG_MAX_LEN * sizeof(unsigned char ));
	if (NULL == RecResBuff)
	{
		BS_DEBUG("managerServerTcp fail to malloc for RecResBuff\n");
		free(RecMsgBuff);
		RecMsgBuff = NULL;
		pthread_exit((void *)-1);
	}
	
	memset(RecMsgBuff, 0, MSG_MAX_LEN);
	memset(RecResBuff, 0, MSG_RES_LEN);
	ServerRecMsgCmd = (RecMsgCommand *)RecMsgBuff;
	ServerRecListCmd = (RecListCommand *)RecMsgBuff;
	cmd = (CommonMsg *)RecMsgBuff;
	rsp = (CommonMsg *)RecResBuff;
	ServerRecConnResponse = (RecConnResponse *)RecResBuff;
	ServerFileCmd = (RecFileCommand *)RecMsgBuff;
	ret = 0;

	XmlMsgBuff = RecMsgBuff;
	XmlResBuff = RecResBuff;
	ServerXmlMsgCommand		= (XmlMsgCommand *)XmlMsgBuff;
	ServerXmlConnResponse		= (XmlConnResponse *)XmlResBuff;
	ServerXmlVerResponse		= (XmlVerResponse *)XmlResBuff;
	ServerXmlDownloadResponse	= (XmlDownloadResponse *)XmlResBuff;
	ServerXmlDownloadFinish		= (XmlDownloadFinish *)XmlResBuff;
	ServerXmlUploadResponse		= (XmlUploadResponse *)XmlResBuff;
	ClientSockfd = *((int *)arg);

	BS_DEBUG("managerServerTcp, ClientSockfd:%d\n", ClientSockfd);
	while(1) 
	{
		//先读取消息的头部，共计12字节
		if (read(ClientSockfd, RecMsgBuff, sizeof(MsgTransHeader)) <= 0) 
		{
			close(ClientSockfd);	//读取消息头部14字节出现错误，关闭连接，并重新建立连接!
			if (conntype >= 0 && conntype < 5)
			{
				links[conntype]--;
			}
			BS_DEBUG("TY-3012S managerServerTcp read ClientSockfd error1: %d!conntype:%d ---lineNo:%d\n", ClientSockfd,conntype+1, __LINE__);
			break;
		}

		#if 0
		printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgHead:     %08x!\n", ServerXmlMsgCommand->MsgHead);
		printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgLength:   %08x!\n", ServerXmlMsgCommand->MsgLength);
		printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgSession1:  %08x!\n", ServerXmlMsgCommand->MsgSession1);
		printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgSession2:  %08x!\n", ServerXmlMsgCommand->MsgSession2);
		printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgId:       %08x!\n", ServerXmlMsgCommand->MsgId);
		#endif

		MsgHead = Ntohs(ServerRecMsgCmd->header.head);
					
		MsgLength= Ntohs(ServerRecMsgCmd->header.length);
		MsgId = Ntohs(ServerRecMsgCmd->header.msgId);
		MsgSession = Ntohs(ServerRecMsgCmd->header.session1 * 0x10000) + Ntohs(ServerRecMsgCmd->header.session2);

		#if 0
		printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgHead:     %08x!\n", ServerXmlMsgCommand->MsgHead);
		printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgLength:   %08x!\n", ServerXmlMsgCommand->MsgLength);
		printf("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgId:       %08x!\n", ServerXmlMsgCommand->MsgId);
		#endif
		#if 1
		//BS_DEBUG("TY-3012S managerServerTcp ServerXmlMsgCommand->MsgHead:     %08x!\n", MsgHead);
		//BS_DEBUG("TY-3012S managerServerTcp ServerXmlMsgCommand->MsgLength:   %08x!\n", MsgLength);
		//BS_DEBUG("TY-3012S managerServerTcp ServerXmlMsgCommand->MsgSession:  %08x!\n", MsgSession);
		BS_DEBUG("TY-3012S managerServerTcp ServerXmlMsgCommand->MsgId:       %08x!\n", MsgId);
		#endif
		//对消息的头部进行解码
		//消息头定位字节错误，关闭连接
		if(MsgHead != 0xF6F62828) 
		{
			close(ClientSockfd);	//消息的定位帧头4字节出现错误，关闭连接，并重新建立连接!
			if (conntype >= 0 && conntype < 5)
			{
				links[conntype]--;
			}
			BS_DEBUG("TY-3012S managerServerTcp decodes head error1: %d! ---lineNo:%d\n", ClientSockfd, __LINE__);
			break;
		}
		//消息保留位字节错误，关闭连接
		#if 0
		if(MsgReserve != 0x0000) {
			close(ClientXmlSockfd);	//消息的保留位2字节出现错误，关闭连接，并重新建立连接!
			printf("TY-3012S ConfigXmlFile decodes reserve error1: %d!\n", ClientXmlSockfd);
			break;
		}
		#endif
		
		//根据消息执行相关命令
		//1、MS_CFG_CONN_REQ	=0x08，配置文件连接请求；MS_CFG_CONN_RESP	=0x09，配置文件连接响应
		//2、MS_CFG_VER_QRY	=0x10，配置文件版本查询；MS_CFG_VER_RESP	=0x11，配置文件查询响应；
		//3、MS_CFG_DOWN_REQ	=0x12，配置文件下载请求；MS_CFG_DOWN_RESP	=0x13，配置文件下载响应；
		//4、MS_CFG_DOWNLOAD	=0x14，配置文件下载开始；MS_CFG_DOWNLOAD_ACK	=0x15，配置文件下载完成；
		//5、MS_CFG_UPLOAD_REQ	=0x16，配置文件上载请求；MS_CFG_UPLOAD		=0x17，配置文件上载开始；
		switch(MsgId) 
		{
			/*长连接管理消息处理部分*/
			/*路由测试*/
			case MS_ROUTE_TEST_REQ:
			{
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 4) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_ROUTE_TEST_REQ read ClientXmlSockfd error2: %d!\n", ClientSockfd);
					break;
				}

				MsgTail = Ntohs(cmd->tail1 * 0x10000) + Ntohs(cmd->tail2);
				BS_DEBUG("TY-3012S MS_ROUTE_TEST_REQ ServerRecListCmd->MsgTail:     %08x!\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_ROUTE_TEST_REQ decodes tail error1: %d!\n", ClientSockfd);
					break;
				}

				/*路由测试,具体功能尚缺*/	
				RESPONSE_ROUTE_ACK(rsp, ClientSockfd, RecResBuff);
			}
				break;
			/*心跳保活响应*/
			case MS_HEART_BEAT_ACK:
			{
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 4) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_HEART_BEAT_ACK read ClientXmlSockfd error2: %d!\n", ClientSockfd);
					break;
				}

				MsgTail = Ntohs(cmd->tail1 * 0x10000) + Ntohs(cmd->tail2);
				BS_DEBUG("TY-3012S MS_HEART_BEAT_ACK ServerRecListCmd->MsgTail:     %08x!\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_HEART_BEAT_ACK decodes tail error1: %d!\n", ClientSockfd);
					break;
				}
				/*后台根据REQ的参数信息决定是否给装置回该消息,装置收到该消息后对应
				心跳保活会有一定动作，尚缺*/
			}
				break;
			/*装置复位请求*/
			case MS_RESET_DEVI_REQ:
			{
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 4) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_ROUTE_TEST_REQ read ClientXmlSockfd error2: %d!\n", ClientSockfd);
					break;
				}

				MsgTail = Ntohs(cmd->tail1 * 0x10000) + Ntohs(cmd->tail2);
				BS_DEBUG("TY-3012S MS_ROUTE_TEST_REQ ServerRecListCmd->MsgTail:     %08x!\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_ROUTE_TEST_REQ decodes tail error1: %d!\n", ClientSockfd);
					break;
				}
				
				RESPONES_RESET_ACK(rsp, ClientSockfd, RecResBuff);
				/*装置复位,回消息后重启*/
				/**/
				//system("/sbin/reboot");
			}
			/*时间同步*/
			case MS_TIMER_SYNC_REQ:
			case MS_SYNC_TIMER_ACK:
			{
				TimerSyncCmd	 *tsPtr;
				tsPtr = (TimerSyncCmd *)RecMsgBuff;
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 12) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_SYNC_TIMER read ClientXmlSockfd error2: %d!\n", ClientSockfd);
					break;
				}

				MsgTail = Ntohs(tsPtr->tail1 * 0x10000) + Ntohs(tsPtr->tail2);
				BS_DEBUG("TY-3012S MS_SYNC_TIMER ServerRecListCmd->MsgTail:     %08x!\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_SYNC_TIMER decodes tail error1: %d!\n", ClientSockfd);
					break;
				}

				/*时间同步,此时应将装置中的时间信息设置为后台传来的时间,功能尚缺*/
				/*时间同步请求要回确认,请求同步响应不需要回消息,设置时间部分功能应一样*/
				if (MsgId == MS_TIMER_SYNC_REQ)
				{
					RESPONES_RESET_ACK(rsp, ClientSockfd, RecResBuff);
				}
			}
				break;
				
			/*网络统计查询*/
			case MS_NET_COUNTE_REQ:
			{
				NetCountCmd *nCountCmd;
				NetCountRsp *nCountRsp;
				
				nCountCmd = (NetCountCmd *)RecMsgBuff;
				nCountRsp = (NetCountRsp *)RecResBuff;
				
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 16) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_NET_COUNTE_REQ read  error2: %d!\n", ClientSockfd);
					break;
				}

				#if 1
				MsgTail = Ntohs(nCountCmd->tail1 * 0x10000) + Ntohs(nCountCmd->tail2);
				#else
				MsgTail = Ntohs(cmd->tail1 * 0x10000) + Ntohs(cmd->tail2);
				#endif
				BS_DEBUG("TY-3012S MS_NET_COUNTE_REQ MsgTail:     %08x!\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp decodes tail error1: %d!\n", ClientSockfd);
					break;
				}

				/*网络查询,从读包分析模块接口获取响应数据,组包上传,功能尚缺*/
				memset(nCountRsp, 1, sizeof(NetCountRsp));
				//RESPONSE_NET_COUNT(nCountRsp, 0x100, ClientSockfd, RecResBuff);
			}
				break;
			/*虚拟面板查询*/
			case MS_DEVICE_LED_REQ:
			{
				DeviceLedRsp *ledRsp;
				unsigned char ledParam[8];
				ledRsp = (DeviceLedRsp *)RecResBuff;
				
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 4) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp read ClientXmlSockfd error2: %d!\n", ClientSockfd);
					break;
				}

				MsgTail = Ntohs(cmd->tail1 * 0x10000) + Ntohs(cmd->tail2);
				BS_DEBUG("TY-3012S MS_DEVICE_LED_REQ ServerRecListCmd->MsgTail:     %08x!\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp decodes tail error1: %d!\n", ClientSockfd);
					break;
				}
				/*虚拟面板查询,此处将各灯状态获取后填写ledParam,发消息给后台*/
				/*暂用全f*/
				memset(ledParam, 0xff, 8);
				RESPONSE_DEVICE_LED_ACK(ledRsp, ClientSockfd, RecResBuff, ledParam);
			}
				break;
			/*立即存盘*/
			case MS_SAVE_HDISK_REQ:
			{
				int chNo;
				SaveFileCmd *saveCmd;
				saveCmd = (SaveFileCmd *)RecMsgBuff;

				printf("recv a save request!!!\n");
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 8) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_SAVE_HDISK_REQ read error: %d!\n", ClientSockfd);
					break;
				}

				MsgTail = Ntohs(saveCmd->tail1 * 0x10000) + Ntohs(saveCmd->tail2);
				BS_DEBUG("TY-3012S MS_SAVE_HDISK_REQ ServerRecListCmd->MsgTail:     %08x!\n", MsgTail);
				if(MsgTail != 0x0909D7D7) 
				{
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_SAVE_HDISK_REQ decodes tail error1: %d!\n", ClientSockfd);
					break;
				}

				/*告知磁盘管理要进行快速存盘*/
				chNo = saveCmd->chNo;
				app_msgSend(MSGQ_RQST_SAVE_FILE, 0, chNo);
				RESPONES_SAVEFILE_ACK(rsp, ClientSockfd, RecResBuff);
			}
				break;
				
			/*启动sv上传*/	
			case MS_SEND_SMVMS_REQ:
			{
				SmvSendCommand *SmvCmd;
				SmvSendResponse *SmvRsp;
				SmvCmd = (SmvSendCommand *)RecMsgBuff;
				SmvRsp = (SmvSendResponse *)RecResBuff;
				
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 20) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("MS_SEND_SMVMS_REQ ClientXmlSockfd error2: %d!---lineNo:%d\n", ClientSockfd, __LINE__);
					break;
				}

				MsgTail = Ntohs(SmvCmd->tail1 * 0x10000) + Ntohs(SmvCmd->tail2);
				BS_DEBUG("TY-3012S MS_SEND_SMVMS_REQ MsgTail:     %08x! ---lineNo:%d\n", MsgTail, __LINE__);
				if(MsgTail != 0x0909D7D7)
				{
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_SEND_SMVMS_REQ decodes tail error1: %d! ---lineNo:%d\n", ClientSockfd, __LINE__);
					break;
				}

				/*此处应先查看是否有录播文件正在上传,如果有，拒绝sv上传请求*/
			
				/*这部分可能有问题,后面再调*/
				printf("sv request links[2]:%d\n", links[2]);
				if (links[2] == 0)
				{
					SmvCmd->Reserved = 0;										/*对应装置上的Enable位*/
					app_setSvCondition(&SmvCmd->CHNLNO);
					printf("svCondition:chNo=%d, enable=%d,appId:%d\n", SmvCmd->CHNLNO, SmvCmd->Reserved, SmvCmd->AppID);					
					
					//回MS_SEND_SMVMS_ACK即可
					//需要根据情况做一些判断,决定回复ACK的值
					RESPONSE_SMV_ACK(SmvRsp, ClientSockfd, RecResBuff, 0);
				}
				else
				{
					/*有SV正在上传*/
					RESPONSE_SMV_ACK(SmvRsp, ClientSockfd, RecResBuff, 2);
				}

			}
				break;

			case MS_STOP_SMVMS_REQ:
			{
				MsgSvInfo svInfo;
				//关闭smv上传的socket
				//回MS_STOP_SMVMS_ACK				
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 20) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S MS_STOP_SMVMS_REQ error2: %d! ---lineNo:%d\n", ClientSockfd, __LINE__);
					break;
				}

				MsgTail = Ntohs(cmd->tail1 * 0x10000) + Ntohs(cmd->tail2);
				BS_DEBUG("TY-3012S MS_STOP_SMVMS_REQ ServerRecListCmd->MsgTail:     %08x!---lineNo:%d\n", MsgTail, __LINE__);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp decodes tail error1: %d! ---lineNo:%d\n", ClientSockfd, __LINE__);
					break;
				}
				
				app_setSvCondition(NULL);				/*清sv上传的条件*/
				svInfo.request = MSG_RQST_SVUPLOAD_STOP;
				bs_svMsgSend(svInfo);
				RESPONSE_SMV_STOP(rsp, ClientSockfd, RecResBuff);
				//TransferStatus.svStatus = 0;
			}
				break;
				
			/*连接请求*/
			case MS_FILETRANS_CONNECT_REQ:
			{
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 8) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S managerServerTcp read ClientSockfd error2: %d!\n", ClientSockfd);
					break;
				}

				if (ServerRecMsgCmd->conn_type == 1)
				{
					BS_DEBUG("TY-3012S conn_type:1! long link manager\n");
					conntype = 0;
					longLinkSockFd = ClientSockfd;				/*记录下来,发告警时调用*/
					links[conntype]++;
					if (links[conntype] > 1)						/*可能是多个长连接都可以,现在暂时只允许一个*/
					{
						RESPONSE_CONNECT_FAILED(ServerRecConnResponse, ClientSockfd, RecResBuff, 1);
						close(ClientSockfd);
						links[conntype]--;
						break;
					}
					
					#if 0
					/*用于测试*/
					printf("begin snd\n");

					bs_gooseWarningMsgsend();
					bs_smvWarningMsgSend();				
					#endif
				}
				else if(ServerRecMsgCmd->conn_type == 2)
				{
					BS_DEBUG("TY-3012S conn_type:2! config file\n");
					conntype = 1;
					links[conntype]++;
					if (links[conntype] > 1)
					{
						RESPONSE_CONNECT_FAILED(ServerRecConnResponse, ClientSockfd, RecResBuff, 1);
						close(ClientSockfd);
						links[conntype]--;
						break;
					}
				}
				else if (ServerRecMsgCmd->conn_type == 3)
				{
					BS_DEBUG("TY-3012S conn_type:3!sv upload\n");
					conntype = 2;
					links[conntype]++;
					if (links[conntype] > 1)
					{
						RESPONSE_CONNECT_FAILED(ServerRecConnResponse, ClientSockfd, RecResBuff, 1);
						BS_DEBUG("sv connect > 1, close current one\n");
						close(ClientSockfd);
						links[conntype]--;
						break;
					}

					/*先回应1000连接请求*/
					RESPONSE_CONNECT_OK(ServerRecConnResponse, ClientSockfd, RecResBuff);
					/*按消息队列消息上传SV数据*/
					//TransferStatus.svStatus = 1;
					svPacketDataUpload(ClientSockfd);
				}
				else if (ServerRecMsgCmd->conn_type == 4)
				{
					BS_DEBUG("TY-3012S conn_type:4! data file upload\n");
					conntype = 3;
					links[conntype]++;
					if (links[conntype] > 1)
					{
						RESPONSE_CONNECT_FAILED(ServerRecConnResponse, ClientSockfd, RecResBuff, 1);
						close(ClientSockfd);
						links[conntype]--;
						break;
					}
				}
				else
				{
					BS_DEBUG("connect type error,conntype:%d ---lineNo:%d\n", conntype, __LINE__);
					RESPONSE_CONNECT_FAILED(ServerRecConnResponse, ClientSockfd, RecResBuff, 2);
					close(ClientSockfd);
					break;
				}

				MsgTail = Ntohs(ServerRecMsgCmd->tail1 * 0x10000) + Ntohs(ServerRecMsgCmd->tail2);
				//消息尾定位字节错误，关闭连接
				BS_DEBUG("TY-3012S recordFileTcp ServerRecMsgCmd->MsgTail:     %08x!\n\n", MsgTail);
				if(MsgTail != 0x0909D7D7) 
				{
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp decodes tail error1: %d!\n", ClientSockfd);
					break;
				}
				
				printf("connect request , links[0]:%d, [1]:%d, [2]:%d, [3]:%d ---lineNo:%d\n", links[0], links[1], links[2], links[3], __LINE__);

				RESPONSE_CONNECT_OK(ServerRecConnResponse, ClientSockfd, RecResBuff);

			}
				break;

			case MS_FILETRANS_FILE_LIST_QRY:
			{
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 24) <= 0)
				{
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp read ClientXmlSockfd error2: %d!\n", ClientSockfd);
					break;
				}

				MsgTail = Ntohs(ServerRecListCmd->tail1 * 0x10000) + Ntohs(ServerRecListCmd->tail2);
				BS_DEBUG("TY-3012S recordFileTcp ServerRecListCmd->MsgTail:     %08x! chNo:%d\n", MsgTail, ServerRecListCmd->channelNo);
				if(MsgTail != 0x0909D7D7) 
				{
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp decodes tail error1: %d!\n", ClientSockfd);
					break;
				}
				chNoTmp = ServerRecListCmd->channelNo;
				dm_msgDiscMngSend(MSGQ_RQST_QUERY, NULL, ServerRecListCmd->channelNo, MsgSession, ClientSockfd);
				
			}
				break;


			case MS_FILETRANS_FILE_UPLOAD_REQ:
			{				
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 68) <= 0)
				{
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp read ClientXmlSockfd error2: %d!\n", ClientSockfd);
					break;
				}

				
				MsgTail = Ntohs(ServerFileCmd->tail1 * 0x10000) + Ntohs(ServerFileCmd->tail2);
				//消息尾定位字节错误，关闭连接
				BS_DEBUG("TY-3012S recordFileTcp MsgTail:%08x!filename:%s ---lineNo:%d\n", MsgTail, ServerFileCmd->filename, __LINE__);
				if(MsgTail != 0x0909D7D7) 
				{
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp decodes tail error1: %d!\n", ClientSockfd);
					break;
				}
				
				dm_msgDiscMngSend(MSGQ_RQST_UPLOAD, ServerFileCmd->filename, chNoTmp, MsgSession, ClientSockfd);
			}
				break;

			case MS_FILETRANS_FILE_UPLOAD_CANCEL:
			{
				if (read(ClientSockfd, (RecMsgBuff + HEADER_LEN), 4) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp read ClientXmlSockfd error2: %d!\n", ClientSockfd);
					break;
				}

				
				MsgTail = Ntohs(cmd->tail1 * 0x10000) + Ntohs(cmd->tail2);
				//消息尾定位字节错误，关闭连接
				BS_DEBUG("TY-3012S recordFileTcp ServerRecMsgCmd->MsgTail:     %08x!\n\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S recordFileTcp decodes tail error1: %d!\n", ClientSockfd);
					break;
				}

				dm_msgDiscMngSend(MSGQ_RQST_STOP_UPLOAD, NULL, 0, 0, ClientSockfd);
				

				#if 0
				rsp->header.head = Htons(0xF6F62828);
				rsp->header.length = Htons(0x000a);
				rsp->header.msgId = Htons(MS_FILETRANS_FILE_UPLOAD_CANCEL_ACK);
				MsgSession++;
				rsp->header.session1 = Htons(MsgSession & 0x0ffff);
				rsp->header.session2 = Htons(MsgSession / 0x10000);
				rsp->tail1 = Htons(0x0909);
				rsp->tail2 = Htons(0xD7D7);
				//将响应消息发送到ClientXmlSockfd
				if (write(ClientSockfd, RecResBuff, sizeof(RecConnResponse)) <= 0) 
				{
					close(ClientSockfd);
					printf("TY-3012S ConfigXmlFile write ClientXmlSockfd error: %d!\n", ClientSockfd);
					break;
				}
				#endif
			}
				break;


			/*----------------配置管理软件通讯消息部分--------------*/
			case MS_CFG_CONN_REQ:
				//将一条消息的所有字节读完，因为没有参数，读出尾部的4个字节
				if (read(ClientSockfd, (XmlMsgBuff + HEADER_LEN), 4) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile read ClientSockfd error:%s, error2: %d!\n", strerror(errno),ClientSockfd);
					break;
				}
				MsgTail = ServerXmlMsgCommand->MsgTail1 * 0x10000 + ServerXmlMsgCommand->MsgTail2;
				//消息尾定位字节错误，关闭连接
				BS_DEBUG("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgTail:     %08x!\n\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile decodes tail error1: %d!\n", ClientSockfd);
					break;
				}
				ServerXmlConnResponse->MsgHead = 0xF6F62828;
				ServerXmlConnResponse->MsgLength = Htons(0x0008);
				MsgSession++;
				ServerXmlConnResponse->MsgSession1 = Htons(MsgSession & 0x0ffff);
				ServerXmlConnResponse->MsgSession2 = Htons(MsgSession / 0x10000);
				ServerXmlConnResponse->MsgId = Htons(MS_CFG_CONN_RESP);
				ServerXmlConnResponse->MsgTail1 = 0x0909;
				ServerXmlConnResponse->MsgTail2 = 0xD7D7;
				/*将响应消息发送到ClientXmlSockfd*/
				if (write(ClientSockfd, XmlResBuff, (HEADER_LEN + 4)) <= 0) 
				{
					close(ClientSockfd);
					BS_DEBUG("TY-3012S ConfigXmlFile write ClientXmlSockfd error1: %d!\n", ClientSockfd);
					break;
				}
				break;
				
			case MS_CFG_VER_QRY:
				//将一条消息的所有字节读完，因为没有参数，读出尾部的4个字节
				if (read(ClientSockfd, (XmlMsgBuff + HEADER_LEN), 4) <= 0) 
				{
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile read ClientXmlSockfd error3: %d!\n", ClientSockfd);
					break;
				}
				//消息尾定位字节错误，关闭连接
				MsgTail = ServerXmlMsgCommand->MsgTail1 * 0x10000 + ServerXmlMsgCommand->MsgTail2;
				BS_DEBUG("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgTail:     %08x!\n\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile decodes tail error2: %d!\n", ClientSockfd);
					break;
				}	
				ServerXmlVerResponse->MsgHead = 0xF6F62828;
				ServerXmlVerResponse->MsgLength = Htons(0x000e);	//14
				MsgSession++;
				ServerXmlVerResponse->MsgSession1 = Htons(MsgSession & 0x0ffff);
				ServerXmlVerResponse->MsgSession2 = Htons(MsgSession / 0x10000);
				ServerXmlVerResponse->MsgId = Htons(MS_CFG_VER_RESP);
				{
					char *version;
					version = (char *)xml_getConfigVersion();
					ServerXmlVerResponse->MsgVersion = (*version) * 0x100 + (*(version + 1));
					BS_DEBUG("check cfg version:%d , del this log --- lineNo:%d\n", *((unsigned short *)version), __LINE__);
				}
				ServerXmlVerResponse->MsgTime1 = Htons(0 & 0x0ffff);
				ServerXmlVerResponse->MsgTime2 = Htons(0 / 0x10000);
				ServerXmlVerResponse->MsgTail1 = 0x0909;
				ServerXmlVerResponse->MsgTail2 = 0xD7D7;
				//将响应消息发送到ClientXmlSockfd
				if (write(ClientSockfd, XmlResBuff, (HEADER_LEN + 10)) <= 0) {
					close(ClientSockfd);
					BS_DEBUG("TY-3012S ConfigXmlFile write ClientXmlSockfd error2: %d!\n", ClientSockfd);
					break;
				}
				break;
				
			case MS_CFG_DOWN_REQ:
				//将一条消息的所有字节读完，因为没有参数，读出尾部的4个字节
				if (read(ClientSockfd, (XmlMsgBuff + HEADER_LEN), 4) <= 0) {
					close(ClientSockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile read ClientXmlSockfd error4: %d!\n", ClientSockfd);
					break;
				}
				//消息尾定位字节错误，关闭连接
				MsgTail = ServerXmlMsgCommand->MsgTail1 * 0x10000 + ServerXmlMsgCommand->MsgTail2;
				BS_DEBUG("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgTail:     %08x!\n\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile decodes tail error3: %d!\n", ClientSockfd);
					break;
				}	
				ServerXmlDownloadResponse->MsgHead = 0xF6F62828;
				ServerXmlDownloadResponse->MsgLength = Htons(0x000a);	//10
				MsgSession++;
				ServerXmlDownloadResponse->MsgSession1 = Htons(MsgSession & 0x0ffff);
				ServerXmlDownloadResponse->MsgSession2 = Htons(MsgSession / 0x10000);
				ServerXmlDownloadResponse->MsgId = Htons(MS_CFG_DOWN_RESP);
				ServerXmlDownloadResponse->MsgEnable = 0x01;
				ServerXmlDownloadResponse->MsgReason = 0x01;
				ServerXmlDownloadResponse->MsgTail1 = 0x0909;
				ServerXmlDownloadResponse->MsgTail2 = 0xD7D7;
				//将响应消息发送到ClientXmlSockfd
				if (write(ClientSockfd, XmlResBuff, (HEADER_LEN + 2 + 4)) <= 0) {
					close(ClientSockfd);
					BS_DEBUG("TY-3012S ConfigXmlFile write ClientXmlSockfd error3: %d!\n", ClientSockfd);
					break;
				}
				break;
				
			case MS_CFG_DOWNLOAD:
				//读出配置Xml文件的字节长度，共计2字节
				if (read(ClientSockfd, (XmlMsgBuff + HEADER_LEN), 2) <= 0) {
					close(ClientSockfd);	//读取Xml配置文件长度2字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile read ClientXmlSockfd error5: %d!\n", ClientSockfd);
					break;
				}
				//判断配置Xml文件的字节长度是否错误，如果错误则关闭连接
				XmlFlieLen = XmlMsgBuff[HEADER_LEN] * 0x100 + XmlMsgBuff[HEADER_LEN + 1];
				BS_DEBUG("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgLen:      %08x!\n", XmlFlieLen);
				if(MsgLength != (XmlFlieLen + 8)) {
					close(ClientSockfd);	//两个长度不匹配，认为消息错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile decodes length error1: %d!\n", ClientSockfd);
					break;
				}
				if(XmlFlieLen > (MSG_MAX_LEN - 18)) {
					close(ClientSockfd);	//消息超长，认为错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile decodes length error2: %d!\n", ClientSockfd);
					break;
				}
				if(XmlFlieLen <= XML_MIN_LEN) {
					close(ClientSockfd);	//消息超短，认为错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile decodes length error3: %d!\n", ClientSockfd);
					break;
				}
				//读出配置Xml文件，包括帧尾
				i = 0;
				while(1) {
					j = read(ClientSockfd, (XmlMsgBuff + HEADER_LEN + 2 + i), (XmlFlieLen + 4 - i));
					if (j <= 0) {
						close(ClientSockfd);	//读取出现错误，关闭连接，并重新建立连接!
						BS_DEBUG("TY-3012S ConfigXmlFile read ClientXmlSockfd error6: %d!\n", ClientSockfd);
						break;
					}
					i = i + j;
					if (i == (XmlFlieLen + 4)) break;
				}
				//确定消息尾定位字节，如果错误，关闭连接
				MsgTail0 = XmlMsgBuff[HEADER_LEN + XmlFlieLen + 2 + 0];
				MsgTail1 = XmlMsgBuff[HEADER_LEN + XmlFlieLen + 2 + 1];
				MsgTail2 = XmlMsgBuff[HEADER_LEN + XmlFlieLen + 2 + 2];
				MsgTail3 = XmlMsgBuff[HEADER_LEN + XmlFlieLen + 2 + 3];
				MsgTail = MsgTail0 * 0x1000000 +MsgTail1 * 0x10000 +MsgTail2 * 0x100 + MsgTail3;
				BS_DEBUG("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgD%d:\n", (i - 4));
				#if 0
				for(i = 0; i < XmlFlieLen; i++) {
					if((i % 0x10) == 0) printf("\n%04x:", i);
					printf(" %02x", XmlMsgBuff[HEADER_LEN + 2 + i]);
				}
				printf("\n");
				#endif
				BS_DEBUG("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgTail:     %08x!\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile decodes tail error4: %d!\n", ClientSockfd);
					break;
				}
				//将Xml配置文件写入磁盘文件DeviceConfig.xml
				if(remove(XML_CFG_FILE) == -1) {
					BS_DEBUG("TY-3012S ConfigXmlFile  rm  DeviceConfig.xml error1:\n");
					//break;
					//del by haopeng ,这里如果删除失败break了会导致装置无回复消息，后台会一直等
				}
				BS_DEBUG("TY-3012S ConfigXmlFile rm  DeviceConfig.xml sucess1:\n");
				XmlFilefd = open(XML_CFG_FILE, O_RDWR | O_CREAT | O_TRUNC, 00700);
				if(XmlFilefd == -1) {
					close(ClientSockfd);	//打开文件出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile open DeviceConfig.xml error1: %d!\n", XmlFilefd);
					break;
				}
				i = write(XmlFilefd, (XmlMsgBuff + HEADER_LEN + 2), XmlFlieLen);
				if(i != XmlFlieLen) {
					close(XmlFilefd);
					close(ClientSockfd);	//写文件出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile write DeviceConfig.xml error1: %d!\n", i);
					break;
				}
				close(XmlFilefd);
				BS_DEBUG("TY-3012S ConfigXmlFile write DeviceConfig.xml sucess1:  %d!\n\n", i);
				ServerXmlDownloadFinish->MsgHead = 0xF6F62828;
				ServerXmlDownloadFinish->MsgLength = Htons(0x0008);
				MsgSession++;
				ServerXmlDownloadFinish->MsgSession1 = Htons(MsgSession & 0x0ffff);
				ServerXmlDownloadFinish->MsgSession2 = Htons(MsgSession / 0x10000);
				ServerXmlDownloadFinish->MsgId = Htons(MS_CFG_DOWNLOAD_ACK);
				ServerXmlDownloadFinish->MsgTail1 = 0x0909;
				ServerXmlDownloadFinish->MsgTail2 = 0xD7D7;
				//将响应消息发送到ClientXmlSockfd
				if (write(ClientSockfd, XmlResBuff, (HEADER_LEN + 4)) <= 0) {
					close(ClientSockfd);
					BS_DEBUG("TY-3012S ConfigXmlFile write ClientXmlSockfd error4: %d!\n", ClientSockfd);
					break;
				}
				/*配置文件下载完成,重启系统*/
				system("reboot");
				break;
			case MS_CFG_UPLOAD_REQ:
				//将一条消息的所有字节读完，因为没有参数，读出尾部的4个字节
				if (read(ClientSockfd, (XmlMsgBuff + HEADER_LEN), 4) <= 0) {
					close(ClientSockfd);
					BS_DEBUG("TY-3012S ConfigXmlFile read ClientXmlSockfd error7: %d!\n", ClientSockfd);
					break;
				}
				//消息尾定位字节错误，关闭连接
				MsgTail = ServerXmlMsgCommand->MsgTail1 * 0x10000 + ServerXmlMsgCommand->MsgTail2;
				BS_DEBUG("TY-3012S XmlTcpServer ServerXmlMsgCommand->MsgTail:     %08x!\n", MsgTail);
				if(MsgTail != 0x0909D7D7) {
					close(ClientSockfd);
					BS_DEBUG("TY-3012S ConfigXmlFile decodes tail error5: %d!\n", ClientSockfd);
					break;
				}
				//将Xml配置文件从磁盘文件DeviceConfig.xml中读出
				#if 1
				XmlFilefd = open(XML_CFG_FILE, O_RDWR, 00700);
				if(XmlFilefd == -1) {
					close(ClientSockfd);	//打开文件出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile open DeviceConfig.xml error2: %d!\n", XmlFilefd);
					break;
				}
				XmlFileLength = read(XmlFilefd, XmlFileBuf, MSG_MAX_LEN);
				if(XmlFileLength < XML_MIN_LEN) {
					close(XmlFilefd);
					close(ClientSockfd);	//读文件出现错误，关闭连接，并重新建立连接!
					BS_DEBUG("TY-3012S ConfigXmlFile read DeviceConfig.xml error1: %d!\n", XmlFileLength);
					break;
				}
				close(XmlFilefd);
				
				ServerXmlUploadResponse->MsgHead = 0xF6F62828;
				ServerXmlUploadResponse->MsgLength = Htons(8 + XmlFileLength);
				MsgSession++;
				ServerXmlUploadResponse->MsgSession2 = Htons(MsgSession & 0x0ffff);
				ServerXmlUploadResponse->MsgSession1 = Htons(MsgSession / 0x10000);
				ServerXmlUploadResponse->MsgId = Htons(MS_CFG_UPLOAD);
				ServerXmlUploadResponse->MsgLen = Htons(XmlFileLength);
				ServerXmlUploadResponse->MsgTail1 = 0x0909;
				ServerXmlUploadResponse->MsgTail2 = 0xD7D7;
				//将响应消息的头部发送到ClientXmlSockfd
				if (write(ClientSockfd, XmlResBuff, HEADER_LEN+2) <= 0) 
				{
					close(ClientSockfd);
					BS_DEBUG("TY-3012S ConfigXmlFile write ClientXmlSockfd error5: %d!\n", ClientSockfd);
					break;
				}
				BS_DEBUG("TY-3012S ConfigXmlFile read DeviceConfig.xml sucess1:   %d!\n\n", XmlFileLength);
				#endif
				//将Xml文件发送到ClientXmlSockfd
				if (write(ClientSockfd, XmlFileBuf, XmlFileLength) <= 0) {
					close(ClientSockfd);
					BS_DEBUG("TY-3012S ConfigXmlFile write ClientXmlSockfd error6: %d!\n", ClientSockfd);
					break;
				}
				//将响应消息的尾部发送到ClientXmlSockfd
//				printf("tail:%x,%x,%x,%x\n", *(XmlResBuff + HEADER_LEN +2),*(XmlResBuff + HEADER_LEN +3),
	//									   *(XmlResBuff + HEADER_LEN +4), *(XmlResBuff + HEADER_LEN +5));
				if (write(ClientSockfd,  (XmlResBuff + HEADER_LEN + 2), 4) <= 0) {
					close(ClientSockfd);
					BS_DEBUG("TY-3012S ConfigXmlFile write ClientXmlSockfd error7: %d!\n", ClientSockfd);
					break;
				}
				break;
								
			default:
				BS_DEBUG("TY-3012S ConfigXmlFile decodes command error1: %d!\n", ClientSockfd);
				break;
		}
	}

	free(RecMsgBuff);
	RecMsgBuff = NULL;
	free(RecResBuff);
	RecResBuff = NULL;
		
	pthread_exit((void *) 1);
}

/*网络端口管理线程主程序,负责接收各种TCP,UDP连接,根据连接类型在条件允许情况下创建新线程进行通讯*/
static void *netServerInit(void *arg)
{
	struct sockaddr_in my_tcpserver_addr;
	int TcpSockfd, UdpSockfd, TcpServerSockFd; 

	struct tm *ConfigRomFileUdpTime;
	time_t LocalTime;
	struct sockaddr_in my_udpserver_addr;
	int UdpSoBroadcast;

	fd_set rdfs;
	int maxfd, i;
	int addr_len = sizeof(struct sockaddr_in);
	int ret;
	pthread_t tcpId;

	memset(links, 0, sizeof(links));
	
	/*create tcp socket & listen*/
	TcpSockfd = socket(AF_INET, SOCK_STREAM, 0);	//建立一个TCP服务器
	if (TcpSockfd < 0) {
		BS_DEBUG("TY-3012S netServerInit creates XmlSockfd error!\n");
		exit(1);	//系统严重错误，需要复位处理!
		ret = -1;
	}

	bzero(&my_tcpserver_addr, sizeof(my_tcpserver_addr));
	my_tcpserver_addr.sin_family = AF_INET;
	my_tcpserver_addr.sin_port = htons(TCP_PORT);
	my_tcpserver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(TcpSockfd, (struct sockaddr *)&my_tcpserver_addr, sizeof(my_tcpserver_addr)) < 0) {
		BS_DEBUG("TY-3012S netServerInit binds TcpSockfd error! ---lineNo:%d\n", __LINE__);
		exit(1);	//系统严重错误，需要复位处理!
		ret = -2;
	}
	if(listen(TcpSockfd, 10) < 0) {	//侦听队列的长度=10!
		BS_DEBUG("TY-3012S netServerInit listens TcpSockfd error!---lineNo:%d\n", __LINE__);
		exit(1);	//系统严重错误，需要复位处理!
		ret = -3;
	}
	maxfd = TcpSockfd;
	
	BS_DEBUG("TY-3012S netServerInit listens TcpSockfd successful: %d!\n", TcpSockfd);

	/*create udp socket*/
	UdpSockfd = socket(AF_INET, SOCK_DGRAM, 0);//建立一个UDP服务器，扫描程序
	if (UdpSockfd < 0) {
		BS_DEBUG("TY3012S ConfigRomFile creates RomSockfd error!\n");
		exit(1);//系统严重错误，需要复位处理!
		ret = -1;
	}
	BS_DEBUG("TY3012S ConfigRomFile creates RomSocket success: %d!\n", UdpSockfd);
	UdpSoBroadcast = 1;
	setsockopt(UdpSockfd, SOL_SOCKET, SO_BROADCAST, &UdpSoBroadcast, sizeof(UdpSoBroadcast));//允许发送广播

	bzero(&my_udpserver_addr, sizeof(my_udpserver_addr));
	my_udpserver_addr.sin_family = AF_INET;
	my_udpserver_addr.sin_port = htons(UDP_PORT);//来自idprom文件!
	my_udpserver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(UdpSockfd, (struct sockaddr *)&my_udpserver_addr, sizeof(my_udpserver_addr)) < 0) {
		BS_DEBUG("TY3012S ConfigRomFile binds RomSocket error!\n");
		exit(1);//系统严重错误，需要复位处理!
		ret = -2;
	}
	
	if (maxfd < UdpSockfd)
	{
		maxfd = UdpSockfd;
	}
	BS_DEBUG("TY3012S ConfigRomFile binds RomSocket successful: %d!\n", my_udpserver_addr.sin_port);

	//memset(linkNum, 0, sizeof(linkNum));
	while(1)
	{
		FD_ZERO(&rdfs);
		FD_SET(TcpSockfd, &rdfs);
		FD_SET(UdpSockfd, &rdfs);

		if (select(maxfd+1, &rdfs, NULL, NULL, NULL) < 0)
		{
			BS_DEBUG("fail to select, --- lineNo:%d\n", __LINE__);
			exit(-1);
		}

		for (i = 0; i <= maxfd; i++)
		{
			if (FD_ISSET(i, &rdfs))
			{
				if (i == TcpSockfd)
				{
					//thread create read sockfd
					if((TcpServerSockFd = accept(TcpSockfd, (struct sockaddr *)&my_tcpserver_addr, &addr_len)) < 0) {
						BS_DEBUG("TY-3012S netServerInit accepts TcpServerSockFd  error!\n");
						exit(-1);	//系统严重错误，需要复位处理!
						ret = -4;
					}
					else 
					{
						//使用阻塞型的数据接收，节省系统资源
						//fcntl(ClientXmlSockfd, F_SETFL, O_NONBLOCK);
						BS_DEBUG("TY-3012S netServerInit accepts TcpServerSockFd successful: %d!\n", TcpServerSockFd);
					}
					
					ret = pthread_create(&tcpId, NULL, managerServerTcp, (void *)&TcpServerSockFd);
					if (ret != 0) 
					{
						BS_DEBUG("backStage error exit system,error:%s ---lineNo:%d!\n",strerror(errno), __LINE__);
						
						//system("/tools/reset");
//						exit(1);
					}

				}
				else if (i == UdpSockfd)
				{
					/*自动扫描配置部分,不另起线程,就放在监听的线程里做*/
					ConfigRomFileUdp(UdpSockfd);
				}
			}
		}

	}
	return;
}


#if 1
/*后台初始化接口,供主流程调用*/
int bs_backStageInit(pthread_t *id1)
{
	int ret;

	if ((SvUploadQid = msgget(700, 0666 | IPC_CREAT)) == -1)
	{
		BS_DEBUG("msgget failed, line:%d\n", __LINE__);
		exit(-1);
	}

	BS_DEBUG("SvUploadQid :%d\n", SvUploadQid);
	
	ret = pthread_create(id1, NULL, netServerInit, NULL);
	if (ret != 0) {
		BS_DEBUG("backStage!\n");
		system("/tools/reset");
		exit(1);
	}

	return ret;
}
#endif


/*
   1、装置的告警上报程序
   2、装置的状态查询程序
*/

#if 0
static void *Ty3012sResponseReportTcp()
{
	struct sockaddr_in my_tcpserver_addr;
	int addr_len = sizeof(struct sockaddr_in);
	int Ty3012sServerSockfd, Ty3012sClientSockfd;
	DevCommand	*sDevCommand;
	DevResponse	*sDevResponse;
	DevRouteAck	*sDevRouteAck;
	DevHeartBeatReq	*sDevHeartBeatReq;
	DevResetAck	*sDevResetAck;
	DevTimeSyncReq	*sDevTimeSyncReq;
	DevLedsRes	*sDevLedsRes;
	DevSaveHdiskReq	*sDevSaveHdiskReq;
	DevFaultReport	*sDevFaultReport;
	DevAlarmReport	*sDevAlarmReport;
	DevStatusReq	*sDevStatusReq;
	DevCounterRes	*sDevCounterRes;
	DevSmvReq	*sDevSmvReq;
	DevSmvAck	*sDevSmvAck;
	int		msgReadn, msgReadp, msgLen, msgRem;
	unsigned char	msgTail0, msgTail1, msgTail2, msgTail3;
	unsigned short	msgLength, msgReserve, msgId;
	unsigned int 	msgHead, msgSession, msgTail;
	int		i, j, ret;

	sDevCommand 	= (*DevCommand)ReqBuff;
	DevResponse	= (*sDevResponse)ResBuff;
	DevRouteAck	= (*sDevRouteAck)AckBuff;
	DevHeartBeatReq	= (*sDevHeartBeatReq);
	DevResetAck	= (*sDevResetAck)AckBuff;
	DevTimeSyncReq	= (*sDevTimeSyncReq);
	DevLedsRes	= (*sDevLedsRes)ResBuff;
	DevSaveHdiskReq	= (*sDevSaveHdiskReq);
	DevFaultReport	= (*sDevFaultReport);
	DevAlarmReport	= (*sDevAlarmReport);
	DevStatusReq	= (*sDevStatusReq);
	DevCounterRes	= (*sDevCounterRes)ResBuff;
	DevSmvReq	= (*sDevSmvReq);
	DevSmvAck	= (*sDevSmvAck)AckBuff;

	memset(MsgBuff, 0, MSG_MAX_LEN);
	memset(AckBuff, 0, MSG_RES_LEN);
	memset(ResBuff, 0, MSG_RES_LEN);

	//建立连接
	ret = 0;
	Ty3012sSsockfd = socket(AF_INET, SOCK_STREAM, 0);	//建立一个TCP服务器，完成装置和后台通信
	if(Ty3012sSsockfd < 0) {
		printf("TY-3012S sTcp create Ty3012sSsockfd error!\n");
		exit(1);	//系统严重错误，需要复位处理!
		ret = -1;
	}
	printf("TY-3012S sTcp creates Ty3012sSsockfd successful: %d!\n", Ty3012sSsockfd);
	bzero(&my_tcpserver_addr, sizeof(my_tcpserver_addr));
	my_tcpserver_addr.sin_family = AF_INET;
	my_tcpserver_addr.sin_port = htons(Ty3012sStcpPort);
	my_tcpserver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(Ty3012sSsockfd, (struct sockaddr *)&my_tcpserver_addr, sizeof(my_tcpserver_addr)) < 0) {
		printf("TY-3012S sTcp binds Ty3012sSsockfd error!\n");
		exit(1);	//系统严重错误，需要复位处理!
		ret = -2;
	}
	printf("TY-3012S sTcp binds Ty3012sSsockfd successful: %d, %d!\n", Ty3012sStcpPort, my_tcpserver_addr.sin_port);
	if(listen(Ty3012sSsockfd, 3) < 0) {	//侦听队列的长度=3!
		printf("TY-3012S sTcp listens Ty3012sSsockfd error!\n");
		exit(1);	//系统严重错误，需要复位处理!
		ret = -3;
	}
	printf("TY-3012S sTcp listens Ty3012sSsockfd successful: %d!\n", 3);
	//等待客户端的连接请求
	loop0:
	while(1) {
		if((Ty3012sCsockfd = accept(Ty3012sSsockfd, (struct sockaddr *)&my_tcpserver_addr, &addr_len)) < 0) {
			printf("TY-3012S sTcp accepts Ty3012sCsockfd error!\n");
			exit(EXIT_FAILURE);	//系统严重错误，需要复位处理!
			ret = -4;
		} else {
			//使用非阻塞型的数据接收，不节省系统资源，但是收发使用一个线程
			fcntl(Ty3012sCsockfd, F_SETFL, O_NONBLOCK);	//非阻塞型的数据接收
			printf("TY-3012S sTcp accepts Ty3012sCsockfd successful: %d!\n", Ty3012sCsockfd);
		}
		loop1;
		//先读取消息的头部，共计14字节
		msgReadn = 0;
		msgReadp = 0;
		msgRem = HEADER_LEN;
		while(1) {
			msgLen = read(Ty3012sCsockfd, (MsgBuff + msgReadp), msgRem);
			if (msgLen < 0) {
				close(Ty3012sCsockfd);	//读取消息头部14字节出现错误，关闭连接，并重新建立连接!
				printf("TY-3012S sTcp read Ty3012sCsockfd error1: %d!\n", Ty3012sCsockfd);
				goto loop0;
			}
			if (msgLen >= msgRem) break;
			msgReadp = msgReadp + msgLen;
			msgRem = msgRem - msgLen;
			printf("TY-3012S sTcp read Ty3012sCsockfd empty1: %d!\n", msgLen);
			sleep(1);	//读取消息头部14字节出现不足
		}
		//读取头部14字节完成，进行解码
		msgHead = DevCommand->MsgHead;
		msgLength= Htons(DevCommand->MsgLength);
		msgSession = Htons(DevCommand->MsgSession2) * 0x10000 + Htons(DevCommand->MsgSession1);
		msgReserve = Htons(DevCommand->MsgReserve);
		msgId = Htons(DevCommand->MsgId);
		#if 0
		printf("TY-3012S sTcp DevCommand->MsgHead:   %08x!\n", DevCommand->MsgHead);
		printf("TY-3012S sTcp DevCommand->MsgLength: %08x!\n", DevCommand->MsgLength);
		printf("TY-3012S sTcp DevCommand->MsgId:     %08x!\n", DevCommand->MsgId);
		#endif
		#if 1
		printf("TY-3012S sTcp DevCommand->MsgHead:   %08x!\n", MsgHead);
		printf("TY-3012S sTcp DevCommand->MsgLength: %08x!\n", MsgLength);
		printf("TY-3012S sTcp DevCommand->MsgSession:%08x!\n", MsgSession);
		printf("TY-3012S sTcp DevCommand->MsgReserve:%08x!\n", MsgReserve);
		printf("TY-3012S sTcp DevCommand->MsgId:     %08x!\n", MsgId);
		#endif
		//对消息的头部进行解码
		//消息头定位字节错误，关闭连接
		if(msgHead != 0xF6F62828) {
			close(Ty3012sCsockfd);	//消息的定位帧头4字节出现错误，关闭连接，并重新建立连接!
			printf("TY-3012S sTcp decodes head error1: %d!\n", Ty3012sCsockfd);
			goto loop0;	//可以使用break
		}
		//消息保留位字节错误，关闭连接
		if(msgReserve != 0x0000) {
			close(Ty3012sCsockfd);	//消息的保留位2字节出现错误，关闭连接，并重新建立连接!
			printf("TY-3012S sTcp decodes reserve error1: %d!\n", Ty3012sCsockfd);
			goto loop0;	//可以使用break
		}
		//根据消息执行相关命令
		//1、MS_ROUTE_TEST_REQ(0x0020):路由测试请求，MS_ROUTE_TEST_ACK(0x0120):路由测试确认，后台》装置
		//2、MS_HEART_BEAT_REQ(0x0121):心跳保活请求，MS_HEART_BEAT_ACK(0x0021):心跳保活确认，装置》后台
		//3、MS_RESET_DEVI_REQ(0x0022):装置复位请求，MS_RESET_DEVI_ACK(0x0122):装置复位确认，后台》装置
		//4、MS_TIMER_SYNC_REQ(0x0023):时间同步请求，MS_TIMER_SYNC_ACK(0x0123):时间同步确认，后台》装置
		//5、MS_SYNC_TIMER_REQ(0x0124):发送同步请求，MS_SYNC_TIMER_ACK(0x0024):时间同步应答，后台》装置
		//6、MS_GPSTIME_SY_REQ(0x0000):发送GPST请求，MS_GPSTIME_SY_ACK(0x0000):GPST同步确认，装置》后台
		//7、MS_DEVICE_LED_REQ(0x0062):虚拟面板查询，MS_DEVICE_LED_ACK(0x0162):虚拟面板应答，后台》装置
		//8、MS_SAVE_HDISK_REQ(0x0063):立即存盘请求，MS_SAVE_HDISK_ACK(0x0163):立即存盘确认，后台》装置
		//9、MS_DEVI_FAULT_REP(0x0164):装置故障上报，MS_DEVI_FAULT_ACK(0x0064):故障上报确认，装置》后台
		//a、MS_DEVI_ALARM_REP(0x0165):装置告警上报，MS_DEVI_ALARM_ACK(0x0065):告警上报确认，装置》后台
		//b、MS_NET_STATUS_REQ(0x0000):网络状态请求，MS_NET_STATUS_ACK(0x0000):网络状态应答，后台》装置
		//c、MS_NET_COUNTE_REQ(0x0066):网络统计查询，MS_CFG_COUNTE_ACK(0x0166):网络统计应答，后台》装置
		//d、MS_SEND_SMVMS_REQ(0x0067):请求对SV上报，MS_SEND_SMVMS_ACK(0x0167):确认对SV上报，后台》装置
		//   MS_STARTS_SMV_ACK(0x0168):开始对SV上报，                                            》装置
		//f、MS_STOP_SMVMS_REQ(0x0069):终止对SV上报，MS_STOP_SMVMS_ACK(0x0169):确认SV已终止，后台》装置
		switch(msgId) {
			case MS_ROUTE_TEST_REQ:
				//将一条消息的所有字节读完，因为没有参数，读出尾部的4个字节
				msgReadn = 0;
				msgReadp = 0;
				msgRem = 4;
				while(1) {
					msgLen = read(Ty3012sCsockfd, (MsgBuff + msgReadp + HEADER_LEN), msgRem);
					if(msgLen < 0) {
						close(Ty3012sCsockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
						printf("TY-3012S sTcp read Ty3012sCsockfd error2: %d!\n", Ty3012sCsockfd);
						goto loop0;
					}
					if(msgLen >= msgRem) break;
					msgReadn++;
					msgReadp = msgReadp + msgLen;
					msgRem = msgRem - msgLen;
					printf("TY-3012S sTcp read Ty3012sCsockfd empty2: %d, %d!\n", msgReadn, msgLen);
					sleep(1);	//读取消息尾部4字节出现不足
					if(msgReadn > 3) {
						close(Ty3012sCsockfd);
						goto loop0;
					}
				}
				msgTail = DevCommand->msgTail1 * 0x10000 + DevCommand->msgTail2;
				//消息尾定位字节错误，关闭连接
				printf("TY-3012S sTcp DevCommand->MsgTail:   %08x!\n\n", msgTail);
				if(msgTail != 0x0909D7D7) {
					close(Ty3012sCsockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					printf("TY-3012S sTcp decodes tail error1: %d!\n", Ty3012sCsockfd);
					goto loop0;
				}
				DevRouteAck->MsgHead = 0xF6F62828;
				DevRouteAck->MsgLength = Htons(0x0008);
				msgSession++;
				DevRouteAck->MsgSession1 = Htons(MsgSession & 0x0ffff);
				DevRouteAck->MsgSession2 = Htons(MsgSession / 0x10000);
				DevRouteAck->MsgReserve = Htons(0x0000);
				DevRouteAck->MsgId = Htons(MS_ROUTE_TEST_ACK);
				DevRouteAck->MsgTail1 = 0x0909;
				DevRouteAck->MsgTail2 = 0xD7D7;
				//将响应消息发送到 Ty3012sCsockfd
				if (write( Ty3012sCsockfd, AckBuff, (HEADER_LEN + 4)) <= 0) {
					close(Ty3012sCsockfd);
					printf("TY-3012S sTcp write Ty3012sCsockfd error1: %d!\n", Ty3012sCsockfd);
					goto loop0;
				}
				break;
			case MS_RESET_DEVI_REQ:
				//将一条消息的所有字节读完，因为没有参数，读出尾部的4个字节
				msgReadn = 0;
				msgReadp = 0;
				msgRem = 4;
				while(1) {
					msgLen = read(Ty3012sCsockfd, (MsgBuff + msgReadp + HEADER_LEN), msgRem);
					if(msgLen < 0) {
						close(Ty3012sCsockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
						printf("TY-3012S sTcp read Ty3012sCsockfd error3: %d!\n", Ty3012sCsockfd);
						goto loop0;
					}
					if(msgLen >= msgRem) break;
					msgReadn++;
					msgReadp = msgReadp + msgLen;
					msgRem = msgRem - msgLen;
					printf("TY-3012S sTcp read Ty3012sCsockfd empty3: %d, %d!\n", msgReadn, msgLen);
					sleep(1);	//读取消息尾部4字节出现不足
					if(msgReadn > 3) {
						close(Ty3012sCsockfd);
						goto loop0;
					}
				}
				//消息尾定位字节错误，关闭连接
				msgTail = DevCommand->msgTail1 * 0x10000 + DevCommand->msgTail2;
				//消息尾定位字节错误，关闭连接
				printf("TY-3012S sTcp DevCommand->MsgTail:   %08x!\n\n", msgTail);
				if(msgTail != 0x0909D7D7) {
					close(Ty3012sCsockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					printf("TY-3012S sTcp decodes tail error2: %d!\n", Ty3012sCsockfd);
					goto loop0;
				}
				//完成数据存盘，停止接收数据，清空各种缓冲区
				//!!!???
				DevResetAck->MsgHead = 0xF6F62828;
				DevResetAck->MsgLength = Htons(0x0008);
				msgSession++;
				DevResetAck->MsgSession1 = Htons(MsgSession & 0x0ffff);
				DevResetAck->MsgSession2 = Htons(MsgSession / 0x10000);
				DevResetAck->MsgReserve = Htons(0x0000);
				DevResetAck->MsgId = Htons(MS_RESET_DEVI_ACK);
				DevResetAck->MsgTail1 = 0x0909;
				DevResetAck->MsgTail2 = 0xD7D7;
				//将响应消息发送到 Ty3012sCsockfd
				if (write( Ty3012sCsockfd, AckBuff, (HEADER_LEN + 4)) <= 0) {
					close(Ty3012sCsockfd);
					printf("TY-3012S sTcp write Ty3012sCsockfd error1: %d!\n", Ty3012sCsockfd);
					goto loop0;
				}
				break;

			case MS_TIMER_SYNC_REQ:
				//将一条消息的所有字节读完，因为没有参数，读出尾部的4个字节
				msgReadn = 0;
				msgReadp = 0;
				msgRem = 4;
				while(1) {
					msgLen = read(Ty3012sCsockfd, (MsgBuff + msgReadp + HEADER_LEN), msgRem);
					if(msgLen < 0) {
						close(Ty3012sCsockfd);	//读取消息尾部4字节出现错误，关闭连接，并重新建立连接!
						printf("TY-3012S sTcp read Ty3012sCsockfd error3: %d!\n", Ty3012sCsockfd);
						goto loop0;
					}
					if(msgLen >= msgRem) break;
					msgReadn++;
					msgReadp = msgReadp + msgLen;
					msgRem = msgRem - msgLen;
					printf("TY-3012S sTcp read Ty3012sCsockfd empty3: %d, %d!\n", msgReadn, msgLen);
					sleep(1);	//读取消息尾部4字节出现不足
					if(msgReadn > 3) {
						close(Ty3012sCsockfd);
						goto loop0;
					}
				}
				//消息尾定位字节错误，关闭连接
				msgTail = DevCommand->msgTail1 * 0x10000 + DevCommand->msgTail2;
				//消息尾定位字节错误，关闭连接
				printf("TY-3012S sTcp DevCommand->MsgTail:   %08x!\n\n", msgTail);
				if(msgTail != 0x0909D7D7) {
					close(Ty3012sCsockfd);	//消息的定位帧尾4字节出现错误，关闭连接，并重新建立连接!
					printf("TY-3012S sTcp decodes tail error2: %d!\n", Ty3012sCsockfd);
					goto loop0;
				}
				DevResetAck->MsgHead = 0xF6F62828;
				DevResetAck->MsgLength = Htons(0x0008);
				msgSession++;
				DevResetAck->MsgSession1 = Htons(MsgSession & 0x0ffff);
				DevResetAck->MsgSession2 = Htons(MsgSession / 0x10000);
				DevResetAck->MsgReserve = Htons(0x0000);
				DevResetAck->MsgId = Htons(MS_RESET_DEVI_ACK);
				DevResetAck->MsgTail1 = 0x0909;
				DevResetAck->MsgTail2 = 0xD7D7;
				//将响应消息发送到 Ty3012sCsockfd
				if (write( Ty3012sCsockfd, AckBuff, (HEADER_LEN + 4)) <= 0) {
					close(Ty3012sCsockfd);
					printf("TY-3012S sTcp write Ty3012sCsockfd error1: %d!\n", Ty3012sCsockfd);
					goto loop0;
				}
				break;



			default:
				close(ClientXmlSockfd);
				printf("TY-3012S ConfigXmlFile decodes command error1: %d!\n", ClientXmlSockfd);
				sleep(1);
				break;
		}
	}
}
#endif

#if 0
static unsigned char	MsgBuffer[MSGMAXLEN];

static int SetMsgBuffer(void)
{
	MsgCommand *ServerMsgCommand;

	ServerMsgCommand = (MsgCommand *)MsgBuffer;
	memset(MsgBuffer, 0x0, MSGMAXLEN);
	ServerMsgCommand->MsgHead	= MSGHEAD;
	ServerMsgCommand->MsgLength	= MSGMAXLEN;
	ServerMsgCommand->MsgSessionId	= MSGSESSIONID;
	ServerMsgCommand->MsgDeviceId	= MSGDEVICEID;
	ServerMsgCommand->MsgVersion	= MSGVERSION;
	ServerMsgCommand->MsgId		= MSGID;
	ServerMsgCommand->MsgChNo	= MSGCHNO1;
	ServerMsgCommand->MsgTail	= MSGTAIL;
	return 0;
}

static void backStageStart()
{
	int			UdpServerSocket, UdpSoBroadcast, i;
	struct sockaddr_in	ServerAddr;		//服务器的地址信息
	int			addr_len;		//地址结构的长度
	int			MsgLen;
	MsgCommand		*ServerMsgCommand;
	MsgChInterface		*ServerChInterface;
	MsgTriggerTime		*ServerTriggerTime;
	MsgGpsInterface		*ServerGpsInterface;
	MsgGpsTime		*ServerGpsTime;
	MsgChFilter		*ServerChFilter;
	MsgChBuffer		*ServerChBuffer;
	MsgOpticalStatus	*ServerOpticalStatus;
	MsgEthernetStatus	*ServerEthernetStatus;
	MsgGpsStatus		*ServerGpsStatus;
	MsgTrigerStatus		*ServerTrigerStatus;
	MsgFrameCounters	*ServerFrameCounters;
	MsgLedStatus		*ServerLedStatus;

	printf("TY-3012S UdpServer main!\n");
	UdpSoBroadcast = 1;
	ServerMsgCommand     = (MsgCommand *)MsgBuffer;
	ServerChInterface    = (MsgChInterface *)MsgBuffer;
	ServerTriggerTime    = (MsgTriggerTime *)MsgBuffer;
	ServerGpsInterface   = (MsgGpsInterface *)MsgBuffer;
	ServerGpsTime        = (MsgGpsTime *)MsgBuffer;
	ServerChFilter       = (MsgChFilter *)MsgBuffer;
	ServerChBuffer       = (MsgChBuffer *)MsgBuffer;
	ServerOpticalStatus  = (MsgOpticalStatus *)MsgBuffer;
	ServerEthernetStatus = (MsgEthernetStatus *)MsgBuffer;
	ServerGpsStatus      = (MsgGpsStatus *)MsgBuffer;
	ServerTrigerStatus   = (MsgTrigerStatus *)MsgBuffer;
	ServerFrameCounters  = (MsgFrameCounters *)MsgBuffer;
	ServerLedStatus      = (MsgLedStatus *)MsgBuffer;
	UdpServerSocket      = socket(AF_INET, SOCK_DGRAM, 0);	//建立udp socket
	if(UdpServerSocket < 0) {
		perror("TY-3012S UdpServerSocket error!\n");	//建立udp socket失败
		return -1;
	} else {						//建立udp socket成功
		printf("TY-3012S UdpServerSocket created, id: %d, LocalPort: %d!\n", UdpServerSocket, LOCALPORT);
	}
	setsockopt(UdpServerSocket, SOL_SOCKET, SO_BROADCAST, &UdpSoBroadcast, sizeof(UdpSoBroadcast));	//允许发送广播
	addr_len = sizeof(struct sockaddr_in);			//获取地址结构的长度
	bzero(&ServerAddr, sizeof(ServerAddr));			//清空地址结构
	ServerAddr.sin_family = AF_INET;			//置协议族
	ServerAddr.sin_port = htons(LOCALPORT);			//服务器端口
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);		//收发地址，服务器地址
	if(bind(UdpServerSocket, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) < 0) {
		perror("TY-3012S UdpServerBind error!\n");
 		return -1;
	} else {
		printf("TY-3012S UdpServerBind OK, LocalPort: %d!\n", LOCALPORT);
	}
	MsgLen = recvfrom(UdpServerSocket, MsgBuffer, sizeof(MsgBuffer), 0, (struct sockaddr *)&ServerAddr, &addr_len);	//从客户端接收
	if(MsgLen < 0) {
		perror("TY-3012S UdpServer recvfrom error!\n");
 		return -1;
	} else {
		printf("TY-3012S UdpServer recvfrom OK, MsgLen: %d!\n", MsgLen);
	}
	#if 0
	printf("TY-3012S UdpServer MsgBuffer: %d!", MsgLen);
	for(i = 0; i < MSGMAXLEN; i++) {
		if((i % 0x10) == 0) printf("\n%02x:", i);
			printf(" %02x", MsgBuffer[i]);
		}
	printf("\n");
	#endif
	//开始测试
	while(1) {
	sleep(10);
	//1、设置通道的以太网接口类型，光接口或电接口，SET_CH_INTERFACE
	#if 0
	SetMsgBuffer();
	ServerChInterface->MsgId = SET_CH_INTERFACE;		//测试以太网接口
	//ServerChInterface->MsgChNo = MSGCHNO1;		//测试通道1
	ServerChInterface->MsgChNo = MSGCHNO2;			//测试通道2
	ServerChInterface->MsgChInterface = EINTERFACE;		//测试以太网电接口
	//ServerChInterface->MsgChInterface = OINTERFACE;	//测试以太网光接口
	#endif
	//2、设置通道外触发消抖时间，单位us，SET_TRIGGER_TIME
	#if 0
	SetMsgBuffer();
	ServerTriggerTime->MsgId = SET_TRIGGER_TIME;		//测试开量输入接口的外触发消抖时间
	//ServerTriggerTime->MsgChNo = MSGCHNO1;		//测试通道1
	ServerTriggerTime->MsgChNo = MSGCHNO2;			//测试通道2
	ServerTriggerTime->MsgTrigerTime = TRIGERTIME;		//测试开量输入接口的外触发消抖时间，单位us
	#endif
	//3、设置GPS接口类型，TTL或485接口，SET_GPS_INTERFACE
	#if 0
	SetMsgBuffer();
	ServerGpsInterface->MsgId = SET_GPS_INTERFACE;		//测试GPS接口
	ServerGpsInterface->MsgChNo = MSGCHNO1;			//测试通道1
	//ServerGpsInterface->MsgChNo = MSGCHNO2;		//测试通道2
	ServerGpsInterface->MsgGpsInterface = GPSTTLINTERFACE;	//测试TTL的GPS接口
	//ServerGpsInterface->MsgGpsInterface = GPS485INTERFACE;//测试485的GPS接口
	#endif
	//4、设置GPS时间，年、月、日、时、分、秒、毫秒、微妙，SET_GPS_TIME
	#if 0
	SetMsgBuffer();
	ServerGpsTime->MsgId		= SET_GPS_TIME;		//测试GPS时间
	//ServerGpsTime->MsgChNo	= MSGCHNO1;		//测试通道1
	ServerGpsTime->MsgChNo		= MSGCHNO2;		//测试通道2
	ServerGpsTime->MsgYear		= GPSYEAR;		//测试GPS时间，年
	ServerGpsTime->MsgMonth		= GPSMONTH;		//测试GPS时间，月
	ServerGpsTime->MsgDay		= GPSDAY;		//测试GPS时间，日
	ServerGpsTime->MsgHour		= GPSHOUR;		//测试GPS时间，时
	ServerGpsTime->MsgMinit		= GPSMINIT;		//测试GPS时间，分
	ServerGpsTime->MsgSecond 	= GPSSECOND;		//测试GPS时间，秒
	ServerGpsTime->MsgMillisecond	= GPSMILLISECOND;	//测试GPS时间，微妙
	ServerGpsTime->MsgMicrosecond	= GPSMICROSECOND;	//测试GPS时间，毫秒
	#endif
	//5、设置通道的过滤器，即过滤条件，SET_CH_FILTER
	#if 0
	SetMsgBuffer();
	ServerChFilter->MsgId		= SET_CH_FILTER;	//测试通道的过滤条件
	ServerChFilter->MsgChNo		= MSGCHNO1;		//测试通道1
	//ServerChFilter->MsgChNo	= MSGCHNO2;		//测试通道2
	ServerChFilter->MsgReset	= CHRESET;		//通道的过滤条件，0不要求复位，1要求复位
	ServerChFilter->MsgMode		= CHMODE;		//通道的过滤条件，过滤模式，1不过滤，0过滤
	ServerChFilter->MsgMinLength	= MINLENGTH;		//通道的过滤条件，最小帧长，隐含是64字节
	ServerChFilter->MsgMaxLength	= MAXLENGTH;		//通道的过滤条件，最大帧长，隐含是1518字节
	ServerChFilter->MsgVlan1Enable	= VLAN1ENABLE;		//通道的过滤条件，Vlan1使能，D0=1使能
	ServerChFilter->MsgVlan2Enable	= VLAN2ENABLE;		//通道的过滤条件，Vlan2使能，D1=1使能
	ServerChFilter->MsgGooseEnable	= GOOSEENABLE;		//通道的过滤条件，Goose使能，D2=1使能
	ServerChFilter->MsgSmvEnable	= SMVENABLE;		//通道的过滤条件，Smv  使能，D3=1使能
	ServerChFilter->MsgMmsEnable	= MMSENABLE;		//通道的过滤条件，Mms  使能，D4=1使能	
	ServerChFilter->MsgVlan1Id	= VLAN1ID;		//通道的过滤条件，Vlan1标签
	ServerChFilter->MsgVlan2Id 	= VLAN2ID;		//通道的过滤条件，Vlan2标签
	ServerChFilter->MsgMacEnable	= MACENABLE;		//通道的过滤条件，Mac  使能，D0到D15=1使能
	for(i = 0; i < 16; i++) {
		ServerChFilter->MsgMacAddr[i*6 + 0] = 0x00 + i;	//通道的过滤条件，00
		ServerChFilter->MsgMacAddr[i*6 + 1] = 0x80 + i;	//通道的过滤条件，80
		ServerChFilter->MsgMacAddr[i*6 + 2] = 0xC8 + i;	//通道的过滤条件，C8
		ServerChFilter->MsgMacAddr[i*6 + 3] = 0xE1 + i;	//通道的过滤条件，E1
		ServerChFilter->MsgMacAddr[i*6 + 4] = 0xD0 + i;	//通道的过滤条件，D0
		ServerChFilter->MsgMacAddr[i*6 + 5] = 0x00 + i;	//通道的过滤条件， i
	}
	#endif
	//6、检查缓冲FIFO中的数据，每次检查128字节，CHECK_CH_BUFFER
	#if 0
	SetMsgBuffer();
	ServerChBuffer->MsgId = CHECK_CH_BUFFER;		//检查接收到的报文
	//ServerChBuffer->MsgChNo = MSGCHNO1;			//测试通道1
	ServerChBuffer->MsgChNo	= MSGCHNO2;			//测试通道2
	ServerChBuffer->MsgByte = 128;				//最大128；
	#endif
	//7、检查通道的以太网接口类型，光接口或电接口，CHECK_CH_INTERFACE
	#if 0
	SetMsgBuffer();
	ServerChInterface->MsgChNo = MSGCHNO1;			//测试通道1
	//ServerChInterface->MsgChNo = MSGCHNO2;		//测试通道2
	ServerChInterface->MsgId = CHECK_CH_INTERFACE;		//检查目前使用的是光接口还是电接口
	#endif
	//8、检查通道光接口的状态，有光或无光信号，以便确定是否光信号丢失，CHECK_OPTICAL_STATUS
	#if 0
	SetMsgBuffer();
	//ServerOpticalStatus->MsgChNo = MSGCHNO1;		//测试通道1
	ServerOpticalStatus->MsgChNo = MSGCHNO2;		//测试通道2
	ServerOpticalStatus->MsgId = CHECK_OPTICAL_STATUS;	//检查是否光接口有信号
	#endif
	//9、检查通道光电接口的连接状态，连接、速度、双工，CHECK_LINK_SPEED
	#if 0
	SetMsgBuffer();
	//ServerEthernetStatus->MsgChNo = MSGCHNO1;		//测试通道1
	ServerEthernetStatus->MsgChNo = MSGCHNO2;		//测试通道2
	ServerEthernetStatus->MsgId = CHECK_OPTICAL_STATUS;	//检查以太网接口的Link，Speed，Duplex
	#endif
	//10、检查GPS时间，年、月、日、时、分、秒、毫秒、微妙，CHECK_GPS_TIMER
	#if 1
	SetMsgBuffer();
	ServerGpsTime->MsgChNo = MSGCHNO1;			//测试通道1
	//ServerGpsTime->MsgChNo = MSGCHNO2;			//测试通道2
	ServerGpsTime->MsgId = CHECK_GPS_TIMER;			//检查GPS时间
	#endif
	//11、检查GPS接口的告警状态，正常、丢失、错误，CHECK_GPS_STATUS
	#if 0
	SetMsgBuffer();
	//ServerGpsStatus->MsgChNo = MSGCHNO1;			//测试通道1
	ServerGpsStatus->MsgChNo = MSGCHNO2;			//测试通道2
	ServerGpsStatus->MsgId = CHECK_GPS_STATUS;		//检查GPS接口的告警状态
	#endif
	//12、检查通道外触发状态，有无触发，是否正在触发，CHECK_TRIGGER_STATUS
	#if 0
	SetMsgBuffer();
	ServerTrigerStatus->MsgChNo = MSGCHNO1;			//测试通道1
	//ServerTrigerStatus->MsgChNo = MSGCHNO2;		//测试通道2
	ServerTrigerStatus->MsgId = CHECK_TRIGGER_STATUS;	//检查通道外触发状态
	#endif
	//13、检查通道的过滤条件，CHECK_CH_FILTER
	#if 0
	SetMsgBuffer();
	ServerChFilter->MsgChNo = MSGCHNO1;			//测试通道1
	//ServerChFilter->MsgChNo = MSGCHNO2;			//测试通道2
	ServerChFilter->MsgId = CHECK_CH_FILTER;		//检查通道的过滤条件
	#endif
	//14、检查帧和字节计数器的统计数据，CHECK_FRAME_COUNTERS
	#if 0
	SetMsgBuffer();
	ServerFrameCounters->MsgChNo = MSGCHNO1;		//测试通道1
	//ServerFrameCounters->MsgChNo = MSGCHNO2;		//测试通道2	
	ServerFrameCounters->MsgId = CHECK_FRAME_COUNTERS;	//检查帧和字节计数器的统计数据
	#endif
	//15、检查前面板LED状态，总共36个LED，CHECK_LED_STATUS
	#if 0
	SetMsgBuffer();
	ServerLedStatus->MsgId = CHECK_LED_STATUS;		//检查前面板LED状态
	#endif
	MsgLen = sendto(UdpServerSocket, MsgBuffer, MSGMAXLEN, 0, (struct sockaddr*)&ServerAddr, addr_len);		//向客户端发送
	if(MsgLen < 0) {
		perror("TY-3012S UdpServer sendto error!\n");
		return -1;
	} else {
		printf("TY-3012S UdpServer sendto OK, MsgLen: %d!\n", MsgLen);
	}
	//测试结果
	MsgLen = recvfrom(UdpServerSocket, MsgBuffer, sizeof(MsgBuffer), 0, (struct sockaddr *)&ServerAddr, &addr_len);	//从客户端接收
	if(MsgLen < 0) {
		perror("TY-3012S UdpServer recvfrom error!!\n");
 		return -1;
	} else {
		printf("TY-3012S UdpServer recvfrom OK, MsgLen: %d!!\n", MsgLen);
	}
	#if 0
	printf("TY-3012S UdpServer MsgBuffer: %d!!", MsgLen);
	for(i = 0; i < MSGMAXLEN; i++) {
		if((i % 0x10) == 0) printf("\n%02x:", i);
			printf(" %02x", MsgBuffer[i]);
		}
	printf("\n");
	#endif
	//ServerMsgCommand->MsgId = ACK_CH_INTERFACE;
	//ServerMsgCommand->MsgId = ACK_TRIGGER_TIME;
	//ServerMsgCommand->MsgId = ACK_GPS_INTERFACE;
	//ServerMsgCommand->MsgId = ACK_GPS_TIME;
	//ServerMsgCommand->MsgId = ACK_CH_FILT;
	//ServerMsgCommand->MsgId = ACK_CH_BUFFER;
	//ServerMsgCommand->MsgId = ACK_CH_STATUS;
	//ServerMsgCommand->MsgId = ACK_OPTICAL_STATUS;
	//ServerMsgCommand->MsgId = ACK_LINK_SPEED;
	//ServerMsgCommand->MsgId = ACK_GPS_TIMER;
	//ServerMsgCommand->MsgId = ACK_GPS_STATUS;
	//ServerMsgCommand->MsgId = ACK_TRIGGER_STATUS;
	//ServerMsgCommand->MsgId = ACK_CH_FILTER;
	//ServerMsgCommand->MsgId = ACK_FRAME_COUNTERS;
	//ServerMsgCommand->MsgId = ACK_LED_STATUS;
	switch(ServerMsgCommand->MsgId) {
		case ACK_CH_INTERFACE:	//01、应答设置通道的以太网接口类型，光接口或电接口
			printf("TY-3012S UdpServer ACK_CH_INTERFACE ok!\n");
			printf("TY-3012S UdpServer ServerChInterface->MsgHead:        %08x!\n", ServerChInterface->MsgHead);
			printf("TY-3012S UdpServer ServerChInterface->MsgLength:      %08x!\n", ServerChInterface->MsgLength);
			printf("TY-3012S UdpServer ServerChInterface->MsgSessionId:   %08x!\n", ServerChInterface->MsgSessionId);
			printf("TY-3012S UdpServer ServerChInterface->MsgDeviceId:    %08x!\n", ServerChInterface->MsgDeviceId);
			printf("TY-3012S UdpServer ServerChInterface->MsgVersion:     %08x!\n", ServerChInterface->MsgSessionId);
			printf("TY-3012S UdpServer ServerChInterface->MsgId:          %08x!\n", ServerChInterface->MsgId);
			printf("TY-3012S UdpServer ServerChInterface->MsgChNo:        %08x!\n", ServerChInterface->MsgChNo);
			printf("TY-3012S UdpServer ServerChInterface->MsgChInterface: %08x!\n", ServerChInterface->MsgChInterface);
			printf("TY-3012S UdpServer ServerChInterface->MsgTail:        %08x!\n", ServerChInterface->MsgTail);
			break;
		case ACK_TRIGGER_TIME:	//02、应答设置通道外触发消抖时间，单位us
			printf("TY-3012S UdpServer ACK_TRIGGER_TIME ok!\n");
			printf("TY-3012S UdpServer ServerTriggerTime->MsgHead:       %08x!\n", ServerTriggerTime->MsgHead);
			printf("TY-3012S UdpServer ServerTriggerTime->MsgLength:     %08x!\n", ServerTriggerTime->MsgLength);
			printf("TY-3012S UdpServer ServerTriggerTime->MsgSessionId:  %08x!\n", ServerTriggerTime->MsgSessionId);
			printf("TY-3012S UdpServer ServerTriggerTime->MsgDeviceId:   %08x!\n", ServerTriggerTime->MsgDeviceId);
			printf("TY-3012S UdpServer ServerTriggerTime->MsgVersion:    %08x!\n", ServerTriggerTime->MsgSessionId);
			printf("TY-3012S UdpServer ServerTriggerTime->MsgId:         %08x!\n", ServerTriggerTime->MsgId);
			printf("TY-3012S UdpServer ServerTriggerTime->MsgChNo:       %08x!\n", ServerTriggerTime->MsgChNo);
			printf("TY-3012S UdpServer ServerTriggerTime->MsgTrigerTime: %08x!\n", ServerTriggerTime->MsgTrigerTime);
			printf("TY-3012S UdpServer ServerTriggerTime->MsgTail:       %08x!\n", ServerTriggerTime->MsgTail);
			break;
		case ACK_GPS_INTERFACE:	//03、应答设置GPS接口类型，TTL或485接口
			printf("TY-3012S UdpServer ACK_GPS_INTERFACE ok!\n");
			printf("TY-3012S UdpServer ServerGpsInterface->MsgHead:         %08x!\n", ServerGpsInterface->MsgHead);
			printf("TY-3012S UdpServer ServerGpsInterface->MsgLength:       %08x!\n", ServerGpsInterface->MsgLength);
			printf("TY-3012S UdpServer ServerGpsInterface->MsgSessionId:    %08x!\n", ServerGpsInterface->MsgSessionId);
			printf("TY-3012S UdpServer ServerGpsInterface->MsgDeviceId:     %08x!\n", ServerGpsInterface->MsgDeviceId);
			printf("TY-3012S UdpServer ServerGpsInterface->MsgVersion:      %08x!\n", ServerGpsInterface->MsgSessionId);
			printf("TY-3012S UdpServer ServerGpsInterface->MsgId:           %08x!\n", ServerGpsInterface->MsgId);
			printf("TY-3012S UdpServer ServerGpsInterface->MsgChNo:         %08x!\n", ServerGpsInterface->MsgChNo);
			printf("TY-3012S UdpServer ServerGpsInterface->MsgGpsInterface: %08x!\n", ServerGpsInterface->MsgGpsInterface);
			printf("TY-3012S UdpServer ServerGpsInterface->MsgTail:         %08x!\n", ServerGpsInterface->MsgTail);
			break;
		case ACK_GPS_TIME:	//04、应答设置GPS时间，年、月、日、时、分、秒、毫秒、微妙
			printf("TY-3012S UdpServer ACK_GPS_TIME ok!\n");
			printf("TY-3012S UdpServer ServerGpsTime->MsgHead:        %08x!\n", ServerGpsTime->MsgHead);
			printf("TY-3012S UdpServer ServerGpsTime->MsgLength:      %08x!\n", ServerGpsTime->MsgLength);
			printf("TY-3012S UdpServer ServerGpsTime->MsgSessionId:   %08x!\n", ServerGpsTime->MsgSessionId);
			printf("TY-3012S UdpServer ServerGpsTime->MsgDeviceId:    %08x!\n", ServerGpsTime->MsgDeviceId);
			printf("TY-3012S UdpServer ServerGpsTime->MsgVersion:     %08x!\n", ServerGpsTime->MsgSessionId);
			printf("TY-3012S UdpServer ServerGpsTime->MsgId:          %08x!\n", ServerGpsTime->MsgId);
			printf("TY-3012S UdpServer ServerGpsTime->MsgChNo:        %08x!\n", ServerGpsTime->MsgChNo);
			printf("TY-3012S UdpServer ServerGpsTime->MsgYear:        %08x!\n", ServerGpsTime->MsgYear);
			printf("TY-3012S UdpServer ServerGpsTime->MsgMonth:       %08x!\n", ServerGpsTime->MsgMonth);
			printf("TY-3012S UdpServer ServerGpsTime->MsgDay:         %08x!\n", ServerGpsTime->MsgDay);
			printf("TY-3012S UdpServer ServerGpsTime->MsgHour:        %08x!\n", ServerGpsTime->MsgHour);
			printf("TY-3012S UdpServer ServerGpsTime->MsgMinit:       %08x!\n", ServerGpsTime->MsgMinit);
			printf("TY-3012S UdpServer ServerGpsTime->MsgSecond:      %08x!\n", ServerGpsTime->MsgSecond);
			printf("TY-3012S UdpServer ServerGpsTime->MsgMillisecond: %08x!\n", ServerGpsTime->MsgMillisecond);
			printf("TY-3012S UdpServer ServerGpsTime->MsgMicrosecond: %08x!\n", ServerGpsTime->MsgMicrosecond);
			printf("TY-3012S UdpServer ServerGpsTime->MsgTail:        %08x!\n", ServerGpsTime->MsgTail);
			break;
		case ACK_CH_FILT:	//05、应答设置通道的过滤条件
			printf("TY-3012S UdpServer ACK_CH_FILT ok!\n");
			printf("TY-3012S UdpServer ServerChFilter->MsgHead:        %08x!\n", ServerChFilter->MsgHead);
			printf("TY-3012S UdpServer ServerChFilter->MsgLength:      %08x!\n", ServerChFilter->MsgLength);
			printf("TY-3012S UdpServer ServerChFilter->MsgSessionId:   %08x!\n", ServerChFilter->MsgSessionId);
			printf("TY-3012S UdpServer ServerChFilter->MsgDeviceId:    %08x!\n", ServerChFilter->MsgDeviceId);
			printf("TY-3012S UdpServer ServerChFilter->MsgVersion:     %08x!\n", ServerChFilter->MsgSessionId);
			printf("TY-3012S UdpServer ServerChFilter->MsgId:          %08x!\n", ServerChFilter->MsgId);
			printf("TY-3012S UdpServer ServerChFilter->MsgChNo:        %08x!\n", ServerChFilter->MsgChNo);
			printf("TY-3012S UdpServer ServerChFilter->MsgReset:       %08x!\n", ServerChFilter->MsgReset);
			printf("TY-3012S UdpServer ServerChFilter->MsgMode:        %08x!\n", ServerChFilter->MsgMode);
			printf("TY-3012S UdpServer ServerChFilter->MsgMinLength:   %08x!\n", ServerChFilter->MsgMinLength);
			printf("TY-3012S UdpServer ServerChFilter->MsgMaxLength:   %08x!\n", ServerChFilter->MsgMaxLength);
			printf("TY-3012S UdpServer ServerChFilter->MsgVlan1Enable: %08x!\n", ServerChFilter->MsgVlan1Enable);
			printf("TY-3012S UdpServer ServerChFilter->MsgVlan2Enable: %08x!\n", ServerChFilter->MsgVlan2Enable);
			printf("TY-3012S UdpServer ServerChFilter->MsgGooseEnable: %08x!\n", ServerChFilter->MsgGooseEnable);
			printf("TY-3012S UdpServer ServerChFilter->MsgSmvEnable:   %08x!\n", ServerChFilter->MsgSmvEnable);
			printf("TY-3012S UdpServer ServerChFilter->MsgMmsEnable:   %08x!\n", ServerChFilter->MsgMmsEnable);
			printf("TY-3012S UdpServer ServerChFilter->MsgVlan1Id:     %08x!\n", ServerChFilter->MsgVlan1Id);
			printf("TY-3012S UdpServer ServerChFilter->MsgVlan2Id:     %08x!\n", ServerChFilter->MsgVlan2Id);
			printf("TY-3012S UdpServer ServerChFilter->MsgMacEnable:   %08x!\n", ServerChFilter->MsgMacEnable);
			for(i = 0; i < 16; i++) {
				printf("TY-3012S UdpServer ServerChFilter->MsgMacAddr%02x", i);
				printf("%02x:%02x:",   ServerChFilter->MsgMacAddr[i*6 + 0], ServerChFilter->MsgMacAddr[i*6 + 1]);
				printf("%02x:%02x:",   ServerChFilter->MsgMacAddr[i*6 + 2], ServerChFilter->MsgMacAddr[i*6 + 3]);
				printf("%02x:%02x!\n", ServerChFilter->MsgMacAddr[i*6 + 4], ServerChFilter->MsgMacAddr[i*6 + 5]);
			}
			printf("TY-3012S UdpServer ServerChFilter->MsgTail:        %08x!\n", ServerChFilter->MsgTail);
			break;
		case ACK_CH_BUFFER:	//06、应答检查缓冲FIFO中的数据，每次应答128字节
			printf("TY-3012S UdpServer ACK_CH_BUFFER ok!\n");
			printf("TY-3012S UdpServer ServerChBuffer->MsgHead:        %08x!\n", ServerChBuffer->MsgHead);
			printf("TY-3012S UdpServer ServerChBuffer->MsgLength:      %08x!\n", ServerChBuffer->MsgLength);
			printf("TY-3012S UdpServer ServerChBuffer->MsgSessionId:   %08x!\n", ServerChBuffer->MsgSessionId);
			printf("TY-3012S UdpServer ServerChBuffer->MsgDeviceId:    %08x!\n", ServerChBuffer->MsgDeviceId);
			printf("TY-3012S UdpServer ServerChBuffer->MsgVersion:     %08x!\n", ServerChBuffer->MsgSessionId);
			printf("TY-3012S UdpServer ServerChBuffer->MsgId:          %08x!\n", ServerChBuffer->MsgId);
			printf("TY-3012S UdpServer ServerChBuffer->MsgChNo:        %08x!\n", ServerChBuffer->MsgChNo);
			printf("TY-3012S UdpServer ServerChBuffer->MsgBuffer:      %08x!\n", ServerChBuffer->MsgByte);
			if(ServerChBuffer->MsgByte > 128) {
				ServerChBuffer->MsgByte = 128;
			}
			printf("TY-3012S UdpServer ServerChBuffer->MsgByte:        %08x!", ServerChBuffer->MsgByte);
			for(i = 0; i < ServerChBuffer->MsgByte; i++) {
				if((i % 0x10) == 0) printf("\n%02x:", i);
				printf(" %02x", ServerChBuffer->MsgBuffer[i]);
			}
			printf("\n");
			printf("TY-3012S UdpServer ServerChBuffer->MsgTail:        %08x!\n", ServerChBuffer->MsgTail);
			break;
		case ACK_CH_STATUS:	//07、应答检查通道的以太网接口类型，光接口或电接口
			printf("TY-3012S UdpServer ACK_CH_STATUS ok!\n");
			printf("TY-3012S UdpServer ServerChInterface->MsgHead:        %08x!\n", ServerChInterface->MsgHead);
			printf("TY-3012S UdpServer ServerChInterface->MsgLength:      %08x!\n", ServerChInterface->MsgLength);
			printf("TY-3012S UdpServer ServerChInterface->MsgSessionId:   %08x!\n", ServerChInterface->MsgSessionId);
			printf("TY-3012S UdpServer ServerChInterface->MsgDeviceId:    %08x!\n", ServerChInterface->MsgDeviceId);
			printf("TY-3012S UdpServer ServerChInterface->MsgVersion:     %08x!\n", ServerChInterface->MsgSessionId);
			printf("TY-3012S UdpServer ServerChInterface->MsgId:          %08x!\n", ServerChInterface->MsgId);
			printf("TY-3012S UdpServer ServerChInterface->MsgChNo:        %08x!\n", ServerChInterface->MsgChNo);
			printf("TY-3012S UdpServer ServerChInterface->MsgChInterface: %08x!\n", ServerChInterface->MsgChInterface);
			printf("TY-3012S UdpServer ServerChInterface->MsgTail:        %08x!\n", ServerChInterface->MsgTail);
			break;
		case ACK_OPTICAL_STATUS://08、应答检查通道光接口的状态，有光或无光信号
			printf("TY-3012S UdpServer ACK_OPTICAL_STATUS ok!\n");
			printf("TY-3012S UdpServer ServerOpticalStatus->MsgHead:          %08x!\n", ServerOpticalStatus->MsgHead);
			printf("TY-3012S UdpServer ServerOpticalStatus->MsgLength:        %08x!\n", ServerOpticalStatus->MsgLength);
			printf("TY-3012S UdpServer ServerOpticalStatus->MsgSessionId:     %08x!\n", ServerOpticalStatus->MsgSessionId);
			printf("TY-3012S UdpServer ServerOpticalStatus->MsgDeviceId:      %08x!\n", ServerOpticalStatus->MsgDeviceId);
			printf("TY-3012S UdpServer ServerOpticalStatus->MsgVersion:       %08x!\n", ServerOpticalStatus->MsgSessionId);
			printf("TY-3012S UdpServer ServerOpticalStatus->MsgId:            %08x!\n", ServerOpticalStatus->MsgId);
			printf("TY-3012S UdpServer ServerOpticalStatus->MsgChNo:          %08x!\n", ServerOpticalStatus->MsgChNo);
			printf("TY-3012S UdpServer ServerOpticalStatus->MsgOpticalStatus: %08x!\n", ServerOpticalStatus->MsgOpticalStatus);
			printf("TY-3012S UdpServer ServerOpticalStatus->MsgTail:          %08x!\n", ServerOpticalStatus->MsgTail);
			break;
		case ACK_LINK_SPEED:	//09、应答检查通道光电接口的连接状态，连接、速度、双工
			printf("TY-3012S UdpServer ACK_LINK_SPEED ok!\n");
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgHead:      %08x!\n", ServerEthernetStatus->MsgHead);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgLength:    %08x!\n", ServerEthernetStatus->MsgLength);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgSessionId: %08x!\n", ServerEthernetStatus->MsgSessionId);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgDeviceId:  %08x!\n", ServerEthernetStatus->MsgDeviceId);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgVersion:   %08x!\n", ServerEthernetStatus->MsgSessionId);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgId:        %08x!\n", ServerEthernetStatus->MsgId);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgChNo:      %08x!\n", ServerEthernetStatus->MsgChNo);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgEthOLink:  %08x!\n", ServerEthernetStatus->MsgEthOLink);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgEthOSpeed: %08x!\n", ServerEthernetStatus->MsgEthOSpeed);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgEthODuplex:%08x!\n", ServerEthernetStatus->MsgEthODuplex);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgEthELink:  %08x!\n", ServerEthernetStatus->MsgEthELink);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgEthESpeed: %08x!\n", ServerEthernetStatus->MsgEthESpeed);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgEthEDuplex:%08x!\n", ServerEthernetStatus->MsgEthEDuplex);
			printf("TY-3012S UdpServer ServerEthernetStatus->MsgTail:      %08x!\n", ServerEthernetStatus->MsgTail);
			break;
		case ACK_GPS_TIMER:	//10、应答检查GPS时间，年、月、日、时、分、秒、毫秒、微妙
			printf("TY-3012S UdpServer ACK_GPS_TIMER ok!\n");
			//printf("TY-3012S UdpServer ServerGpsTime->MsgHead:        %08x!\n", ServerGpsTime->MsgHead);
			//printf("TY-3012S UdpServer ServerGpsTime->MsgLength:      %08x!\n", ServerGpsTime->MsgLength);
			//printf("TY-3012S UdpServer ServerGpsTime->MsgSessionId:   %08x!\n", ServerGpsTime->MsgSessionId);
			//printf("TY-3012S UdpServer ServerGpsTime->MsgDeviceId:    %08x!\n", ServerGpsTime->MsgDeviceId);
			//printf("TY-3012S UdpServer ServerGpsTime->MsgVersion:     %08x!\n", ServerGpsTime->MsgSessionId);
			//printf("TY-3012S UdpServer ServerGpsTime->MsgId:          %08x!\n", ServerGpsTime->MsgId);
			printf("TY-3012S UdpServer ServerGpsTime->MsgChNo:        %08x!\n", ServerGpsTime->MsgChNo);
			//printf("TY-3012S UdpServer ServerGpsTime->MsgYear:        %08x!\n", ServerGpsTime->MsgYear);
			//printf("TY-3012S UdpServer ServerGpsTime->MsgMonth:       %08x!\n", ServerGpsTime->MsgMonth);
			printf("TY-3012S UdpServer ServerGpsTime->MsgDay:         %08d!\n", ServerGpsTime->MsgDay);
			printf("TY-3012S UdpServer ServerGpsTime->MsgHour:        %08d!\n", ServerGpsTime->MsgHour);
			printf("TY-3012S UdpServer ServerGpsTime->MsgMinit:       %08d!\n", ServerGpsTime->MsgMinit);
			printf("TY-3012S UdpServer ServerGpsTime->MsgSecond:      %08d!\n", ServerGpsTime->MsgSecond);
			printf("TY-3012S UdpServer ServerGpsTime->MsgMillisecond: %08d!\n", ServerGpsTime->MsgMillisecond);
			printf("TY-3012S UdpServer ServerGpsTime->MsgMicrosecond: %08d!\n", ServerGpsTime->MsgMicrosecond);
			printf("TY-3012S UdpServer ServerGpsTime->MsgTail:        %08x!\n", ServerGpsTime->MsgTail);
			break;
		case ACK_GPS_STATUS:	//11、应答检查GPS告警状态，正常、丢失、错误
			printf("TY-3012S UdpServer ACK_GPS_STATUS ok!\n");
			printf("TY-3012S UdpServer ServerGpsStatus->MsgHead:      %08x!\n", ServerGpsStatus->MsgHead);
			printf("TY-3012S UdpServer ServerGpsStatus->MsgLength:    %08x!\n", ServerGpsStatus->MsgLength);
			printf("TY-3012S UdpServer ServerGpsStatus->MsgSessionId: %08x!\n", ServerGpsStatus->MsgSessionId);
			printf("TY-3012S UdpServer ServerGpsStatus->MsgDeviceId:  %08x!\n", ServerGpsStatus->MsgDeviceId);
			printf("TY-3012S UdpServer ServerGpsStatus->MsgVersion:   %08x!\n", ServerGpsStatus->MsgSessionId);
			printf("TY-3012S UdpServer ServerGpsStatus->MsgId:        %08x!\n", ServerGpsStatus->MsgId);
			printf("TY-3012S UdpServer ServerGpsStatus->MsgChNo:      %08x!\n", ServerGpsStatus->MsgChNo);
			printf("TY-3012S UdpServer ServerGpsStatus->MsgGpsStatus: %08x!\n", ServerGpsStatus->MsgGpsStatus);
			printf("TY-3012S UdpServer ServerGpsStatus->MsgTail:      %08x!\n", ServerGpsStatus->MsgTail);
			break;
		case ACK_TRIGGER_STATUS://12、应答检查通道外触发状态，有无触发，是否正在触发
			printf("TY-3012S UdpServer ACK_TRIGGER_STATUS ok!\n");
			printf("TY-3012S UdpServer ServerTrigerStatus->MsgHead:         %08x!\n", ServerTrigerStatus->MsgHead);
			printf("TY-3012S UdpServer ServerTrigerStatus->MsgLength:       %08x!\n", ServerTrigerStatus->MsgLength);
			printf("TY-3012S UdpServer ServerTrigerStatus->MsgSessionId:    %08x!\n", ServerTrigerStatus->MsgSessionId);
			printf("TY-3012S UdpServer ServerTrigerStatus->MsgDeviceId:     %08x!\n", ServerTrigerStatus->MsgDeviceId);
			printf("TY-3012S UdpServer ServerTrigerStatus->MsgVersion:      %08x!\n", ServerTrigerStatus->MsgSessionId);
			printf("TY-3012S UdpServer ServerTrigerStatus->MsgId:           %08x!\n", ServerTrigerStatus->MsgId);
			printf("TY-3012S UdpServer ServerTrigerStatus->MsgChNo:         %08x!\n", ServerTrigerStatus->MsgChNo);
			printf("TY-3012S UdpServer ServerTrigerStatus->MsgTrigerStatus: %08x!\n", ServerTrigerStatus->MsgTrigerStatus);
			printf("TY-3012S UdpServer ServerTrigerStatus->MsgTail:         %08x!\n", ServerTrigerStatus->MsgTail);
			break;
		case ACK_CH_FILTER:	//13、应答检查通道的过滤条件
			printf("TY-3012S UdpServer ACK_CH_FILTER ok!\n");
			printf("TY-3012S UdpServer ServerChFilter->MsgHead:        %08x!\n", ServerChFilter->MsgHead);
			printf("TY-3012S UdpServer ServerChFilter->MsgLength:      %08x!\n", ServerChFilter->MsgLength);
			printf("TY-3012S UdpServer ServerChFilter->MsgSessionId:   %08x!\n", ServerChFilter->MsgSessionId);
			printf("TY-3012S UdpServer ServerChFilter->MsgDeviceId:    %08x!\n", ServerChFilter->MsgDeviceId);
			printf("TY-3012S UdpServer ServerChFilter->MsgVersion:     %08x!\n", ServerChFilter->MsgSessionId);
			printf("TY-3012S UdpServer ServerChFilter->MsgId:          %08x!\n", ServerChFilter->MsgId);
			printf("TY-3012S UdpServer ServerChFilter->MsgChNo:        %08x!\n", ServerChFilter->MsgChNo);			
			printf("TY-3012S UdpServer ServerChFilter->MsgReset:       %08x!\n", ServerChFilter->MsgReset);
			printf("TY-3012S UdpServer ServerChFilter->MsgMode:        %08x!\n", ServerChFilter->MsgMode);
			printf("TY-3012S UdpServer ServerChFilter->MsgMinLength:   %08x!\n", ServerChFilter->MsgMinLength);
			printf("TY-3012S UdpServer ServerChFilter->MsgMaxLength:   %08x!\n", ServerChFilter->MsgMaxLength);
			printf("TY-3012S UdpServer ServerChFilter->MsgVlan1Enable: %08x!\n", ServerChFilter->MsgVlan1Enable);
			printf("TY-3012S UdpServer ServerChFilter->MsgVlan2Enable: %08x!\n", ServerChFilter->MsgVlan2Enable);
			printf("TY-3012S UdpServer ServerChFilter->MsgGooseEnable: %08x!\n", ServerChFilter->MsgGooseEnable);
			printf("TY-3012S UdpServer ServerChFilter->MsgSmvEnable:   %08x!\n", ServerChFilter->MsgSmvEnable);
			printf("TY-3012S UdpServer ServerChFilter->MsgMmsEnable:   %08x!\n", ServerChFilter->MsgMmsEnable);
			printf("TY-3012S UdpServer ServerChFilter->MsgVlan1Id:     %08x!\n", ServerChFilter->MsgVlan1Id);
			printf("TY-3012S UdpServer ServerChFilter->MsgVlan2Id:     %08x!\n", ServerChFilter->MsgVlan2Id);
			printf("TY-3012S UdpServer ServerChFilter->MsgMacEnable:   %08x!\n", ServerChFilter->MsgMacEnable);
			for(i = 0; i < 16; i++) {
				printf("TY-3012S UdpServer ServerChFilter->MsgMacAddr%02d:" ,i);
				printf("%02x:%02x:",   ServerChFilter->MsgMacAddr[i*6 + 0], ServerChFilter->MsgMacAddr[i*6 + 1]);
				printf("%02x:%02x:",   ServerChFilter->MsgMacAddr[i*6 + 2], ServerChFilter->MsgMacAddr[i*6 + 3]);
				printf("%02x:%02x!\n", ServerChFilter->MsgMacAddr[i*6 + 4], ServerChFilter->MsgMacAddr[i*6 + 5]);
			}
			printf("TY-3012S UdpServer ServerChFilter->MsgTail:        %08x!\n", ServerChFilter->MsgTail);
			break;
		case ACK_FRAME_COUNTERS://14、应答检查帧和字节计数器的统计数据
			printf("TY-3012S UdpServer ACK_FRAME_COUNTERS ok!\n");
			printf("TY-3012S UdpServer ServerFrameCounters->MsgHead:            %08x!\n", ServerFrameCounters->MsgHead);
			printf("TY-3012S UdpServer ServerFrameCounters->MsgLength:          %08x!\n", ServerFrameCounters->MsgLength);
			printf("TY-3012S UdpServer ServerFrameCounters->MsgSessionId:       %08x!\n", ServerFrameCounters->MsgSessionId);
			printf("TY-3012S UdpServer ServerFrameCounters->MsgDeviceId:        %08x!\n", ServerFrameCounters->MsgDeviceId);
			printf("TY-3012S UdpServer ServerFrameCounters->MsgVersion:         %08x!\n", ServerFrameCounters->MsgSessionId);
			printf("TY-3012S UdpServer ServerFrameCounters->MsgId:              %08x!\n", ServerFrameCounters->MsgId);
			printf("TY-3012S UdpServer ServerFrameCounters->MsgChNo:            %08x!\n", ServerFrameCounters->MsgChNo);
			printf("TY-3012S UdpServer ServerFrameCounters->MsgAllFrame:        %08x!\n", ServerFrameCounters->MsgAllFrame);	//00:总帧数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgAllByte:         %08x!\n", ServerFrameCounters->MsgAllByte);		//01:总字节数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgBroadcastFrame:  %08x!\n", ServerFrameCounters->MsgBroadcastFrame);	//02:广播帧数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgBroadcastByte:   %08x!\n", ServerFrameCounters->MsgBroadcastByte);	//03:广播字节数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgMulticastFrame:  %08x!\n", ServerFrameCounters->MsgMulticastFrame);	//04:多播帧数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgMulticastByte:   %08x!\n", ServerFrameCounters->MsgMulticastByte);	//05:多播字节数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgCrcErrFrame:     %08x!\n", ServerFrameCounters->MsgCrcErrFrame);	//06:CRC错误帧数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgErrFrame:        %08x!\n", ServerFrameCounters->MsgErrFrame);	//07:错误帧数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgAllignErrFrame:  %08x!\n", ServerFrameCounters->MsgAllignErrFrame);	//08:定位错误帧数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgShortFrame:      %08x!\n", ServerFrameCounters->MsgShortFrame);	//09:超短帧数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgLongFrame:       %08x!\n", ServerFrameCounters->MsgLongFrame);	//10:超长帧数L
			printf("TY-3012S UdpServer ServerFrameCounters->MsgAllFrameH:       %08x!\n", ServerFrameCounters->MsgAllFrameH);	//00:总帧数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgAllByteH:        %08x!\n", ServerFrameCounters->MsgAllByteH);	//01:总字节数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgBroadcastFrameH: %08x!\n", ServerFrameCounters->MsgBroadcastFrameH);	//02:广播帧数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgBroadcastByteH:  %08x!\n", ServerFrameCounters->MsgBroadcastByteH);	//03:广播字节数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgMulticastFrameH: %08x!\n", ServerFrameCounters->MsgMulticastFrameH);	//04:多播帧数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgMulticastByteH:  %08x!\n", ServerFrameCounters->MsgMulticastByteH);	//05:多播字节数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgCrcErrFrameH:    %08x!\n", ServerFrameCounters->MsgCrcErrFrameH);	//06:CRC错误帧数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgErrFrameH:       %08x!\n", ServerFrameCounters->MsgErrFrameH);	//07:错误帧数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgAllignErrFrameH: %08x!\n", ServerFrameCounters->MsgAllignErrFrameH);	//08:定位错误帧数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgShortFrameH:     %08x!\n", ServerFrameCounters->MsgShortFrameH);	//09:超短帧数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgLongFrameH:      %08x!\n", ServerFrameCounters->MsgLongFrameH);	//10:超长帧数H
			printf("TY-3012S UdpServer ServerFrameCounters->MsgTail:            %08x!\n", ServerFrameCounters->MsgTail);
			break;
		case ACK_LED_STATUS:	//15、应答前面板LED状态，总共36个LED
			printf("TY-3012S UdpServer ACK_LED_STATUS ok!\n");
			printf("TY-3012S UdpServer ServerLedStatus->MsgHead:        %08x!\n", ServerLedStatus->MsgHead);
			printf("TY-3012S UdpServer ServerLedStatus->MsgLength:      %08x!\n", ServerLedStatus->MsgLength);
			printf("TY-3012S UdpServer ServerLedStatus->MsgSessionId:   %08x!\n", ServerLedStatus->MsgSessionId);
			printf("TY-3012S UdpServer ServerLedStatus->MsgDeviceId:    %08x!\n", ServerLedStatus->MsgDeviceId);
			printf("TY-3012S UdpServer ServerLedStatus->MsgVersion:     %08x!\n", ServerLedStatus->MsgSessionId);
			printf("TY-3012S UdpServer ServerLedStatus->MsgId:          %08x!\n", ServerLedStatus->MsgId);
			printf("TY-3012S UdpServer ServerLedStatus->MsgChNo:        %08x!\n", ServerLedStatus->MsgChNo);
			printf("TY-3012S UdpServer ServerLedStatus->MsgLedStatus:");
			for(i = 0; i < 36; i++) {
				if((i % 0x10) == 0) printf("\n%02x:", i);
				printf(" %02x", ServerLedStatus->MsgLedStatus[i]);
			}
			printf("\n");
			printf("TY-3012S UdpServer ServerLedStatus->MsgTail:        %08x!\n", ServerLedStatus->MsgTail);
			break;
		default:
			printf("TY-3012S UdpServer ACK error!\n");
			break;
	}
	}
	return 0;
}
#endif


