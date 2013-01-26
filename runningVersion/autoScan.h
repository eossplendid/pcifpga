#ifndef __AUTOSCAN_H__
#define __AUTOSCAN_H__

//装置网络Rom配置文件下载、上载、查询程序的消息命令
#define	MSG_DN_RESET		0x03	//强制系统进行复位：整个装置进行复位，所有程序重启，该消息不需要回应
#define	MSG_DN_IP_RESET		0xE0	//恢复装置出厂设置：恢复出厂默认配置，然后系统复位，该消息不需要回应
#define	MSG_QUERY_DN		0xF6	//查询装置的DN消息：该消息不需要参数，需要应答
#define	MSG_QUERY_DN_ACK	0xF7	//查询DN消息的应答：应答Name，Type，Mac，Ip1，Mask1，Gateway1，Ip2，Mask2，Gateway2，Note等
#define	MSG_QUERY_CFG_INFO	0xF8	//查询装置设置消息：该消息不需要参数，需要应答
#define	MSG_QUERY_CFG_INFO_ACK	0xF9	//查询设置消息应答：应答Name，Type，Mac，Note等
#define	MSG_PARAM_CFG		0xFA	//设置装置的网络参数
#define	MSG_PARAM_CFG_ACK	0xFB	//设置装置的网络参数的应答
#define	MSG_SET_IP		0xFC	//设置装置的IP地址：该消息不需要回应，完成后系统要进行复位
#define	MSG_SET_IP_AND_PARAM	0xFD	//设置装置的IP地址和参数：该消息不需要回应，完成后系统要进行复位
#define	MSG_RESTART_SERVICE	0xFE	//重新启动装置的一些服务进程，目前没有该功能

typedef struct FactoryInfo		// 文件名称DeviceConfig_factory.rom
{					// 256字节
	unsigned char flag[4]; 		// device flag:‘0000’
	unsigned char name[16];		// device name:"Testing System1 "
	unsigned char type[16];		// device type:"TY-3012S-000001 "
	unsigned char sn[16];		// device serial number:"Ctdt 0123456789 "
	unsigned char passwd[4];	// device super password:"ctdt"
	unsigned char mac1[6];		// device mac address:00:80:c8:e1:cf:5d
	unsigned char mac2[6];		// device mac address:00:80:c8:e1:cf:5e
	unsigned char ip1[4];		// device ip address:192.168.0.100
	unsigned char ip2[4];		// device ip address:192.168.1.100
	unsigned char mask1[4];		// device subnet mask: 255.255.255.0
	unsigned char mask2[4];		// device subnet mask: 255.255.255.0
	unsigned char gateway1[4];	// device gateway ip address: 192.168.0.1
	unsigned char gateway2[4];	// device gateway ip address: 192.168.1.1
	unsigned char udp_port1[2];	// device udp port: 10001扫描程序，设置DeviceConfig_user.rom文件
	unsigned char udp_port2[2];	// server udp port: 30718功能扩展，目前保留
	unsigned char tcp_port1[2];	// device tcp port: 53456上报实时数据，上报SMV数据
	unsigned char tcp_port2[2];	// device tcp port: 53457上报实时告警，上报实时告警
	unsigned char tcp_port3[2];	// device tcp port: 53458传输配置数据，上传xmlDocunment文件
	unsigned char tcp_port4[2];	// device tcp port: 53459上报历史数据，上传存储的报文文件
	unsigned char tcp_port5[2];	// device tcp port: 53460装置查询
	unsigned char tcp_port6[2];	// device tcp port: 53460功能扩展，目前保留
	unsigned char hardware[4];	// device hardware version:"010a"
	unsigned char software[4];	// device software version:"010a"
	unsigned char date[10];		// device date:"2010.10.20"
	unsigned char note[32];		// device notes:"http://www.dxyb.com 10-62301187!"
	unsigned char reserved1[32];	// device "www.ctdt.com.cn www.ctdt.com.cn!"
	unsigned char reserved2[64];	// device "Testing System for Communication Network of Digital Substation! "		
	unsigned char crc[2];		// idprom default section crc16 code:		
}RomFactoryInfo;

typedef struct UserInfo			// DeviceConfig_user.rom
{					// 256字节
	unsigned char name[16];		// device name:"Testing System1 "
	unsigned char type[16];		// device type:"TY-3012S-000001 "
	unsigned char passwd[4];	// device super password:"ctdt"
	unsigned char ip1[4];		// device ip address:192.168.0.100
	unsigned char ip2[4];		// device ip address:192.168.1.100
	unsigned char mask1[4];		// device subnet mask:255.255.255.0
	unsigned char mask2[4];		// device subnet mask:255.255.255.0
	unsigned char gateway1[4];	// device gateway ip address:192.168.0.1
	unsigned char gateway2[4];	// device gateway ip address:192.168.1.1
	unsigned char note[32];		// device notes:"http://www.dxyb.com 10-62301187!"
	unsigned char reserved1[32];	// device "www.ctdt.com.cn www.ctdt.com.cn!"
	unsigned char reserved2[130];	// device reserved2:all zero
	unsigned char crc[2];		// idprom default section crc16 code: 00
}RomUserInfo;

