#ifndef 	__MSGDRIVER_H__
#define	__MSGDRIVER_H__
//定义装置配置消息
#define SET_ETH_INTERFACE	0x01	//01IO、设置监测通道的以太网接口类型，光接口或电接口
#define SET_GPS_INTERFACE	0x02	//02IO、设置GPS或1588接口，以及接口类型是TTL或485接口
#define SET_FPGA_TIME		0x03	//03WR、设置FPGA的时间，年、月、日、时、分、秒、毫秒、微妙
#define SET_TRIGGER_TIME	0x04	//04IO、设置监测通道外触发消抖时间，单位us
#define SET_ALARM_OUT		0x05	//05IO、设置监测通道外外告警输出，即继电器闭合
#define SET_BEEP_ALARM		0x06	//06IO、设置内部蜂鸣器告警输出，蜂鸣器响
#define SET_WARCH_DOG		0x07	//07IO、设置即清楚外部看门狗，看门狗要定时清楚
#define SET_CH_FILTER		0x08	//08WR、设置监测通道的过滤器，即过滤条件
#define SET_FILTER_MAC		0x09	//09WR、设置监测通道的源MAC过滤地址，即接收该MAC地址数据
#define SET_BUFFER_EMPTY	0x0A	//0AIO、设置清除FIFO中的数据，即清空FIFO
//定义配置消息应答
#define ACK_ETH_INTERFACE	0x81	//01IO、应答监测通道的以太网接口类型，光接口或电接口
#define ACK_GPS_INTERFACE	0x82	//02IO、应答GPS或1588接口，以及接口类型是TTL或485接口
#define ACK_FPGA_TIME		0x83	//03WR、应答FPGA的时间，年、月、日、时、分、秒、毫秒、微妙
#define ACK_TRIGGER_TIME	0x84	//04IO、应答监测通道外触发消抖时间，单位us
#define ACK_ALARM_OUT		0x85	//05IO、应答监测通道外外告警输出，即继电器闭合
#define ACK_BEEP_ALARM		0x86	//06IO、应答内部蜂鸣器告警输出，蜂鸣器响
#define ACK_WARCH_DOG		0x87	//07IO、应答即清楚外部看门狗，看门狗要定时清楚
#define ACK_SET_FILTER		0x88	//08WR、应答监测通道的过滤器，即过滤条件
#define ACK_FILTER_MACSET	0x89	//09WR、应答监测通道的源MAC地址过滤，即接收该MAC地址数据
#define ACK_BUFFER_EMPTY	0x8A	//0AIO、应答清除FIFO中的数据，即清空FIFO
//定义状态查询消息
#define READ_CH_BUFFER		0x11	//11RD、读取录波缓冲FIFO中的数据，每次最大2M字节
#define CHECK_CH_INTERFACE	0x12	//12IO、检查监测通道的以太网接口类型，光接口或电接口
#define CHECK_CH_STATUS		0x13	//13IO、检查监测通道光电接口的连接状态，连接、速度、双工
#define CHECK_FPGA_TIMER	0x14	//14RD、检查FPGA时间，年、月、日、时、分、秒、毫秒、微妙
#define CHECK_TRIGGER_STATUS	0x15	//15IO、检查监测通道外触发状态，有无触发，是否正在触发
#define CHECK_KEY_STATUS	0x16	//16IO、检查前面板按键状态，有无按键，是否正在按下
#define CHECK_CH_FILTER		0x17	//17RD、检查监测通道的过滤条件
#define CHECK_FILTER_MAC	0x18	//18RD、检查监测通道的源MAC过滤地址，一种MAC过滤条件
#define CHECK_FRAME_COUNTERS	0x19	//19RD、检查监测通道帧和字节计数器的统计数据
//定义查询消息应答
#define ACK_CH_BUFFER		0x90	//10RD、应答录波缓冲FIFO中的数据，每次最大2M字节
#define ACK_CH_INTERFACE	0x91	//11IO、应答监测通道的以太网接口类型，光接口或电接口
#define ACK_CH_STATUS		0x92	//12IO、应答查监测通道光电接口的连接状态，连接、速度、双工
#define ACK_FPGA_TIMER		0x93	//13RD、应答FPGA时间，年、月、日、时、分、秒、毫秒、微妙
#define ACK_TRIGGER_STATUS	0x95	//15IO、应答监测通道外触发状态，有无触发，是否正在触发
#define ACK_KEY_STATUS		0x96	//16IO、应答前面板按键状态，有无按键，是否正在按下
#define ACK_CH_FILTER		0x97	//17RD、应答监测通道的过滤条件
#define ACK_FILTER_MAC		0x98	//18RD、检查监测通道的源MAC地址过滤，一种MAC过滤条件
#define ACK_FRAME_COUNTERS	0x99	//19RD、应答监测通道帧和字节计数器的统计数据
//定义面板查询消息
#define SET_DEVICE_LED		0x21	//23WR、设置前面板LED状态，总共64个LED
#define CHECK_LED_STATUS	0x22	//24RD、检查前面板LED状态，总共64个LED
//定义面板查询应答
#define ACK_DEVICE_LED		0x0a1	//23WR、应答前面板LED状态，总共64个LED
#define ACK_LED_STATUS		0x0a2	//24RD、应答前面板LED状态，总共64个LED

