#ifndef 	__BACKSTAGE_H__
#define __BACKSTAGE_H__

//3、TCP端口1：53456，实时SV/SMV数据上报通道，（SMV Data Port）
//4、TCP端口2：53457，实时告警数据上报通道（RealTime Alarm Data Port）
//5、TCP端口3：53458，配置数据通道（XML Config Data Port）
//6、TCP端口4：53459，历史文件上传通道（History Data Port）
//7、TCP端口5：53460，TCP备用端口
//8、TCP端口6：53461，系统命令端口（System Command Port）
//9：UDP端口1：10001，装置扫描数据通过该端口传输
//a：UDP端口2：10001，主程序1和主程序2通过该端口传输命令和消息
//b：UDP端口3：30718，UDP备用端口，用于功能扩展

//长连接命令
#define 	MS_MANAGER_SMV_REQ					0x0067	// 启动SMV报文上报
#define	MS_MANAGER_SMV_RESP					0x0167	// 启动SMV报文确认
#define	MS_MANAGER_SMV_UPLOAD 				0x0168	// SMV报文上传
#define 	MS_MANAGER_SMV_STOP					0x0069   // 终止SMV报文上传
#define	MS_MANAGER_SMV_STOP_RSP			0x0169	//终止SMV报文上传确认

//装置Xml配置文件下载、上载、查询程序的消息命令
#define	MS_CFG_CONN_REQ		0x08	//配置数据连接请求
#define	MS_CFG_CONN_RESP		0x09	//配置数据连接响应
#define	MS_CFG_VER_QRY		0x10	//配置数据版本查询请求
#define	MS_CFG_VER_RESP		0x11	//配置数据版本查询响应
#define	MS_CFG_DOWN_REQ		0x12	//配置数据下载请求
#define	MS_CFG_DOWN_RESP		0x13	//配置数据下载响应
#define	MS_CFG_DOWNLOAD		0x14	//配置数据下载开始
#define	MS_CFG_DOWNLOAD_ACK	0x15	//配置数据下载完成
#define	MS_CFG_UPLOAD_REQ	0x16	//配置数据上载请求
#define	MS_CFG_UPLOAD			0x17	//配置数据上载开始

//录播文件查询和上传的消息命令
#define 	MS_FILETRANS_CONNECT_REQ				0x1000		//录播文件连接请求
#define	MS_FILETRANS_CONNECT_RSP				0x1001		//录播文件连接响应
#define	MS_FILETRANS_FILE_LIST_QRY				0x1002		//录播文件列表查询
#define	MS_FILETRANS_FILE_LIST_RSP				0x1003		//录播文件列表响应
#define 	MS_FILETRANS_FILE_UPLOAD_REQ			0x1004		//录播文件上传请求
#define	MS_FILETRANS_FILE_INFORMATION			0x1005		//文件信息
#define	MS_FILETRANS_FILE_UPLOAD					0x1006		//录播文件上传
#define	MS_FILETRANS_FILE_UPLOAD_CANCEL			0x1007		//录播文件终止上传
#define	MS_FILETRANS_FILE_UPLOAD_CANCEL_ACK	0x1008		//录播文件终止上传响应

#define	TCP_PORT				53456
#define	UDP_PORT				10001
#define 	recTcpPort				4567			//可能会与xml一致
#define 	XmlTcpPort				5678			//应该来自IDPROM，确切是53458
#define 	MSG_MAX_LEN			(64*1024)		//设置消息的缓冲区64K字节，Xml文件最大约64K字节-20字节
#define 	MSG_RES_LEN			32			//根据消息定义，确定基本响应消息的长度不超过32字节
#define	XML_MIN_LEN			0			//确定Xml文件的最小长度
#define	MAX_NAME_LEN			64
#define	MAX_MAC_LEN			6

//#define 	HEADER_LEN				14			//根据消息定义，确定消息的头部长度是14字节
#define 	HEADER_LEN				sizeof(MsgTransHeader)			//根据消息定义，确定消息的头部长度是12字节

#define 	XML_CFG_FILE			"/root/hp/DeviceConfig.xml"




//装置Xml配置文件下载、上载、查询程序的消息结构
#if 0
typedef struct MsgCommand			//配置程序的一般消息格式
{						//配置消息头部共计18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前的字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展
	unsigned short	MsgId;			//消息命令2字节，消息MS_CFG_****_REQ=0x08,0x10,0x12,0x14,0x16
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}XmlMsgCommand;
#endif 

typedef struct
{
	unsigned char svStatus;			/*sv上传状态,0为无,1为sv上传中*/
	unsigned char recStatus;			/*录播文件上传状态,0为无,1为上传中*/
	unsigned char reverse[2];			/*保留,以后可能还会有其他上传*/
}UploadStatus;

typedef struct 
{
	unsigned int		head;		//消息头部4字节，0xF6F62828
	unsigned short	length;		//消息长度2字节，长度域之后，尾部域之前的字节，共计8字节
	unsigned short	msgId;			//消息命令2字节，消息MS_CFG_****_REQ=0x08,0x10,0x12,0x14,0x16
	unsigned short	session1;		//消息会话2字节，第一字节
	unsigned short	session2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
}MsgTransHeader;

