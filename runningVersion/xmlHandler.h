#ifndef __XML_HANDLER__
#define __XML_HANDLER__

#include <libxml/xmlmemory.h>	//新增加的库函数
#include <libxml/parser.h>	//新增加的库函数



#define XML_RECORD_FILE1A	"/sda/diskA/fileRecord.xml"
#define XML_RECORD_FILE1C	"/sda/diskC/fileRecord.xml"
#define XML_RECORD_FILE2A	"/sdb/diskA/fileRecord.xml"
#define XML_RECORD_FILE2C	"/sdb/diskC/fileRecord.xml"

#define XML_CONFIGFILE		"/root/hp/DeviceConfig.xml"

#define DISKA_STRING		"DiskA"
#define DISKB_STRING		"DiskB"
#define DISKC_STRING		"DiskC"
#define DISKD_STRING		"DiskD"
#define DISKE_STRING		"DiskE"
#define DISKF_STRING		"DiskF"

#define nMAC				16	//设置每个通道的Mac过滤源地址
#define nVLAN				2	//设置每个通道的Vlan过滤地址
#define nIED					16	//每台装置每个通道可以测试IED的数量，目前的2U装置每个通道可以测试16个IED
#define nCHANNEL			2	//每台装置的通道数量，目前的2U装置有2个通道
#define NAMELEN				32	//名称等的字符串长度