typedef struct MsgAckHeader		// 消息命令和响应的头部16字节，固定的格式
{					// 共256字节，只前面的16字节有定义
	unsigned char Header[3];	// 消息的头，共3字节，固定的3字节都是0
	unsigned char MsgID;		// 消息命令，共8条命令（目前实现7条），3条应答，共编制了10条
	unsigned char Passwd[4];	// 消息密码，共4字节，固定为ctdt
	unsigned char Mac[6];		// Mac 地址，共6字节，和自己的Mac地址匹配才能执行相应的命令
	unsigned char Reserved[2];	// 保留字节，共2字节，固定的2字节都是0
	unsigned char Parameter[240];	// 消息参数，共240字节
}RomMsgAckHeader;

typedef struct DeviceConfInfo		// 设置装置的网络和参数，查询装置的DN时使用
{					// 152 byte，16 + 136 byte
	unsigned char Header[3];	// 消息的头，共3字节，固定的3字节都是0
	unsigned char MsgID;		// 消息命令，共8条命令，3条应答
	unsigned char Passwd[4];	// 消息密码，共4字节，固定为ctdt
	unsigned char Mac[6];		// Mac 地址，共6字节，和自己的Mac地址匹配才能执行相应的命令
	unsigned char Reserved[2];	// 保留字节，共2字节，固定的2字节都是0
	unsigned char name[16];		// device name
	unsigned char type[16];		// device type
	unsigned char mac[6];		// device mac address, eth0
	unsigned char reserved0[2];
	unsigned char yearh;		// year, max byte
	unsigned char yearl;		// year, min byte
	unsigned char mouth;		// month,   date of this program running
	unsigned char date;		// date,    date of this program running
	unsigned char hours;		// hours,   time of this program running
	unsigned char minutes;		// minutes, time of this program running
	unsigned char seconds;		// seconds, time of this program running
	unsigned char reserved1;
	unsigned char ip1[4];		// eth0 Ip address
	unsigned char mask1[4];		// eth0 Sub mask
	unsigned char gateway1[4];	// eth0 Gateway ip address
	unsigned char ip2[4];		// eth1 Ip address
	unsigned char mask2[4];		// eth1 Sub mask
	unsigned char gateway2[4];	// eth1 Gateway ip address
	unsigned char note[32];		// First 4 byte are Telnet login password. 4 byte 0 means not set
	unsigned char reserved2[32];
}RomDeviceConfInfo;

typedef struct ParamConfInfo		// 设置装置的参数，查询装置的参数时使用
{					// 112 byte，16 + 96 byte
	unsigned char Header[3];	// 消息的头，共3字节，固定的3字节都是0
	unsigned char MsgID;		// 消息命令，共8条命令，3条应答
	unsigned char Passwd[4];	// 消息密码，共4字节，固定为ctdt
	unsigned char Mac[6];		// Mac 地址，共6字节，和自己的Mac地址匹配才能执行相应的命令
	unsigned char Reserved[2];	// 保留字节，共2字节，固定的2字节都是0
	unsigned char name[16];		// device name
	unsigned char type[16];		// device type
	unsigned char note[32];		// device mac address, eth0
	unsigned char reserved[32];	//
}RomParamConfInfo;

typedef struct ConfIp			// 设置装置的网络信息和参数时使用
{					// 52 byte，16 + 36 byte
	unsigned char Header[3];	// 消息的头，共3字节，固定的3字节都是0
	unsigned char MsgID;		// 消息命令，共8条命令，3条应答
	unsigned char Passwd[4];	// 消息密码，共4字节，固定为ctdt
	unsigned char Mac[6];		// Mac 地址，共6字节，和自己的Mac地址匹配才能执行相应的命令
	unsigned char Reserved[2];	// 保留字节，共2字节，固定的2字节都是0
	unsigned char ipsetup[8];	// 固定为字符串:IP-SETUP
	unsigned char signal;		// 处理标记（=1:只设置网口；=2：只设置时钟口；=3：都设置）
	unsigned char reserved[3];	// 保留字节，共3字节，固定的3字节都是0
	unsigned char ip1[4];		// eth0 Ip address
	unsigned char mask1[4];		// eth0 Sub mask
	unsigned char gateway1[4];	// eth0 Gateway ip address
	unsigned char ip2[4];		// eth1 Ip address
	unsigned char mask2[4];		// eth1 Sub mask
	unsigned char gateway2[4];	// eth1 Gateway ip address
}RomConfIp;

//#define RomUdpPort		5677		//应该来自IDPROM
//#define RomUdpPort		3456		//应该来自IDPROM

#define FACTORY_USER_LEN	256		//设置出场参数缓冲区160字节
#define MSG_ACK_LEN		256		//根据消息定义，确定消息和响应的长度不超过128字节

void ConfigRomFileUdp(int RomSockfd);
#endif