typedef struct 
{
	MsgTransHeader header;			//定长的头,12字节
	unsigned char conn_type;			//连接类型
	unsigned char reverse[3];			//保留
	unsigned short tail1;				//消息尾部2字节，第一字节，0x0909
	unsigned short tail2;				//消息尾部2字节，第二字节，0xD7D7
}RecMsgCommand;

typedef struct 			//配置程序连接响应消息格式
{						//配置连接响应消息共计18字节，不包括消息尾部共计14字节
	MsgTransHeader header;			//定长的头,12字节
	unsigned char		result;			//结果	
	unsigned char		reason;			//原因
	unsigned char 	reserve[2];		//保留
	unsigned short	tail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	tail2;		//消息尾部2字节，第二字节，0xD7D7
}RecConnResponse;

typedef struct
{
	MsgTransHeader header;			//定长的头,12字节
	unsigned short channelNo;			//通道号
	unsigned short reserve;			//保留
	unsigned char dataTime1[8];		//起始时间
	unsigned char dataTime2[8];		//结束时间
	unsigned short tail1;
	unsigned short tail2;
}RecListCommand;

typedef struct
{
	MsgTransHeader header;			//定长的头,12字节
	unsigned long	file_size;				//文件大小
	unsigned char fault_reason;			//错误原因
	unsigned char reserve[3];
	unsigned short tail1;
	unsigned short tail2;
}RecFileInfoResponse;

typedef struct 
{
	MsgTransHeader header;
	unsigned int packets_count;
	unsigned int current_packet;
	unsigned short data_len;
	unsigned short tail1;
	unsigned short tail2;
}RecFileTransRsp;


typedef struct
{
	MsgTransHeader header;
	unsigned char filename[MAX_NAME_LEN];
	unsigned short tail1;
	unsigned short tail2;
}RecFileCommand;

typedef struct
{
	MsgTransHeader header;			//定长的头,12字节
	unsigned short tail1;
	unsigned short tail2;
}CommonMsg;

typedef struct
{
	MsgTransHeader header;			//定长的头,12字节
	unsigned char dataTime[8];			/*同步时间,7字节年月日时分秒*/
	unsigned short tail1;
	unsigned short tail2;
}TimerSyncCmd;						/*时间同步请求*/

typedef TimerSyncCmd	DeviceLedRsp;		/*虚拟面板查询响应,结构同时间同步,8字节为led*/

typedef struct
{
	MsgTransHeader header;			//定长的头,12字节
	unsigned char chNo;				/*通道号*/
	unsigned char reverse[3];			/*对齐*/
	unsigned short tail1;
	unsigned short tail2;
}SaveFileCmd;

typedef struct
{
	MsgTransHeader header;				/*定长的头,12字节*/
	unsigned char chNo;					/*通道号*/
	unsigned char style;					/*分类类型*/
	unsigned char iedMac[MAX_MAC_LEN]; 	/*ied的MAC地址*/
	unsigned int 	appId;					/*app ID*/
	unsigned short tail1;					
	unsigned short tail2;
}NetCountCmd;						/*网络统计命令*/

typedef struct 
{
	MsgTransHeader header;				/*定长的头,12字节*/
	unsigned char chNo;					/*通道号*/
	unsigned char style;					/*分类类型*/
	unsigned char iedMac[MAX_MAC_LEN]; 	/*ied的MAC地址*/
	unsigned char startTime[8];				/*起始时间*/
	long long		param[30];				/*最多30项,240字节*/
	unsigned short tail1;					
	unsigned short tail2;
}NetCountRsp;

typedef struct 	//装置与后台计算机通信程序的响应消息格式
{						
	MsgTransHeader 	header;					/*定长的头,12字节*/
	unsigned int		msgAckTag;				/*消息参数4字节，需要应答标志!*/
	unsigned char		almCatg;					/*消息参数1字节，告警分组*/
	unsigned char 	chNo;					/*消息参数1字节，通道号*/
	unsigned char 	srcMac[MAX_MAC_LEN];	/*消息参数6字节，源MAC地址*/
	unsigned char 	dstMac[MAX_MAC_LEN];	/*消息参数6字节，目的MAC地址*/
	unsigned short	appId;					/*消息参数2字节，AppId*/
	unsigned int		MsgAlarmCode;			/*消息参数4字节，故障编码!*/
	unsigned int 		dataTime1;				/*消息参数4字节，故障时间!UTC时间年月日*/
	unsigned int 		dataTime2;				/*时分秒*/
	unsigned short	tail1;				/*消息尾部2字节，第一字节，0x0909*/
	unsigned short	tail2;				/*消息尾部2字节，第二字节，0xD7D7*/
}AlarmReportReq;							/*告警上报请求*/


typedef struct
{
	MsgTransHeader	header;			//定长帧头,12字节
	unsigned char 	CHNLNO;		//通道号
	unsigned char 	Reserved;		//保留
	unsigned char 	SrcMAC[MAX_MAC_LEN];		//源MAC地址
	unsigned char 	DstMac[MAX_MAC_LEN];		//目的MAC地址
	unsigned short	AppID;			//SMV AppID
	unsigned short 	tail1;			//帧尾
	unsigned short 	tail2;
}SmvSendCommand;

typedef struct
{
	MsgTransHeader	header;			//定长帧头,12字节
	unsigned int		ACKParam;		//参数编码，0:允许启动波形复原,1:启动参数错误,2:拒绝服务
	unsigned short 	tail1;			//帧尾
	unsigned short 	tail2;
}SmvSendResponse;