//说明:
//Device简称Dev,Version简称Ver,LocalIp(LocIp)简称LoIp,SubnetMask简称SnMask,Gateway简称Gway
//RemoteIp简称ReIp,RemotePort简称RePort,SubNo简称SubN,SubName简称Sname,ChannelNum简称ChNum
//FileFormat简称DevFileF,FileSavedPeriod简称FileP,FileMaxSize简称FileS
//HarddiskCheckedPercentage简称Hdisk,TimeZone简称DevTimeZ,HeartBeatPeriod简称HeartBeatP
//GpsInterface简称GpsIf,ChannelNo简称ChNo,ChannelInterfaceType简称ChIfType
//ChannelTriggerTime简称ChTrtime,ChannelCapturePacket简称ChPacket
//ChannelMinFrameLen简称ChMinLen,ChannelMaxFrameLen简称ChMaxLen
//ChannelVlanNo简称ChVlanNo,ChannelVlanId简称ChVlanId,ChannelMacNo简称ChMacNo
//ChannelMacAddr简称ChMacAdd,ChannelPacketType简称ChMacPkt
//ChannelIedName简称ChIedName,ChannelIedMac简称ChIedMac,ChannelIedType简称ChIedType
//ChannelIedBay简称ChIedBay,ChannelIedManufacturer简称ChIedMan
//ChannelIedDesc简称ChIedDesc,ChIedBayIp简称ChIedBip
//ChannelIedMuType简称ChMuType,ChannelIedMuDestMac简称ChIedMuDmac,ChannelIedMuSourceMac简称ChMuSmac
//ChannelIedMuSmpRate简称ChMuSmpR,ChannelIedMuSmpDeviation简称ChMuSmpD
//ChannelIedMuVlanId简称ChMuVlanI,ChannelIedMuPriority简称ChMuPrio
//ChannelIedMuAppId简称ChMuApId,ChannelIedMuSmvId简称ChMuSvId
//ChannelIedMuDatasetRef简称ChMuDref,ChannelIedMuChannelDesc简称ChMuDesc
//
//
#if 0
//定义装置配置消息
#define SET_CH_INTERFACE		0x01		//01、设置通道的以太网接口类型，光接口或电接口
//#define SET_TRIGGER_TIME		0x02		//02、设置通道外触发消抖时间，单位us
//#define SET_GPS_INTERFACE		0x03		//03、设置GPS接口类型，TTL或485接口
//#define SET_TRIGGER_TIME			0x04		//04、设置GPS时间，年、月、日、时、分、秒、毫秒、微妙
//#define SET_CH_FILTER			0x05		//05、设置通道的过滤器，即过滤条件
//定义配置消息应答
#define ACK_CH_INTERFACE		0x81		//01、应答设置通道的以太网接口类型，光接口或电接口
//#define ACK_TRIGGER_TIME		0x82		//02、应答设置通道外触发消抖时间，单位us
//#define ACK_GPS_INTERFACE		0x83		//03、应答设置GPS接口类型，TTL或485接口
#define ACK_GPS_TIME			0x84		//04、应答设置GPS时间，年、月、日、时、分、秒、毫秒、微妙
#define ACK_CH_FILT				0x85		//05、应答设置通道的过滤条件
//定义状态查询消息
#define READ_CH_BUFF			0x10		//06、读取缓冲FIFO中的数据，每次最大2M字节
#define CHECK_CH_BUFFER		0x11		//06、检查缓冲FIFO中的数据，每次检查128字节
#define CHECK_CH_INTERFACE		0x12		//07、检查通道的以太网接口类型，光接口或电接口
#define CHECK_OPTICAL_STATUS	0x13		//08、检查通道光接口的状态，有光或无光信号，以便确定是否光信号丢失
#define CHECK_LINK_SPEED		0x14		//09、检查通道光电接口的连接状态，连接、速度、双工
#define CHECK_GPS_TIMER		0x15		//10、检查GPS时间，年、月、日、时、分、秒、毫秒、微妙
#define CHECK_GPS_STATUS		0x16		//11、检查GPS接口的告警状态，正常、丢失、错误
//#define CHECK_TRIGGER_STATUS	0x17		//12、检查通道外触发状态，有无触发，是否正在触发
//#define CHECK_CH_FILTER		0x18		//13、检查通道的过滤条件
#define CHECK_FRAME_COUNTERS	0x19		//14、检查帧和字节计数器的统计数据
//定义查询消息应答
#define ACK_CH_BUFFER			0x91		//06、应答检查缓冲FIFO中的数据，每次应答128字节
#define ACK_CH_STATUS			0x92		//07、应答检查通道的以太网接口类型，光接口或电接口
#define ACK_OPTICAL_STATUS		0x93		//08、应答检查通道光接口的状态，有光或无光信号
#define ACK_LINK_SPEED			0x94		//09、应答检查通道光电接口的连接状态，连接、速度、双工
#define ACK_GPS_TIMER			0x95		//10、应答检查GPS时间，年、月、日、时、分、秒、毫秒、微妙
#define ACK_GPS_STATUS			0x96		//11、应答检查GPS告警状态，正常、丢失、错误
#define ACK_TRIGGER_STATUS	0x97		//12、应答检查通道外触发状态，有无触发，是否正在触发
#define ACK_CH_FILTER			0x98		//13、应答检查通道的过滤条件
#define ACK_FRAME_COUNTERS	0x99		//14、应答检查帧和字节计数器的统计数据
//定义面板查询消息
#define CHECK_LED_STATUS		0x21		//15、检查前面板LED状态，总共36个LED
//定义面板查询应答
#define ACK_LED_STATUS			0xa1		//15、应答前面板LED状态，总共36个LED

#endif

#define	MSGHEAD				0x1a2b3c4d		//消息头部4字节，1a2b3c4d
#define	MSGSESSIONID			0x0			//消息会话4字节，服务器端发送0，客户端加1发送回去
#define	MSGDEVICEID			0x0			//消息装置4字节，该种类型的装置专用，定义为0
#define	MSGVERSION			0x1			//消息版本4字节，消息的版本号，定义为1.0版本
#define	MSGID					0x0			//消息命令4字节，空定义为0
#define	MSGCHNO1				0x1			//消息通道4字节，通道1，值=1
#define	MSGCHNO2				0x2			//消息通道4字节，通道2，值=2
#define	MSGTAIL				0xa1b2c3d4		//消息尾部4字节，a1b2c3d4

