/*
文 件 名：ProcessLayerHandler.h
功能描述：过程层业务处理模块：主要包括以下两个功能的接口
          1 SMV和GOOSE的解码，
          2 smv和goose报文的故障判断
作    者：
版    权：
创建日期：2012-09-19
修改记录：2012-09-24

说    明：
        该程序模块是在 windows intel 架构下开发，移植到power pc下，要考虑字节序问题，需要测试
*/


#ifndef _CTD_DSMNET_PROCESS_LAYER_HANDLER_H
#define _CTD_DSMNET_PROCESS_LAYER_HANDLER_H


//typedef __int64 longlong;//在 linux环境下，去掉该语句
typedef unsigned char bool;

#ifndef MAX_MAC_LEN
#define	MAX_MAC_LEN	6
#endif
/********************************************
*
*      通用常量
*
********************************************/
#define MAX_SMV_CHNL_NUM  24              /* smv最大通道数 暂定为24*/
#define MAX_SMV_ASDU_NUM  8               /* asdu最大个数*/
#define MAX_OBJECT_NAME_LENGTH 64         /* 7-2 规定 ，ObjectName的最大长度是 32（第二版为64）*/
#define MAX_OBJECT_REFERENCE_LENGTH 256   /* 7-2 规定 ，ObjectRefe的最大长度是 256   */

/********************************************
*
*      SMV GOOSE TAG
*
********************************************/
#define  TAG_APDU_92           0x60
#define  TAG_APDU_91           0x80
#define  TAG_ASDU_NUM          0x80
#define  TAG_SECURITY          0x81
#define  TAG_SEQUENCE_OF_ASDU  0xa2
#define  TAG_ASDU              0x30

#define  TAG_SV_ID        0x80
#define  TAG_DATASET_REF  0x81
#define  TAG_SMP_CNT      0x82
#define  TAG_CONF_REV     0x83
#define  TAG_REFR_TM      0x84
#define  TAG_SMP_SYNCH    0x85
#define  TAG_SMP_RATE     0x86
#define  TAG_DATASET      0x87


#define TAG_GOOSE_APDU              0x61
#define TAG_GOOSE_GOCBREF           0x80
#define TAG_GOOSE_TIMEALLOWEDTOLIVE 0x81
#define TAG_GOOSE_DATASETREF        0x82
#define TAG_GOOSE_GOID              0x83
#define TAG_GOOSE_TIMESTAMP         0x84
#define TAG_GOOSE_STNUM             0x85
#define TAG_GOOSE_SQNUM             0x86
#define TAG_GOOSE_TEST              0x87
#define TAG_GOOSE_CONFREV           0x88
#define TAG_GOOSE_NEEDCOM           0x89
#define TAG_GOOSE_DATASETENTRIESNUM 0x8a

/********************************************
*
*      SMV type
*
********************************************/
#define  SMV_91     0
#define  SMV_92LE   1
#define  SMV_92     2

/********************************************
*
*      位操作宏
*
********************************************/
#define SETBIT(a, b) ( a |=  (1 << b))
#define CLRBIT(a, b) ( a &= ~(1 << b))


/********************************************
*
*      SMV 报文故障位置定义
*
********************************************/
#define  SMV_Priority           0
#define  SMV_VlanID             1
#define  SMV_APPID              2
#define  SMV_DataNum            6
#define  SMV_DstMAC             7

#define  SMV_SmpPointLost       8   /*采样点丢失*/
#define  SMV_SmpPointRepeated   9   /*采样点重复*/
#define  SMV_SmpReverseOrder    10  /*采样逆序*/
#define  SMV_SmpSynLost         11  /*采样同步丢失*/
#define  SMV_SmpFreq            12  /*采样频率异常*/
#define  SMV_MuClockSyn         13  /*MU时钟同步异常*/
#define  SMV_SmpQuality         14  /*采样值品质异常*/
#define  SMV_SmpData            15  /*采样值数据异常*/

/********************************************
*
*      GOOSE 报文故障位置定义
*
********************************************/
#define		GOOSE_Priority           0
#define		GOOSE_VlanID             1
#define		GOOSE_APPID              2
#define		GOOSE_GocbRef            3
#define		GOOSE_DatSetRef          4
#define		GOOSE_GoID               5
#define		GOOSE_DataEntriesNum     6     /*数据集中数据条目个数*/
#define		GOOSE_DstMAC             7