typedef struct
{
	MsgTransHeader	header;
	unsigned int 		packLen;		//报文长度
	unsigned short 	tail1;			//帧尾
	unsigned short 	tail2;
}SmvUploadResponse;


typedef struct MsgCommand			//配置程序的一般消息格式
{						//配置消息头部共计18字节，不包括消息尾部共计14字节
	unsigned int		MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前的字节，共计8字节
	unsigned short	MsgId;			//消息命令2字节，消息MS_CFG_****_REQ=0x08,0x10,0x12,0x14,0x16
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}XmlMsgCommand;


typedef struct ConnResponse			//配置程序连接响应消息格式
{						//配置连接响应消息共计18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgId;			//消息命令2字节，消息MS_CFG_CONN_RESP=0x09
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}XmlConnResponse;

typedef struct VerResponse			//配置程序版本响应消息格式
{						//配置版本响应消息共计24字节
	unsigned int		MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计14字节
	unsigned short	MsgId;			//消息命令2字节，消息MS_CFG_VER_RESP=0x11
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgVersion;		//主版本号1字节，子版本号1字节
	unsigned short	MsgTime1;		//文件时间2字节，第一字节
	unsigned short	MsgTime2;		//文件时间2字节，第二字节
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}XmlVerResponse;

typedef struct DownloadResponse			//配置程序下载响应消息格式
{						//配置连接响应消息共计20字节
	unsigned int		MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计10字节
	unsigned short	MsgId;			//消息命令2字节，消息MS_CFG_DOWN_RESP=0x13
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	//unsigned short	MsgEnable;		//下载使能2字节，
	unsigned char 	MsgEnable;		//下载使能2字节，
	unsigned char		MsgReason;		//不能下载时的原因
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}XmlDownloadResponse;

typedef struct DownloadFinish			//配置程序下载完成消息格式
{						//配置连接响应消息共计18字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgId;			//消息命令2字节，消息MS_CFG_DOWNLOAD_ACK=0x17
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}XmlDownloadFinish;

typedef struct UploadResponse			//配置程序上载完成消息格式
{						//配置连接响应消息共计18字节
	unsigned int		MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgId;			//消息命令2字节，消息MS_CFG_DOWNLOAD_ACK=0x17
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgLen;			//消息参数2字节，Xml配置文件的长度
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}XmlUploadResponse;


/*第二部分*/

/*
   1、装置的告警上报程序
   2、装置的状态查询程序
*/

//HeartBeat

//装置状态查询程序的消息命令
#define	MS_ROUTE_TEST_REQ	0x0020	//01、01a:路由测试请求，计算机发该消息
#define	MS_ROUTE_TEST_ACK	0x0120	//02、01b:路由测试确认，装置发出该消息
#define	MS_HEART_BEAT_REQ	0x0121	//03、02b:心跳保活请求，装置发出该消息
#define	MS_HEART_BEAT_ACK	0x0021	//04、02a:心跳保活确认，计算机发该消息
#define	MS_RESET_DEVI_REQ		0x0022	//05、03a:装置复位请求，计算机发该消息
#define	MS_RESET_DEVI_ACK		0x0122	//06、03b:装置复位确认，装置发出该消息
#define	MS_TIMER_SYNC_REQ	0x0023	//07、04a:时间同步请求，计算机发该消息
#define	MS_TIMER_SYNC_ACK	0x0123	//08、04b:时间同步确认，装置发出该消息
#define	MS_SYNC_TIMER_REQ	0x0124	//09、05b:发送同步请求，装置机发该消息
#define	MS_SYNC_TIMER_ACK	0x0024	//10、05a:时间同步应答，计算机发该消息
#define	MS_GPSTIME_SY_REQ	0x	//11、05b:发送GPST请求，装置机发该消息
#define	MS_GPSTIME_SY_ACK	0x	//12、05a:GPST同步确认，计算机发该消息
#define	MS_DEVICE_LED_REQ		0x0062	//13、06a:虚拟面板查询，计算机发该消息
#define	MS_DEVICE_LED_ACK		0x0162	//14、06b:虚拟面板应答，装置发出该消息
#define	MS_SAVE_HDISK_REQ	0x0063	//15、07a:立即存盘请求，计算机发该消息
#define	MS_SAVE_HDISK_ACK		0x0163	//16、07b:立即存盘确认，装置发出该消息
#define	MS_DEVI_FAULT_REP		0x0164	//17、08b:装置故障上报，装置发出该消息
#define	MS_DEVI_FAULT_ACK		0x0064	//18、08a:故障上报确认，计算机发该消息
#define	MS_DEVI_ALARM_REQ	0x0165	//19、09a:装置告警上报，计算机发该消息
#define	MS_DEVI_ALARM_ACK	0x0065	//20、09b:告警上报确认，计算机发该消息
#define	MS_NET_STATUS_REQ	0x	//21、10a:网络状态请求，计算机发该消息!
#define	MS_NET_STATUS_ACK	0x	//22、10b:网络状态应答，装置发出该消息!
#define	MS_NET_COUNTE_REQ	0x0066	//23、11a:网络统计查询，计算机发该消息
#define	MS_CFG_COUNTE_ACK	0x0166	//24、11b:网络统计应答，装置发出该消息
#define	MS_SEND_SMVMS_REQ	0x0067	//25、12a:请求SMV上报， 计算机发该消息
#define	MS_SEND_SMVMS_ACK	0x0167	//26、12b:确认SMV上报， 装置发出该消息
#define	MS_STARTS_SMV_ACK	0x0168	//27、12c:开始SMV上报， 装置发出该消息
#define	MS_STOP_SMVMS_REQ	0x0069	//28、12d:终止SMV上报， 计算机发该消息
#define	MS_STOP_SMVMS_ACK	0x0169	//29、12e:确认SMV终止， 装置发出该消息