#define	EINTERFACE				0x00			//接口类型4字节，以太网电接口=0
#define	OINTERFACE				0x01			//接口类型4字节，以太网光接口=1
#define	TRIGERTIME				0x5a			//通道外触发消抖时间，单位us，最小0，最大256
#define	GPSTTLINTERFACE		0x0			//接口类型4字节，GPS接口类型，TTL或485接口，0=TTL，1=485
#define	GPS485INTERFACE		0x1			//接口类型4字节，GPS接口类型，TTL或485接口，0=TTL，1=485

#define	GPSYEAR				2011			//GPS时间，年
#if 0
#define	GPSMONTH				11			//GPS时间，月
#define	GPSDAY					07			//GPS时间，日
#define	GPSHOUR				10			//GPS时间，时
#define	GPSMINIT				30			//GPS时间，分
#endif

#define	GPSSECOND				0			//GPS时间，秒
#define	GPSMILLISECOND		0			//GPS时间，毫秒
#define	GPSMICROSECOND		0			//GPS时间，微秒

#define	CHRESET				0x01			//复位控制，0不要求复位，1要求复位
#define	CHMODE					0x01			//过滤模式，1不过滤，0过滤
#define	MINLENGTH				64			//最小帧长，隐含是64字节
#define	MAXLENGTH				1518			//最大帧长，隐含是1518字节
#define	VLAN1ENABLE			0x01			//Vlan1使能，D0=1使能
#define	VLAN2ENABLE			0x02			//Vlan2使能，D1=1使能
#define	GOOSEENABLE			0x04			//Goose使能，D2=1使能
#define	SMVENABLE				0x08			//Smv  使能，D3=1使能
#define	MMSENABLE				0x10			//Mms  使能，D4=1使能
#define	VLAN1ID					0x0255			//Vlan1标签
#define	VLAN2ID					0x01aa			//Vlan2标签
#define	MACENABLE				0x55			//Mac  使能，D0到D15=1使能


//#define MAXRECORDS		3400		//录播文件最多个数
#define PATITION_NUMS			6			//分区个数
#define MAXFILES					400			//每个分区最多文件个数
#define NODE_DISKA				0
#define NODE_DISKB				1
#define NODE_DISKC				2
#define NODE_DISKD				3
#define NODE_DISKE				4
#define NODE_DISKF				5