#define	DEBUG_PRINT		
#define 	APP_DEBUG 			printf
#define 	PLH_DEBUG			DEBUG_PRINT
#define 	DM_DEBUG 			DEBUG_PRINT
#define 	BS_DEBUG			DEBUG_PRINT
#define	CONSOLE_DEBUG		DEBUG_PRINT

typedef struct Command			//驱动的消息格式64字节
{					//驱动的消息包括：帧头，硬件版本、驱动版本、会话，消息命令，消息参数或数据
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，2字节会话，应用端发0，驱动端发1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	Reserve[12];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgCommand;

typedef struct ChInterface		//设置或检查监测通道的以太网接口类型
{					//设置或检查监测通道的以太网接口类型是光接口还是电接口
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	Ch1Interface;	//接口类型4字节，监测通道1的接口类型，光接口1，电接口0
	unsigned int	Ch2Interface;	//接口类型4字节，监测通道2的接口类型，光接口1，电接口0
	unsigned int	Reserve[10];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgChInterface;

typedef struct GpsInterface		//设置或检查使用GPS或1588
{					//设置或检查装置是使用GPS还是1588时钟
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	Gps1588;	//时钟选择4字节，选择GPS接口或1588接口，1选1588，0选IRIGB
	unsigned int	InterfaceType;	//接口类型4字节，选择平衡或非平衡，1平衡接口，0非平衡75欧接口
	unsigned int	Reserve[10];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgGpsInterface;

typedef struct FpgaTime			//设置或检查系统时间，保证系统在闰年或闰秒时正确
{					//根据GPS或1588时间，设置FPGA的年、月、日、时、分、秒、毫秒、微妙
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	Day;		//通道时间，天
	unsigned int	Hour;		//通道时间，时
	unsigned int	Minute;		//通道时间，分
	unsigned int	Second;		//通道时间，秒
	unsigned int	Millisecond;	//通道时间，毫秒
	unsigned int	Microsecond;	//通道时间，微妙
	unsigned int	GpsStatus;	//时钟状态4字节，GPS告警，0=GPS正常，1=GPS时钟丢失
	unsigned int	PtpStatus;	//时钟状态4字节，1588告警，0=正常，1=1588时钟丢失
	unsigned int	ErrStatus;	//时钟错误4字节，FPGA错误，0=本地时钟低于GPS，=1高于GPS
	unsigned int	ErrValue;	//时钟错误4字节，本地与外时钟每秒偏差数，10MHz单位
	unsigned int	Reserve[2];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgFpgaTime;

typedef struct TriggerTime		//设置外触发输入接口1或2的消抖时间
{					//设置外触发继电器1或2的消抖时间
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	Sw1TrigerTime;	//消抖时间4字节，外触发接口1，单位us，最小0，最大256
	unsigned int	Sw2TrigerTime;	//消抖时间4字节，外触发接口2，单位us，最小0，最大256
	unsigned int	Reserve[10];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgTriggerTime;

typedef struct AlarmOut			//设置外告警接口1或2输出
{					//设置外告警接口1或2的继电器闭合
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	Sw1AlarmOut;	//告警输出4字节，外告警接口1，1=闭合，0=断开
	unsigned int	Sw2AlarmOut;	//告警输出4字节，外告警接口2，1=闭合，0=断开
	unsigned int	Reserve[10];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgAlarmOut;

typedef struct BeepAlarm		//设置内蜂鸣器响
{					//设置内蜂鸣器告警
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	BeepAlarm;	//蜂鸣器响4字节，内蜂鸣器告警，D1=1蜂鸣器响一声（按键），自清除，D0=1长鸣
	unsigned int	Reserve[11];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgBeepAlarm;

typedef struct WarchDog			//设置内看门狗芯片清除
{					//清除内看门狗芯片的计数器
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	WarchDog;	//清看门狗4字节，写0xFF清除看门狗，然后自动清除D7～D0数据
	unsigned int	Reserve[11];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgWarchDog;

typedef struct ClearFifo		//设置FIFO空，即清除FIFO数据，指针归零
{					//清除FPGA的64K字节FIFO缓冲器
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	ClearFifo1;	//清看FIFO4字节，写0xFF清除FIFO，然后自动清除D7～D0数据
	unsigned int	ClearFifo2;	//清看FIFO4字节，写0xFF清除FIFO，然后自动清除D7～D0数据
	unsigned int	Reserve[10];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgClearFifo;

typedef struct ChFilter			//设置或检查监测通道的过滤条件
{					//过滤条件：不过滤，广播帧，VLAN，GOOSE，SMV，MMS，61850类型，MAC原，MAC与61850帧，VLAN内61850帧
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	ChNo;		//消息通道4字节，通道1或通道2
	unsigned int	Reset;
	unsigned int	Mode;		//1:过滤模式：DO=1不过滤，0过滤；D1=0只收广播帧，1根据下面条件过滤；00接收广播帧，01不过滤，10根据条件过滤
	unsigned int	MinLength;	//2:最小帧长，隐含是64字节
	unsigned int	MaxLength;	//3:最大帧长，隐含是1518字节
	unsigned int	FilterEnable;	//4:过滤条件，1使能，D0～9: VLAN1，VLAN2，GOOSE，SMV，MMS，类型，MAC源，VLAN过滤，MAC源与类型，VLAN与类型
	unsigned int	Vlan1Id;	//5:Vlan1标签，D11～D0，VID
	unsigned int	Vlan2Id;	//6:Vlan2标签，D11～D0，VID
	unsigned int	MacEnable;	//7:16个MAC源地址使能，bit位置1使能，D15～D0，SourceMac16～1，1=enable，0=disenable
	unsigned int	Reserve[3];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgChFilter;

typedef struct FilterMac		//设置或检查监测通道的过滤MAC
{					//D31～D16保留，D15～D0，SourceMacAdd，47～32 bit；D31～D00使用，D31～D0，SourceMacAdd，31～00 bit
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	ChNo;		//消息通道4字节，通道1或通道2
	unsigned int	GroupNo;	//消息组号4字节，通道1或通道2的第几组，1到4
	unsigned int	MacAddr1H;	//MAC源地址，每个MAC有6字节，前面（高位）的2字节
	unsigned int	MacAddr1L;	//MAC源地址，每个MAC有6字节，后面（低位）的4字节
	unsigned int	MacAddr2H;	//MAC源地址，每个MAC有6字节，前面（高位）的2字节
	unsigned int	MacAddr2L;	//MAC源地址，每个MAC有6字节，后面（低位）的4字节
	unsigned int	MacAddr3H;	//MAC源地址，每个MAC有6字节，前面（高位）的2字节
	unsigned int	MacAddr3L;	//MAC源地址，每个MAC有6字节，后面（低位）的4字节
	unsigned int	MacAddr4H;	//MAC源地址，每个MAC有6字节，前面（高位）的2字节
	unsigned int	MacAddr4L;	//MAC源地址，每个MAC有6字节，后面（低位）的4字节
	unsigned int	Reserve[2];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgFilterMac;

typedef struct ChStatus			//检查监测通道的以太网接口状态，Los/Link/Speed/Duplex
{					//检查监测通道1和2的以太网接口状态，Los/Link/Speed/Duplex
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	EthOLink;	//接口状态4字节，0光口Los，1光口Link
	unsigned int	EthELink;	//接口状态4字节，0不Link，1=Link10半双工，2=Link10全双工，3=Link100半双工，4Link100全双工，
	unsigned int	Reserve[10];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgChStatus;

typedef struct TriggerStatus		//检查外触发接口1或2的状态
{					//检查外触发接口1或2的当前和历时状态
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	Sw1NowStatus;	//接口状态4字节，外触发接口1当前状态，1表示目前继电器闭合
	unsigned int	Sw1HisStatus;	//接口状态4字节，外触发接口1历史状态，1表示曾经继电器闭合
	unsigned int	Sw2NowStatus;	//接口状态4字节，外触发接口2当前状态，1表示目前继电器闭合
	unsigned int	Sw2HisStatus;	//接口状态4字节，外触发接口2历史状态，1表示曾经继电器闭合
	unsigned int	Reserve[8];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgTriggerStatus;

typedef struct KeyStatus		//检查按键1或2的状态
{					//检查按键1或2的当前和历时状态
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	Key1NowStatus;	//按键状态4字节，按键1当前状态，1表示目前按键闭合
	unsigned int	Key1HisStatus;	//按键状态4字节，按键1历史状态，1表示曾经按键闭合
	unsigned int	Key2NowStatus;	//按键状态4字节，按键2当前状态，1表示目前按键闭合
	unsigned int	Key2HisStatus;	//按键状态4字节，按键2历史状态，1表示曾经按键闭合
	unsigned int	Reserve[8];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgKeyStatus;

typedef struct FrameCounters		//检查监测通道的各种计数器数值
{					//检查监测通道1和2的各个计数器数值
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	ChNo;		//消息通道4字节，通道1或通道2
	unsigned int	AllFrame;	//0:总帧数L
	unsigned int	AllByte;	//1:总字节数L
	unsigned int	BroadcastFrame;	//2:广播帧数L
	unsigned int	BroadcastByte;	//3:广播字节数L
	unsigned int	MulticastFrame;	//4:多播帧数L
	unsigned int	MulticastByte;	//5:多播字节数L
	unsigned int	CrcErrFrame;	//6:CRC错误帧数L
	unsigned int	ErrFrame;	//7:错误帧数L
	unsigned int	AllignErrFrame;	//8:定位错误帧数L
	unsigned int	ShortFrame;	//9:超短帧数L
	unsigned int	LongFrame;	//10:超长帧数L
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgFrameCounters;

typedef struct LedStatus		//设置或检查装置前面板的LED
{					//地址0x0B为低位双字，地址0x0C为高位双字
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	LedL;		//32个LED
	unsigned int	LedH;		//32个LED
	unsigned int	Reserve[10];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgLedStatus;

typedef struct ReadBuffer		//读取通道的FIFO数据
{
	unsigned int	Head;		//消息头部4字节，1a2b3c4d
	unsigned int	VersionSid;	//消息会话4字节，1字节硬件版本，1字节驱动版本，会话2字节，驱动端是应用端+1
	unsigned int	Id;		//消息命令4字节，消息命令使用最后1字节
	unsigned int	ChNo;		//消息通道4字节，通道1或通道2
	unsigned int	Byte;		//消息通道4字节，读取字节数
	unsigned int	Reserve[10];	//保留字节x字节，保留字节是为了保证消息为固定的64字节
	unsigned int	Tail;		//消息尾部4字节，a1b2c3d4
}MsgReadBuffer;

//消息变量
#define	MSG_BUF_LEN	0x40		//定义消息的最大长度
#define	CH_BUF_LEN		0x200000	//定义接收报文帧的最大长度
#define	READ_BUF_LEN	0x10000		//定义每次读取报文帧的长度

#define	MSGMAXLEN		0x40			//消息的最大长度，应用传送给驱动读函数的最大数据长度256字节
#define 	DATALEN		0x200000		//测试使用，数据的缓冲区长度2M字节

//#define	REMOTEPORT	5678			//服务器udp端口，以后来自idprom文件，目前定义为5678
#define	REMOTEIP	0x7f000001		//"127.0.0.1",服务器ip地址，使用内部地址

#define	MSGHEAD		0x1a2b3c4d		//消息头部4字节，1a2b3c4d
#define	MSGSESSIONID	0x0			//消息会话4字节，服务器端发送0，客户端加1发送回去
#define	MSGDEVICEID	0x0			//消息装置4字节，该种类型的装置专用，定义为0
#define	MSGVERSION	0x1			//消息版本4字节，消息的版本号，定义为1.0版本
#define	MSGID		0x0			//消息命令4字节，空定义为0
#define	MSGCHNO1	0x1			//消息通道4字节，通道1，值=1
#define	MSGCHNO2	0x2			//消息通道4字节，通道2，值=2
#define	MSGTAIL		0xa1b2c3d4		//消息尾部4字节，a1b2c3d4

#define	EINTERFACE	0x00			//接口类型4字节，以太网电接口=0
#define	OINTERFACE	0x01			//接口类型4字节，以太网光接口=1

#define	TRIGERTIME	0x5a			//通道外触发消抖时间，单位us，最小0，最大256

#define	GPSTTLINTERFACE	0x0			//接口类型4字节，GPS接口类型，TTL或485接口，0=TTL，1=485
#define	GPS485INTERFACE	0x1			//接口类型4字节，GPS接口类型，TTL或485接口，0=TTL，1=485

#define	GPSYEAR		2011			//GPS时间，年
#define	GPSMONTH	5			//GPS时间，月
#define	GPSDAY		18			//GPS时间，日
#define	GPSHOUR		23			//GPS时间，时
#define	GPSMINIT	40			//GPS时间，分
#define	GPSSECOND	0			//GPS时间，秒
#define	GPSMILLISECOND	0			//GPS时间，毫秒
#define	GPSMICROSECOND	0			//GPS时间，微秒

#define	CHRESET		0x01			//复位控制，0不要求复位，1要求复位
#define	CHMODE		0x01			//过滤模式，1不过滤，0过滤
#define	MINLENGTH	64			//最小帧长，隐含是64字节
#define	MAXLENGTH	1518			//最大帧长，隐含是1518字节
#define	VLAN1ENABLE	0x01			//Vlan1使能，D0=1使能
#define	VLAN2ENABLE	0x02			//Vlan2使能，D1=1使能
#define	GOOSEENABLE	0x04			//Goose使能，D2=1使能
#define	SMVENABLE	0x08			//Smv  使能，D3=1使能
#define	MMSENABLE	0x10			//Mms  使能，D4=1使能
#define	VLAN1ID		0x0255			//Vlan1标签
#define	VLAN2ID		0x01aa			//Vlan2标签
#define	MACENABLE	0x55			//Mac  使能，D0到D15=1使能


//从FPGA抓IEC61850报文的定义部分
#define	READ_DIRVER		"/dev/pcifpga"	//驱动名称
#define	LBCRD_BUFF_DATA		0x01		//读取FIFO数据
#define	LBCRD_BUFF_CH1		0x02		//读取通道1的FIFO数据
#define	LBCRD_BUFF_CH2		0x01		//读取通道2的FIFO数据
//#define	DriverBufSize		2048		//2*1024*1024	//至少应存储100ms的数据，暂定2M字节（每秒100M字节/8字节×十分之一秒）＝1.25M字节
#define	DriverBufSize		(512*1024)		//2*1024*1024	//至少应存储100ms的数据，暂定2M字节（每秒100M字节/8字节×十分之一秒）＝1.25M字节

#define	FRAME_HEADE	0x1A2B3C4D
#define	FRAME_TAIL		0xA1B2C3D4
#define	FRAME_HEADE_LEN		4
#define	FRAME_LEN_LEN		2
#define	FRAME_TIME_LEN	16
#define	FRAME_STATE_LEN		2
#define	FRAME_FCS_LEN		4

#define	FRAME_TAIL_LEN		4
#define	SELF_FRAME_TAG		0x7e7e

#define	Htonl(x)		(x)				//powerpc为大端存储,与网络字节序一致,所以不需要转换
#define	Htons(x)		(x)
#define 	Ntohs(x)		(x)
#define	Ntohl(x)		(x)

#endif