#if 0
//装置与后台计算机通信程序的消息结构
typedef struct Ty2012sCommand			//装置与后台计算机通信程序的一般消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息帧头4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前的字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展
	unsigned short	MsgId;			//消息命令2字节，消息命令
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevCommand;

typedef struct Ty3012sResponse			//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息响应
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevResponse;

//MS_ROUTE_TEST_REQ:路由测试请求，计算机发该消息，如果接收到请求
//MS_ROUTE_TEST_ACK:路由测试确认，装置发出该消息，需立即回复确认
typedef struct Ty3012sRouteAck			//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息响应=MS_ROUTE_TEST_ACK!
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevRouteAck;

//MS_HEART_BEAT_REQ:心跳保活请求，装置发出该消息，定时发送该请求，设置定时器线程
//MS_HEART_BEAT_ACK:心跳保活确认，计算机不发消息，装置不处理消息
typedef struct Ty3012sHeartBeatReq		//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息请求=MS_HEART_BEAT_REQ!
	unsigned short	MsgHeart;		//消息参数2字节，目前是0!
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevHeartBeatReq;

//MS_RESET_DEVI_REQ:装置复位请求，计算机发该消息，停止所有的进程，所有进程退出，主要是主进程!
//MS_RESET_DEVI_ACK:装置复位确认，装置发出该消息，要回复复位确认，系统进行复位
typedef struct Ty3012sResetAck			//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息确认=MS_RESET_DEVI_ACK!
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevResetAck;

//MS_TIMER_SYNC_REQ:时间同步请求，计算机发该消息
//MS_TIMER_SYNC_ACK:时间同步确认，装置发出该消息
//MS_SYNC_TIMER_REQ:发送同步请求，装置机发该消息
//MS_SYNC_TIMER_ACK:时间同步应答，计算机发该消息
//MS_GPSTIME_SY_REQ:发送GPST请求，装置机发该消息
//MS_GPSTIME_SY_ACK:时间同步确认，计算机发该消息
typedef struct Ty3012sTimeSyncReq		//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息请求=MS_TIMER_SYNC_REQ!
	unsigned short	MsgYear;		//消息参数2字节，年!
	unsigned short	MsgMonthDay;		//消息参数2字节，月和日!
	unsigned short	MsgHourMinite;		//消息参数2字节，时和分!
	unsigned short	MsgSecondMilli;		//消息参数2字节，秒和毫秒!
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevTimeSyncReq;

//MS_DEVICE_LED_REQ:虚拟面板查询，计算机发该消息
//MS_DEVICE_LED_RES:虚拟面板应答，装置发出该消息
typedef struct Ty3012sLedsRes			//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息应答=MS_DEVICE_LED_RES!
	unsigned short	MsgLed1;		//消息参数2字节，第一组LED!
	unsigned short	MsgLed2;		//消息参数2字节，第二组LED!
	unsigned short	MsgLed3;		//消息参数2字节，第三组LED!
	unsigned short	MsgLed4;		//消息参数2字节，第四组LED!
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevLedsRes;

//MS_SAVE_HDISK_REQ:立即存盘请求，计算机发该消息
//MS_SAVE_HDISK_ACK:立即存盘确认，装置发出该消息
typedef struct Ty3012sSaveHdiskReq		//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息请求=MS_SAVE_HDISK_REQ!
	unsigned short	MsgChannel;		//消息参数2字节，通道号!
	unsigned short	MsgParamete;		//消息参数2字节，备用参数，目前是0!
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevSaveHdiskReq;

//MS_DEVI_FAULT_REQ:装置故障上报，装置发出该消息
//MS_DEVI_FAULT_ACK:故障上报确认，计算机发该消息
typedef struct Ty3012sFaultReport		//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息请求=MS_DEVI_FAULT_REQ!
	unsigned short	MsgAckTagh;		//消息参数2字节，需要应答标志!
	unsigned short	MsgAckTagl;		//消息参数2字节，需要应答标志!
	unsigned short	MsgErrorCodeh;		//消息参数2字节，故障编码!目前GPS类?以太网接口类?软件类?
	unsigned short	MsgErrorCodel;		//消息参数2字节，故障编码!目前GPS类?以太网接口类?软件类?
	unsigned short	MsgDateh;		//消息参数2字节，故障时间!使用Linux系统时间?GPS时间?
	unsigned short	MsgDatel;		//消息参数2字节，故障时间!使用Linux系统时间?GPS时间?
	unsigned short	MsgTimeh;		//消息参数2字节，故障时间!使用Linux系统时间?GPS时间?
	unsigned short	MsgTimel;		//消息参数2字节，故障时间!使用Linux系统时间?GPS时间?
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevFaultReport;

