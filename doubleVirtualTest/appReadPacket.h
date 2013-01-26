#ifndef	__APPREADPACKET_H__
#define	__APPREADPACKET_H__
#if 0
const static unsigned short TPID = 0x8100;
const static unsigned short ETHERNET_TYPE_GOOSE  = 0x88B8;
const static unsigned short ETHERNET_TYPE_SMV    = 0x88BA;
const static unsigned short ETHERNET_TYPE_VLAN   = 0x8100;
const static unsigned short ETHERNET_TYPE_IP     = 0x0800;
const static unsigned short ETHERNET_TYPE_ARP    = 0x0806;
const static unsigned short ETHERNET_TYPE_RARP   = 0x8035;
const static unsigned short ETHERNET_TYPE_GMRP   = 0x0001;	//不确定
#endif
//#define MBYTES		(1024 * 1024)
#define MBYTES		0x100000			/*1MB*/
#define MBYTES_30	0x1E00000			/*30MB*/
#define MBYTES_20	0x1400000			/*20MB*/
#define MBYTES_10	0xA00000			/*10MB*/
#define MAXQSIZE 	0x2000000			/*32MB*/
#ifndef MAX_MAC_LEN
#define	MAX_MAC_LEN		6
#endif

#define	TPID	0x8100
#define	ETHERNET_TYPE_GOOSE		0x88B8
#define	ETHERNET_TYPE_SMV		0x88BA
#define	ETHERNET_TYPE_VLAN		0x8100
#define	ETHERNET_TYPE_IP			0x0800
#define	ETHERNET_TYPE_ARP		0x0806
#define	ETHERNET_TYPE_RARP		0x8035
#define	ETHERNET_TYPE_GMRP		0x0001

#define	TYPE_APPREAD_A					1		/*1通道接收方消息类型号*/
#define	TYPE_APPREAD_B					2		/*2通道*/

#define 	MSGQ_RQST_WRITE_END				1		/*写盘结束，更新缓冲标志*/
#define	MSGQ_RQST_DRV_IOCTRL			2		/*驱动控制,由UDP接收命令后发出*/
#define	MSGQ_RQST_SAVE_FILE				3		/*快速存盘,结束文件*/
#define	MSGQ_RQST_SAVE_DATA				4		/*立即存盘,存数据*/

#define	APP_SAVE_TIMEOUT					0		/*超时存盘*/
#define	APP_SAVE_NORMAL					1		/*正常存盘*/
typedef struct 
{
	long 	mtype;				//消息接收方的类型
	u8 		msgRequest;			//消息请求的类型
	u8 		msgReverse[3];		//结构对齐
	//int		msgParam;			/*通道号*/
	unsigned int 		msgParam2;			/*虚拟分区号,1,2,3*/
	unsigned long 	msgParam3;			/*写盘大小*/
}MsgAppBuf;


#define 	READ_DRIVER		"/dev/pcifpga"



typedef struct
{
	unsigned char *buf;				/*缓冲区指针*/
	unsigned char *rear;				/*尾指针,指向缓冲区当前数据尾部*/
	unsigned char *analyz;				/*分析指针，指向整包包头*/
	unsigned char *bufBStart;			/*缓冲2区的起始位置*/
	unsigned char *bufCStart;			/*缓冲3区的起始位置*/
	unsigned char currentBuffer;		/*30MB缓冲分为三个区,0:1区,1:2区,2:3区*/
	unsigned char bufStatus[3];			/*0为闲,1为忙(存盘中)*/
	//unsigned long bufALength;			/*缓冲1区的存盘长度*/
	//unsigned long bufBLength;			/*缓冲2区的存盘长度*/
	//unsigned long bufCLength;			/*缓冲3区的存盘长度*/
}BufferStatus;

typedef struct
{
	unsigned char 	chNo;						/*通道号*/
	unsigned char 	svEnable;			/*0为关闭sv上传,1为使能sv上传*/
	unsigned char 	dstMac[MAX_MAC_LEN];		/*目的MAC地址*/
	unsigned char 	srcMAC[MAX_MAC_LEN];		/*源MAC地址*/
	unsigned short 	appID;						/*SMV AppID*/
}SvUploadCondition;							/*指定SV上传的具体条件*/

typedef struct
{
	unsigned char stopParse;			/*停止报文分析标志,0为关闭,1为开启(不分析报文)*/
	unsigned char queueFull;			/*队列满标志,0为可用(清空后置0),1为满*/
	unsigned char newFile;				/*0表示没有已打开的文件,1表示已有,0时申请新文件名*/
	unsigned char networkStatus;		/*0为忙,1为闲(无数据包)*/
	unsigned char time[16];				/*首尾包的时间*/
	unsigned long totalPackets;			/*总包数*/
	unsigned long	totalLength;			/*已存数据大小*/
	unsigned long svPackets;			/*用于sv报文个数统计*/
	unsigned long goosePackets;		/*用于goose报文个数统计*/
	BufferStatus	bufferStatus;			/*缓冲区的一些状态*/
}Flags;

#define APP_MSGBUFF_READ_SET(ptr, no, chNo, buf) \
do \
{ \
	AppReadBuffer[no]->ChNo = chNo; \
	AppReadBuffer[no]->Byte = CH_BUF_LEN; \
	memcpy(ptr, buf, MSG_BUF_LEN); \
}while(0)

#define	APP_FILE_SAVE_END_SET(appFlag, chNo)	\
do \
{\
	dm_msgDiscMngSend(MSGQ_RQST_SAVE_END, NULL, chNo, appFlag->totalPackets, 0); \
	appFlag->totalLength = 0; \
	appFlag->totalPackets = 0; \
	appFlag->newFile = 0; \
}while(0)

#endif