#define		GOOSE_PacketLost           8   /*报文丢失*/
#define		GOOSE_PacketRepeated       9   /*报文重复*/
#define		GOOSE_PacketSqReverseOrder 10  /*报文顺序逆转*/
#define		GOOSE_PacketStJump         11  /*报文状态跳跃*/
#define		GOOSE_PacketStReverseOrder 12  /*报文状态逆转*/
#define		GOOSE_NdsCom               13  /*需要重新配置*/
#define		GOOSE_StChange             14  /*状态正常变位*/
#define		GOOSE_IEDReset             15  /*IED复归*/
#define		GOOSE_CommuBreak           16  /*GOOSE 断链*/
#define		GOOSE_Test                 17  /*测试信号*/
#define		GOOSE_Invalid              18  /*非法GOOSE信号*/

#define	SELFDEF_HEADER_LEN 	24
/* 报头定义 */
typedef struct tagSelfDefPacketHeader
{
	unsigned int dwTimeStamp1; /*年月日时分秒*/
	unsigned int dwTimeStamp2; /*妙的小数部分*/
	unsigned int dwCapLen;     /*包长*/
	unsigned int dwTag;        /*TAG*/
}SelfDefPacketHeader;

typedef struct tagSmvDataSet
{
	long v;
	unsigned long q;
}SmvDataSet;

typedef struct tagSmvAsduInfo
{
	unsigned short wSmpCnt;    /*采样计数器*/
	bool           bSmpSynch;  /*采样同步*/
	unsigned char  nChnlNum;   /*采样通道数*/
	SmvDataSet dataSet[MAX_SMV_CHNL_NUM];
}SmvAsduInfo;

/*smv for error analyzer*/
typedef struct tagSmvPacketInfo
{
	int nType;       /*采样值协议类型,如果是9-1，不判断业务信息，因为没哟采样计数*/
	int nPriority;
	int nVlanID;
	unsigned short wAppID;
	unsigned char szDstMac[6];
	int nAsduNum;
	SmvAsduInfo asdu[MAX_SMV_ASDU_NUM];/*如果对品质解析有要求，则需要这个，第一阶段先不做品质告警*/
	unsigned int 	dataTime1;
	unsigned int 	dataTime2;
	unsigned char srcMac[MAX_MAC_LEN];
	unsigned char reverse[2];
}SmvPacketInfo;

typedef struct
{
	int nPriority;
	int nVlanID;
	unsigned short appId;					/*AppId*/
	unsigned char destMac[6];				/*配置文件中对应某个IED的目的mac*/
}SmvCfgInfo;


/* goose for error analyzer */
typedef struct tagGoosePacketInfo
{
	int nPriority;
	int nVlanID;
	unsigned short wAppID;
	unsigned char szDstMac[6];
	//
	char szGocbRef[MAX_OBJECT_REFERENCE_LENGTH];
	char szDataSetRef[MAX_OBJECT_REFERENCE_LENGTH]; /*数据集引用*/
	char szGoID[MAX_OBJECT_NAME_LENGTH];

	unsigned long stNum;
	unsigned long sqNum;
	bool  bTest;        /*测试信号*/
	bool  bNdsCom;      /*需要重新配置*/

	int nDataEntriesNum;//数据集内数据条目的个数	

	unsigned int 	dataTime1;
	unsigned int 	dataTime2;
	unsigned char srcMac[MAX_MAC_LEN];
	unsigned char reverse[2];
}GoosePacketInfo;

typedef struct 
{
	int nPriority;
	int nVlanID;
	unsigned short wAppID;
	unsigned char szDstMac[MAX_MAC_LEN];
	char szGocbRef[MAX_OBJECT_REFERENCE_LENGTH];
	char szDataSetRef[MAX_OBJECT_REFERENCE_LENGTH]; /*数据集引用*/
	char szGoID[MAX_OBJECT_NAME_LENGTH];
}GooseCfgInfo;						/*告警分析需要用的配置信息*/

#define	MAX_PROCESS_NUM				10
#define	MSG_PROCESS_SVTYPE			0
#define	MSG_PROCESS_GOOSETYPE		1

typedef struct
{
	long	mtype;					//消息接收方的类型
	unsigned char  type[MAX_PROCESS_NUM];				/*每个包的类型,0为sv,1为goose*/
	unsigned char request;			/*请求命令*/
	unsigned char chNo;			/*通道号*/
	unsigned int count;			/*需要分析的包个数*/
	unsigned char *packets[MAX_PROCESS_NUM];		/*10个sv和goose报文包的指针*/
}MsgProcessLayer;


typedef struct
{
	int no;					/*队列的序号0对应1通道,1对应2通道*/
	int key;					/*创建消息队列的键值*/
}PlhArgs;					/*分析线程启动参数*/