typedef struct tagDevice
{
	//1、装置的列表部分，名称、Ip地址、编号，3变量
	char*	DevName;				//装置名称，记录分析装置的名称!
	char*	DevIp;					//装置的ip，记录分析装置网管接口的ip地址!
	char*	DevNo;					//装置编号，记录分析装置的编号，每台装置给一个编号?
	//2、装置的公共部分，版本号，1变量
	char*	DevVer;				//装置版本，记录分析装置版本号，xml文件的版本号!
	//3、装置的地址部分，Ip地址、子网掩码、网关、网管服务器Ip地址、网管服务器端口，5变量
	char*	DevLoIp;			//装置的ip，记录分析装置网管接口的ip地址!
	char*	DevSnMask;			//装置掩码，记录分析装置网管接口子网掩码!
	char*	DevGway;			//装置网关，记录分析装置网管接口网关!
	char*	DevReIp;			//装置远端Ip，配置计算机服务器Ip地址，谁给的配置文件!
	char*	DevRePort;			//装置远端口，配置计算机服务器端口号，不需要!
	//4、装置的参数部分，编号、名称、通道数量、报文存储格式、存储周期、最大容量、如何检查、时区、GPS接口，10变量
	char*	DevSubN;			//装置的子编号，记录分析装置的编号，每台装置给一个编号!
	char*	DevSname;			//装置的子名称，记录分析装置的名称!
	char*	DevChNum;			//装置通道数量，记录分析装置有2个通道，隐含2个通道!
	char*	DevFileF;			//装置文件格式，记录分析装置文件存储格式，隐含自定义!
	char*	DevFileP;			//装置存储周期，记录分析装置文件存储周期，隐含600秒!
	char*	DevFileS;			//装置文件大小，记录分析装置文件存储大小，50M!
	char*	DevHdisk;			//装置磁盘检查，记录分析装置磁盘百分比，50％!
	char*	DevTimeZ;			//装置所在时区，记录分析装置时间的时区，隐含时区8!
	char*	DevBeatP;			//装置心跳周期，记录分析装置和服务器的联系周期，隐含3秒!
	char*	DevGpsIf;			//装置GPS接口， 记录分析装置GPS接口类型，每通道1个GPS接口，2通道相同，0:TTL，1:485，缺省为0!
	//5、装置的通道部分，通道号、光电接口类型、触发时间、抓包类型，2个通道8变量
	char*	ChNo[nCHANNEL];			//装置通道编号，每台设备或测试卡有2个通道，No1为1通道!
	char*	ChIfType[nCHANNEL];		//装置接口类型，每个测试通道有2种接口，光接口或电接口，0电，1光，隐含电!
	char*	ChTrTime[nCHANNEL];		//装置消抖时间，每个测试通道有1个外触发接口，设定该外触发接口的消抖时间?目前没有
	char*	ChPacket[nCHANNEL];		//装置抓包类型，每个测试通道可以抓取SMV/GOOSE/MMS三种报文，如何定义!?
	char*  	ChAcsLocation[nCHANNEL];	//接入点描述
	//6、装置通道n过滤部分
	char*	ChMinLen[nCHANNEL];		//装置最小帧长，接收最小帧长64字节，不能小于64字节!
	char*	ChMaxLen[nCHANNEL];		//装置最大帧长，接收最大帧长1514字节!
	char*	ChVlanNo[nCHANNEL][nVLAN];	//装置的Vlan1， 通道1的Vlan1的编号，有编号则使能Vlan1!
	char*	ChVlanId[nCHANNEL][nVLAN];	//装置Vlan标签，Vlan1的值/Vlan2的值!
	char*	ChMacNo [nCHANNEL][nMAC];	//装置MAC过滤号，有编号则使能相应Mac地址，1到16个号码!
	char*	ChMacAdd[nCHANNEL][nMAC];	//装置MAC源地址，1到16个Mac源过滤地址!
	char*	ChMacPkt[nCHANNEL];		//装置源MAC地址过滤使能!!!?
	//7、装置通道n的被测IED部分，1个通道记录分析16个IED
	char*	ChIedName[nCHANNEL][nIED];	//测试通道n的被测IED的名称!
	char*	ChIedMac [nCHANNEL][nIED];	//测试通道n的被测IED的Mac地址!
	char* 	ChIedType[nCHANNEL][nIED];	//测试通道n的被测IED的类型，合并单元、保护设备、MMS客户端或服务器，编辑出来的!
	char* 	ChIedBay [nCHANNEL][nIED];	//测试通道n的被测IED的间隔，间隔号!
	char* 	ChIedMan [nCHANNEL][nIED];	//测试通道n的被测IED的厂家，厂家名称!
	char* 	ChIedDesc[nCHANNEL][nIED];	//测试通道n的被测IED的说明，厂家装置型号或说明!
	char* 	ChIedBip1[nCHANNEL][nIED];	//测试通道n的被测IED的间隔IP1!
	char*	ChIedBip2[nCHANNEL][nIED];	//测试通道n的被测IED的间隔IP2!
	//8、装置通道n的被测IED的SMV部分
	char*	ChMuType[nCHANNEL][nIED];	//测试通道n的被测IED合并单元类型!
	char*	ChMuDmac[nCHANNEL][nIED];	//测试通道n的被测IED合并单元目的Mac地址!
	char* 	ChMuSmac[nCHANNEL][nIED];	//测试通道n的被测IED合并单元源Mac地址!
	char* 	ChMuSmpR[nCHANNEL][nIED];	//测试通道n的被测IED合并单元采样值速率!
	char* 	ChMuSmpD[nCHANNEL][nIED];	//测试通道n的被测IED合并单元采样值间隔!
	char* 	ChMuVlId[nCHANNEL][nIED];	//测试通道n的被测IED合并单元Vlan!
	char* 	ChMuPrio[nCHANNEL][nIED];	//测试通道n的被测IED合并单元优先级!
	char* 	ChMuApId[nCHANNEL][nIED];	//测试通道n的被测IED合并单元AppId!
	char*	ChMuSvId[nCHANNEL][nIED];	//测试通道n的被测IED合并单元SmvId(Smv92)!
	char*	ChMuDref[nCHANNEL][nIED];	//测试通道n的被测IED合并单元IedMuDatasetRef(Smv92)!
	char*	ChMuDesc[nCHANNEL][nIED];	//测试通道n的被测IED合并单元IedMuChannelDesc(Smv92)!
	//9、装置通道1的被测IED的GOOSE部分
	//	add by haopeng 2012.10.6
	//属性部分
	char*	ChGSAref[nCHANNEL][nIED];	//测试通道n的被测IED合并单元GOOSE属性cbRef
	char*	ChGSADref[nCHANNEL][nIED];  //测试通道n的被测IED合并单元GOOSE属性datasetRef
	char*	ChGSAGoId[nCHANNEL][nIED];	//GOOSE属性goID
	//GOOSE节点部分
	char*	ChGSGoId[nCHANNEL][nIED];	//gooseId
	char*	ChGSDmac[nCHANNEL][nIED];	//goose目的mac
	char* 	ChGSSmac[nCHANNEL][nIED];	//goose源mac1
	char*	ChGSSmac2[nCHANNEL][nIED];	//goose源mac2
	char*	ChGSVlId[nCHANNEL][nIED];	//VlanId
	char*	ChGSPrio[nCHANNEL][nIED];	//Priority
	char*	ChGSApId[nCHANNEL][nIED];	//appId
	char* 	ChGSref[nCHANNEL][nIED];	//CbRef
	char*	ChGSDref[nCHANNEL][nIED];	//datasetRef
	char*	ChGSLdInst[nCHANNEL][nIED];	//LdInst
	char*	ChGSminTm[nCHANNEL][nIED];	//minTime
	char*	ChGSmaxTm[nCHANNEL][nIED];	//maxTime
	#if 0
	char*	ChGSChDesc					//no use
	#endif
	//
}DeviceStr;