//MS_DEVI_ALARM_REQ:告警上报请求，计算机发该消息
//MS_DEVI_ALARM_ACK:告警上报确认，计算机发该消息
typedef struct Ty3012sAlarmReport		//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息请求=MS_DEVI_FAULT_REQ!
	unsigned short	MsgAckTagh;		//消息参数2字节，需要应答标志!
	unsigned short	MsgAckTagl;		//消息参数2字节，需要应答标志!
	unsigned short	MsgIedMac0;		//消息参数2字节，需要应答IED的MAC地址，0000!
	unsigned short	MsgIedMac1;		//消息参数2字节，需要应答IED的MAC地址，第1和第2个Mac地址!
	unsigned short	MsgIedMac2;		//消息参数2字节，需要应答IED的MAC地址，第3和第4个Mac地址!
	unsigned short	MsgIedMac3;		//消息参数2字节，需要应答IED的MAC地址，第5和第6个Mac地址!
	unsigned short	MsgAlarmType;		//消息参数2字节，告警类型!目前SMV,GOOSE,MMS,PTP,ETHERNET,NETWORK?
	unsigned short	MsgAlarmCode;		//消息参数2字节，故障编码!目前?
	unsigned short	MsgDateh;		//消息参数2字节，故障时间!使用Linux系统时间?GPS时间?
	unsigned short	MsgDatel;		//消息参数2字节，故障时间!使用Linux系统时间?GPS时间?
	unsigned short	MsgTimeh;		//消息参数2字节，故障时间!使用Linux系统时间?GPS时间?
	unsigned short	MsgTimel;		//消息参数2字节，故障时间!使用Linux系统时间?GPS时间?
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevAlarmReport;

//MS_NET_STATUS_REQ:网络状态请求，计算机发该消息!
//MS_NET_STATUS_ACK:网络状态应答，装置发出该消息!
typedef struct Ty3012sStatusReq			//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息应答=MS_DEVICE_LED_RES!
	unsigned short	MsgStatus1;		//消息参数2字节，第一组状态，以太网接口!
	unsigned short	MsgStatus2;		//消息参数2字节，第二组状态，GPS接口!
	unsigned short	MsgStatus3;		//消息参数2字节，第三组状态，开入开出量!
	unsigned short	MsgStatus4;		//消息参数2字节，第四组状态，过滤情况!
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevStatusReq;

//MS_NET_COUNTE_REQ:网络统计查询，计算机发该消息
//MS_CFG_COUNTE_ACK:网络统计应答，装置发出该消息
typedef struct Ty3012sCounterRes		//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息应答=MS_DEVICE_LED_RES!
	unsigned short	MsgChNo;		//消息参数2字节，通道1或通道2
	unsigned long	MsgAllFrame;		//消息参数4字节，0:总帧数L
	unsigned long	MsgAllByte;		//消息参数4字节，1:总字节数L
	unsigned long	MsgBroadcastFrame;	//消息参数4字节，2:广播帧数L
	unsigned long	MsgBroadcastByte;	//消息参数4字节，3:广播字节数L
	unsigned long	MsgMulticastFrame;	//消息参数4字节，4:多播帧数L
	unsigned long	MsgMulticastByte;	//消息参数4字节，5:多播字节数L
	unsigned long	MsgCrcErrFrame;		//消息参数4字节，6:CRC错误帧数L
	unsigned long	MsgErrFrame;		//消息参数4字节，7:错误帧数L
	unsigned long	MsgAllignErrFrame;	//消息参数4字节，8:定位错误帧数L
	unsigned long	MsgShortFrame;		//消息参数4字节，9:超短帧数L
	unsigned long	MsgLongFrame;		//消息参数4字节，10:超长帧数L
	unsigned long	MsgAllFrameH;		//消息参数4字节，0:总帧数H
	unsigned long	MsgAllByteH;		//消息参数4字节，1:总字节数H
	unsigned long	MsgBroadcastFrameH;	//消息参数4字节，2:广播帧数H
	unsigned long	MsgBroadcastByteH;	//消息参数4字节，3:广播字节数H
	unsigned long	MsgMulticastFrameH;	//消息参数4字节，4:多播帧数H
	unsigned long	MsgMulticastByteH;	//消息参数4字节，5:多播字节数H
	unsigned long	MsgCrcErrFrameH;	//消息参数4字节，6:CRC错误帧数H
	unsigned long	MsgErrFrameH;		//消息参数4字节，7:错误帧数H
	unsigned long	MsgAllignErrFrameH;	//消息参数4字节，8:定位错误帧数H
	unsigned long	MsgShortFrameH;		//消息参数4字节，9:超短帧数H
	unsigned long	MsgLongFrameH;		//消息参数4字节，10:超长帧数H	
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevCounterRes;

//MS_SEND_SMVMS_REQ:请求SMV上报， 计算机发该消息
//MS_SEND_SMVMS_ACK:确认SMV上报， 装置发出该消息
//MS_STARTS_SMV_ACK:开始SMV上报， 装置发出该消息
//MS_STOP_SMVMS_REQ:终止SMV上报， 计算机发该消息
//MS_STOP_SMVMS_ACK:确认SMV终止， 装置发出该消息
typedef struct Ty3012sSmvReq			//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息请求=MS_DEVI_FAULT_REQ!
	unsigned short	MsgIedMac0;		//消息参数2字节，需要应答IED的MAC地址，第1和第2个Mac地址!
	unsigned short	MsgIedMac1;		//消息参数2字节，需要应答IED的MAC地址，第3和第4个Mac地址!
	unsigned short	MsgIedMac2;		//消息参数2字节，需要应答IED的MAC地址，第5和第6个Mac地址!
	unsigned short	MsgChannel;		//消息参数2字节，通道号！
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevSmvReq;

