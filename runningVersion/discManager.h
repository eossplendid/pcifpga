#ifndef	__DISCMANAGER__H__
#define 	__DISCMANAGER__H__

typedef unsigned char 		u8;
typedef char				s8;
typedef unsigned short 	u16;
typedef short				s16;
typedef unsigned int		u32;
typedef int				s32;



#define 	FALSE 								-1
#define 	TRUE								0

#define 	REFRESH_CMD_NEWFILE				0x01
#define	REFRESH_CMD_ENDFILE				0x02
#define 	REFRESH_CMD_FORMATE				0x03
#define 	REFRESH_CMD_POSITION				0x04
#define 	REFRESH_CMD_CREATE				0x05


#define 	FILENAME_LEN						64

#define   TYPE_DSCMANAGER_A				1
#define	TYPE_DSCMANAGER_B				2

#define 	MSGQ_RQST_SAVE					1		//存盘
#define	MSGQ_RQST_SAVE_END				2		//存盘结束
#define	MSGQ_RQST_NEWFILE				3		//新文件名
#define	MSGQ_RQST_FORMAT				4		//格式化分区
#define 	MSGQ_RQST_QUERY					5		//录播文件查询
//#define	MSGQ_RQST_QSAVE					6		//立即存盘
#define	MSGQ_RQST_CLEARREC				7		//清除文件记录信息，按分区清除
#define 	MSGQ_RQST_DISKCHECK				8		//磁盘故障检测
#define	MSGQ_RQST_UPLOAD					9		//录播文件上传请求
//#define	MSGQ_RQST_UPLOADING				10		//分段上传
#define 	MSGQ_RQST_STOP_UPLOAD			11		//终止传输

/*存盘文件文件头定义*/
#if 0
#define	FILE_HEADER_MAGIC_SELF		0xA5B6C7D8		/*标识位,a1b2c3d4*/
#define	FILE_HEADER_MAGIC		0xA1B2C3D4		/*标识位,a1b2c3d4*/
#define	FILE_HEADER_MAJOR		0x02			/*主版本号,默认0x02*/
#define	FILE_HEADER_MINOR		0x04			/*副版本号,默认0x04*/
#define	FILE_HEADER_THISZONE	0x0				/*区域时间,没用,全0*/
#define	FILE_HEADER_SIGFIGS	0x0				/*精确时间戳,没用，全0*/
#define	FILE_HEADER_SNAPLEN	0xffffffff		/*数据包最大长度,0--65535*/
#define	FILE_HEADER_LINKTYPE_ETH	0x01		/*链路层类型,0x01表示以太网类型*/
#else
#define	FILE_HEADER_MAGIC_SELF		0xD8C7B6A5		/*标识位,a1b2c3d4*/
#define	FILE_HEADER_MAGIC		0xD4C3B2A1		/*标识位,a1b2c3d4*/
#define	FILE_HEADER_MAJOR		0x0200			/*主版本号,默认0x02*/
#define	FILE_HEADER_MINOR		0x0400			/*副版本号,默认0x04*/
#define	FILE_HEADER_THISZONE	0x0				/*区域时间,没用,全0*/
#define	FILE_HEADER_SIGFIGS	0x0				/*精确时间戳,没用，全0*/
#define	FILE_HEADER_SNAPLEN	0xffffffff		/*数据包最大长度,0--65535*/
#define	FILE_HEADER_LINKTYPE_ETH	0x01000000		/*链路层类型,0x01表示以太网类型*/
#endif
#define	FILE_TAIL_TAG		0x7e7e7e7e			/*文件尾标识位*/
typedef struct
{
	unsigned long tailTag;			/*标识位,0x7e7e7e7e*/
	unsigned long packetCount;		/*总包数*/
}FileTail;

//一些定义
typedef struct tagFileHeader{			// 文件的头部共计24字节
	u32		dwMagic;	// 固定0x1A2B3C4D
	u16		wMajor;		// 主版本号 0x0200
	u16		wMinor;		// 次版本号 0x0400
	u32		dwThisZone;	// 当地的标准时间，默认全零
	u32		dwSigFigs;	// 时间戳的精度，默认全零
	u32		dwSnapLen;	// 报文最大存储长度，填充0x0000FFFF，4字节
	u32		dwLinkType;	// 链路类型，这里填0x01000000，表示是Ethernet链路
}FileHeader;

typedef struct
{
	FileHeader header;				/*标准文件头*/
	unsigned int cfginfolen;			/*配置文件长度*/
	unsigned char *pCfgInfo;					/*配置文件内容*/
}CustomFHeader;

typedef struct MsgQueueBuf{
	long 	mtype;				//消息接收方的类型
	long 	msgSrc;				//消息发送方的类型
	u8 		msgRequest;			//消息请求的类型
	u8		msgResponse;		//消息回复的结果
	u8 		msgReverse[2];		//结构对齐
	u8		msgParam[FILENAME_LEN];			//存放获得的文件路径
	s32 		msgParam2;			//通道号
	long 	msgParam3;			//包个数,写盘大小
	s32		msgParam4;			/*Socket描述符/缓冲区号*/
}MsgQueueBuf;


typedef struct
{
	int chNo;
	int sockfd;
	unsigned long session;
}QueryParam;

typedef struct
{
	char filename[FILENAME_LEN];
	int sockfd;
	unsigned long session;
}FileInfoParam;


typedef struct
{
	char year[4];					
	char mon[2];
	char day[2];
	char hour[2];
	char min[2];
	char sec[2];
}TimeStr;

void dm_discManagerThread( );													//磁盘管理线程入口
void dm_msgDiscMngSend(int quest, char *param1, int param2, long param3, int param4);			//磁盘管理发消息接口
#endif