typedef struct parDevice
{
	//1、装置的列表部分，名称、Ip地址、编号，3变量（40字节）
	char	DevName[NAMELEN];			//装置名称，记录分析装置的名称!
	char	DevIp[NAMELEN];				//装置的ip，记录分析装置网管接口的ip地址!
	int	DevNo;					//装置编号，记录分析装置的编号，每台装置给一个编号?
	//2、装置的公共部分，版本号，1变量（4字节）
	char	DevVer[NAMELEN];			//装置版本，记录分析装置版本号，xml文件的版本号!
	//3、装置的地址部分，Ip地址、子网掩码、网关、网管服务器Ip地址、网管服务器端口，5变量（20字节）
	char	DevLoIp[NAMELEN];			//装置的ip，记录分析装置网管接口的ip地址!
	char	DevSnMask[NAMELEN];			//装置掩码，记录分析装置网管接口子网掩码!
	char	DevGway[NAMELEN];			//装置网关，记录分析装置网管接口网关!
	char	DevReIp[NAMELEN];			//装置远端Ip，配置计算机服务器Ip地址，谁给的配置文件!
	int	DevRePort;				//装置远端口，配置计算机服务器端口号，不需要!
	//4、装置的参数部分，编号、名称、通道数量、报文存储格式、存储周期、最大容量、如何检查、时区、GPS接口，10变量（68字节）
	int	DevSubN;				//装置的子编号，记录分析装置的编号，每台装置给一个编号!
	char	DevSname[NAMELEN];			//装置的子名称，记录分析装置的名称!
	int	DevChNum;				//装置通道数量，记录分析装置有2个通道，隐含2个通道!
	int	DevFileF;				//装置文件格式，记录分析装置文件存储格式，隐含自定义!
	int	DevFileP;				//装置存储周期，记录分析装置文件存储周期，隐含600秒!
	int	DevFileS;				//装置文件大小，记录分析装置文件存储大小，50M!
	int	DevHdisk;				//装置磁盘检查，记录分析装置磁盘百分比，50％!
	int	DevTimeZ;				//装置所在时区，记录分析装置时间的时区，隐含时区8!
	int	DevBeatP;				//装置心跳周期，记录分析装置和服务器的联系周期，隐含3秒!
	int	DevGpsIf;				//装置GPS接口， 记录分析装置GPS接口类型，每通道1个GPS接口，2通道相同，0:TTL，1:485，缺省为0!
	//5、装置的通道部分，通道号、光电接口类型、触发时间、抓包类型，2个通道8变量（16字节）
	int	ChNo[nCHANNEL];				//装置通道编号，每台设备或测试卡有2个通道，No1为1通道!
	int	ChIfType[nCHANNEL];			//装置接口类型，每个测试通道有2种接口，光接口或电接口，0电，1光，隐含电!
	int	ChTrTime[nCHANNEL];			//装置消抖时间，每个测试通道有1个外触发接口，设定该外触发接口的消抖时间?目前没有
	int	ChPacket[nCHANNEL];			//装置抓包类型，每个测试通道可以抓取SMV/GOOSE/MMS三种报文，如何定义!?
	char ChAcsLocation[nCHANNEL][NAMELEN];	//接入点名称记录

	//6、装置通道n过滤部分
		//ChPacket[nChannel]:
		//0:接收全部帧,包括广播帧;1:接收SMV帧,即SV过滤使能,2:接收GOOSE帧,即GOOSE过滤使能;3:接收MMS帧,即MMS帧使能
		//4:接收SMV帧和GOOSE帧,即SV和GOOSE过滤使能,5:接收VLAN1帧,即VLAN1使能,6:接收VLAN2帧,即VLAN2使能
		//7:接收VLAN1和VLAN2帧,即VLAN2和VLAN2使能,8:接收MAC流
		//ChMacPkt[nChannel]:MAC流,16bit定义16个MAC流
	int	ChMinLen[nCHANNEL];			//装置最小帧长，接收最小帧长64字节，不能小于64字节!
	int	ChMaxLen[nCHANNEL];			//装置最大帧长，接收最大帧长1514字节!
	int	ChVlanNo[nCHANNEL][nVLAN];		//装置的Vlan1， 通道1的Vlan1的编号，有编号则使能Vlan1!
	int	ChVlanId[nCHANNEL][nVLAN];		//装置Vlan标签，Vlan1的值/Vlan2的值!
	int	ChMacNo [nCHANNEL][nMAC];		//装置MAC过滤号，有编号则使能相应Mac地址，1到16个号码!
	int	ChMacAdd[nCHANNEL][nMAC][6];		//装置MAC源地址，1到16个Mac源过滤地址!
	int	ChMacPkt[nCHANNEL];			//装置抓包类型、过滤方式和过滤使能!!!?
	//7、装置通道n的被测IED部分，1个通道记录分析16个IED
	char	ChIedName[nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED的名称!
	char	ChIedMac [nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED的Mac地址!
	char	ChIedType[nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED的类型，合并单元、保护设备、MMS客户端或服务器，编辑出来的!
	char	ChIedBay [nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED的间隔，间隔号!
	char	ChIedMan [nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED的厂家，厂家名称!
	char	ChIedDesc[nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED的说明，厂家装置型号或说明!
	char	ChIedBip1[nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED的间隔IP1!
	char	ChIedBip2[nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED的间隔IP2!
	//8、装置通道n的被测IED的SMV部分
	int	ChMuType[nCHANNEL][nIED];		//测试通道n的被测IED合并单元类型!
	char	ChMuDmac[nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED合并单元目的Mac地址!
	char 	ChMuSmac[nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED合并单元源Mac地址!
	int 	ChMuSmpR[nCHANNEL][nIED];		//测试通道n的被测IED合并单元采样值速率!
	int 	ChMuSmpD[nCHANNEL][nIED];	//测试通道n的被测IED合并单元采样值间隔!
	int 	ChMuVlId[nCHANNEL][nIED];		//测试通道n的被测IED合并单元Vlan!
	int 	ChMuPrio[nCHANNEL][nIED];		//测试通道n的被测IED合并单元优先级!
	int 	ChMuApId[nCHANNEL][nIED];		//测试通道n的被测IED合并单元AppId!
	int	ChMuSvId[nCHANNEL][nIED];		//测试通道n的被测IED合并单元SmvId(Smv92)!
	int	ChMuDref[nCHANNEL][nIED];		//测试通道n的被测IED合并单元IedMuDatasetRef(Smv92)!
	char	ChMuDesc[nCHANNEL][nIED][NAMELEN];//测试通道n的被测IED合并单元IedMuChannelDesc(Smv92)!
	//9、装置通道1的被测IED的GOOSE部分
	
	char	ChGSAref[nCHANNEL][nIED][NAMELEN];	//测试通道n的被测IED合并单元GOOSE属性cbRef
	char	ChGSADref[nCHANNEL][nIED][NAMELEN];  //测试通道n的被测IED合并单元GOOSE属性datasetRef
	int		ChGSAGoId[nCHANNEL][nIED];	//GOOSE属性goID
	//GOOSE节点部分
	int		ChGSGoId[nCHANNEL][nIED];	//gooseId
	char	ChGSDmac[nCHANNEL][nIED][NAMELEN];	//goose目的mac
	char 	ChGSSmac[nCHANNEL][nIED][NAMELEN];	//goose源mac1
	char	ChGSSmac2[nCHANNEL][nIED][NAMELEN];	//goose源mac2
	int		ChGSVlId[nCHANNEL][nIED];	//VlanId
	int		ChGSPrio[nCHANNEL][nIED];	//Priority
	int		ChGSApId[nCHANNEL][nIED];	//appId
	char 	ChGSref[nCHANNEL][nIED][NAMELEN];	//CbRef
	char	ChGSDref[nCHANNEL][nIED][NAMELEN];	//datasetRef
	char	ChGSLdInst[nCHANNEL][nIED][NAMELEN];	//LdInst
	int		ChGSminTm[nCHANNEL][nIED];	//minTime
	int		ChGSmaxTm[nCHANNEL][nIED];	//maxTime
	//
}DevicePar;

typedef struct pRec{
	char * amount;							//记录文件中记录的文件总数
	char * currentPos;					//当前所用来存储文件的盘符
	char *diskAmount[PATITION_NUMS];		//暂定分为6个区
}pRecord;

typedef struct diskPtr {					/*记录记录文件中各节点指针,方便增删改查操作*/
	xmlNodePtr diskx[PATITION_NUMS];
	xmlNodePtr amtx[PATITION_NUMS];
	xmlNodePtr total;
	xmlNodePtr curDisk;
	xmlNodePtr diskList;
	xmlDocPtr doc;
}diskPtr;

typedef struct nodePtr {			//用于结束存盘时更新记录用，可避免重复搜索，提高速度
	xmlNodePtr status;			
	xmlNodePtr packCnt;
	xmlAttrPtr endTime;
}nodePtr;

typedef struct curFileInfo{
	int attrNo;					/*属性:序号*/
	char attrStartTm[16];			/*属性:开始时间*/
	char attrEndTm[16];			/*属性:结束时间*/
	int status;					/*状态,0未存完,1已存完*/
	char position[8];				/*盘区位置,diskA,B,C,D,E,F*/
	char filename[24];				/*文件名:XXX-D-C-YYYYMMDDHHmmSS*/
	char path[12];				/*路径*/
	long packCnt;					/*总包数*/
	nodePtr	xmlnodePtr;			/*xml文件指针*/
}curFileInfo;


int xml_getConfigDevFileS();
char  xml_getCurrentDiskInfo();
int xml_setCurrentDiskInfo(int chNo, char position);
int xml_getConfigDevFileF();

#endif