typedef struct Ty3012sSmvAck			//装置与后台计算机通信程序的响应消息格式
{						//消息头共18字节，不包括消息尾部共计14字节
	unsigned int	MsgHead;		//消息头部4字节，0xF6F62828
	unsigned short	MsgLength;		//消息长度2字节，长度域之后，尾部域之前字节，共计8字节
	unsigned short	MsgSession1;		//消息会话2字节，第一字节
	unsigned short	MsgSession2;		//消息会话2字节，第二字节，客户端发送0，服务器端加1发送回去
	unsigned short	MsgReserve;		//保留字节2字节，保留字节是为了保证消息扩展，目前设置为0
	unsigned short	MsgId;			//消息命令2字节，消息请求=MS_DEVI_FAULT_REQ!
	unsigned short	MsgSmvAckh;		//消息参数2字节，需要应答状态!1=启动参数错误(譬如没有要求的Mac),2=拒绝服务
//	unsigned short	MsgSmvAckh;		//消息参数2字节，需要应答状态!
	unsigned short	MsgTail1;		//消息尾部2字节，第一字节，0x0909
	unsigned short	MsgTail2;		//消息尾部2字节，第二字节，0xD7D7
}DevSmvAck;

#endif

#if 0
//定义装置配置消息
#define SET_CH_INTERFACE	0x01		//设置通道的以太网接口类型，光接口或电接口
#define SET_TRIGGER_TIME	0x02		//设置通道外触发消抖时间，单位us
#define SET_GPS_INTERFACE	0x03		//设置GPS接口类型，TTL或485接口
#define SET_GPS_TIME		0x04		//设置GPS时间，年、月、日、时、分、秒、毫秒、微妙
#define SET_CH_FILTER		0x05		//设置通道的过滤器，即过滤条件
//定义配置消息应答
#define ACK_CH_INTERFACE	0x81		//应答设置通道的以太网接口类型，光接口或电接口
#define ACK_TRIGGER_TIME	0x82		//应答设置通道外触发消抖时间，单位us
#define ACK_GPS_INTERFACE	0x83		//应答设置GPS接口类型，TTL或485接口
#define ACK_GPS_TIME		0x84		//应答设置GPS时间，年、月、日、时、分、秒、毫秒、微妙
#define ACK_CH_FILT		0x85		//应答设置通道的过滤条件
//定义状态查询消息
#define CHECK_CH_BUFFER		0x11		//检查缓冲FIFO中的数据，每次检查128字节
#define CHECK_CH_INTERFACE	0x12		//检查通道的以太网接口类型，光接口或电接口
#define CHECK_OPTICAL_STATUS	0x13		//检查通道光接口的状态，有光或无光信号，以便确定是否光信号丢失
#define CHECK_LINK_SPEED	0x14		//检查通道光电接口的连接状态，连接、速度、双工
#define CHECK_GPS_TIMER		0x15		//检查GPS时间，年、月、日、时、分、秒、毫秒、微妙
#define CHECK_GPS_STATUS	0x16		//检查GPS接口的告警状态，正常、丢失、错误
#define CHECK_TRIGGER_STATUS	0x17		//检查通道外触发状态，有无触发，是否正在触发
#define CHECK_CH_FILTER		0x18		//检查通道的过滤条件
#define CHECK_FRAME_COUNTERS	0x19		//检查帧和字节计数器的统计数据
//定义查询消息应答
#define ACK_CH_BUFFER		0x91		//应答检查缓冲FIFO中的数据，每次应答128字节
#define ACK_CH_STATUS		0x92		//应答检查通道的以太网接口类型，光接口或电接口
#define ACK_OPTICAL_STATUS	0x93		//应答检查通道光接口的状态，有光或无光信号
#define ACK_LINK_SPEED		0x94		//应答检查通道光电接口的连接状态，连接、速度、双工
#define ACK_GPS_TIMER		0x95		//应答检查GPS时间，年、月、日、时、分、秒、毫秒、微妙
#define ACK_GPS_STATUS		0x96		//应答检查GPS告警状态，正常、丢失、错误
#define ACK_TRIGGER_STATUS	0x97		//应答检查通道外触发状态，有无触发，是否正在触发
#define ACK_CH_FILTER		0x98		//应答检查通道的过滤条件
#define ACK_FRAME_COUNTERS	0x99		//应答检查帧和字节计数器的统计数据
//定义面板查询消息
#define CHECK_LED_STATUS	0x21		//检查前面板LED状态，总共36个LED
//定义面板查询应答
#define ACK_LED_STATUS		0xa1		//应答前面板LED状态，总共36个LED
#endif

#define	MAX_LINK_TYPES	4			/*TCP&&UDP连接最多个数*/