/*告警分析用链表结构*/
typedef struct ParseStandard 
{
	unsigned char		chNo;						/*通道号,用于告警重置*/
	unsigned char 	threadNo;					/*以后如果做成一个分析一个线程,这个编号会有用,现在用不到*/
	unsigned char 	reverse[2];	
	unsigned char 	srcMac[MAX_MAC_LEN];		/*源mac,用于标示IED*/
	unsigned char		smvEffect;					/*smv参考值是否有效,0为无效,1为有效*/
	unsigned char 	gooseEffect;					/*goose参考值是否有效,0为无效,1为有效*/
	SmvPacketInfo	smvInfo;						/*用于前后包比较的smv信息*/
	SmvCfgInfo		smvCfg;						/*用于配置比较的smv信息*/
	GoosePacketInfo	gooseInfo;					/*用于比较的goose信息*/
	GooseCfgInfo		gooseCfg;					/*用于配置比较的goose信息*/
	struct ParseStandard  *next;
}ParseStandardList;						



#define	MSG_RQST_PROCESS		1 	/*分析发来的n个包*/
#define	MSG_RQST_RESET_WARNING	2	/*重置告警分析标志*/

#define	TYPE_PROCESSLAYER	1	/*协议分析告警线程消息队列*/

#define	PROCESS_MSGQUEUE_FULL	-2

#ifndef nCHANNEL
#define	nCHANNEL	2
#endif
#ifndef nIED
#define	nIED		16
#endif
/***************************************************************************
//
// 功能接口
//
****************************************************************************/

/******************************************************************
功   能 ：根据ASN编码规则，得出某个域的长度值，和长度尺寸
输入参数：pointer为某个域 L 的首指针，
输出参数：nValSize为域的长度，nLenSize为长度域L的尺寸（字节数）
返 回 值：成功返回true，失败返回 false
*******************************************************************/
bool GetFieldLength( unsigned char * pointer, int  *nValSize,int  *nLenSize);

/******************************************************************
功    能：根据给出的某个域的字节数，计算出数值并按照 ( longlong )类型返回(小端格式)
输入参数：pointer为某个域的首指针，nSize为
返 回 值：返回指定域的值（小端格式）
*******************************************************************/
long long GetFieldValue(unsigned char * pointer, int nSize);

/*********************************************************************

功    能：Goose解码-用于故障判断
输入参数：pszSource为没有解码前的原始数据包指针，
          nFileFormat为文件格式
输出参数：packetInfo为解码的数据结构

返 回 值：goose 解码成功返回 true，goose报文编码错误时 返回 false

**********************************************************************/
bool  GooseDecode(unsigned char* pszSource, int nFileFormat,GoosePacketInfo *packetInfo);

/*********************************************************************

功    能：Smv解码-用于故障判断,   
输入参数：pszSource为没有解码前的原始数据包指针，
          nFileFormat为文件格式
输出参数：packetInfo为解码的数据结构

返 回 值：解码成功返回 true，解码失败返回 false

**********************************************************************/
bool SmvDecode(unsigned char* pszSource, int nFileFormat, SmvPacketInfo *packetInfo);



/*********************************************************************

功    能：smv 故障、告警分析调用接口,   
输入参数：firstPacketInfo 为上一个smv报文的解码后的信息，
          secondPacketInfo 为下一个相邻 smv报文的解码后的信息
		  pSmvCfgInfo 是smv的配置信息 （这里暂且注释掉，到时候，用装置里边的配置信息结构）
          nDiffTime   为这两个相邻报文之间的时间间隔，单位为微妙

返 回 值：返回告警状态字

**********************************************************************/
unsigned long SmvErrAnalyse(SmvPacketInfo *firstPacketInfo, SmvPacketInfo *secondPacketInfo, int nDiffTime, SmvCfgInfo  *pSmvCfgInfo);

/*********************************************************************

功    能：GOOSE 故障、告警分析调用接口,   
输入参数：firstPacketInfo 为上一个smv报文的解码后的信息，
          secondPacketInfo 为下一个相邻 smv报文的解码后的信息
		  pGooseCfgInfo 是 GOOSE 的配置信息 （这里暂且注释掉，到时候，用装置里边的配置信息结构）
          nDiffTime   为这两个相邻报文之间的时间间隔，单位为微妙

返 回 值：返回告警状态字

**********************************************************************/
unsigned long GooseErrAnalyse( GoosePacketInfo *firstPacketInfo, GoosePacketInfo *secondPacketInfo, int nDiffTime, GooseCfgInfo *pGooseCfgInfo);

#endif