#define	RESPONSE_CMD_RSP(Res, Rsp, Sockfd, Buff)		\
do \
{ \
	Res->header.head = Htons(0xF6F62828); \
	Res->header.length = Htons(0x0006); \
	Res->header.msgId = Htons(Rsp); \
	MsgSession++; \
	Res->header.session1 = Htons(MsgSession & 0x0ffff); \
	Res->header.session2 = Htons(MsgSession / 0x10000); \
	Res->tail1 = Htons(0x0909); \
	Res->tail2 = Htons(0xD7D7); \
	if (write(Sockfd, Buff, sizeof(CommonMsg)) <= 0) \
	{ \
		close(Sockfd); \
		printf("TY-3012S CMD_RSP write ClientXmlSockfd error: %d!rsp:%d\n", Sockfd, Rsp); \
		break; \
	} \
}while(0)


#define 	RESPONSE_SMV_STOP(Res, Sockfd, Buff)		RESPONSE_CMD_RSP(Res, MS_STOP_SMVMS_ACK, Sockfd, Buff)
#define 	RESPONSE_UPLOAD_STOP(Res, Sockfd, Buff)	RESPONSE_CMD_RSP(Res, MS_FILETRANS_FILE_UPLOAD_CANCEL_ACK, Sockfd, Buff)
#define	RESPONSE_ROUTE_ACK(Res, Sockfd, Buff)		RESPONSE_CMD_RSP(Res, MS_ROUTE_TEST_ACK, Sockfd, Buff)
#define	RESPONES_RESET_ACK(Res, Sockfd, Buff)		RESPONSE_CMD_RSP(Res, MS_RESET_DEVI_REQ, Sockfd, Buff)
#define	RESPONES_TIMESYNC_ACK(Res, Sockfd, Buff)	RESPONSE_CMD_RSP(Res, MS_TIMER_SYNC_ACK, Sockfd, Buff)
#define	RESPONES_SAVEFILE_ACK(Res, Sockfd, Buff)	RESPONSE_CMD_RSP(Res, MS_SAVE_HDISK_ACK, Sockfd, Buff)

#define	RESPONSE_SMV_ACK(Res, Sockfd, Buff, Ack)		\
do \
{ \
	Res->header.head = Htons(0xF6F62828); \
	Res->header.length = Htons(0x000a); \
	Res->header.msgId = Htons(MS_SEND_SMVMS_ACK); \
	MsgSession++; \
	Res->header.session1 = Htons(MsgSession & 0x0ffff); \
	Res->header.session2 = Htons(MsgSession / 0x10000); \
	Res->ACKParam = Ack; \
	Res->tail1 = Htons(0x0909); \
	Res->tail2 = Htons(0xD7D7); \
	if (write(Sockfd, Buff, sizeof(SmvSendResponse)) <= 0) \
	{ \
		close(Sockfd); \
		printf("TY-3012S SMV_ACK write ClientXmlSockfd error: %d!\n", Sockfd); \
		break; \
	} \
}while(0)

#define	RESPONSE_NET_COUNT(Res, len, Sockfd, Buff)		\
do \
{ \
	Res->header.head = Htons(0xF6F62828); \
	Res->header.length = Htons(len); \
	Res->header.msgId = Htons(MS_CFG_COUNTE_ACK); \
	MsgSession++; \
	Res->header.session1 = Htons(MsgSession & 0x0ffff); \
	Res->header.session2 = Htons(MsgSession / 0x10000); \
	Res->tail1 = Htons(0x0909); \
	Res->tail2 = Htons(0xD7D7); \
	if (write(Sockfd, Buff, sizeof(NetCountRsp)) <= 0) \
	{ \
		close(Sockfd); \
		printf("TY-3012S DEVICE_LED_ACK write ClientSockfd error: %d!\n", Sockfd); \
		break; \
	} \
}while(0)


#define	RESPONSE_DEVICE_LED_ACK(Res, Sockfd, Buff, LedParam)		\
do \
{ \
	Res->header.head = Htons(0xF6F62828); \
	Res->header.length = Htons(0x000e); \
	Res->header.msgId = Htons(MS_DEVICE_LED_ACK); \
	MsgSession++; \
	Res->header.session1 = Htons(MsgSession & 0x0ffff); \
	Res->header.session2 = Htons(MsgSession / 0x10000); \
	memcpy(Res->dataTime, LedParam, 8); \
	Res->tail1 = Htons(0x0909); \
	Res->tail2 = Htons(0xD7D7); \
	if (write(Sockfd, Buff, sizeof(DeviceLedRsp)) <= 0) \
	{ \
		close(Sockfd); \
		printf("TY-3012S DEVICE_LED_ACK write ClientSockfd error: %d!\n", Sockfd); \
		break; \
	} \
}while(0)

#define	RESPONSE_CONNECT_FAILED(Res, Sockfd, Buff, Reason)	\
do \
{ \
	Res->header.head = Htons(0xF6F62828); \
	Res->header.length = Htons(0x000a); \
	Res->header.msgId = Htons(MS_FILETRANS_CONNECT_RSP); \
	MsgSession++; \
	Res->header.session1 = Htons(MsgSession & 0x0ffff); \
	Res->header.session2 = Htons(MsgSession / 0x10000); \
	Res->result = 0; \
	Res->reason = Reason; \
	Res->tail1 = Htons(0x0909); \
	Res->tail2 = Htons(0xD7D7); \
	if (write(Sockfd, Buff, sizeof(RecConnResponse)) <= 0) \
	{ \
		close(Sockfd); \
		printf("TY-3012S CONNECT_FAILED write ClientXmlSockfd error: %d!\n", Sockfd); \
		break; \
	} \
}while(0)

#define	RESPONSE_CONNECT_OK(Res, Sockfd, Buff)		\
do \
{ \
	Res->header.head = Htons(0xF6F62828); \
	Res->header.length = Htons(0x000a); \
	Res->header.msgId = Htons(MS_FILETRANS_CONNECT_RSP); \
	MsgSession++; \
	Res->header.session1 = Htons(MsgSession & 0x0ffff); \
	Res->header.session2 = Htons(MsgSession / 0x10000); \
	Res->result = 1; \
	Res->reason = 0; \
	Res->tail1 = Htons(0x0909); \
	Res->tail2 = Htons(0xD7D7); \
	if (write(Sockfd, Buff, sizeof(RecConnResponse)) <= 0) \
	{ \
		close(Sockfd); \
		printf("TY-3012S CONNECT_OK write ClientXmlSockfd error: %d!\n", Sockfd); \
		break; \
	} \
}while(0)

#define	BS_SV_NEED_TO_RETURN(Qid, info) \
do \
{ \
	if (msgctl(Qid, IPC_STAT, &info) < 0) \
	{ \
		PLH_DEBUG("SvUploadQid msgctl failed, errno:%d, %s, ---lineNo:%d\n", errno, strerror(errno), __LINE__); \
	} \
	if (info.msg_cbytes == 0) \
	{ \
		return 0; \
	} \
}while(0)


/*SV上传部分*/
#define	MAX_UPLOAD_NUM			50				/*一条消息上传sv的最大数*/
#define	TYPE_SVUPLOAD				6				/*消息队列接收消息的类型,sv上传为6*/
#define	MSG_RQST_SVUPLOAD 		1				/*消息命令sv上传*/
#define	MSG_RQST_SVUPLOAD_STOP 	2				/*消息命令停止上传*/

typedef struct
{
	long mtype;									/*接收消息类型*/
	unsigned char request;							/*请求命令*/
	unsigned char chNo;							/*通道号*/
	unsigned char reverse[2];						/*保留*/
	unsigned char *packet[MAX_UPLOAD_NUM];		/*帧头指针,一次最多上传10个*/
	unsigned long length[MAX_UPLOAD_NUM];			/*每帧的长度*/
	unsigned int count;							/*需要上传的个数,不超过10个*/
}MsgSvInfo;									/*Sv消息*/


#if 0
#define	MSGHEAD		0x1a2b3c4d	//消息头部4字节，1a2b3c4d
#define	MSGSESSIONID	0x0		//消息会话4字节，服务器端发送0，客户端加1发送回去
#define	MSGDEVICEID	0x0		//消息装置4字节，该种类型的装置专用，定义为0
#define	MSGVERSION	0x1		//消息版本4字节，消息的版本号，定义为1.0版本
#define	MSGID		0x0		//消息命令4字节，空定义为0
#define	MSGCHNO1	0x1		//消息通道4字节，通道1，值=1
#define	MSGCHNO2	0x2		//消息通道4字节，通道2，值=2
#define	MSGTAIL		0xa1b2c3d4	//消息尾部4字节，a1b2c3d4

#define	EINTERFACE	0x00		//接口类型4字节，以太网电接口=0
#define	OINTERFACE	0x01		//接口类型4字节，以太网光接口=1

#define	TRIGERTIME	0x55		//通道外触发消抖时间，单位us，最小0，最大256

#define	GPSTTLINTERFACE	0x0		//接口类型4字节，GPS接口类型，TTL或485接口，0=TTL，1=485
#define	GPS485INTERFACE	0x1		//接口类型4字节，GPS接口类型，TTL或485接口，0=TTL，1=485

#define	GPSYEAR		2011		//GPS时间，年
#define	GPSMONTH	5		//GPS时间，月
#define	GPSDAY		18		//GPS时间，日
#define	GPSHOUR		16		//GPS时间，时
#define	GPSMINIT	36		//GPS时间，分
#define	GPSSECOND	0		//GPS时间，秒
#define	GPSMILLISECOND	0		//GPS时间，毫秒
#define	GPSMICROSECOND	0		//GPS时间，微秒

#define	CHRESET		0x01		//复位控制，0不要求复位，1要求复位
#define	CHMODE		0x01		//过滤模式，1不过滤，0过滤
#define	MINLENGTH	64		//最小帧长，隐含是64字节
#define	MAXLENGTH	1518		//最大帧长，隐含是1518字节
#define	VLAN1ENABLE	0x01		//Vlan1使能，D0=1使能
#define	VLAN2ENABLE	0x02		//Vlan2使能，D1=1使能
#define	GOOSEENABLE	0x04		//Goose使能，D2=1使能
#define	SMVENABLE	0x08		//Smv  使能，D3=1使能
#define	MMSENABLE	0x10		//Mms  使能，D4=1使能
#define	VLAN1ID		0x0355		//Vlan1标签
#define	VLAN2ID		0x03aa		//Vlan2标签
#define	MACENABLE	0x55		//Mac  使能，D0到D15=1使能
#endif

#define	LOCALPORT	5678		//服务器udp端口，以后来自idprom文件
//#define	REMOTEIP	"127.0.0.1"	//服务器ip地址，使用内部地址
//#define	MSGMAXLEN	0x100		//消息的最大长度，应用传送给驱动读函数的最大数据长度256字节

/*interface list*/
int bs_tcpserverInit(pthread_t *id1, pthread_t *id2);

#endif
