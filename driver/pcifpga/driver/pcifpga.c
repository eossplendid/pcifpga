//Version 1
//PCIFPGA卡驱动程序，2012年8月16日
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/if_vlan.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <asm/unaligned.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/signal.h>
#include <asm/irq.h>
#include <linux/mm.h>

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
	unsigned int	DD;		//通道时间，天
	unsigned int	HH;		//通道时间，时
	unsigned int	MM;		//通道时间，分
	unsigned int	SS;		//通道时间，秒
	unsigned int	MS;		//通道时间，毫秒
	unsigned int	US;		//通道时间，微妙
	unsigned int	GS;		//时钟状态4字节，GPS告警，0=GPS正常，1=GPS时钟丢失
	unsigned int	PS;		//时钟状态4字节，1588告警，0=正常，1=1588时钟丢失
	unsigned int	ES;		//时钟错误4字节，FPGA错误，0=本地时钟低于GPS，=1高于GPS
	unsigned int	EV;		//时钟错误4字节，本地与外时钟每秒偏差数，10MHz单位
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

//PCIFPGA测试卡的PCI相关定义，有关PCI的
#define CTD_VENDOR_ID		0x1100		//Vendor ID
#define CTD_SUB_VENDOR		0x1100		//SubVendor ID
#define CTD_DEVICE_ID		0x4258		//Device ID
#define CTD_SUB_DEVICE		0x4258		//SubDevice ID
#define CTD_PCI_NAME		"CTDPCI-0x1100"
#define CTD_REVISION_ID		0x00		//Revision ID，硬件版本				保留定义
#define CTD_CLASS_CODE		0x04		//Base Class Code 04h Multimedia Device，	保留定义
#define CTD_SUB_CLASS		0x00		//Sub-class Code 00h Video Device，		保留定义

//PCIFPGA的厂家和器件号码
int			ctd_pci_vendor;		//PCI设备配置空间中的Device ID
int			ctd_pci_device;		//PCI设备配置空间中的Vendor ID
//PCIFPGA存储器空间，基地址0（bar0），1K字节，
static unsigned long	chx_mem0_base;		//PCI的存储器空间基地址0，物理地址
static unsigned int	chx_mem0_range;		//PCI的存储器空间基地址范围，1K字节
static void	__iomem	*ChxBaseAddr0;		//PCI的存储器空间基地址0的虚地址，Linux地址
//PCIFPGA存储器空间，基地址1（bar1），1K字节，
static unsigned long	ch1_mem1_base;		//PCI的存储器空间基地址1，物理地址
static unsigned int	ch1_mem1_range;		//PCI的存储器空间基地址范围，1K字节
static void	__iomem	*Ch1BaseAddr1;		//PCI的存储器空间基地址1的虚地址，Linux地址
//PCIFPGA存储器空间，基地址2（bar0），1K字节，
static unsigned long	ch2_mem2_base;		//PCI的存储器空间基地址2，物理地址
static unsigned int	ch2_mem2_range;		//PCI的存储器空间基地址范围，1K字节
static void	__iomem	*Ch2BaseAddr2;		//PCI的存储器空间基地址2的虚地址，Linux地址
//PCIFPGA存储器空间，基地址3（bar3），64K字节，
static unsigned long	ch1_mem3_base;		//PCI的存储器空间基地址3，物理地址0x90020000
static unsigned int	ch1_mem3_range;		//PCI的存储器空间基地址范围，64K字节
static void	__iomem	*Ch1BaseAddr3;		//PCI的存储器空间基地址3的虚地址，Linux地址
//PCIFPGA存储器空间，基地址4（bar4），64K字节，
static unsigned long	ch2_mem4_base;		//PCI的存储器空间基地址4，物理地址0x90040000
static unsigned int	ch2_mem4_range;		//PCI的存储器空间基地址范围，64K字节
static void	__iomem	*Ch2BaseAddr4;		//PCI的存储器空间基地址3的虚地址，Linux地址
//PCIFPGA的中断申请
static int		pcifpga_irq;		//通道1和通道2的PCI的中断号
static int		m_second = 0;		//4毫秒一次中断，中断后m_second加1
static int		second_int = 0;		//1秒加1
#define MAX_M_SECOND	250			//每秒需要读各个计数器的值，4毫秒*250=1秒

//PCIFPGA的相关变量
struct pci_dev		*pcifpga_card_dev;
int    major;

#define	DEVICE_NAME	"pcifpga"		//设备驱动的名称,中断使用,本PCIFPGA卡没有用
#define	PCIFPGA_MAJOR 	245			//设备的驱动号

struct pcifpga_dev {				/* 设备结构体规范格式 */
	struct cdev cdev;			/* cdev结构体 */
};
struct pcifpga_dev *pcifpga_devp;		/* 设备结构体指针 */

//MPC8377CPU有关，MPC8377物理地址对应的虚地址
#define SYSREGADDR	0xff400000		//IMMR reg address，System Configuration
#define DMAMR0		0x8100			//dma0 mode        reg,DMA0的模式寄存器
#define DMASR0		0x8104			//dma0 status      reg,DMA0的状态寄存器
#define DMASAR0		0x8110			//dma0 source      reg,DMA0的原地址寄存器
#define DMADAR0		0x8118			//dma0 destination reg,DMA0目的地址寄存器
#define DMABCR0		0x8120			//dma0 byte count  reg,DMA0的计数器寄存器
#define	SYSREGADDRLEN	0x100000		//sysreg base address对应的1M空间
static void __iomem	*SysRegaddr;		//CPU系统寄存器虚地址,基地址0xff400000
//DMA需要的变量
static unsigned int	*pcifpga_card_dma1;	//申请一块连续的缓存,2M字节用于DMA接收FIFO1数据
static unsigned long	PciDmaBufPhyAddr1;	//FIFO1的DMA缓存对应的物理地址
static unsigned int	*pcifpga_card_dma2;	//申请一块连续的缓存,2M字节用于DMA接收FIFO2数据
static unsigned long	PciDmaBufPhyAddr2;	//FIFO2的DMA缓存对应的物理地址
#define PCIFPGA_FIFO_BYTELEN	0x10000		//PCIFPGA测试卡的FIFO是16K双字,没有用到
#define PCIFPGA_FIFO_DWORDLEN	0x4000		//PCIFPGA测试卡的FIFO是64K字节,没有用到
#define PCIFPGA_DMABUF_BYTELEN	0x80000		//0x1000	//0x80000	//申请128K双字DMA缓冲区,512K字节的DMA缓冲区
#define PCIFPGA_DMABUF_DWORDLEN	0x20000		//0x400		//0x20000	//申请128K双字DMA缓冲区,512K字节的DMA缓冲区
//PCIFPGA测试卡通道1路变量
static unsigned int	ReadCh1Pointer = 0;	//第一路的FIFO读指针,双字指针
static unsigned int	WriteCh1Pointer = 0;	//第一路的FIFO写指针,双字指针
static unsigned int	Ch1FifoFull = 0;	//第一路的FIFO满标志
static unsigned int	Ch1Cnt = 0;		//第一路FIFO已收到的双字数
static unsigned int	Ch1DmaBufFull = 0;	//第一路的DMABUF满标志
//PCIFPGA测试卡通道2路变量
static unsigned int	ReadCh2Pointer = 0;	//第二路的FIFO读指针,双字指针
static unsigned int	WriteCh2Pointer = 0;	//第二路的FIFO写指针,双字指针
static unsigned int	Ch2FifoFull = 0;	//第二路的FIFO满标志
static unsigned int	Ch2Cnt = 0;		//第二路FIFO已收到的双字数
static unsigned int	Ch2DmaBufFull = 0;	//第一路的DMABUF满标志

//解帧需要的变量
#define PCIFPGA_PACKET_BYTELEN	0x800		//申请2K双字缓存区,暂存解出的包,包最长2K字节
#define PCIFPGA_PACKET_DWORDLEN	0x200		//申请512双字缓存区,暂存解出的包,包最长2K字节
#define PCIFPGA_BUF_BYTELEN	0x200000	//0x1000	//0x200000	//申请512K双字缓存,存储待应用读取走解出的包
#define PCIFPGA_BUF_DWORDLEN	0x80000		//0x400		//0x80000	//申请512K双字缓存,存储待应用读取走解出的包
#define	PACKET_MIN_LEN		0x18		//设定包最小包的双字数,24双字
#define	PACKET_OVERHEAD_LEN	0x08		//设定包开销亮的双字数,08双字
#define	PACKET_HEAD_LEN		0x01		//设定包包头域的双字数
#define	PACKET_FCS_LEN		0x01		//设定包校验域的双字数
#define	PACKET_TAIL_LEN		0x01		//设定包包尾域的双字数
#define	FRAME_HEAD_LEN		0x04		//设定帧头部域的字节数
#define	FRAME_LEN_LEN		0x02		//设定帧帧长域的字节数
#define	FRAME_TIME_LEN		0x10		//设定帧时间域的字节数
#define	FRAME_STATUS_LEN	0x02		//设定帧状态域的字节数
#define	FRAME_TAIL_LEN		0x04		//设定帧尾部域的字节数
#define	FRAME_MAX_LEN		0x612		//设定包最大包的双字数,16+1536+2=1554字节
#define	PACKET_HEADE		0x01a2b3c4d	//包头
#define	PACKET_TAIL0		0x0a1b2c3d4	//包尾1
#define	PACKET_TAIL1		0x0b2c3d4ff	//包尾2
#define	PACKET_TAIL2		0x0c3d4ffff	//包尾3
#define	PACKET_TAIL3		0x0d4ffffff	//包尾4
#define	UTC20130101		0x050e1b680	//2013年1月1日0时0分0秒的UTC时间
//解帧缓冲区
static unsigned int	*pcifpga_card_buf0;	//申请一块连续的1K双字缓存解帧
static unsigned int	*pcifpga_card_buf1;	//申请一块连续的512K双字缓存解帧
static unsigned int	*pcifpga_card_buf2;	//申请一块连续的512K双字缓存解帧
//PCIFPGA测试卡通道1路变量
static unsigned int	ReadBuf1Pointer = 0;	//第一路的BUF读指针,双字指针
static unsigned int	ReadCh1Dword = 0;	//第一路的BUF读双字数
static unsigned int	WriteBuf1Pointer = 0;	//第一路的BUF写指针,双字指针
static unsigned int	Ch1PacketCnt = 0;	//第一路已经解出的包数
static unsigned int	Ch1PacketDw = 0;	//第一路已经解出的双字数
static unsigned int	Ch1BufFull = 0;		//第一路的BUF满标志
//PCIFPGA测试卡通道2路变量
static unsigned int	ReadBuf2Pointer = 0;	//第二路的BUF读指针,双字指针
static unsigned int	ReadCh2Dword = 0;	//第二路的BUF读双字数
static unsigned int	WriteBuf2Pointer = 0;	//第二路的BUF写指针,双字指针
static unsigned int	Ch2PacketCnt = 0;	//第二路已经解出的包数
static unsigned int	Ch2PacketDw = 0;	//第二路已经解出的双字数
static unsigned int	Ch2BufFull = 0;		//第二路的BUF满标志

//Base Address 0 0～256字节，双字读写
//Configure.v模块
#define SW1TRIGGERTIME		0x00	//D31～D08保留，D7～0开量入消抖时间,缺省1us,最小1us,最大255us
#define SW2TRIGGERTIME		0x01	//D31～D08保留，D7～0开量入消抖时间,缺省1us,最小1us,最大255us
#define SWALARMOUT		0x02	//D31～D02保留，D1～0告警出2和1,缺省为开,1闭合,0断开
#define BEEPALARMOUT		0x03	//D31～D02保留，D1置1响一声(按键)后清除,D0置1长鸣,缺省不响
#define CHINTERFACESEL		0x04	//D31～D02保留，D1～0监测通道1和2,缺省电,1光,0电(00全电,01一光二电,02一电二光,03全光)
#define GPSINTERFACESEL		0x05	//D31～D02保留，D1置1选1588,0选IRIGB,D0置1平衡,0非平衡,缺省GPS平衡
#define WARCHDOGOUT		0x06	//D31～D08保留，D7～0置全1看门狗脉冲输出1次,然后自动清除D7～0数据
#define SWTRIGGERTIME		0x0064	//消抖时间1毫秒
//Sw_in.v模块,根据设定的消抖时间检测两路开入量信号,并产生状态信号和中断信号
#define SW1STATUS		0x07	//D31～D02保留，D1=1曾有开量输入,D0=1目前继电器闭合,D1触发中断,查询后D1清除
#define SW2STATUS		0x08	//D31～D02保留，D1=1曾有开量输入,D0=1目前继电器闭合,D1触发中断,查询后D1清除
//Key_in.v模块,面板按键扫描,对面板按键做15ms的消抖处理,产生按键状态信号和中断信号
#define KEY1STATUS		0x09	//D31～D02保留，D1=1曾有按键输入,D0=1目前按键闭合,D1触发中断,查询后D1清除
#define KEY2STATUS		0x0A	//D31～D02保留，D1=1曾有按键输入,D0=1目前按键闭合,D1触发中断,查询后D1清除
//Display.v模块,面板LED显示,显示数据64bit,写入两个双字,按照高位在前低位在后的顺序移位显示,每秒检测一次数据的变化,并显示
#define LED1DISPLAY		0x0B	//D31～D00使用，前面板LED显示的低位双字,全部使用
#define LED2DISPLAY		0x0C	//D31～D00使用，前面板LED显示的低位双字,全部使用
//Irq.v模块,设置各中断相关参数,中断使能,产生中断信号
#define INTERRUPTTIMER		0x0D	//D31～D16保留，定时中断时间,D15～0设置1～65535ms,缺省0不中断
#define INTERRUPTENABLE		0x0E	//D31～D08保留，D7总,D6/5第2/1路FIFO满,D4/3开入2/1,D2/1按键2/1,0定时,1使能,缺省0禁止
#define INTERRUPTSTATUS		0x0F	//D31～D07保留，D6/5第2/1路64K满，D4/3开入2/1，D2/1按键2/1，0定时，1中断，读后清除
#define INTTIMERV		0x0004	//4毫秒中断
#define INTTIMERENABLE		0x00ff	//定时中断使能
#define INTTIMERDISABLE		0x0000	//定时中断禁止
//Eth_miim.v模块,以太网接口管理模块，模拟以太网PHY管理接口（MDC、MDIO），完成对外部以太网PHY（多个DP83640）的控制及状态读取
#define ETHMIIMMDC		0x10	//D31～D08保留，MDC分频寄存器,D7～0设置时钟分频数(MDC),MDC由主时钟33MHz经2～255分频获得
#define ETHMIIMPHYADDREG	0x11	//D31～D13保留，PHY和寄存器地址,D12～8PHY地址,D4～0寄存器地址
#define ETHMIIMWRDATA		0x12	//D31～D16保留，D15～D0用于写入控制数据
#define ETHMIIMCOMMAND		0x13	//D31～D04保留，D3置1扫描,不能恢复,置0退出,D2置1读PHY,D1置1写PHY,D0置1有32bit全1前导码
#define ETHMIIMRDDATA		0x14	//D31～D16保留，D15～0外部PHY状态数据
#define ETHMIIMPHYSTS		0x15	//D31～D03保留，PHY状态寄存器,D2=1Nvalid(无效),D1=1Busy(忙),D0=1LinkFail(连接失败)
//Timer.v模块,完成IRIG_B码解码,对时及本地时钟的计时,为两路以太网录波信号提供实时间时标
#define IRIGB1588STATUS		0x20	//D31～D04保留，GPS和1588告警,D3～0对应1588正常,1588时钟丢失,GPS正常,GPS时钟丢失,高有效
#define IRIGB1588TIMEERR	0x21	//D31～D25保留，D24=0本地时钟低于GPS,=1高于GPS,D23～D0本地与外时钟每秒偏差数,10MHz单位
//CPU接收IEEE1588的TOD(UTC)计算后(十进制8421码)在一定的时间内(100ms)写入该寄存器,在IEEE1588下一时刻的PPS到来时置入本地时间计数器
#define IRIGB1588WRSECOND	0x22	//D31～D31保留，D31-28秒十,D27-24个,D23-20毫百,D19-16十,D15-12个,D11-8微百,D7-4十,D3-0个
#define IRIGB1588WRDAY		0x23	//D31～D26保留，D25-24天百,D23-20十.D19～16个,D13-12时十,D11-8个,D7-4分十,D3～0个
#define IRIGB1588TIMEPUT	0x24	//D31～D08保留，D7～0置全1,把0x22和0x23地址中时间置入计时器,D7～0写后自清除
#define IRIGB1588E		0x00ff
//CPU接收IEEE1588的TOD（UTC）计算后（十进制8421码）在一定的时间内（100ms）写入该寄存器，在IEEE1588下一时刻的PPS到来时置入本地时间计数器
#define IRIGB1588TIMEGET	0x25	//D31～D08保留，D7～0置全1,把当前计时器时间置入0x26和0x27地址,D7～0写后自清除
#define IRIGB1588RDSECOND	0x26	//D31～D00使用，D31-28秒十,D27-24个,D23-20毫百,D19-16十,D15-12个,D11-8微百,D7-4十,D3-0个
#define IRIGB1588RDDAY		0x27	//D31～D26保留，D25-24天百,D23-20十.D19～16个,D13-12时十,D11-8个,D7-4分十,D3～0个
//FIFO数据、状态、命令和计数器寄存器的偏移量
#define CH1FIFORESET		0x28	//D31～D08保留，D7～0置全1报文存储FIFO复位,状态和计数器复位,后自动清除
#define CH1FIFOSTATUS		0x29	//D31～D05保留，报文FIFO状态,D4空,D3满,D2半满,D1将空,D0将满,高有效,产生中断
#define CH1FIFOCOUNT		0x2A	//D31～D30保留，取FIFO计数,D29-16为读[13:0],D13-0为写[13:0],读前写0xff,延时1us,再读
#define CH2FIFORESET		0x2B	//D31～D08保留，D7～0置全1报文存储FIFO复位,状态和计数器复位,后自动清除
#define CH2FIFOSTATUS		0x2C	//D31～D05保留，报文FIFO状态,D4空,D3满,D2半满,D1将空,D0将满,高有效,产生中断
#define CH2FIFOCOUNT		0x2D	//D31～D30保留，取FIFO计数,D29-16为读[13:0],D13-0为写[13:0],读前写0xff,延时1us,再读
#define CHFIFORESET		0x00ff	//报文FIFO中数据复位使能,0xFF,D7～D0写后自动清除
#define CHFIFOREADE		0x00ff	//报文FIFO计数器读取使能,0xFF,D7～D0写后自动清除

//Base Address 1 0～256字节，双字读写，第1路以太网监测口接收部分控制命令和状态
//Base Address 2 0～256字节，双字读写，第2路以太网监测口接收部分控制命令和状态
//Eth_rxregisters.v模块
#define CHFILTERRESET		0x00	//D31～D00使用，写0xA5A5A5A5复位除寄存器模块外的所有模块,配置完之后做1次
#define CHFILTERMODE		0x01	//D31～D02保留，D0=0Fileter,=1NoFilerer,D1=0ReceiveAllBroadcast,=1RejiectAllBroadcast
#define CHFILTERMINLEN		0x02	//D31～D16保留，最短帧长,D15～0写入MinimumEthernetPacket,缺省64
#define CHFILTERMAXLEN		0x03	//D31～D16保留，最长帧长,D15～0写入MaximumEthernetPacket,缺省1536
#define CHFILTERENABLE		0x04	//D31～D16保留，1=e,0=d,D0～9:VLAN1,VLAN2,GOOSE,SMV,MMS,类型,MAC源,VLAN过滤,MAC源与类型,VLAN与类型
#define CHFILTERVID1		0x05	//D31～D12保留，D11～0,VlanID1
#define CHFILTERVID2		0x06	//D31～D12保留，D11～0,VlanID2
#define CHFILTERMACEN		0x07	//D31～D16保留，D15～0,SourceMac16～1,1=enable,0=disenable
#define CHFILTERMACH		0x08	//D31～D16保留，D15～0,SourceMacAdd,47～32bit
#define CHFILTERMACL		0x09	//D31～D00使用，D31～0,SourceMacAdd,31～00bit
//新功能
#define STOPRX			0x28	//D31～D01保留，D0=0正常接收帧,D0=1停止接收帧,各个计数器正常计数
#define RESETCOUNTER		0x29	//D31～D08使用，D7～0=AC各个计数器全部复位,即清零
//Eth_rxstatistics.v模块,传输性能统计模块的计数锁存值,每个计数器4字节长度,循环计数
//总帧和字节计数,广播帧和字节计数,多播帧和字节计数,CRC错误帧计数,接收错误帧计数,非整数字节帧计数,超短帧计数,超长帧计数
//锁存方式:系统每次读上一次的锁存值,只有4个字节全部读完时才能锁存,并在最后一个字节读之后锁存计数值,提供下一次读的锁存值
#define CHALLFRAMEH		0x30	//[63:32]
#define CHALLFRAMEL		0x31	//[31:00]
#define CHALLBYTEH		0x32	//[63:32]
#define CHALLBYTEL		0x33	//[31:00]
#define CHBROADCASTFRAMEH	0x34	//[63:32]
#define CHBROADCASTFRAMEL	0x35	//[31:00]
#define CHBROADCASTBYTEH	0x36	//[63:32]
#define CHBROADCASTBYTEL	0x37	//[31:00]
#define CHMULTCASTFRAMEH	0x38	//[63:32]
#define CHMULTCASTFRAMEL	0x39	//[31:00]
#define CHMULTCASTBYTEH		0x3A	//[63:32]
#define CHMULTCASTBYTEL		0x3B	//[31:00]
#define CHCRCERRFRAMEH		0x3C	//[63:32]
#define CHCRCERRFRAMEL		0x3D	//[31:00]
#define CHERRORFRAMEH		0x3E	//[63:32]
#define CHERRORFRAMEL		0x3F	//[31:00]
#define CHALLIGNERRFRAMEH	0x40	//[63:32]
#define CHALLIGNERRFRAMEL	0x41	//[31:00]
#define CHSHORTFRAMEH		0x42	//[63:32]
#define CHSHORTFRAMEL		0x43	//[31:00]
#define CHLONGFRAMEH		0x44	//[63:32]
#define CHLONGFRAMEL		0x45	//[31:00]

//消息变量
#define	MSG_BUF_LEN	0x40		//定义消息的最大长度
MsgCommand		*DrvMsgCommand;
MsgChInterface		*DrvChInterface;
MsgGpsInterface		*DrvGpsInterface;
MsgFpgaTime		*DrvFpgaTime;
MsgTriggerTime		*DrvTriggerTime;
MsgAlarmOut		*DrvAlarmOut;
MsgBeepAlarm		*DrvBeepAlarm;
MsgWarchDog		*DrvWarchDog;
MsgClearFifo		*DrvClearFifo;
MsgChFilter		*DrvChFilter;
MsgFilterMac		*DrvFilterMac;
MsgChStatus		*DrvChStatus;
MsgTriggerStatus	*DrvTriggerStatus;
MsgKeyStatus		*DrvKeyStatus;
MsgFrameCounters	*DrvFrameCounters;
MsgLedStatus		*DrvLedStatus;
MsgReadBuffer		*DrvReadBuffer;
static unsigned char	*MsgBuffer;

static int ReadFifo1(unsigned int FifoCnt)//FifoCnt以双字为单位
{
	unsigned int BufRemainLen, RemLen;
	//unsigned int i;

	//直接读入FIFO数据,并将读出的数据存储在DMA缓存器1中
	if(ReadCh1Pointer <= WriteCh1Pointer) {
		if(((WriteCh1Pointer + FifoCnt) == PCIFPGA_DMABUF_DWORDLEN) && (ReadCh1Pointer == 0)) {
			Ch1DmaBufFull++;//缓存将满
			//printk("TY-3012S PciDrv WriteCh1DmaBuf full!\n");//显示缓存满
			return 0;
		}
		if((WriteCh1Pointer + FifoCnt) <= PCIFPGA_DMABUF_DWORDLEN) {
			memcpy((pcifpga_card_dma1 + WriteCh1Pointer), Ch1BaseAddr3, 4*FifoCnt);
			#if 0
			printk("TY-3012S PciDrv Test Fifo1:    %08x!\n", FifoCnt);
			for(i = 0; i < FifoCnt; i++) {
				if((i % 0x04) == 0) printk("\n");
				printk(".%08x", *(pcifpga_card_dma1 + WriteCh1Pointer + i));
			}
			printk("!\n");
			#endif
			WriteCh1Pointer += FifoCnt;
			if(WriteCh1Pointer == PCIFPGA_DMABUF_DWORDLEN) WriteCh1Pointer = 0;
			Ch1Cnt += FifoCnt;
			//printk("TY-3012S PciDrv WrCh1Pointer:  %08x!\n", WriteCh1Pointer);//显示写入了数据
			return FifoCnt;
		}
		if((WriteCh1Pointer + FifoCnt) > PCIFPGA_DMABUF_DWORDLEN) {
			BufRemainLen = PCIFPGA_DMABUF_DWORDLEN - WriteCh1Pointer;
			RemLen = FifoCnt - BufRemainLen;
			if(RemLen >= ReadCh1Pointer) {//缓存将满
				Ch1DmaBufFull++;
				//printk("TY-3012S PciDrv WriteCh1DmaBuf full!!\n");//显示缓存满
				return 0;
			}
			memcpy((pcifpga_card_dma1 + WriteCh1Pointer), Ch1BaseAddr3, 4*BufRemainLen);
			WriteCh1Pointer = 0;
			memcpy((pcifpga_card_dma1 + WriteCh1Pointer), Ch1BaseAddr3, 4*RemLen);
			WriteCh1Pointer = RemLen;
			Ch1Cnt += FifoCnt;
			//printk("TY-3012S PciDrv WrCh1Pointer:  %08x!!\n", WriteCh1Pointer);//显示写入了数据
			return FifoCnt;
		}
	} else {
		if((WriteCh1Pointer + FifoCnt) >= ReadCh1Pointer) {//缓存将满
			Ch1DmaBufFull++;
			//printk("TY-3012S PciDrv WriteCh1DmaBuf full!!!\n");//显示缓存满
			return 0;
		} else {
			memcpy((pcifpga_card_dma1 + WriteCh1Pointer), Ch1BaseAddr3, 4*FifoCnt);
			WriteCh1Pointer += FifoCnt;
			Ch1Cnt += FifoCnt;
			//printk("TY-3012S PciDrv WrCh1Pointer:  %08x!!!\n", WriteCh1Pointer);//显示写入了数据
			return FifoCnt;
		}
	}
	return 0;
}

static int ReadFifo2(unsigned int FifoCnt)
{
	unsigned int BufRemainLen, RemLen;

	//直接读入FIFO数据,并将读出的数据存储在DMA缓存器2中
	if(ReadCh2Pointer <= WriteCh2Pointer) {
		if(((WriteCh2Pointer + FifoCnt) == PCIFPGA_DMABUF_DWORDLEN) && (ReadCh2Pointer == 0)) {
			Ch2DmaBufFull++;//缓存将满
			//printk("TY-3012S PciDrv WriteCh2DmaBuf full!\n");
			return 0;
		}
		if((WriteCh2Pointer + FifoCnt) <= PCIFPGA_DMABUF_DWORDLEN) {
			memcpy((pcifpga_card_dma2 + WriteCh2Pointer), Ch2BaseAddr4, 4*FifoCnt);
			WriteCh2Pointer += FifoCnt;
			if(WriteCh2Pointer == PCIFPGA_DMABUF_DWORDLEN) WriteCh2Pointer = 0;
			Ch2Cnt += FifoCnt;
			//printk("TY-3012S PciDrv WrCh2Pointer:  %08x!\n", WriteCh2Pointer);
			return FifoCnt;
		}
		if((WriteCh2Pointer + FifoCnt) > PCIFPGA_DMABUF_DWORDLEN) {
			BufRemainLen = PCIFPGA_DMABUF_DWORDLEN - WriteCh2Pointer;
			RemLen = FifoCnt - BufRemainLen;
			if(RemLen >= ReadCh2Pointer) {//缓存将满
				Ch2DmaBufFull++;
				//printk("TY-3012S PciDrv WriteCh2DmaBuf full!!\n");
				return 0;
			}
			memcpy((pcifpga_card_dma2 + WriteCh2Pointer), Ch2BaseAddr4, 4*BufRemainLen);
			WriteCh2Pointer = 0;
			memcpy((pcifpga_card_dma2 + WriteCh2Pointer), Ch2BaseAddr4, 4*RemLen);
			WriteCh2Pointer = RemLen;
			Ch2Cnt += FifoCnt;
			//printk("TY-3012S PciDrv WrCh2Pointer:  %08x!!\n", WriteCh2Pointer);
			return FifoCnt;
		}
	} else {
		if((WriteCh2Pointer + FifoCnt) >= ReadCh2Pointer) {//缓存将满
			Ch2DmaBufFull++;
			//printk("TY-3012S PciDrv WriteCh2DmaBuf full!!!\n");
			return 0;
		} else {
			memcpy((pcifpga_card_dma2 + WriteCh2Pointer), Ch2BaseAddr4, 4*FifoCnt);
			WriteCh2Pointer += FifoCnt;
			Ch2Cnt += FifoCnt;
			//printk("TY-3012S PciDrv WrCh2Pointer:  %08x!!!\n", WriteCh2Pointer);
			return FifoCnt;
		}
	}
	return 0;
}

static int ReadCh1Buf(unsigned int *buf, unsigned int len)//读1路数据给应用,双字数
{
	unsigned int BufRemainLen, RemLen;

	if(ReadBuf1Pointer == WriteBuf1Pointer) {
		//printk("TY-3012S PciDrv ReadBuf1 Empty!\n");
		return 0;
	}
	if(ReadBuf1Pointer < WriteBuf1Pointer) {
		if((ReadBuf1Pointer + len) <= WriteBuf1Pointer) {
			copy_to_user(buf, pcifpga_card_buf1 + ReadBuf1Pointer, 4*len);
			ReadBuf1Pointer += len;
			ReadCh1Dword += len;
			//printk("TY-3012S PciDrv RdBuf1Pointer: %08x!\n", ReadBuf1Pointer);
			return len;
		} else {
			BufRemainLen = WriteBuf1Pointer - ReadBuf1Pointer;
			copy_to_user(buf, pcifpga_card_buf1 + ReadBuf1Pointer, 4*BufRemainLen);
			ReadBuf1Pointer += BufRemainLen;
			ReadCh1Dword += BufRemainLen;
			//printk("TY-3012S PciDrv RdBuf1Pointer: %08x!!\n", ReadBuf1Pointer);
			return BufRemainLen;
		}
	}
	if(ReadBuf1Pointer > WriteBuf1Pointer) {
		if((ReadBuf1Pointer + len) <= PCIFPGA_BUF_DWORDLEN) {
			copy_to_user(buf, pcifpga_card_buf1 + ReadBuf1Pointer, 4*len);
			ReadBuf1Pointer += len;
			if(ReadBuf1Pointer == PCIFPGA_BUF_DWORDLEN) ReadBuf1Pointer = 0;
			ReadCh1Dword += len;
			//printk("TY-3012S PciDrv RdBuf1Pointer: %08x!!!\n", ReadBuf1Pointer);
			return len;
		}
		if((ReadBuf1Pointer + len) > PCIFPGA_BUF_DWORDLEN) {
			BufRemainLen = PCIFPGA_BUF_DWORDLEN - ReadBuf1Pointer;
			RemLen = len - BufRemainLen;
			copy_to_user(buf, pcifpga_card_buf1 + ReadBuf1Pointer, 4*BufRemainLen);
			ReadBuf1Pointer = 0;
			ReadCh1Dword += BufRemainLen;
			return BufRemainLen;
			if(RemLen >= WriteBuf1Pointer) {
				copy_to_user((buf + BufRemainLen), pcifpga_card_buf1, 4*WriteBuf1Pointer);
				ReadBuf1Pointer += WriteBuf1Pointer;
				ReadCh1Dword += WriteBuf1Pointer;
				return BufRemainLen + WriteBuf1Pointer;
			} else {
				copy_to_user((buf + BufRemainLen), pcifpga_card_buf1, 4*RemLen);
				ReadBuf1Pointer += RemLen;
				ReadCh1Dword += RemLen;
				return len;
			}
			//printk("TY-3012S PciDrv RdBuf1Pointer: %08x!!!!\n", ReadBuf1Pointer);
		}
	}
	return 0;
}

static int ReadCh2Buf(unsigned int *buf, unsigned int len)//读2路数据给应用,双字数
{
	unsigned int BufRemainLen, RemLen;

	if(ReadBuf2Pointer == WriteBuf2Pointer) {
		//printk("TY-3012S PciDrv ReadBuf2 Empty!\n");
		return 0;
	}
	if(ReadBuf2Pointer < WriteBuf2Pointer) {
		if((ReadBuf2Pointer + len) <= WriteBuf2Pointer) {
			copy_to_user(buf, pcifpga_card_buf2 + ReadBuf2Pointer, 4*len);
			ReadBuf2Pointer += len;
			ReadCh2Dword += len;
			//printk("TY-3012S PciDrv RdBuf2Pointer: %08x!\n", ReadBuf2Pointer);
			return len;
		} else {
			BufRemainLen = WriteBuf2Pointer - ReadBuf2Pointer;
			copy_to_user(buf, pcifpga_card_buf2 + ReadBuf2Pointer, 4*BufRemainLen);
			ReadBuf2Pointer += BufRemainLen;
			ReadCh2Dword += BufRemainLen;
			//printk("TY-3012S PciDrv RdBuf2Pointer: %08x!!\n", ReadBuf2Pointer);
			return BufRemainLen;
		}
	}
	if(ReadBuf2Pointer > WriteBuf2Pointer) {
		if((ReadBuf2Pointer + len) <= PCIFPGA_BUF_DWORDLEN) {
			copy_to_user(buf, pcifpga_card_buf2 + ReadBuf2Pointer, 4*len);
			ReadBuf2Pointer += len;
			if(ReadBuf2Pointer == PCIFPGA_BUF_DWORDLEN) ReadBuf2Pointer = 0;
			ReadCh2Dword += len;
			//printk("TY-3012S PciDrv RdBuf2Pointer: %08x!!!\n", ReadBuf2Pointer);
			return len;
		}
		if((ReadBuf2Pointer + len) > PCIFPGA_BUF_DWORDLEN) {
			BufRemainLen = PCIFPGA_BUF_DWORDLEN - ReadBuf2Pointer;
			RemLen = len - BufRemainLen;
			copy_to_user(buf, pcifpga_card_buf2 + ReadBuf2Pointer, 4*BufRemainLen);
			ReadBuf2Pointer = 0;
			ReadCh2Dword += BufRemainLen;
			return BufRemainLen;
			if(RemLen >= WriteBuf2Pointer) {
				copy_to_user((buf + BufRemainLen), pcifpga_card_buf2, 4*WriteBuf2Pointer);
				ReadBuf2Pointer += WriteBuf2Pointer;
				ReadCh2Dword += WriteBuf2Pointer;
				return BufRemainLen + WriteBuf2Pointer;
			} else {
				copy_to_user((buf + BufRemainLen), pcifpga_card_buf2, 4*RemLen);
				ReadBuf2Pointer += RemLen;
				ReadCh2Dword += RemLen;
				return len;
			}
			//printk("TY-3012S PciDrv RdBuf2Pointer: %08x!!!!\n", ReadBuf2Pointer);
		}
	}
	return 0;
}

static int WriteCh1Buf(unsigned int PacketLen)//双字
{
	unsigned int BufRemainLen, RemLen;
	unsigned char *ptr0, *ptr1;
	unsigned int PacketStatusH, PacketStatusL;
	unsigned int FrameLen, PacketPadLen;
	unsigned int dd, hh, mm, ss, ms, us;
	unsigned int pcapss, pcapus, pcaplen;
	//unsigned int i;

	FrameLen = *(pcifpga_card_buf0 + (2*PACKET_HEAD_LEN)) >> 0x10;//确定包长,因为保留4字节,双字单位
	PacketPadLen = (FrameLen + FRAME_LEN_LEN) % 4;//确定包填充字节数
	ptr0 = (unsigned char *)(pcifpga_card_buf0) + FrameLen + 2*FRAME_HEAD_LEN;//包状态位置
	PacketStatusH = *ptr0;
	PacketStatusL = *(ptr0 + 1);
	ptr0 = (unsigned char *)(pcifpga_card_buf0);//按照字节拷贝
	ptr1 = (unsigned char *)(pcifpga_card_buf1);//按照字节拷贝
	memcpy((ptr0 + FRAME_STATUS_LEN), (ptr0 + FRAME_HEAD_LEN), (FRAME_HEAD_LEN + FRAME_LEN_LEN));//移位包头和帧长位置,腾出帧状态字节的位置
	*(ptr0 + 2*FRAME_HEAD_LEN) = PacketStatusH;
	*(ptr0 + 2*FRAME_HEAD_LEN + 1) = PacketStatusL;
	memcpy((ptr0 + 2*FRAME_HEAD_LEN + FrameLen), (ptr0 + 2*FRAME_HEAD_LEN + FRAME_LEN_LEN + FrameLen), (PacketPadLen + 3*FRAME_TAIL_LEN));//移位去掉尾部帧状态
	#if 0
	printk("TY-3012S PciDrv WriteCh1Buf:   %08x!", 4*PacketLen);
	for(i = 0; i < 4*PacketLen; i++) {
		if((i % 16) == 0) printk("\n");
		printk(".%02x", *(ptr0 + 2 + i));
	}
	printk("!\n");
	#endif
	//计算时间
	dd = (*(ptr0 + 17) & 0x0f)*100 + (*(ptr0 + 18) >> 4)*10 + (*(ptr0 + 18) & 0x0f);//4*FRAME_HEAD_LEN+FRAME_LEN_LEN+0=18;
	if(dd > 0) dd -= 1;
	hh = (*(ptr0 + 19) >> 4)*10 + (*(ptr0 + 19) & 0x0f);//4*FRAME_HEAD_LEN+FRAME_LEN_LEN+1=19;
	mm = (*(ptr0 + 20) >> 4)*10 + (*(ptr0 + 20) & 0x0f);//4*FRAME_HEAD_LEN+FRAME_LEN_LEN+2=20;
	ss = (*(ptr0 + 21) >> 4)*10 + (*(ptr0 + 21) & 0x0f);//4*FRAME_HEAD_LEN+FRAME_LEN_LEN+3=21;
	ms = (*(ptr0 + 22)*100) + (*(ptr0 + 23) >> 4)*10 + (*(ptr0 + 23) & 0x0f);//5*FRAME_HEAD_LEN+FRAME_LEN_LEN+0/1=22/23;
	us = (*(ptr0 + 24)*100) + (*(ptr0 + 25) >> 4)*10 + (*(ptr0 + 25) & 0x0f);//5*FRAME_HEAD_LEN+FRAME_LEN_LEN+2/3=24/25;
	pcapss = UTC20130101 + 86400*dd + 3600*hh + 60*mm + ss;//24*3600*dd + 3600*hh + 60*mm + ss;
	*(ptr0 + 10) = pcapss & 0x000000ff;
	pcapss >>= 8;
	*(ptr0 + 11) = pcapss & 0x000000ff;
	pcapss >>= 8;
	*(ptr0 + 12) = pcapss & 0x000000ff;
	pcapss >>= 8;
	*(ptr0 + 13) = pcapss & 0x000000ff;
	pcapus = 1000*ms + us;
	*(ptr0 + 14) = pcapus & 0x000000ff;
	pcapus >>= 8;
	*(ptr0 + 15) = pcapus & 0x000000ff;
	pcapus >>= 8;
	*(ptr0 + 16) = pcapus & 0x000000ff;
	pcapus >>= 8;
	*(ptr0 + 17) = pcapus & 0x000000ff;
	pcaplen = FrameLen - 18;//16字节时间+n字节帧+2字节状态
	*(ptr0 + 22) = *(ptr0 + 18) = pcaplen & 0x000000ff;
	pcaplen >>= 8;
	*(ptr0 + 23) = *(ptr0 + 19) = pcaplen & 0x000000ff;
	*(ptr0 + 24) = *(ptr0 + 20) = 0;
	*(ptr0 + 25) = *(ptr0 + 21) = 0;
	#if 0
	printk("TY-3012S PciDrv WriteCh1Buf:   %08x!!", 4*PacketLen);
	for(i = 0; i < 4*PacketLen; i++) {
		if((i % 16) == 0) printk("\n");
		printk(".%02x", *(ptr0 + 2 + i));
	}
	printk("!\n");
	#endif
	//将解出的数据帧传送给通道1的数据缓存器
	if(ReadBuf1Pointer <= WriteBuf1Pointer) {
		if(((WriteBuf1Pointer + PacketLen) == PCIFPGA_BUF_DWORDLEN) && (ReadBuf1Pointer == 0)) {
			Ch1BufFull++;//缓存将满
			//printk("TY-3012S PciDrv WriteCh1Buf full!\n");//显示缓存满
			return 0;
		}
		if((WriteBuf1Pointer + PacketLen) <= PCIFPGA_BUF_DWORDLEN) {
			memcpy((ptr1 + 4*WriteBuf1Pointer), (ptr0 + 2), 4*PacketLen);//复制内容:时间+净荷+校验+帧尾
			#if 0
			printk("TY-3012S PciDrv WriteCh1Buf:   %08x!!!", PacketLen);
			for(i = 0; i < PacketLen; i++) {
				if((i % 0x04) == 0) printk("\n");
				printk(".%08x", *(pcifpga_card_buf1 + WriteBuf1Pointer + i));
			}
			printk("!\n");
			#endif
			WriteBuf1Pointer += PacketLen;
			if(WriteBuf1Pointer == PCIFPGA_BUF_DWORDLEN) WriteBuf1Pointer = 0;
			Ch1PacketCnt ++;
			Ch1PacketDw += PacketLen;
			//printk("TY-3012S PciDrv WrBuf1Pointer: %08x!\n", WriteBuf1Pointer);//显示写入了数据
			return PacketLen;
		}
		if((WriteBuf1Pointer + PacketLen) > PCIFPGA_BUF_DWORDLEN) {
			BufRemainLen = PCIFPGA_BUF_DWORDLEN - WriteBuf1Pointer;
			if((PacketLen - BufRemainLen) >= ReadBuf1Pointer) {//缓存将满
				Ch1BufFull++;
				//printk("TY-3012S PciDrv WriteCh1Buf full!!\n");//显示缓存满
				return 0;
			}
			memcpy((ptr1 + 4*WriteBuf1Pointer), (ptr0 + 2), 4*BufRemainLen);
			WriteBuf1Pointer = 0;
			RemLen = PacketLen - BufRemainLen;
			memcpy(ptr1, (ptr0 + 2 + 4*BufRemainLen), 4*RemLen);
			WriteBuf1Pointer = RemLen;
			Ch1PacketCnt ++;
			Ch1PacketDw += PacketLen;
			//printk("TY-3012S PciDrv WrBuf1Pointer: %08x!!\n", WriteBuf1Pointer);//显示写入了数据
			return PacketLen;
		}
	} else {
		if((WriteBuf1Pointer + PacketLen) >= ReadBuf1Pointer) {//缓存将满
			Ch1BufFull++;
			printk("TY-3012S PciDrv WriteCh1Buf full!!!\n");//显示缓存满
			return 0;
		} else {
			memcpy((ptr1 + 4*WriteBuf1Pointer), (ptr0 + 2), 4*PacketLen);//复制内容:时间+净荷+校验+帧尾
			WriteBuf1Pointer += PacketLen;
			Ch1PacketCnt ++;
			Ch1PacketDw += PacketLen;
			//printk("TY-3012S PciDrv WrBuf1Pointer: %08x!!!\n", WriteBuf1Pointer);//显示写入了数据
			return PacketLen;
		}
	}
	return 0;
}

static int WriteCh2Buf(unsigned int PacketLen)//双字
{
	unsigned int BufRemainLen, RemLen;
	unsigned char *ptr0, *ptr2;
	unsigned int PacketStatusH, PacketStatusL;
	unsigned int FrameLen, PacketPadLen;
	unsigned int dd, hh, mm, ss, ms, us;
	unsigned int pcapss, pcapus, pcaplen;

	FrameLen = *(pcifpga_card_buf0 + (2*PACKET_HEAD_LEN)) >> 0x10;//确定包长,因为保留4字节,双字单位
	PacketPadLen = (FrameLen + FRAME_LEN_LEN) % 4;//确定包填充字节数
	ptr0 = (unsigned char *)(pcifpga_card_buf0) + FrameLen + 2*FRAME_HEAD_LEN;//包状态位置
	PacketStatusH = *ptr0;
	PacketStatusL = *(ptr0 + 1);
	ptr0 = (unsigned char *)(pcifpga_card_buf0);//按照字节拷贝
	ptr2 = (unsigned char *)(pcifpga_card_buf2);//按照字节拷贝
	memcpy((ptr0 + FRAME_STATUS_LEN), (ptr0 + FRAME_HEAD_LEN), (FRAME_HEAD_LEN + FRAME_LEN_LEN));//移位包头和帧长位置,腾出帧状态字节的位置
	*(ptr0 + 2*FRAME_HEAD_LEN) = PacketStatusH;
	*(ptr0 + 2*FRAME_HEAD_LEN + 1) = PacketStatusL;
	memcpy((ptr0 + 2*FRAME_HEAD_LEN + FrameLen), (ptr0 + 2*FRAME_HEAD_LEN + FRAME_LEN_LEN + FrameLen), (PacketPadLen + 3*FRAME_TAIL_LEN));//移位去掉尾部帧状态
	//计算时间
	dd = (*(ptr0 + 17) & 0x0f)*100 + (*(ptr0 + 18) >> 4)*10 + (*(ptr0 + 18) & 0x0f);//4*FRAME_HEAD_LEN+FRAME_LEN_LEN+0=18;
	if(dd > 0) dd -= 1;
	hh = (*(ptr0 + 19) >> 4)*10 + (*(ptr0 + 19) & 0x0f);//4*FRAME_HEAD_LEN+FRAME_LEN_LEN+1=19;
	mm = (*(ptr0 + 20) >> 4)*10 + (*(ptr0 + 20) & 0x0f);//4*FRAME_HEAD_LEN+FRAME_LEN_LEN+2=20;
	ss = (*(ptr0 + 21) >> 4)*10 + (*(ptr0 + 21) & 0x0f);//4*FRAME_HEAD_LEN+FRAME_LEN_LEN+3=21;
	ms = (*(ptr0 + 22)*100) + (*(ptr0 + 23) >> 4)*10 + (*(ptr0 + 23) & 0x0f);//5*FRAME_HEAD_LEN+FRAME_LEN_LEN+0/1=22/23;
	us = (*(ptr0 + 24)*100) + (*(ptr0 + 25) >> 4)*10 + (*(ptr0 + 25) & 0x0f);//5*FRAME_HEAD_LEN+FRAME_LEN_LEN+2/3=24/25;
	pcapss = UTC20130101 + 86400*dd + 3600*hh + 60*mm + ss;//24*3600*dd + 3600*hh + 60*mm + ss;
	*(ptr0 + 10) = pcapss & 0x000000ff;
	pcapss >>= 8;
	*(ptr0 + 11) = pcapss & 0x000000ff;
	pcapss >>= 8;
	*(ptr0 + 12) = pcapss & 0x000000ff;
	pcapss >>= 8;
	*(ptr0 + 13) = pcapss & 0x000000ff;
	pcapus = 1000*ms + us;
	*(ptr0 + 14) = pcapus & 0x000000ff;
	pcapus >>= 8;
	*(ptr0 + 15) = pcapus & 0x000000ff;
	pcapus >>= 8;
	*(ptr0 + 16) = pcapus & 0x000000ff;
	pcapus >>= 8;
	*(ptr0 + 17) = pcapus & 0x000000ff;
	pcaplen = FrameLen - 18;//16字节时间+n字节帧+2字节状态
	*(ptr0 + 22) = *(ptr0 + 18) = pcaplen & 0x000000ff;
	pcaplen >>= 8;
	*(ptr0 + 23) = *(ptr0 + 19) = pcaplen & 0x000000ff;
	*(ptr0 + 24) = *(ptr0 + 20) = 0;
	*(ptr0 + 25) = *(ptr0 + 21) = 0;
	//将解出的数据帧传送给通道2的数据缓存器
	if(ReadBuf2Pointer <= WriteBuf2Pointer) {
		if(((WriteBuf2Pointer + PacketLen) == PCIFPGA_BUF_DWORDLEN) && (ReadBuf2Pointer == 0)) {
			Ch2BufFull++;//缓存将满
			//printk("TY-3012S PciDrv WriteCh2Buf full!\n");
			return 0;
		}
		if((WriteBuf2Pointer + PacketLen) <= PCIFPGA_BUF_DWORDLEN) {
			memcpy((ptr2 + 4*WriteBuf2Pointer), (ptr0 + 2), 4*PacketLen);//复制内容:时间+净荷+校验+帧尾
			WriteBuf2Pointer += PacketLen;
			if(WriteBuf2Pointer == PCIFPGA_BUF_DWORDLEN) WriteBuf2Pointer = 0;
			Ch2PacketCnt ++;
			Ch2PacketDw += PacketLen;
			//printk("TY-3012S PciDrv WrBuf2Pointer: %08x!\n", WriteBuf2Pointer);
			return PacketLen;
		}
		if((WriteBuf2Pointer + PacketLen) > PCIFPGA_BUF_DWORDLEN) {
			BufRemainLen = PCIFPGA_BUF_DWORDLEN - WriteBuf2Pointer;
			if((PacketLen - BufRemainLen) >= ReadBuf2Pointer) {//缓存将满
				Ch2BufFull++;
				//printk("TY-3012S PciDrv WriteCh2Buf full!!\n");
				return 0;
			}
			memcpy((ptr2 + 4*WriteBuf2Pointer), (ptr0 + 2), 4*BufRemainLen);
			WriteBuf2Pointer = 0;
			RemLen = PacketLen - BufRemainLen;
			memcpy(ptr2 , (ptr0 + 2 + 4*BufRemainLen), 4*RemLen);
			WriteBuf2Pointer = RemLen;
			Ch2PacketCnt ++;
			Ch2PacketDw += PacketLen;
			//printk("TY-3012S PciDrv WrBuf2Pointer: %08x!!\n", WriteBuf2Pointer);
			return PacketLen;
		}
	} else {
		if((WriteBuf2Pointer + PacketLen) >= ReadBuf2Pointer) {//缓存将满
			Ch2BufFull++;
			//printk("TY-3012S PciDrv WriteCh2Buf full!!!\n");
			return 0;
		} else {
			memcpy((ptr2 + 4*WriteBuf2Pointer), (ptr0 + 2), 4*PacketLen);//复制内容:时间+净荷+校验+帧尾
			WriteBuf2Pointer += PacketLen;
			Ch2PacketCnt ++;
			Ch2PacketDw += PacketLen;
			//printk("TY-3012S PciDrv WrBuf2Pointer: %08x!!!\n", WriteBuf2Pointer);
			return PacketLen;
		}
	}
	return 0;
}

static int ReadFrameBuf1(void)
{
	unsigned int CanReadLen;//目前缓冲区中有效的字节数
	unsigned int *pPacketHead, *pPacketTail;//定义包头和包尾指针
	unsigned int FrameLen, FramePadLen, PacketPadLen;//定义帧长字节,16字节时间+净荷字节+2字节状态,填充字节数0到3,填充的双字数
	unsigned int PacketLen, bufRemLen, remLen, PacketTailTrue;

	//包的变量是以双字(4Byte)为单位,包的最小长度PACKET_MIN_LEN=24个双字=96字节
	//原包:4字节包头+2字节长度+16字节时间+64字节帧+2字节状态+4字节校验+4字节包尾=96字节=24个双字
	//新包:4字节包头+2字节长度+2字节状态+16字节时间+64字节帧+4字节校验+4字节包尾=96字节=24个双字
	//FrameLen=接收到的帧字节数+16字节时间+2字节帧状态
	if(ReadCh1Pointer <= WriteCh1Pointer)
		CanReadLen = WriteCh1Pointer - ReadCh1Pointer;//计算包可能的长度,双字为单位
	else	CanReadLen = PCIFPGA_DMABUF_DWORDLEN - ReadCh1Pointer + WriteCh1Pointer;//双字计数
	if(CanReadLen < PACKET_MIN_LEN) return 0;//包长度小于24个双字,需要等待
	//开始在包中寻找帧,并且将它解出来
	while(CanReadLen >= PACKET_MIN_LEN) {//达到24个双字再开始寻找帧
		if(ReadCh1Pointer < WriteCh1Pointer) {
			pPacketHead = pcifpga_card_dma1 + ReadCh1Pointer;//均为双字变量,包头位置指针
			if(*pPacketHead == PACKET_HEADE) {//找到包头0x1a2b3c4d
				//printk("TY-3012S PciDrv *pPacketHead:  %08x!\n", *pPacketHead);//显示找到包头
				FrameLen = *(pcifpga_card_dma1 + ReadCh1Pointer + PACKET_HEAD_LEN) >> 0x10;//包长2字节,在高位
				//printk("TY-3012S PciDrv FrameLen:      %08x!\n", FrameLen);//显示帧长度
				if(FrameLen > FRAME_MAX_LEN) {//以太网帧长+32字节开销
					printk("TY-3012S PciDrv FrameLenE:     %08d!\n", FrameLen);//显示包长度错误,非常严重
					ReadCh1Pointer++;//帧长太长错误,增加1个双字,继续
					CanReadLen--;
					continue;//继续执行while
				}
				//计算填充字节数:帧头+帧长度+帧时间+帧净荷+帧状态+帧校验+帧尾+填充:是整数双字节
				FramePadLen = (FRAME_LEN_LEN + FrameLen) % 4;//非双字,即多出充字节数量
				if(FramePadLen == 0) PacketPadLen = 0;//无填充,双字的倍数
				else PacketPadLen = 1;//有填充,非双字倍数,1个内双字节填充
				//计算出包的长度,双字数
				PacketLen = PACKET_HEAD_LEN + ((FRAME_LEN_LEN + FrameLen) / 0x04) + PacketPadLen + PACKET_FCS_LEN + PACKET_TAIL_LEN;
				//printk("TY-3012S PciDrv PacketLen:     %08x!\n", PacketLen);//显示包总长度
				if(PacketLen <= CanReadLen) {//包长度符合要求,可以找到包尾
					pPacketTail = pcifpga_card_dma1 + ReadCh1Pointer + PacketLen - PACKET_TAIL_LEN;
					PacketTailTrue = 0;
					switch(FramePadLen) {
						case 0: if(*pPacketTail == PACKET_TAIL0) PacketTailTrue = 1; break;//0xa1b2c3d4
						case 1: if(*pPacketTail == PACKET_TAIL3) PacketTailTrue = 1; break;//0xd4ffffff
						case 2: if(*pPacketTail == PACKET_TAIL2) PacketTailTrue = 1; break;//0xc3d4ffff
						case 3: if(*pPacketTail == PACKET_TAIL1) PacketTailTrue = 1; break;//0xb2c3d4ff
						default: PacketTailTrue = 0; break;
					}
					if(PacketTailTrue == 1) {//找到下一个有效的包尾
						//printk("TY-3012S PciDrv *pPacketTail:  %08x!\n", *pPacketTail);//显示找到包尾
						memcpy((pcifpga_card_buf0 + PACKET_HEAD_LEN), (pcifpga_card_dma1 + ReadCh1Pointer), 4*PacketLen);
						ReadCh1Pointer += PacketLen;
						return PacketLen;
					} else {//没有找到帧尾
						printk("TY-3012S PciDrv *pPacketTailE: %08x!\n", *pPacketTail);//显示包尾错误,非常严重
						ReadCh1Pointer++;//不是包尾
						CanReadLen--;
					}
				} else {
					return 0;//接收的数据太少,需要等待进一步的数据
				}
			} else {
				//printk("TY-3012S PciDrv *pPacketHeadE: %08x!\n", *pPacketHead);//显示发现包头错误,不太严重
				ReadCh1Pointer++;//不是包头,增加1个双字
				CanReadLen--;
			}
		} else {
			pPacketHead = pcifpga_card_dma1 + ReadCh1Pointer;//均为双字变量,包头位置指针
			if(*pPacketHead == PACKET_HEADE) {//找到包头PACKET_HEADE=0x1a2b3c4d
				//printk("TY-3012S PciDrv *pPacketHead:  %08x!!\n", *pPacketHead);//显示找到包头
				if((ReadCh1Pointer + PACKET_HEAD_LEN) == PCIFPGA_DMABUF_DWORDLEN)
					FrameLen = *pcifpga_card_dma1 >> 0x10;//包长2字节,在高位,字节计数
				else	FrameLen = *(pcifpga_card_dma1 + ReadCh1Pointer + PACKET_HEAD_LEN) >> 0x10;//包长2字节,在高位,字节计数
				//printk("TY-3012S PciDrv FrameLen:      %08x!!\n", FrameLen);//显示帧长度
				if(FrameLen > FRAME_MAX_LEN) {//以太网帧长+32字节开销
					printk("TY-3012S PciDrv FrameLenE:     %08d!!\n", FrameLen);//显示帧长度错误,非常严重
					if((ReadCh1Pointer + PACKET_HEAD_LEN) == PCIFPGA_DMABUF_DWORDLEN) ReadCh1Pointer = 0;
					else ReadCh1Pointer++;//帧长太长错误,增加1个双字,继续
					CanReadLen--;
					continue;//继续执行while
				}
				//计算填充字节数:帧头+帧长度+帧时间+帧净荷+帧状态+帧校验+帧尾+填充:是整数双字节
				FramePadLen = (FRAME_LEN_LEN + FrameLen) % 4;//非双字,即多出充字节数量
				if(FramePadLen == 0) PacketPadLen = 0;//无填充,双字的倍数
				else PacketPadLen = 1;//有填充,非双字倍数,1个内双字节填充
				//计算出包的长度,双字数
				PacketLen = PACKET_HEAD_LEN + ((FRAME_LEN_LEN + FrameLen) / 0x04) + PacketPadLen + PACKET_FCS_LEN + PACKET_TAIL_LEN;
				//printk("TY-3012S PciDrv PacketLen:     %08x!!\n", PacketLen);//显示包总长度
				if(PacketLen <= CanReadLen) {//包长度符合要求,可以找到包尾
					if((ReadCh1Pointer + PacketLen) > PCIFPGA_DMABUF_DWORDLEN) {
						pPacketTail = pcifpga_card_dma1 + ReadCh1Pointer + PacketLen - PCIFPGA_DMABUF_DWORDLEN - PACKET_TAIL_LEN;
					} else	{
						pPacketTail = pcifpga_card_dma1 + ReadCh1Pointer + PacketLen - PACKET_TAIL_LEN;
					}
					PacketTailTrue = 0;
					switch(FramePadLen) {
						case 0: if(*pPacketTail == PACKET_TAIL0) PacketTailTrue = 1; break;//0xa1b2c3d4
						case 1: if(*pPacketTail == PACKET_TAIL3) PacketTailTrue = 1; break;//0xd4ffffff
						case 2: if(*pPacketTail == PACKET_TAIL2) PacketTailTrue = 1; break;//0xc3d4ffff
						case 3: if(*pPacketTail == PACKET_TAIL1) PacketTailTrue = 1; break;//0xb2c3d4ff
						default: PacketTailTrue = 0; break;
					}
					if(PacketTailTrue == 1){//找到下一个有效的包尾
						//printk("TY-3012S PciDrv *pPacketTail:  %08x!!\n", *pPacketTail);//显示找到包尾
						if((ReadCh1Pointer + PacketLen) <= PCIFPGA_DMABUF_DWORDLEN) {
							memcpy((pcifpga_card_buf0 + PACKET_HEAD_LEN), (pcifpga_card_dma1 + ReadCh1Pointer), 4*PacketLen);
							ReadCh1Pointer += PacketLen;
							if(ReadCh1Pointer == PCIFPGA_DMABUF_DWORDLEN) ReadCh1Pointer = 0;
							return PacketLen;
						}
						if((ReadCh1Pointer + PacketLen) > PCIFPGA_DMABUF_DWORDLEN) {
							bufRemLen = PCIFPGA_DMABUF_DWORDLEN - ReadCh1Pointer;
							memcpy((pcifpga_card_buf0 + PACKET_HEAD_LEN), (pcifpga_card_dma1 + ReadCh1Pointer), 4*bufRemLen);
							ReadCh1Pointer = 0;
							remLen = PacketLen - bufRemLen;
							memcpy((pcifpga_card_buf0 + PACKET_HEAD_LEN + bufRemLen), pcifpga_card_dma1, 4*remLen);
							ReadCh1Pointer += remLen;
							return PacketLen;
						}
					} else {//对应没有找到下一个包头
						printk("TY-3012S PciDrv *pPacketTailE: %08x!!\n", *pPacketTail);//显示包尾错误,非常严重
						if((ReadCh1Pointer + 1) == PCIFPGA_DMABUF_DWORDLEN) ReadCh1Pointer = 0;
						else ReadCh1Pointer++;//增加1个双字
						CanReadLen--;
					}
				} else {//接收的数据太少,需要等待进一步的数据
					return 0;
				}
			} else {//对应没有找到帧头
				//printk("TY-3012S PciDrv *pPacketHeadE: %08x!!\n", *pPacketHead);//显示发现包头错误,不太严重
				if((ReadCh1Pointer + 1) == PCIFPGA_DMABUF_DWORDLEN) ReadCh1Pointer = 0;
				else ReadCh1Pointer++;
				CanReadLen--;
			}
		}
	}
	return 0;
}

static int ReadFrameBuf2(void)
{
	unsigned int CanReadLen;//目前缓冲区中有效的字节数
	unsigned int *pPacketHead, *pPacketTail;//定义包头和包尾指针
	unsigned int FrameLen, FramePadLen, PacketPadLen;//定义帧长字节,16字节时间+净荷字节+2字节状态,填充字节数0到3,填充的双字数
	unsigned int PacketLen, bufRemLen, remLen, PacketTailTrue;

	//包的变量是以双字(4Byte)为单位,包的最小长度PACKET_MIN_LEN=24个双字=96字节
	//原包:4字节包头+2字节长度+16字节时间+64字节帧+2字节状态+4字节校验+4字节包尾=96字节=24个双字
	//新包:4字节包头+2字节长度+2字节状态+16字节时间+64字节帧+4字节校验+4字节包尾=96字节=24个双字
	//FrameLen=接收到的帧字节数+16字节时间+2字节帧状态
	if(ReadCh2Pointer <= WriteCh2Pointer)
		CanReadLen = WriteCh2Pointer - ReadCh2Pointer;//计算包可能的长度,双字为单位
	else	CanReadLen = PCIFPGA_DMABUF_DWORDLEN - ReadCh2Pointer + WriteCh2Pointer;//双字计数
	if(CanReadLen < PACKET_MIN_LEN) return 0;//包长度小于24个双字,需要等待
	//开始在包中寻找帧,并且将它解出来
	while(CanReadLen >= PACKET_MIN_LEN) {//达到24个双字再开始寻找帧
		if(ReadCh2Pointer < WriteCh2Pointer) {
			pPacketHead = pcifpga_card_dma2 + ReadCh2Pointer;//均为双字变量,包头位置指针
			if(*pPacketHead == PACKET_HEADE) {//找到包头0x1a2b3c4d
				//printk("TY-3012S PciDrv *pPacketHead:  %08x!\n", *pPacketHead);//显示找到包头
				FrameLen = *(pcifpga_card_dma2 + ReadCh2Pointer + PACKET_HEAD_LEN) >> 0x10;//包长2字节,在高位
				//printk("TY-3012S PciDrv FrameLen:      %08x!\n", FrameLen);//显示帧长度
				if(FrameLen > FRAME_MAX_LEN) {//以太网帧长+32字节开销
					printk("TY-3012S PciDrv FrameLenE:     %08d!\n", FrameLen);//显示包长度错误,非常严重
					ReadCh2Pointer++;//帧长太长错误,增加1个双字,继续
					CanReadLen--;
					continue;//继续执行while
				}
				//计算填充字节数:帧头+帧长度+帧时间+帧净荷+帧状态+帧校验+帧尾+填充:是整数双字节
				FramePadLen = (FRAME_LEN_LEN + FrameLen) % 4;//非双字,即多出充字节数量
				if(FramePadLen == 0) PacketPadLen = 0;//无填充,双字的倍数
				else PacketPadLen = 1;//有填充,非双字倍数,1个内双字节填充
				//计算出包的长度,双字数
				PacketLen = PACKET_HEAD_LEN + ((FRAME_LEN_LEN + FrameLen) / 0x04) + PacketPadLen + PACKET_FCS_LEN + PACKET_TAIL_LEN;
				//printk("TY-3012S PciDrv PacketLen:     %08x!\n", PacketLen);//显示包总长度
				if(PacketLen <= CanReadLen) {//包长度符合要求,可以找到包尾
					pPacketTail = pcifpga_card_dma2 + ReadCh2Pointer + PacketLen - PACKET_TAIL_LEN;
					PacketTailTrue = 0;
					switch(FramePadLen) {
						case 0: if(*pPacketTail == PACKET_TAIL0) PacketTailTrue = 1; break;//0xa1b2c3d4
						case 1: if(*pPacketTail == PACKET_TAIL3) PacketTailTrue = 1; break;//0xd4ffffff
						case 2: if(*pPacketTail == PACKET_TAIL2) PacketTailTrue = 1; break;//0xc3d4ffff
						case 3: if(*pPacketTail == PACKET_TAIL1) PacketTailTrue = 1; break;//0xb2c3d4ff
						default: PacketTailTrue = 0; break;
					}
					if(PacketTailTrue == 1) {//找到下一个有效的包尾
						//printk("TY-3012S PciDrv *pPacketTail:  %08x!\n", *pPacketTail);//显示找到包尾
						memcpy((pcifpga_card_buf0 + PACKET_HEAD_LEN), (pcifpga_card_dma2 + ReadCh2Pointer), 4*PacketLen);
						ReadCh2Pointer += PacketLen;
						return PacketLen;
					} else {//没有找到帧尾
						printk("TY-3012S PciDrv *pPacketTailE: %08x!\n", *pPacketTail);//显示包尾错误,非常严重
						ReadCh2Pointer++;//不是包尾
						CanReadLen--;
					}
				} else {
					return 0;//接收的数据太少,需要等待进一步的数据
				}
			} else {
				//printk("TY-3012S PciDrv *pPacketHeadE: %08x!\n", *pPacketHead);//显示发现包头错误,不太严重
				ReadCh2Pointer++;//不是包头,增加1个双字
				CanReadLen--;
			}
		} else {
			pPacketHead = pcifpga_card_dma2 + ReadCh2Pointer;//均为双字变量,包头位置指针
			if(*pPacketHead == PACKET_HEADE) {//找到包头PACKET_HEADE=0x1a2b3c4d
				//printk("TY-3012S PciDrv *pPacketHead:  %08x!!\n", *pPacketHead);//显示找到包头
				if((ReadCh2Pointer + PACKET_HEAD_LEN) == PCIFPGA_DMABUF_DWORDLEN)
					FrameLen = *pcifpga_card_dma2 >> 0x10;//包长2字节,在高位,字节计数
				else	FrameLen = *(pcifpga_card_dma2 + ReadCh2Pointer + PACKET_HEAD_LEN) >> 0x10;//包长2字节,在高位,字节计数
				//printk("TY-3012S PciDrv FrameLen:      %08x!!\n", FrameLen);//显示帧长度
				if(FrameLen > FRAME_MAX_LEN) {//以太网帧长+32字节开销
					printk("TY-3012S PciDrv FrameLenE:     %08d!!\n", FrameLen);//显示帧长度错误,非常严重
					if((ReadCh2Pointer + PACKET_HEAD_LEN) == PCIFPGA_DMABUF_DWORDLEN) ReadCh2Pointer = 0;
					else ReadCh2Pointer++;//帧长太长错误,增加1个双字,继续
					CanReadLen--;
					continue;//继续执行while
				}
				//计算填充字节数:帧头+帧长度+帧时间+帧净荷+帧状态+帧校验+帧尾+填充:是整数双字节
				FramePadLen = (FRAME_LEN_LEN + FrameLen) % 4;//非双字,即多出充字节数量
				if(FramePadLen == 0) PacketPadLen = 0;//无填充,双字的倍数
				else PacketPadLen = 1;//有填充,非双字倍数,1个内双字节填充
				//计算出包的长度,双字数
				PacketLen = PACKET_HEAD_LEN + ((FRAME_LEN_LEN + FrameLen) / 0x04) + PacketPadLen + PACKET_FCS_LEN + PACKET_TAIL_LEN;
				//printk("TY-3012S PciDrv PacketLen:     %08x!!\n", PacketLen);//显示包总长度
				if(PacketLen <= CanReadLen) {//包长度符合要求,可以找到包尾
					if((ReadCh2Pointer + PacketLen) > PCIFPGA_DMABUF_DWORDLEN) {
						pPacketTail = pcifpga_card_dma2 + ReadCh2Pointer + PacketLen - PCIFPGA_DMABUF_DWORDLEN - PACKET_TAIL_LEN;
					} else	{
						pPacketTail = pcifpga_card_dma2 + ReadCh2Pointer + PacketLen - PACKET_TAIL_LEN;
					}
					PacketTailTrue = 0;
					switch(FramePadLen) {
						case 0: if(*pPacketTail == PACKET_TAIL0) PacketTailTrue = 1; break;//0xa1b2c3d4
						case 1: if(*pPacketTail == PACKET_TAIL3) PacketTailTrue = 1; break;//0xd4ffffff
						case 2: if(*pPacketTail == PACKET_TAIL2) PacketTailTrue = 1; break;//0xc3d4ffff
						case 3: if(*pPacketTail == PACKET_TAIL1) PacketTailTrue = 1; break;//0xb2c3d4ff
						default: PacketTailTrue = 0; break;
					}
					if(PacketTailTrue == 1){//找到下一个有效的包尾
						//printk("TY-3012S PciDrv *pPacketTail:  %08x!!\n", *pPacketTail);//显示找到包尾
						if((ReadCh2Pointer + PacketLen) <= PCIFPGA_DMABUF_DWORDLEN) {
							memcpy((pcifpga_card_buf0 + PACKET_HEAD_LEN), (pcifpga_card_dma2 + ReadCh2Pointer), 4*PacketLen);
							ReadCh2Pointer += PacketLen;
							if(ReadCh2Pointer == PCIFPGA_DMABUF_DWORDLEN) ReadCh2Pointer = 0;
							return PacketLen;
						}
						if((ReadCh2Pointer + PacketLen) > PCIFPGA_DMABUF_DWORDLEN) {
							bufRemLen = PCIFPGA_DMABUF_DWORDLEN - ReadCh2Pointer;
							memcpy((pcifpga_card_buf0 + PACKET_HEAD_LEN), (pcifpga_card_dma2 + ReadCh2Pointer), 4*bufRemLen);
							ReadCh2Pointer = 0;
							remLen = PacketLen - bufRemLen;
							memcpy((pcifpga_card_buf0 + PACKET_HEAD_LEN + bufRemLen), pcifpga_card_dma2, 4*remLen);
							ReadCh2Pointer += remLen;
							return PacketLen;
						}
					} else {//对应没有找到下一个包头
						printk("TY-3012S PciDrv *pPacketTailE: %08x!!\n", *pPacketTail);//显示包尾错误,非常严重
						if((ReadCh2Pointer + 1) == PCIFPGA_DMABUF_DWORDLEN) ReadCh2Pointer = 0;
						else ReadCh2Pointer++;//增加1个双字
						CanReadLen--;
					}
				} else {//接收的数据太少,需要等待进一步的数据
					return 0;
				}
			} else {//对应没有找到帧头
				//printk("TY-3012S PciDrv *pPacketHeadE: %08x!!\n", *pPacketHead);//显示发现包头错误,不太严重
				if((ReadCh2Pointer + 1) == PCIFPGA_DMABUF_DWORDLEN) ReadCh2Pointer = 0;
				else ReadCh2Pointer++;
				CanReadLen--;
			}
		}
	}
	return 0;
}

static int ReadFpgaTimer(void)
{
	//从FPGA读入时间数据：天，小时，分钟，秒，毫秒，微秒
	//同时将8421编码转换成二进制编码
	unsigned int tt;

	//锁定时间，以便读取正确的GPS时间
	iowrite32(IRIGB1588E, ChxBaseAddr0 + 4 * IRIGB1588TIMEGET);
	tt = ioread32(ChxBaseAddr0 + 4 * IRIGB1588RDDAY);
	DrvFpgaTime->DD = 0x00;
	DrvFpgaTime->DD = DrvFpgaTime->DD + ((tt & 0x03000000) >> 24) * 100;
	DrvFpgaTime->DD = DrvFpgaTime->DD + ((tt & 0x00f00000) >> 20) * 10;
	DrvFpgaTime->DD = DrvFpgaTime->DD + ((tt & 0x000f0000) >> 16);
	DrvFpgaTime->HH = 0x00;
	DrvFpgaTime->HH = DrvFpgaTime->HH + ((tt & 0x00003000) >> 12) * 10;
	DrvFpgaTime->HH = DrvFpgaTime->HH + ((tt & 0x00000f00) >>  8);
	DrvFpgaTime->MM = 0x00;
	DrvFpgaTime->MM = DrvFpgaTime->MM + ((tt & 0x000000f0) >>  4) * 10;
	DrvFpgaTime->MM = DrvFpgaTime->MM + ((tt & 0x0000000f) >>  0);
	tt = ioread32(ChxBaseAddr0 + 4 * IRIGB1588RDSECOND);
	DrvFpgaTime->SS = 0x00;
	DrvFpgaTime->SS = DrvFpgaTime->SS + ((tt & 0x70000000) >> 28) * 10;
	DrvFpgaTime->SS = DrvFpgaTime->SS + ((tt & 0x0f000000) >> 24);
	DrvFpgaTime->MS = 0x00;
	DrvFpgaTime->MS = DrvFpgaTime->MS + ((tt & 0x00f00000) >> 20) * 100;
	DrvFpgaTime->MS = DrvFpgaTime->MS + ((tt & 0x000f0000) >> 16) * 10;
	DrvFpgaTime->MS = DrvFpgaTime->MS + ((tt & 0x0000f000) >> 12);
	DrvFpgaTime->US = DrvFpgaTime->US + ((tt & 0x00000f00) >>  8) * 100;
	DrvFpgaTime->US = DrvFpgaTime->US + ((tt & 0x000000f0) >>  4) * 10;
	DrvFpgaTime->US = DrvFpgaTime->US + ((tt & 0x0000000f) >>  0);
	//读取秒偏差状态和值
	tt = ioread32(ChxBaseAddr0 + 4 * IRIGB1588TIMEERR);
	if((tt & 0x01000000) == 0x01000000) DrvFpgaTime->ES = 1;
	else DrvFpgaTime->ES = 0;
	DrvFpgaTime->EV = tt & 0x0ffffff;
	//读取GPS状态
	DrvFpgaTime->PS = 0;
	DrvFpgaTime->GS = 0;
	tt = ioread32(ChxBaseAddr0 + 4 * IRIGB1588STATUS);
	if((tt & 0x08) == 0x08) DrvFpgaTime->PS = 1;
	if((tt & 0x04) == 0x04) DrvFpgaTime->PS = 0;
	if((tt & 0x02) == 0x02) DrvFpgaTime->GS = 1;
	if((tt & 0x01) == 0x01) DrvFpgaTime->GS = 0;
	return MSG_BUF_LEN;
}

static int WriteFpgaTimer(void)
{
	//向FPGA写入时间数据：天，小时，分钟，秒，毫秒，微秒
	//同时将二进制编码编码转换成8421编码
	unsigned int t;
	int i, j, k;

	//计算天、小时、秒、毫秒、微妙
	i = DrvFpgaTime->DD / 100;//天的百位
	j = (DrvFpgaTime->DD - (i * 100)) / 10;//天的十位
	k = DrvFpgaTime->DD % 10;//天的个位
	t = (i << 24) + (j << 20) + (k << 16);//天的8421编码
	//计算时
	i = DrvFpgaTime->HH / 10;//时的十位
	j = DrvFpgaTime->HH % 10;//时的个位
	t = t + (i << 12) + (j << 8);//天、时的8421编码
	//计算分
	i = DrvFpgaTime->MM / 10;//秒的十位
	j = DrvFpgaTime->MM % 10;//秒的个位
	t = t + (i << 4) + j;//天、时、分的8421编码
	iowrite32(t, ChxBaseAddr0 + 4 * IRIGB1588WRDAY);
	//计算秒
	i = DrvFpgaTime->SS / 10;//秒的十位
	j = DrvFpgaTime->SS % 10;//秒的个位
	t = (i << 28) + (j << 24);//秒的8421编码
	//计算毫秒
	i = DrvFpgaTime->MS / 100;//毫秒的百位
	j = (DrvFpgaTime->MS - (i * 100)) / 10;//毫秒的十位
	k = DrvFpgaTime->MS % 10;//毫秒的个位
	t = t + (i << 20) + (j << 16) + (k << 12);//秒、毫秒的8421编码
	//计算微妙
	i = DrvFpgaTime->US / 100;//微妙的百位
	j = (DrvFpgaTime->US - (i * 100)) / 10;//微妙的十位
	k = DrvFpgaTime->US % 10;//微妙的个位
	t = t + (i << 8) + (j << 4) + k;//秒、毫秒、微妙的8421编码
	iowrite32(t, ChxBaseAddr0 + 4 * IRIGB1588WRSECOND);
	iowrite32(IRIGB1588E, ChxBaseAddr0 + 4 * IRIGB1588TIMEPUT);
	return MSG_BUF_LEN;
}

static int DisplayFpgaTimer(void)
{
	unsigned int tt, dd, hh, mm, ss, ms, us;
	unsigned int ErrStatus, ErrValue, PtpStatus, GpsStatus = 0;

	//显示时间
	iowrite32(IRIGB1588E, ChxBaseAddr0 + 4 * IRIGB1588TIMEGET);
	tt = ioread32(ChxBaseAddr0 + 4 * IRIGB1588RDDAY);
	dd = 0x00;
	dd = dd + ((tt & 0x03000000) >> 24) * 100;
	dd = dd + ((tt & 0x00f00000) >> 20) * 10;
	dd = dd + ((tt & 0x000f0000) >> 16);
	printk("\nTY-3012S PciDrv Fpga Day:      %08d!\n", dd);
	hh = 0x00;
	hh = hh + ((tt & 0x00003000) >> 12) * 10;
	hh = hh + ((tt & 0x00000f00) >>  8);
	mm = 0x00;
	mm = mm + ((tt & 0x000000f0) >>  4) * 10;
	mm = mm + ((tt & 0x0000000f) >>  0);
	tt = ioread32(ChxBaseAddr0 + 4 * IRIGB1588RDSECOND);
	ss = 0x00;
	ss = ss + ((tt & 0x70000000) >> 28) * 10;
	ss = ss + ((tt & 0x0f000000) >> 24);
	printk("TY-3012S PciDrv Fpga Time:     %02d:%02d:%02d!\n", hh, mm, ss);
	ms = 0x00;
	ms = ms + ((tt & 0x00f00000) >> 20) * 100;
	ms = ms + ((tt & 0x000f0000) >> 16) * 10;
	ms = ms + ((tt & 0x0000f000) >> 12);
	printk("TY-3012S PciDrv Millisecond:   %08d!\n", ms);
	us = 0x00;
	us = us + ((tt & 0x00000f00) >>  8) * 100;
	us = us + ((tt & 0x000000f0) >>  4) * 10;
	us = us + ((tt & 0x0000000f) >>  0);
	printk("TY-3012S PciDrv Microsecond:   %08d!\n", us);
	//读取秒偏差状态和值
	//D24=0本地时钟低于GPS,=1高于GPS,D23～D0本地与外时钟每秒偏差数,10MHz单位
	tt = ioread32(ChxBaseAddr0 + 4 * IRIGB1588TIMEERR);
	if((tt & 0x01000000) == 0x01000000) ErrStatus = 1;
	else ErrStatus = 0;
	ErrValue = tt & 0x0ffffff;
	printk("TY-3012S PciDrv TimeErrStatus: %08d!\n", ErrStatus);
	printk("TY-3012S PciDrv TimeErrValue:  %08d!\n", ErrValue);
	//读取GPS状态
	//D3～0对应1588正常,1588时钟丢失,GPS正常,GPS时钟丢失,高有效
        PtpStatus = 0;
        GpsStatus = 0;
	tt = ioread32(ChxBaseAddr0 + 4 * IRIGB1588STATUS);
	printk("TY-3012S PciDrv PtpGpsStatus:  %08d!\n\n", tt);
	if((tt & 0x08) == 0x08) PtpStatus = 1;
	if((tt & 0x04) == 0x04) PtpStatus = 0;
	if((tt & 0x02) == 0x02) GpsStatus = 1;
	if((tt & 0x01) == 0x01) GpsStatus = 0;
	//printk("TY-3012S PciDrv GpsStatus:     %08d!\n", GpsStatus);
	//printk("TY-3012S PciDrv PtpStatus:     %08d!\n", PtpStatus);
	return 0;
}

static int ReadChFilter(unsigned int ch)
{
	//读取过滤寄存器
	if(ch == 1) {
		DrvChFilter->Reset        = ioread32(Ch1BaseAddr1 + 4 * CHFILTERRESET);
		DrvChFilter->Mode         = ioread32(Ch1BaseAddr1 + 4 * CHFILTERMODE);
		DrvChFilter->MinLength    = ioread32(Ch1BaseAddr1 + 4 * CHFILTERMINLEN);
		DrvChFilter->MaxLength    = ioread32(Ch1BaseAddr1 + 4 * CHFILTERMAXLEN);
		DrvChFilter->FilterEnable = ioread32(Ch1BaseAddr1 + 4 * CHFILTERENABLE);
		DrvChFilter->Vlan1Id      = ioread32(Ch1BaseAddr1 + 4 * CHFILTERVID1);
		DrvChFilter->Vlan2Id      = ioread32(Ch1BaseAddr1 + 4 * CHFILTERVID2);
		DrvChFilter->MacEnable    = ioread32(Ch1BaseAddr1 + 4 * CHFILTERMACEN);
	}
	if(ch == 2) {
		DrvChFilter->Reset        = ioread32(Ch2BaseAddr2 + 4 * CHFILTERRESET);
		DrvChFilter->Mode         = ioread32(Ch2BaseAddr2 + 4 * CHFILTERMODE);
		DrvChFilter->MinLength    = ioread32(Ch2BaseAddr2 + 4 * CHFILTERMINLEN);
		DrvChFilter->MaxLength    = ioread32(Ch2BaseAddr2 + 4 * CHFILTERMAXLEN);
		DrvChFilter->FilterEnable = ioread32(Ch2BaseAddr2 + 4 * CHFILTERENABLE);
		DrvChFilter->Vlan1Id      = ioread32(Ch2BaseAddr2 + 4 * CHFILTERVID1);
		DrvChFilter->Vlan2Id      = ioread32(Ch2BaseAddr2 + 4 * CHFILTERVID2);
		DrvChFilter->MacEnable    = ioread32(Ch2BaseAddr2 + 4 * CHFILTERMACEN);
	}
	return MSG_BUF_LEN;
}

static int WriteChFilter(unsigned int ch)
{
	//写入过滤寄存器
	if(ch == 1) {
		iowrite32(DrvChFilter->Reset,        Ch1BaseAddr1 + 4 * CHFILTERRESET);
		iowrite32(DrvChFilter->Mode,         Ch1BaseAddr1 + 4 * CHFILTERMODE);
		iowrite32(DrvChFilter->MinLength,    Ch1BaseAddr1 + 4 * CHFILTERMINLEN);
		iowrite32(DrvChFilter->MaxLength,    Ch1BaseAddr1 + 4 * CHFILTERMAXLEN);
		iowrite32(DrvChFilter->FilterEnable, Ch1BaseAddr1 + 4 * CHFILTERENABLE);
		iowrite32(DrvChFilter->Vlan1Id,      Ch1BaseAddr1 + 4 * CHFILTERVID1);
		iowrite32(DrvChFilter->Vlan2Id,      Ch1BaseAddr1 + 4 * CHFILTERVID2);
		iowrite32(DrvChFilter->MacEnable,    Ch1BaseAddr1 + 4 * CHFILTERMACEN);
	}
	if(ch == 2) {
		iowrite32(DrvChFilter->Reset,        Ch2BaseAddr2 + 4 * CHFILTERRESET);
		iowrite32(DrvChFilter->Mode,         Ch2BaseAddr2 + 4 * CHFILTERMODE);
		iowrite32(DrvChFilter->MinLength,    Ch2BaseAddr2 + 4 * CHFILTERMINLEN);
		iowrite32(DrvChFilter->MaxLength,    Ch2BaseAddr2 + 4 * CHFILTERMAXLEN);
		iowrite32(DrvChFilter->FilterEnable, Ch2BaseAddr2 + 4 * CHFILTERENABLE);
		iowrite32(DrvChFilter->Vlan1Id,      Ch2BaseAddr2 + 4 * CHFILTERVID1);
		iowrite32(DrvChFilter->Vlan2Id,      Ch2BaseAddr2 + 4 * CHFILTERVID2);
		iowrite32(DrvChFilter->MacEnable,    Ch2BaseAddr2 + 4 * CHFILTERMACEN);
	}
	return MSG_BUF_LEN;
}

static int ReadFilterMac(unsigned int ch, unsigned int group)
{
	unsigned int i;
	//写入MAC过滤寄存器
	if((group >= 1) && (group <= 4)) i = group - 1;
	else return 0;
	if(ch == 1) {
		DrvFilterMac->MacAddr1H = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACH + i*4 + 0));
		DrvFilterMac->MacAddr1L = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACL + i*4 + 0));
		DrvFilterMac->MacAddr2H = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACH + i*4 + 1));
		DrvFilterMac->MacAddr2L = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACL + i*4 + 1));
		DrvFilterMac->MacAddr3H = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACH + i*4 + 2));
		DrvFilterMac->MacAddr3L = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACL + i*4 + 2));
		DrvFilterMac->MacAddr4H = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACH + i*4 + 3));
		DrvFilterMac->MacAddr4L = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACL + i*4 + 3));
	}
	if(ch == 2) {
		DrvFilterMac->MacAddr1H = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACH + i*4 + 0));
		DrvFilterMac->MacAddr1L = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACL + i*4 + 0));
		DrvFilterMac->MacAddr2H = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACH + i*4 + 1));
		DrvFilterMac->MacAddr2L = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACL + i*4 + 1));
		DrvFilterMac->MacAddr3H = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACH + i*4 + 2));
		DrvFilterMac->MacAddr3L = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACL + i*4 + 2));
		DrvFilterMac->MacAddr4H = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACH + i*4 + 3));
		DrvFilterMac->MacAddr4L = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACL + i*4 + 3));
	}
	return MSG_BUF_LEN;
}

static int WriteFilterMac(unsigned int ch, unsigned int group)
{
	unsigned int i;
	//写入MAC过滤寄存器
	if((group >= 1) && (group <= 4)) i = group - 1;
	else return 0;
	if(ch == 1) {
		iowrite32(DrvFilterMac->MacAddr1H, Ch1BaseAddr1 + 4*(CHFILTERMACH + i*4 + 0));
		iowrite32(DrvFilterMac->MacAddr1L, Ch1BaseAddr1 + 4*(CHFILTERMACL + i*4 + 0));
		iowrite32(DrvFilterMac->MacAddr2H, Ch1BaseAddr1 + 4*(CHFILTERMACH + i*4 + 1));
		iowrite32(DrvFilterMac->MacAddr2L, Ch1BaseAddr1 + 4*(CHFILTERMACL + i*4 + 1));
		iowrite32(DrvFilterMac->MacAddr3H, Ch1BaseAddr1 + 4*(CHFILTERMACH + i*4 + 2));
		iowrite32(DrvFilterMac->MacAddr3L, Ch1BaseAddr1 + 4*(CHFILTERMACL + i*4 + 2));
		iowrite32(DrvFilterMac->MacAddr4H, Ch1BaseAddr1 + 4*(CHFILTERMACH + i*4 + 3));
		iowrite32(DrvFilterMac->MacAddr4L, Ch1BaseAddr1 + 4*(CHFILTERMACL + i*4 + 3));
	}
	if(ch == 2) {
		iowrite32(DrvFilterMac->MacAddr1H, Ch2BaseAddr2 + 4*(CHFILTERMACH + i*4 + 0));
		iowrite32(DrvFilterMac->MacAddr1L, Ch2BaseAddr2 + 4*(CHFILTERMACL + i*4 + 0));
		iowrite32(DrvFilterMac->MacAddr2H, Ch2BaseAddr2 + 4*(CHFILTERMACH + i*4 + 1));
		iowrite32(DrvFilterMac->MacAddr2L, Ch2BaseAddr2 + 4*(CHFILTERMACL + i*4 + 1));
		iowrite32(DrvFilterMac->MacAddr3H, Ch2BaseAddr2 + 4*(CHFILTERMACH + i*4 + 2));
		iowrite32(DrvFilterMac->MacAddr3L, Ch2BaseAddr2 + 4*(CHFILTERMACL + i*4 + 2));
		iowrite32(DrvFilterMac->MacAddr4H, Ch2BaseAddr2 + 4*(CHFILTERMACH + i*4 + 3));
		iowrite32(DrvFilterMac->MacAddr4L, Ch2BaseAddr2 + 4*(CHFILTERMACL + i*4 + 3));
	}
	return MSG_BUF_LEN;
}

static int ReadChCounter(unsigned int ch)
{
	if(ch == 1) {
		DrvFrameCounters->AllFrame	 = ioread32(Ch1BaseAddr1 + 4 * CHALLFRAMEL);
		DrvFrameCounters->AllByte        = ioread32(Ch1BaseAddr1 + 4 * CHALLBYTEL);
		DrvFrameCounters->BroadcastFrame = ioread32(Ch1BaseAddr1 + 4 * CHBROADCASTFRAMEL);
		DrvFrameCounters->BroadcastByte  = ioread32(Ch1BaseAddr1 + 4 * CHBROADCASTBYTEL);
		DrvFrameCounters->MulticastFrame = ioread32(Ch1BaseAddr1 + 4 * CHMULTCASTFRAMEL);
		DrvFrameCounters->MulticastByte  = ioread32(Ch1BaseAddr1 + 4 * CHMULTCASTBYTEL);
		DrvFrameCounters->CrcErrFrame    = ioread32(Ch1BaseAddr1 + 4 * CHCRCERRFRAMEL);
		DrvFrameCounters->AllignErrFrame = ioread32(Ch1BaseAddr1 + 4 * CHERRORFRAMEL);
		DrvFrameCounters->ShortFrame     = ioread32(Ch1BaseAddr1 + 4 * CHALLIGNERRFRAMEL);
		DrvFrameCounters->LongFrame      = ioread32(Ch1BaseAddr1 + 4 * CHSHORTFRAMEL);
		DrvFrameCounters->ErrFrame       = ioread32(Ch1BaseAddr1 + 4 * CHLONGFRAMEL);
	}
	if(ch == 2) {
		DrvFrameCounters->AllFrame	 = ioread32(Ch2BaseAddr2 + 4 * CHALLFRAMEL);
		DrvFrameCounters->AllByte        = ioread32(Ch2BaseAddr2 + 4 * CHALLBYTEL);
		DrvFrameCounters->BroadcastFrame = ioread32(Ch2BaseAddr2 + 4 * CHBROADCASTFRAMEL);
		DrvFrameCounters->BroadcastByte  = ioread32(Ch2BaseAddr2 + 4 * CHBROADCASTBYTEL);
		DrvFrameCounters->MulticastFrame = ioread32(Ch2BaseAddr2 + 4 * CHMULTCASTFRAMEL);
		DrvFrameCounters->MulticastByte  = ioread32(Ch2BaseAddr2 + 4 * CHMULTCASTBYTEL);
		DrvFrameCounters->CrcErrFrame    = ioread32(Ch2BaseAddr2 + 4 * CHCRCERRFRAMEL);
		DrvFrameCounters->AllignErrFrame = ioread32(Ch2BaseAddr2 + 4 * CHERRORFRAMEL);
		DrvFrameCounters->ShortFrame     = ioread32(Ch2BaseAddr2 + 4 * CHALLIGNERRFRAMEL);
		DrvFrameCounters->LongFrame      = ioread32(Ch2BaseAddr2 + 4 * CHSHORTFRAMEL);
		DrvFrameCounters->ErrFrame       = ioread32(Ch2BaseAddr2 + 4 * CHLONGFRAMEL);
	}
	return MSG_BUF_LEN;
}

/* 设备打开函数，规范结构 */
static int pcifpga_open(struct inode *inode, struct file *filp)  
{
	//printk("TY-3012S PciDrv pcifpga_open opend!\n");
	return 0;
}

/* 设备读数函数，规范结构 */
static ssize_t pcifpga_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	int len = 0;

	if(copy_from_user(MsgBuffer, buff, MSG_BUF_LEN)) {//读出应用程序传来的消息命令和数据，最长64字节
		printk("TY-3012S PciDrv copy_from_user error!\n");
		return -1;
	}
	switch(DrvMsgCommand->Id) {
		case READ_CH_BUFFER://0x11、读取录波缓冲FIFO中的数据，每次最大2M字节
			#if 0
			printk("TY-3012S PciDrv DrvReadBuffer->Head: %08x!\n",  DrvReadBuffer->Head);
			printk("TY-3012S PciDrv DrvReadBuffer->Sid:  %08x!\n",  DrvReadBuffer->VersionSid);
			printk("TY-3012S PciDrv DrvReadBuffer->Id:   %08x!\n",  DrvReadBuffer->Id);
			printk("TY-3012S PciDrv DrvReadBuffer->ChNo: %08x!\n",  DrvReadBuffer->ChNo);
			printk("TY-3012S PciDrv DrvReadBuffer->Byte: %08x!\n",  DrvReadBuffer->Byte);
			printk("TY-3012S PciDrv DrvReadBuffer->Tail: %08x!\n",  DrvReadBuffer->Tail);
			#endif
			if(DrvReadBuffer->ChNo == 1) {
				len = ReadCh1Buf((unsigned int *)buff, count);//读通道1缓冲FIFO1的数据
			} else {
				if(DrvReadBuffer->ChNo == 2) {
					len = ReadCh2Buf((unsigned int *)buff, count);//读通道2缓冲FIFO2的数据
				} else {//通道错误
					len = 0;
					printk("TY-3012S PciDrv READ_CH_BUFFER error %x!\n", DrvReadBuffer->ChNo);
				}
			}
			return len;
			break;
		case CHECK_FPGA_TIMER://0x14、检查FPGA时间，年、月、日、时、分、秒、毫秒、微妙
			#if 1
			printk("TY-3012S PciDrv DrvFpgaTime->Head:   %08x!\n",  DrvFpgaTime->Head);
			printk("TY-3012S PciDrv DrvFpgaTime->Sid:    %08x!\n",  DrvFpgaTime->VersionSid);
			printk("TY-3012S PciDrv DrvFpgaTime->Id:     %08x!\n",  DrvFpgaTime->Id);
			printk("TY-3012S PciDrv DrvFpgaTime->Tail:   %08x!\n",  DrvFpgaTime->Tail);
			#endif
			len = ReadFpgaTimer();
			DrvFpgaTime->Id = ACK_FPGA_TIMER;
			#if 1
			printk("TY-3012S PciDrv DrvFpgaTime->Head:   %08x!!\n", DrvFpgaTime->Head);
			printk("TY-3012S PciDrv DrvFpgaTime->Sid:    %08x!!\n", DrvFpgaTime->VersionSid);
			printk("TY-3012S PciDrv DrvFpgaTime->Id:     %08x!!\n", DrvFpgaTime->Id);
			printk("TY-3012S PciDrv DrvFpgaTime->DD:     %08x!!\n", DrvFpgaTime->DD);
			printk("TY-3012S PciDrv DrvFpgaTime->HH:     %08x!!\n", DrvFpgaTime->HH);
			printk("TY-3012S PciDrv DrvFpgaTime->MM:     %08x!!\n", DrvFpgaTime->MM);
			printk("TY-3012S PciDrv DrvFpgaTime->SS:     %08x!!\n", DrvFpgaTime->SS);
			printk("TY-3012S PciDrv DrvFpgaTime->MS:     %08x!!\n", DrvFpgaTime->MS);
			printk("TY-3012S PciDrv DrvFpgaTime->US:     %08x!!\n", DrvFpgaTime->US);
			printk("TY-3012S PciDrv DrvFpgaTime->GS:     %08x!!\n", DrvFpgaTime->GS);
			printk("TY-3012S PciDrv DrvFpgaTime->PS:     %08x!!\n", DrvFpgaTime->PS);
			printk("TY-3012S PciDrv DrvFpgaTime->ES:     %08x!!\n", DrvFpgaTime->ES);
			printk("TY-3012S PciDrv DrvFpgaTime->EV:     %08x!!\n", DrvFpgaTime->EV);
			printk("TY-3012S PciDrv DrvFpgaTime->Tail:   %08x!!\n", DrvFpgaTime->Tail);
			#endif
			break;
		case CHECK_CH_FILTER://0x17、检查监测通道的过滤条件
			#if 1
			printk("TY-3012S PciDrv DrvChFilter->Head:   %08x!\n",  DrvChFilter->Head);
			printk("TY-3012S PciDrv DrvChFilter->Sid:    %08x!\n",  DrvChFilter->VersionSid);
			printk("TY-3012S PciDrv DrvChFilter->Id:     %08x!\n",  DrvChFilter->Id);
			printk("TY-3012S PciDrv DrvChFilter->ChNo:   %08x!\n",  DrvChFilter->ChNo);
			printk("TY-3012S PciDrv DrvChFilter->Tail:   %08x!\n",  DrvChFilter->Tail);
			#endif
			if(DrvChFilter->ChNo == 1) len = ReadChFilter(1);//读通道1的过滤条件
			else if(DrvChFilter->ChNo == 2) len = ReadChFilter(2);//读通道2的过滤条件
				else {//通道错误
					DrvChFilter->ChNo = 0;
					DrvChFilter->Reset = 0;
					DrvChFilter->Mode = 0;
					DrvChFilter->MinLength = 0;
					DrvChFilter->MaxLength = 0;
					DrvChFilter->FilterEnable = 0;
					DrvChFilter->Vlan1Id = 0;
					DrvChFilter->Vlan2Id = 0;
					DrvChFilter->MacEnable = 0;
					len = 0;
					printk("TY-3012S PciDrv CHECK_CH_FILTER error %x!\n", DrvChFilter->ChNo);
				}
			DrvChFilter->Id = ACK_CH_FILTER;
			#if 1
			printk("TY-3012S PciDrv DrvChFilter->Head:   %08x!!\n", DrvChFilter->Head);
			printk("TY-3012S PciDrv DrvChFilter->Sid:    %08x!!\n", DrvChFilter->VersionSid);
			printk("TY-3012S PciDrv DrvChFilter->Id:     %08x!!\n", DrvChFilter->Id);
			printk("TY-3012S PciDrv DrvChFilter->ChNo:   %08x!!\n", DrvChFilter->ChNo);
			printk("TY-3012S PciDrv DrvChFilter->Reset:  %08x!!\n", DrvChFilter->Reset);
			printk("TY-3012S PciDrv DrvChFilter->Mode:   %08x!!\n", DrvChFilter->Mode);
			printk("TY-3012S PciDrv DrvChFilter->MinLen: %08x!!\n", DrvChFilter->MinLength);
			printk("TY-3012S PciDrv DrvChFilter->MaxLen: %08x!!\n", DrvChFilter->MaxLength);
			printk("TY-3012S PciDrv DrvChFilter->FilEn:  %08x!!\n", DrvChFilter->FilterEnable);
			printk("TY-3012S PciDrv DrvChFilter->Vlan1:  %08x!!\n", DrvChFilter->Vlan1Id);
			printk("TY-3012S PciDrv DrvChFilter->Vlan2:  %08x!!\n", DrvChFilter->Vlan2Id);
			printk("TY-3012S PciDrv DrvChFilter->MacEn:  %08x!!\n", DrvChFilter->MacEnable);
			printk("TY-3012S PciDrv DrvChFilter->Tail:   %08x!!\n", DrvChFilter->Tail);
			#endif
			break;
		case CHECK_FILTER_MAC://0x18、检查监测通道的源MAC过滤地址，一种MAC过滤条件
			#if 1
			printk("TY-3012S PciDrv DrvFilterMac->Head:  %08x!\n",  DrvFilterMac->Head);
			printk("TY-3012S PciDrv DrvFilterMac->Sid:   %08x!\n",  DrvFilterMac->VersionSid);
			printk("TY-3012S PciDrv DrvFilterMac->Id:    %08x!\n",  DrvFilterMac->Id);
			printk("TY-3012S PciDrv DrvFilterMac->ChNo:  %08x!\n",  DrvFilterMac->ChNo);
			printk("TY-3012S PciDrv DrvFilterMac->Group: %08x!\n",  DrvFilterMac->GroupNo);
			printk("TY-3012S PciDrv DrvFilterMac->Tail:  %08x!\n",  DrvFilterMac->Tail);
			#endif
			DrvFilterMac->ChNo = 0;
			DrvFilterMac->GroupNo = 0;
			DrvFilterMac->MacAddr1L = 0;
			DrvFilterMac->MacAddr1H = 0;
			DrvFilterMac->MacAddr2L = 0;
			DrvFilterMac->MacAddr2H = 0;
			DrvFilterMac->MacAddr3L = 0;
			DrvFilterMac->MacAddr3H = 0;
			DrvFilterMac->MacAddr4L = 0;
			DrvFilterMac->MacAddr4H = 0;
			len = 0;
			if((DrvFilterMac->ChNo == 1) || (DrvFilterMac->ChNo == 2)) {
				len = ReadFilterMac(DrvFilterMac->ChNo, DrvFilterMac->GroupNo);//读通道1或2的过滤MAC
				if(len == 0)
					printk("TY-3012S PciDrv CHECK_FILTER_MAC error %x!\n", DrvFilterMac->GroupNo);
			} else//通道错误
				printk("TY-3012S PciDrv CHECK_FILTER_MAC error %x!!\n", DrvFilterMac->ChNo);
			DrvFilterMac->Id = ACK_FILTER_MAC;
			#if 1
			printk("TY-3012S PciDrv DrvFilterMac->Head:  %08x!!\n", DrvFilterMac->Head);
			printk("TY-3012S PciDrv DrvFilterMac->Sid:   %08x!!\n", DrvFilterMac->VersionSid);
			printk("TY-3012S PciDrv DrvFilterMac->Id:    %08x!!\n", DrvFilterMac->Id);
			printk("TY-3012S PciDrv DrvFilterMac->ChNo:  %08x!!\n", DrvFilterMac->ChNo);
			printk("TY-3012S PciDrv DrvFilterMac->Group: %08x!!\n", DrvFilterMac->GroupNo);
			printk("TY-3012S PciDrv DrvFilterMac->Mac1L: %08x!!\n", DrvFilterMac->MacAddr1L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac1H: %08x!!\n", DrvFilterMac->MacAddr1H);
			printk("TY-3012S PciDrv DrvFilterMac->Mac2L: %08x!!\n", DrvFilterMac->MacAddr2L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac2H: %08x!!\n", DrvFilterMac->MacAddr2H);
			printk("TY-3012S PciDrv DrvFilterMac->Mac3L: %08x!!\n", DrvFilterMac->MacAddr3L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac3H: %08x!!\n", DrvFilterMac->MacAddr3H);
			printk("TY-3012S PciDrv DrvFilterMac->Mac4L: %08x!!\n", DrvFilterMac->MacAddr4L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac4H: %08x!!\n", DrvFilterMac->MacAddr4H);
			printk("TY-3012S PciDrv DrvFilterMac->Tail:  %08x!!\n", DrvFilterMac->Tail);
			#endif
			break;
		case CHECK_FRAME_COUNTERS://0x19、检查监测通道帧和字节计数器的统计数据
			#if 1
			printk("TY-3012S PciDrv DrvFrameCnts->Head:  %08x!\n",  DrvFrameCounters->Head);
			printk("TY-3012S PciDrv DrvFrameCnts->Sid:   %08x!\n",  DrvFrameCounters->VersionSid);
			printk("TY-3012S PciDrv DrvFrameCnts->Id:    %08x!\n",  DrvFrameCounters->Id);
			printk("TY-3012S PciDrv DrvFrameCnts->ChNo:  %08x!\n",  DrvFrameCounters->ChNo);
			printk("TY-3012S PciDrv DrvFrameCnts->Tail:  %08x!\n",  DrvFrameCounters->Tail);
			#endif
			if((DrvFrameCounters->ChNo == 1) || (DrvFrameCounters->ChNo == 2)) {
				len = ReadChCounter(DrvFrameCounters->ChNo);//读通道1和2的帧和字节计数器
			} else {//通道错误
				DrvFrameCounters->ChNo = 0;
				DrvFrameCounters->AllFrame = 0;
				DrvFrameCounters->AllByte = 0;
				DrvFrameCounters->BroadcastFrame = 0;
				DrvFrameCounters->BroadcastByte = 0;
				DrvFrameCounters->MulticastFrame = 0;
				DrvFrameCounters->MulticastByte = 0;
				DrvFrameCounters->CrcErrFrame = 0;
				DrvFrameCounters->ErrFrame = 0;
				DrvFrameCounters->AllignErrFrame = 0;
				DrvFrameCounters->ShortFrame = 0;
				DrvFrameCounters->LongFrame = 0;
				len = 0;
				printk("TY-3012S PciDrv CHECK_FRAME_COUNTERS error %x!\n", DrvFrameCounters->ChNo);
			}
			DrvFrameCounters->Id = ACK_FRAME_COUNTERS;
			#if 1
			printk("TY-3012S PciDrv DrvFrameCnts->Head:  %08x!!\n", DrvFrameCounters->Head);
			printk("TY-3012S PciDrv DrvFrameCnts->Sid:   %08x!!\n", DrvFrameCounters->VersionSid);
			printk("TY-3012S PciDrv DrvFrameCnts->Id:    %08x!!\n", DrvFrameCounters->Id);
			printk("TY-3012S PciDrv DrvFrameCnts->ChNo:  %08x!!\n", DrvFrameCounters->ChNo);
			printk("TY-3012S PciDrv DrvFrameCnts->AllF:  %08x!!\n", DrvFrameCounters->AllByte);
			printk("TY-3012S PciDrv DrvFrameCnts->AllB:  %08x!!\n", DrvFrameCounters->BroadcastFrame);
			printk("TY-3012S PciDrv DrvFrameCnts->BroaF: %08x!!\n", DrvFrameCounters->BroadcastByte);
			printk("TY-3012S PciDrv DrvFrameCnts->BroaB: %08x!!\n", DrvFrameCounters->MulticastFrame);
			printk("TY-3012S PciDrv DrvFrameCnts->MultF: %08x!!\n", DrvFrameCounters->MulticastByte);
			printk("TY-3012S PciDrv DrvFrameCnts->MultB: %08x!!\n", DrvFrameCounters->CrcErrFrame);
			printk("TY-3012S PciDrv DrvFrameCnts->CrcF:  %08x!!\n", DrvFrameCounters->ErrFrame);
			printk("TY-3012S PciDrv DrvFrameCnts->ErrF:  %08x!!\n", DrvFrameCounters->AllignErrFrame);
			printk("TY-3012S PciDrv DrvFrameCnts->Allig: %08x!!\n", DrvFrameCounters->ShortFrame);
			printk("TY-3012S PciDrv DrvFrameCnts->Short: %08x!!\n", DrvFrameCounters->LongFrame);
			printk("TY-3012S PciDrv DrvFrameCnts->Long:  %08x!!\n", DrvFrameCounters->Tail);			
			printk("TY-3012S PciDrv DrvFrameCnts->Tail:  %08x!!\n", DrvFrameCounters->Tail);			
			#endif
			break;
		case CHECK_LED_STATUS://0x22、检查前面板LED状态，总共64个LED
			#if 1
			printk("TY-3012S PciDrv DrvLedStatus->Head:  %08x!\n",  DrvLedStatus->Head);
			printk("TY-3012S PciDrv DrvLedStatus->Sid:   %08x!\n",  DrvLedStatus->VersionSid);
			printk("TY-3012S PciDrv DrvLedStatus->Id:    %08x!\n",  DrvLedStatus->Id);
			printk("TY-3012S PciDrv DrvLedStatus->Tail:  %08x!\n",  DrvLedStatus->Tail);
			#endif
			DrvLedStatus->LedL =ioread32(ChxBaseAddr0 + 4 * LED1DISPLAY);
			DrvLedStatus->LedH =ioread32(ChxBaseAddr0 + 4 * LED2DISPLAY);
			DrvLedStatus->Id = ACK_LED_STATUS;
			#if 1
			printk("TY-3012S PciDrv DrvLedStatus->Head:  %08x!!\n", DrvLedStatus->Head);
			printk("TY-3012S PciDrv DrvLedStatus->Sid:   %08x!!\n", DrvLedStatus->VersionSid);
			printk("TY-3012S PciDrv DrvLedStatus->Id:    %08x!!\n", DrvLedStatus->Id);
			printk("TY-3012S PciDrv DrvLedStatus->LedL:  %08x!!\n", DrvLedStatus->LedL);
			printk("TY-3012S PciDrv DrvLedStatus->LedH:  %08x!!\n", DrvLedStatus->LedL);
			printk("TY-3012S PciDrv DrvLedStatus->Tail:  %08x!!\n", DrvLedStatus->Tail);
			len = MSG_BUF_LEN;
			#endif
			break;
		default:
			len = 0;
			printk("TY-3012S PciDrv command error!\n");
			break;
	}
	if(copy_to_user(buff, MsgBuffer, len)) {
		printk("TY-3012S PciDrv copy_to_user error!\n");
		return -1;
	}
	return len;
}

/* 设备写函数，规范结构 */
static ssize_t pcifpga_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp)
{
	int len = 0;

	if(copy_from_user(MsgBuffer, buf, count)) {
		printk("TY-3012S PciDrv copy_from_user error!!\n");
		return -1;
	}
	switch(DrvMsgCommand->Id) {
		case SET_FPGA_TIME://03、设置FPGA的时间，年、月、日、时、分、秒、毫秒、微妙
			#if 1
			printk("TY-3012S PciDrv DrvFpgaTime->Head:   %08x!\n",  DrvFpgaTime->Head);
			printk("TY-3012S PciDrv DrvFpgaTime->Sid:    %08x!\n",  DrvFpgaTime->VersionSid);
			printk("TY-3012S PciDrv DrvFpgaTime->Id:     %08x!\n",  DrvFpgaTime->Id);
			printk("TY-3012S PciDrv DrvFpgaTime->DD:     %08x!\n",  DrvFpgaTime->DD);
			printk("TY-3012S PciDrv DrvFpgaTime->HH:     %08x!\n",  DrvFpgaTime->HH);
			printk("TY-3012S PciDrv DrvFpgaTime->MM:     %08x!\n",  DrvFpgaTime->MM);
			printk("TY-3012S PciDrv DrvFpgaTime->SS:     %08x!\n",  DrvFpgaTime->SS);
			printk("TY-3012S PciDrv DrvFpgaTime->MS:     %08x!\n",  DrvFpgaTime->MS);
			printk("TY-3012S PciDrv DrvFpgaTime->US:     %08x!\n",  DrvFpgaTime->US);
			printk("TY-3012S PciDrv DrvFpgaTime->GS:     %08x!\n",  DrvFpgaTime->GS);
			printk("TY-3012S PciDrv DrvFpgaTime->PS:     %08x!\n",  DrvFpgaTime->PS);
			printk("TY-3012S PciDrv DrvFpgaTime->ES:     %08x!\n",  DrvFpgaTime->ES);
			printk("TY-3012S PciDrv DrvFpgaTime->EV:     %08x!\n",  DrvFpgaTime->EV);
			printk("TY-3012S PciDrv DrvFpgaTime->Tail:   %08x!\n",  DrvFpgaTime->Tail);
			#endif
 			len = WriteFpgaTimer();
			len = ReadFpgaTimer();
			DrvFpgaTime->Id = ACK_FPGA_TIME;
			#if 1
			printk("TY-3012S PciDrv DrvFpgaTime->Head:   %08x!!\n", DrvFpgaTime->Head);
			printk("TY-3012S PciDrv DrvFpgaTime->Sid:    %08x!!\n", DrvFpgaTime->VersionSid);
			printk("TY-3012S PciDrv DrvFpgaTime->Id:     %08x!!\n", DrvFpgaTime->Id);
			printk("TY-3012S PciDrv DrvFpgaTime->DD:     %08x!!\n", DrvFpgaTime->DD);
			printk("TY-3012S PciDrv DrvFpgaTime->HH:     %08x!!\n", DrvFpgaTime->HH);
			printk("TY-3012S PciDrv DrvFpgaTime->MM:     %08x!!\n", DrvFpgaTime->MM);
			printk("TY-3012S PciDrv DrvFpgaTime->SS:     %08x!!\n", DrvFpgaTime->SS);
			printk("TY-3012S PciDrv DrvFpgaTime->MS:     %08x!!\n", DrvFpgaTime->MS);
			printk("TY-3012S PciDrv DrvFpgaTime->US:     %08x!!\n", DrvFpgaTime->US);
			printk("TY-3012S PciDrv DrvFpgaTime->GS:     %08x!!\n", DrvFpgaTime->GS);
			printk("TY-3012S PciDrv DrvFpgaTime->PS:     %08x!!\n", DrvFpgaTime->PS);
			printk("TY-3012S PciDrv DrvFpgaTime->ES:     %08x!!\n", DrvFpgaTime->ES);
			printk("TY-3012S PciDrv DrvFpgaTime->EV:     %08x!!\n", DrvFpgaTime->EV);
			printk("TY-3012S PciDrv DrvFpgaTime->Tail:   %08x!!\n", DrvFpgaTime->Tail);
			#endif
			break;
		case SET_CH_FILTER://0x08、设置监测通道的过滤器，即过滤条件
			#if 1
			printk("TY-3012S PciDrv DrvChFilter->Head:   %08x!\n",  DrvChFilter->Head);
			printk("TY-3012S PciDrv DrvChFilter->Sid:    %08x!\n",  DrvChFilter->VersionSid);
			printk("TY-3012S PciDrv DrvChFilter->Id:     %08x!\n",  DrvChFilter->Id);
			printk("TY-3012S PciDrv DrvChFilter->ChNo:   %08x!\n",  DrvChFilter->ChNo);
			printk("TY-3012S PciDrv DrvChFilter->Reset:  %08x!\n",  DrvChFilter->Reset);
			printk("TY-3012S PciDrv DrvChFilter->Mode:   %08x!\n",  DrvChFilter->Mode);
			printk("TY-3012S PciDrv DrvChFilter->MinLen: %08x!\n",  DrvChFilter->MinLength);
			printk("TY-3012S PciDrv DrvChFilter->MaxLen: %08x!\n",  DrvChFilter->MaxLength);
			printk("TY-3012S PciDrv DrvChFilter->FilEn:  %08x!\n",  DrvChFilter->FilterEnable);
			printk("TY-3012S PciDrv DrvChFilter->Vlan1:  %08x!\n",  DrvChFilter->Vlan1Id);
			printk("TY-3012S PciDrv DrvChFilter->Vlan2:  %08x!\n",  DrvChFilter->Vlan2Id);
			printk("TY-3012S PciDrv DrvChFilter->MacEn:  %08x!\n",  DrvChFilter->MacEnable);
			printk("TY-3012S PciDrv DrvChFilter->Tail:   %08x!\n",  DrvChFilter->Tail);
			#endif
			if(DrvChFilter->ChNo == 1) {
				len = WriteChFilter(1);
				len = ReadChFilter(1);		//读通道1的过滤条件
			} else {
				if(DrvChFilter->ChNo == 2) {
					len = WriteChFilter(2);
					len = ReadChFilter(2);	//读通道2的过滤条件
				} else {
					DrvChFilter->ChNo = 0;
					DrvChFilter->Reset = 0;
					DrvChFilter->Mode = 0;
					DrvChFilter->MinLength = 0;
					DrvChFilter->MaxLength = 0;
					DrvChFilter->FilterEnable = 0;
					DrvChFilter->Vlan1Id = 0;
					DrvChFilter->Vlan2Id = 0;
					DrvChFilter->MacEnable = 0;
					len = 0;
					printk("TY-3012S PciDrv SET_CH_FILTER error %x!\n", DrvChFilter->ChNo);//通道错误
				}
			}
			DrvChFilter->Id = ACK_SET_FILTER;
			#if 1
			printk("TY-3012S PciDrv DrvChFilter->Head:   %08x!!\n", DrvChFilter->Head);
			printk("TY-3012S PciDrv DrvChFilter->Sid:    %08x!!\n", DrvChFilter->VersionSid);
			printk("TY-3012S PciDrv DrvChFilter->Id:     %08x!!\n", DrvChFilter->Id);
			printk("TY-3012S PciDrv DrvChFilter->ChNo:   %08x!!\n", DrvChFilter->ChNo);
			printk("TY-3012S PciDrv DrvChFilter->Reset:  %08x!!\n", DrvChFilter->Reset);
			printk("TY-3012S PciDrv DrvChFilter->Mode:   %08x!!\n", DrvChFilter->Mode);
			printk("TY-3012S PciDrv DrvChFilter->MinLen: %08x!!\n", DrvChFilter->MinLength);
			printk("TY-3012S PciDrv DrvChFilter->MaxLen: %08x!!\n", DrvChFilter->MaxLength);
			printk("TY-3012S PciDrv DrvChFilter->FilEn:  %08x!!\n", DrvChFilter->FilterEnable);
			printk("TY-3012S PciDrv DrvChFilter->Vlan1:  %08x!!\n", DrvChFilter->Vlan1Id);
			printk("TY-3012S PciDrv DrvChFilter->Vlan2:  %08x!!\n", DrvChFilter->Vlan2Id);
			printk("TY-3012S PciDrv DrvChFilter->MacEn:  %08x!!\n", DrvChFilter->MacEnable);
			printk("TY-3012S PciDrv DrvChFilter->Tail:   %08x!!\n", DrvChFilter->Tail);
			#endif
			break;
		case SET_FILTER_MAC://0x09、设置监测通道的源MAC过滤地址，即接收该MAC地址数据
			#if 1
			printk("TY-3012S PciDrv DrvFilterMac->Head:  %08x!\n",  DrvFilterMac->Head);
			printk("TY-3012S PciDrv DrvFilterMac->Sid:   %08x!\n",  DrvFilterMac->VersionSid);
			printk("TY-3012S PciDrv DrvFilterMac->Id:    %08x!\n",  DrvFilterMac->Id);
			printk("TY-3012S PciDrv DrvFilterMac->ChNo:  %08x!\n",  DrvFilterMac->ChNo);
			printk("TY-3012S PciDrv DrvFilterMac->Group: %08x!\n",  DrvFilterMac->GroupNo);
			printk("TY-3012S PciDrv DrvFilterMac->Mac1L: %08x!\n",  DrvFilterMac->MacAddr1L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac1H: %08x!\n",  DrvFilterMac->MacAddr1H);
			printk("TY-3012S PciDrv DrvFilterMac->Mac2L: %08x!\n",  DrvFilterMac->MacAddr2L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac2H: %08x!\n",  DrvFilterMac->MacAddr2H);
			printk("TY-3012S PciDrv DrvFilterMac->Mac3L: %08x!\n",  DrvFilterMac->MacAddr3L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac3H: %08x!\n",  DrvFilterMac->MacAddr3H);
			printk("TY-3012S PciDrv DrvFilterMac->Mac4L: %08x!\n",  DrvFilterMac->MacAddr4L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac4H: %08x!\n",  DrvFilterMac->MacAddr4H);
			printk("TY-3012S PciDrv DrvFilterMac->Tail:  %08x!\n",  DrvFilterMac->Tail);
			#endif
			if((DrvFilterMac->ChNo == 1) || (DrvFilterMac->ChNo == 2)) {
				len = WriteFilterMac(DrvFilterMac->ChNo, DrvFilterMac->GroupNo);
				len = ReadFilterMac(DrvFilterMac->ChNo, DrvFilterMac->GroupNo);//读通道1或2的过滤MAC
				if(len == 0) {
					DrvFilterMac->ChNo = 0;
					DrvFilterMac->GroupNo = 0;
					DrvFilterMac->MacAddr1L = 0;
					DrvFilterMac->MacAddr1H = 0;
					DrvFilterMac->MacAddr2L = 0;
					DrvFilterMac->MacAddr2H = 0;
					DrvFilterMac->MacAddr3L = 0;
					DrvFilterMac->MacAddr3H = 0;
					DrvFilterMac->MacAddr4L = 0;
					DrvFilterMac->MacAddr4H = 0;
					printk("TY-3012S PciDrv SET_FILTER_MAC error %x!\n", DrvFilterMac->GroupNo);
				}
			} else {
				DrvFilterMac->ChNo = 0;
				DrvFilterMac->GroupNo = 0;
				DrvFilterMac->MacAddr1L = 0;
				DrvFilterMac->MacAddr1H = 0;
				DrvFilterMac->MacAddr2L = 0;
				DrvFilterMac->MacAddr2H = 0;
				DrvFilterMac->MacAddr3L = 0;
				DrvFilterMac->MacAddr3H = 0;
				DrvFilterMac->MacAddr4L = 0;
				DrvFilterMac->MacAddr4H = 0;
				len = 0;
				printk("TY-3012S PciDrv SET_FILTER_MAC error %x!!\n", DrvFilterMac->ChNo);//通道错误
			}
			DrvFilterMac->Id =  ACK_FILTER_MACSET;
			#if 1
			printk("TY-3012S PciDrv DrvFilterMac->Head:  %08x!!\n", DrvFilterMac->Head);
			printk("TY-3012S PciDrv DrvFilterMac->Sid:   %08x!!\n", DrvFilterMac->VersionSid);
			printk("TY-3012S PciDrv DrvFilterMac->Id:    %08x!!\n", DrvFilterMac->Id);
			printk("TY-3012S PciDrv DrvFilterMac->ChNo:  %08x!!\n", DrvFilterMac->ChNo);
			printk("TY-3012S PciDrv DrvFilterMac->Group: %08x!!\n", DrvFilterMac->GroupNo);
			printk("TY-3012S PciDrv DrvFilterMac->Mac1L: %08x!!\n", DrvFilterMac->MacAddr1L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac1H: %08x!!\n", DrvFilterMac->MacAddr1H);
			printk("TY-3012S PciDrv DrvFilterMac->Mac2L: %08x!!\n", DrvFilterMac->MacAddr2L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac2H: %08x!!\n", DrvFilterMac->MacAddr2H);
			printk("TY-3012S PciDrv DrvFilterMac->Mac3L: %08x!!\n", DrvFilterMac->MacAddr3L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac3H: %08x!!\n", DrvFilterMac->MacAddr3H);
			printk("TY-3012S PciDrv DrvFilterMac->Mac4L: %08x!!\n", DrvFilterMac->MacAddr4L);
			printk("TY-3012S PciDrv DrvFilterMac->Mac4H: %08x!!\n", DrvFilterMac->MacAddr4H);
			printk("TY-3012S PciDrv DrvFilterMac->Tail:  %08x!!\n", DrvFilterMac->Tail);
			#endif
			break;
		case SET_DEVICE_LED://0x21、设置前面板LED状态,总共64个LED
			#if 1
			printk("TY-3012S PciDrv DrvLedStatus->Head:  %08x!\n",  DrvLedStatus->Head);
			printk("TY-3012S PciDrv DrvLedStatus->Sid:   %08x!\n",  DrvLedStatus->VersionSid);
			printk("TY-3012S PciDrv DrvLedStatus->Id:    %08x!\n",  DrvLedStatus->Id);
			printk("TY-3012S PciDrv DrvLedStatus->Tail:  %08x!\n",  DrvLedStatus->Tail);
			#endif
			iowrite32(DrvLedStatus->LedL, ChxBaseAddr0 + 4 * LED1DISPLAY);
			iowrite32(DrvLedStatus->LedH, ChxBaseAddr0 + 4 * LED2DISPLAY);
			DrvLedStatus->LedL =ioread32(ChxBaseAddr0 + 4 * LED1DISPLAY);
			DrvLedStatus->LedH =ioread32(ChxBaseAddr0 + 4 * LED2DISPLAY);
			DrvLedStatus->Id = ACK_DEVICE_LED;
			#if 1
			printk("TY-3012S PciDrv DrvLedStatus->Head:  %08x!!\n", DrvLedStatus->Head);
			printk("TY-3012S PciDrv DrvLedStatus->Sid:   %08x!!\n", DrvLedStatus->VersionSid);
			printk("TY-3012S PciDrv DrvLedStatus->Id:    %08x!!\n", DrvLedStatus->Id);
			printk("TY-3012S PciDrv DrvLedStatus->LedL:  %08x!!\n", DrvLedStatus->LedL);
			printk("TY-3012S PciDrv DrvLedStatus->LedH:  %08x!!\n", DrvLedStatus->LedH);
			printk("TY-3012S PciDrv DrvLedStatus->Tail:  %08x!!\n", DrvLedStatus->Tail);
			len = MSG_BUF_LEN;
			#endif
			break;
		default:
			len = 0;
			printk("TY-3012S PciDrv command error!!\n");
			break;
	}
	if(copy_to_user((char *)buf, MsgBuffer, len)) {
		printk("TY-3012S PciDrv copy_to_user error!!\n");
		return -1;
	}
	return len;
}

/* 设备控制函数，规范结构 */
static int pcifpga_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned i, j;
	int err = 0;

	switch(cmd) {
		case SET_ETH_INTERFACE://0x01、设置监测通道的以太网接口类型，光接口或电接口
			DrvChInterface->Head		= 0x1a2b3c4d;
			DrvChInterface->VersionSid	= 0;
			DrvChInterface->Id		= ACK_ETH_INTERFACE;
			DrvChInterface->Ch1Interface	= (arg >> 16) & 0x0001;//D1第2路，1选光，0选电，缺省电
			DrvChInterface->Ch2Interface	= (arg >> 00) & 0x0001;//D0第1路，1选光，0选电，缺省电
			DrvChInterface->Tail		= 0x0a1b2c3d4;
			i = (DrvChInterface->Ch2Interface << 1) | DrvChInterface->Ch1Interface;
			iowrite32(i, ChxBaseAddr0 + 4 * CHINTERFACESEL);
			j = ioread32(ChxBaseAddr0 + 4 * CHINTERFACESEL);
			if(i == j) err = 0;
			else err = -1;
			break;
		case SET_GPS_INTERFACE://0x02、设置GPS或1588接口，以及接口类型是TTL或485接口
			DrvGpsInterface->Head		= 0x1a2b3c4d;
			DrvGpsInterface->VersionSid	= 0;
			DrvGpsInterface->Id		= ACK_GPS_INTERFACE;
			DrvGpsInterface->Gps1588	= (arg >> 16) & 0x0001;//D1=1选1588，D1=0选IRIGB，缺省IRIGB
			DrvGpsInterface->InterfaceType	= (arg >> 00) & 0x0001;//D0=1平衡，D0=0非平衡75欧，缺省非平衡
			DrvGpsInterface->Tail		= 0x0a1b2c3d4;
			i = (DrvGpsInterface->Gps1588 << 1) | DrvGpsInterface->InterfaceType;
			iowrite32(i, ChxBaseAddr0 + 4 * GPSINTERFACESEL);
			j = ioread32(ChxBaseAddr0 + 4 * GPSINTERFACESEL);
			if(i == j) err = 0;
			else err = -1;
			break;
		case SET_TRIGGER_TIME://0x04、设置监测通道外触发消抖时间，单位us
			DrvTriggerTime->Head		= 0x1a2b3c4d;
			DrvTriggerTime->VersionSid	= 0;
			DrvTriggerTime->Id		= ACK_TRIGGER_TIME;
			DrvTriggerTime->Sw1TrigerTime	= (arg >> 16) & 0x00ff;//外触发接口1，单位us，最小0，最大256
			DrvTriggerTime->Sw2TrigerTime	= (arg >> 00) & 0x00ff;//外触发接口2，单位us，最小0，最大256
			DrvTriggerTime->Tail		= 0x0a1b2c3d4;
			iowrite32(DrvTriggerTime->Sw1TrigerTime, ChxBaseAddr0 + 4 * SW1TRIGGERTIME);
			iowrite32(DrvTriggerTime->Sw2TrigerTime, ChxBaseAddr0 + 4 * SW2TRIGGERTIME);
			i = ioread32(ChxBaseAddr0 + 4 * SW1TRIGGERTIME);
			j = ioread32(ChxBaseAddr0 + 4 * SW2TRIGGERTIME);
			if((i == DrvTriggerTime->Sw1TrigerTime) && (j == DrvTriggerTime->Sw2TrigerTime)) err = 0;
			else err = -1;
			break;
		case SET_ALARM_OUT://0x05、设置监测通道外外告警输出，继电器闭合
			DrvAlarmOut->Head		= 0x1a2b3c4d;
			DrvAlarmOut->VersionSid		= 0;
			DrvAlarmOut->Id			= ACK_ALARM_OUT;
			DrvAlarmOut->Sw1AlarmOut	= (arg >> 16) & 0x0001;//外告警接口1，D0=1闭合，0=断开
			DrvAlarmOut->Sw2AlarmOut	= (arg >> 00) & 0x0001;//外告警接口2，D1=1闭合，0=断开
			DrvAlarmOut->Tail		= 0x0a1b2c3d4;
			i = (DrvAlarmOut->Sw2AlarmOut << 1) | DrvAlarmOut->Sw1AlarmOut;
			iowrite32(i, ChxBaseAddr0 + 4 * SWALARMOUT);
			j = ioread32(ChxBaseAddr0 + 4 * SWALARMOUT);
			if(i == j) err = 0;
			else err = -1;
			break;
		case SET_BEEP_ALARM://0x06、设置内部蜂鸣器告警输出，蜂鸣器响
			DrvBeepAlarm->Head		= 0x1a2b3c4d;
			DrvBeepAlarm->VersionSid	= 0;
			DrvBeepAlarm->Id		= ACK_BEEP_ALARM;
			DrvBeepAlarm->BeepAlarm		= arg & 0x03;//D1=1响一声（按键），D0=1长鸣
			DrvBeepAlarm->Tail		= 0x0a1b2c3d4;
			i = DrvBeepAlarm->BeepAlarm;
			iowrite32(i, ChxBaseAddr0 + 4 * BEEPALARMOUT);
			j = ioread32(ChxBaseAddr0 + 4 * BEEPALARMOUT);
			if(i == j) err = 0;
			else err = -1;
			break;
		case SET_WARCH_DOG://0x07、设置即清楚外部看门狗，看门狗要定时清楚
			DrvWarchDog->Head		= 0x1a2b3c4d;
			DrvWarchDog->VersionSid		= 0;
			DrvWarchDog->Id			= ACK_WARCH_DOG;
			DrvWarchDog->WarchDog		= arg & 0x0ff;//写0xFF清除看门狗，然后自清除D7～D0数据
			DrvWarchDog->Tail		= 0x0a1b2c3d4;
			iowrite32(DrvWarchDog->WarchDog, ChxBaseAddr0 + 4 * WARCHDOGOUT);
			err = 0;
			break;
		case SET_BUFFER_EMPTY://0x0A、设置清除FIFO中的数据
			DrvClearFifo->Head		= 0x1a2b3c4d;
			DrvClearFifo->VersionSid	= 0;
			DrvClearFifo->Id		= ACK_BUFFER_EMPTY;
			DrvClearFifo->ClearFifo1	= (arg >> 16) & 0x00ff;//写0xFF清FIFO，然后自清除D7～D0
			DrvClearFifo->ClearFifo2	= (arg >> 00) & 0x00ff;//写0xFF清FIFO，然后自清除D7～D0
			DrvClearFifo->Tail		= 0x0a1b2c3d4;
			iowrite32(DrvClearFifo->ClearFifo1, ChxBaseAddr0 + 4 * CH1FIFORESET);
			iowrite32(DrvClearFifo->ClearFifo2, ChxBaseAddr0 + 4 * CH2FIFORESET);
			err = 0;
			break;
		case CHECK_CH_INTERFACE://0x12、检查监测通道的以太网接口类型，光接口或电接口
			DrvChInterface->Head		= 0x1a2b3c4d;
			DrvChInterface->VersionSid	= 0;
			DrvChInterface->Id		= ACK_CH_INTERFACE;
			DrvChInterface->Tail		= 0x0a1b2c3d4;
			i = ioread32(ChxBaseAddr0 + 4 * CHINTERFACESEL);
			DrvChInterface->Ch1Interface	= 0;
			DrvChInterface->Ch2Interface	= 0;
			if((i & 0x01) == 0x01)
				DrvChInterface->Ch1Interface = 0x0001;//D0第1路，1选光，0选电，缺省电
			if((i & 0x02) == 0x02)
				DrvChInterface->Ch2Interface = 0x0001;//D1第2路，1选光，0选电，缺省电
			err = (DrvChInterface->Ch1Interface << 16) | DrvChInterface->Ch2Interface;
			break;
		case CHECK_TRIGGER_STATUS://0x15、检查监测通道外触发状态，有无触发，是否正在触发
			DrvTriggerStatus->Head		= 0x1a2b3c4d;
			DrvTriggerStatus->VersionSid	= 0;
			DrvTriggerStatus->Id		= ACK_TRIGGER_STATUS;
			DrvTriggerStatus->Tail		= 0x0a1b2c3d4;
			i = ioread32(ChxBaseAddr0 + 4 * SW1STATUS);
			j = ioread32(ChxBaseAddr0 + 4 * SW2STATUS);
			DrvTriggerStatus->Sw1NowStatus = 0;
			DrvTriggerStatus->Sw1HisStatus = 0;
			DrvTriggerStatus->Sw2NowStatus = 0;
			DrvTriggerStatus->Sw2HisStatus = 0;
			if((i & 0x01) == 0x01)
				DrvTriggerStatus->Sw1NowStatus = 0x01;//D0=1表示目前继电器闭合
			if((i & 0x02) == 0x02)
				DrvTriggerStatus->Sw1HisStatus = 0x01;//D1=1表示曾经有开量输入
			if((j & 0x01) == 0x01)
				DrvTriggerStatus->Sw2NowStatus = 0x01;//D0=1表示目前继电器闭合
			if((j & 0x02) == 0x02)
				DrvTriggerStatus->Sw2HisStatus = 0x01;//D1=1表示曾经有开量输入
			err = 0;
			err = (err << 8) | DrvTriggerStatus->Sw1NowStatus;
			err = (err << 8) | DrvTriggerStatus->Sw1HisStatus;
			err = (err << 8) | DrvTriggerStatus->Sw2NowStatus;
			err = (err << 8) | DrvTriggerStatus->Sw2HisStatus;
			break;
		case CHECK_KEY_STATUS://0x16、检查前面板按键状态，有无按键，是否正在按
			DrvKeyStatus->Head		= 0x1a2b3c4d;
			DrvKeyStatus->VersionSid	= 0;
			DrvKeyStatus->Id		= ACK_KEY_STATUS;
			DrvKeyStatus->Tail		= 0x0a1b2c3d4;
			i = ioread32(ChxBaseAddr0 + 4 * KEY1STATUS);
			j = ioread32(ChxBaseAddr0 + 4 * KEY2STATUS);
			DrvKeyStatus->Key1NowStatus = 0;
			DrvKeyStatus->Key1HisStatus = 0;
			DrvKeyStatus->Key2NowStatus = 0;
			DrvKeyStatus->Key2HisStatus = 0;
			if((i & 0x01) == 0x01)
				DrvKeyStatus->Key1NowStatus = 0x01;//D0=1表示目前继电器闭合
			if((i & 0x02) == 0x02)
				DrvKeyStatus->Key1HisStatus = 0x01;//D1=1表示曾经有开量输入
			if((j & 0x01) == 0x01)
				DrvKeyStatus->Key2NowStatus = 0x01;//D0=1表示目前继电器闭合
			if((j & 0x02) == 0x02)
				DrvKeyStatus->Key2HisStatus = 0x01;//D1=1表示曾经有开量输入
			err = 0;
			err = (err << 8) | DrvKeyStatus->Key1NowStatus;
			err = (err << 8) | DrvKeyStatus->Key1HisStatus;
			err = (err << 8) | DrvKeyStatus->Key2NowStatus;
			err = (err << 8) | DrvKeyStatus->Key2HisStatus;
			break;
		case CHECK_CH_STATUS://0x13、检查监测通道光电接口的连接状态，连接、速度、双工
			DrvChStatus->Head	= 0x1a2b3c4d;
			DrvChStatus->VersionSid	= 0;
			DrvChStatus->Id		= ACK_CH_STATUS;
			DrvChStatus->EthOLink	= 0;
			DrvChStatus->EthELink	= 4;
			DrvChStatus->Tail	= 0x0a1b2c3d4;
			//读出4个PHY的状态字
			//PHYADD1 = 0x01;	//电接口1的PHY地址
			//PHYADD2 = 0x02;	//光接口1的PHY地址
			//PHYADD3 = 0x03;	//电接口2的PHY地址
			//PHYADD2 = 0x04;	//光接口2的PHY地址
			//PHYREG1 = 0x04;	//PHY状态寄存器地址
			//PHYRDC  = 0x02;	//写PHY的寄存器命令
			//PHYRDC  = 0x04;	//读PHY的寄存器命令
			//iowrite32(0x10, ChxBaseAddr0 + 4 * ETHMIIMMDC);//分频计数器，16分频=2MHz
			//iowrite32(((PHYADD1 << 8) | PHYREG1), ChxBaseAddr0 + 4 * ETHMIIMPHYADDREG);//PHY地址和PHY状态寄存器
			//iowrite32(PHYRDC, ChxBaseAddr0 + 4 * ETHMIIMCOMMAND);
			//i = ioread32(ChxBaseAddr0 + 4 * ETHMIIMRDDATA);
			break;
		default:
			printk("TY-3012S PciDrv ioctl command err!\n");
			err = -1;
			break;
	}
	return err;
}

/* 设备关闭函数，规范结构 */
static int pcifpga_release(struct inode *inode, struct file *filp)
{
	//printk("TY-3012S PciDrv pcifpga_release closed!\n");
	return 0;
}

static struct file_operations pcifpga_fops = {
	.owner		= THIS_MODULE,
	.open		= pcifpga_open,
	.read		= pcifpga_read,
	.write		= pcifpga_write,
	.ioctl		= pcifpga_ioctl,
	.release	= pcifpga_release,
};

//指明驱动程序适用的PCI设备ID
static struct pci_device_id pcifpga_table[] __initdata = {
	{ CTD_VENDOR_ID, CTD_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, },
	{ 0, }
};	//厂商ID, 设备ID, 子厂商ID, 子设备ID

MODULE_DEVICE_TABLE(pci, pcifpga_table);

static unsigned char get_pcifpga_revision(struct pci_dev *pcifpga_dev)
{
	u8 pcifpga_revision;

	pci_read_config_byte(pcifpga_dev, PCI_REVISION_ID, &pcifpga_revision);	//PCI_REVISION_ID已经定义
	printk("TY-3012S PciDrv PCI_REVISION_ID: %x!\n", pcifpga_revision);	//显示驱动程序的版本号
	return pcifpga_revision;
}

//PCIFPGA测试卡的中断服务程序
static irqreturn_t pcifpga_interrupt(int irq, void *dev_id)
{
	unsigned int ints, fifo1s, fifo2s, fifo1c, fifo2c;
	unsigned int key1s, key2s, rely1, rely2;
	unsigned int ch1fc, ch2fc, ch1bc, ch2bc;
	//unsigned int *dmaptr1, *dmaptr2;
	unsigned int i, j;

	//从PCIFPGA的FIFO1和FIFO2缓冲区中读出数据到DMA缓冲区
	ints = ioread32(ChxBaseAddr0 + 4 * INTERRUPTSTATUS);
	//D6/5第2/1路64K满,D4/3开入2/1,D2/1按键2/1,D0定时,1中断,读后自动清除
	//printk("TY-3012S PciDrv INTSTATUS:     %08x!\n", ints);
	//调用开量和按键处理函数
	fifo1c = 0;
	fifo2c = 0;
	#if 1
	if((ints & 0x02) == 0x02) {
		key1s = ioread32(ChxBaseAddr0 + 4 * KEY1STATUS);
		//D1=1曾有按键入,D0=1目前按键正闭合,查询后自动清除
		//printk("TY-3012S PciDrv KEY1TRIGGERS:  %08x!\n", key1s);
		//显示计数器统计值
		ch1fc = ioread32(Ch1BaseAddr1 + 4 * CHALLFRAMEH);
		printk("\nTY-3012S PciDrv CH1ALLFRAMEH:  %08x!\n", ch1fc);
		ch1fc = ioread32(Ch1BaseAddr1 + 4 * CHALLFRAMEL);
		printk("TY-3012S PciDrv CH1ALLFRAMEL:  %08x!\n", ch1fc);
		ch1bc = ioread32(Ch1BaseAddr1 + 4 * CHALLBYTEH);
		printk("TY-3012S PciDrv CH1ALLBYTEH:   %08x!\n", ch1bc);
		ch1bc = ioread32(Ch1BaseAddr1 + 4 * CHALLBYTEL);
		printk("TY-3012S PciDrv CH1ALLBYTEL:   %08x!\n", ch1bc);
		ch2fc = ioread32(Ch2BaseAddr2 + 4 * CHALLFRAMEH);
		printk("TY-3012S PciDrv CH2ALLFRAMEH:  %08x!\n", ch2fc);
		ch2fc = ioread32(Ch2BaseAddr2 + 4 * CHALLFRAMEL);
		printk("TY-3012S PciDrv CH2ALLFRAMEL:  %08x!\n", ch2fc);
		ch2bc = ioread32(Ch2BaseAddr2 + 4 * CHALLBYTEH);
		printk("TY-3012S PciDrv CH2ALLBYTEH:   %08x!\n", ch2bc);
		ch2bc = ioread32(Ch2BaseAddr2 + 4 * CHALLBYTEL);
		printk("TY-3012S PciDrv CH2ALLBYTEL:   %08x!\n", ch2bc);
		//从FIFO取出的总双字数,解出的包数,解出双字数,读走双字数
		printk("\nTY-3012S PciDrv Ch1: FifoDword, Packet, WrDword, RdDord!\n");
		printk("TY-3012S PciDrv Fifo1DwCnt:    %08x!\n", Ch1Cnt);
		printk("TY-3012S PciDrv Ch1PacketCnt:  %08x!\n", Ch1PacketCnt);
		printk("TY-3012S PciDrv Ch1PacketDw:   %08x!\n", Ch1PacketDw);
		printk("TY-3012S PciDrv ReadCh1Dword:  %08x!\n", ReadCh1Dword);
		printk("\nTY-3012S PciDrv Ch2: FifoDword, Packet, WrDword, RdDord!\n");
		printk("TY-3012S PciDrv Fifo2DwCnt:    %08x!\n", Ch2Cnt);
		printk("TY-3012S PciDrv Ch2PacketCnt:  %08x!\n", Ch2PacketCnt);
		printk("TY-3012S PciDrv Ch2PacketDw:   %08x!\n", Ch2PacketDw);
		printk("TY-3012S PciDrv ReadCh2Dword:  %08x!\n", ReadCh2Dword);
		//结束计数器统计值
	}
	if((ints & 0x04) == 0x04) {
		key2s = ioread32(ChxBaseAddr0 + 4 * KEY2STATUS);
		//D1=1曾有按键入,D0=1目前按键正闭合,查询后自动清除
		//printk("TY-3012S PciDrv KEY2TRIGGERS:  %08x!\n", key2s);
		i = DisplayFpgaTimer();
	}
	if((ints & 0x08) == 0x08) {
		rely1 = ioread32(ChxBaseAddr0 + 4 * SW1STATUS);
		//D1=1曾有开量入,D0=1目前继电器闭合,查询后自动清除
		printk("TY-3012S PciDrv SW1TRIGGERS:   %08x!\n", rely1);
	}
	if((ints & 0x10) == 0x10) {
		rely2 = ioread32(ChxBaseAddr0 + 4 * SW2STATUS);
		//D1=1曾有开量入,D0=1目前继电器闭合,查询后自动清除
		printk("TY-3012S PciDrv SW2TRIGGERS:   %08x!\n", rely2);
	}
	if((ints & 0x20) == 0x20) {
		fifo1s = ioread32(ChxBaseAddr0 + 4 * CH1FIFOSTATUS);
		//D4空,D3满,D2半满,D1几乎空,D0几乎满,高有效
		printk("TY-3012S PciDrv CH1FIFOSTATUS: %08x!\n", fifo1s);
		iowrite32(CHFIFORESET, ChxBaseAddr0 + 4 * CH1FIFORESET);
	}
	if((ints & 0x40) == 0x40) {
		fifo2s = ioread32(ChxBaseAddr0 + 4 * CH2FIFOSTATUS);
		//D4空,D3满,D2半满,D1几乎空,D0几乎满,高有效
		printk("TY-3012S PciDrv CH2FIFOSTATUS: %08x!\n", fifo2s);
		iowrite32(CHFIFORESET, ChxBaseAddr0 + 4 * CH2FIFORESET);
	}
	#endif
	//定时中断
	#if 1
	//FIFO状态,D4空,D3满,D2半满,D1将空,D0将满,高有效,产生中断
	if((ints & 0x01) == 0x01) {
		fifo1s = ioread32(ChxBaseAddr0 + 4 * CH1FIFOSTATUS);
		//printk("TY-3012S PciDrv CH1FIFOSTATUS: %08x!\n", fifo1s);
		fifo2s = ioread32(ChxBaseAddr0 + 4 * CH2FIFOSTATUS);
		//printk("TY-3012S PciDrv CH2FIFOSTATUS: %08x!\n", fifo2s);
		fifo1c = ioread32(ChxBaseAddr0 + 4 * CH1FIFOCOUNT) >> 16;
		if(fifo1c >= 0x3fff) printk("TY-3012S PciDrv CH1FIFOCOUNTE: %08x!\n", fifo1c);
		fifo1c = fifo1c & 0x3fff;//双字数
		//printk("TY-3012S PciDrv CH1FIFOCOUNT:  %08x!\n", fifo1c);
		fifo2c = ioread32(ChxBaseAddr0 + 4 * CH2FIFOCOUNT) >> 16;
		if(fifo2c >= 0x3fff) printk("TY-3012S PciDrv CH2FIFOCOUNTE: %08x!\n", fifo2c);
		fifo2c = fifo2c & 0x3fff;//双字数
		//printk("TY-3012S PciDrv CH2FIFOCOUNT:  %08x!\n", fifo2c);
		if((fifo1s & 0x08) == 0x08) {
			//printk("TY-3012S PciDrv CH2FIFOFULL:   %08x!\n", fifo1s);
			Ch1FifoFull++;
			iowrite32(CHFIFORESET, ChxBaseAddr0 + 4 * CH1FIFORESET);//清空FIFO
			//fifo1s = ioread32(ChxBaseAddr0 + 4 * CH1FIFOSTATUS);
			//printk("TY-3012S PciDrv CH1FIFOSTATUS: %08x!!\n", fifo1s);
			fifo1c = 0;
		}
		if(fifo1c > 0) ReadFifo1(fifo1c);
		if((fifo2s & 0x08) == 0x08) {
			//printk("TY-3012S PciDrv CH1FIFOFULL:   %08x!\n", fifo2s);
			Ch2FifoFull++;
			iowrite32(CHFIFORESET, ChxBaseAddr0 + 4 * CH2FIFORESET);//清空FIFO
			//fifo2s = ioread32(ChxBaseAddr0 + 4 * CH2FIFOSTATUS);
			//printk("TY-3012S PciDrv CH2FIFOSTATUS: %08x!!\n", fifo2s);
			fifo2c = 0;
		}
		if(fifo2c > 0) ReadFifo2(fifo2c);
	}
	#endif
	#if 1
	i = 1;
	while(i) {
		i = ReadFrameBuf1();
		if(i != 0) {
			//printk("TY-3012S PciDrv ReadFrameBuf1: %08x!\n", i);
			j = WriteCh1Buf(i);//双字
			//printk("TY-3012S PciDrv WriteCh1Buf:   %08x!\n", j);
		}
	}
	i =1;
	while(i) {
		i = ReadFrameBuf2();
		if(i != 0) {
			//printk("TY-3012S PciDrv ReadFrameBuf2: %08x!\n", i);
			j = WriteCh2Buf(i);//双字
			//printk("TY-3012S PciDrv WriteCh2Buf:   %08x!\n", j);
		}
	}
	#endif
	//通过DMABUF测试FIFO1和FIFO2读取速率,速率11MByte/s
	#if 0
	if(fifo1c > 0) {
		dmaptr1 = (unsigned int *)pcifpga_card_dma1;
		dmaptr2 = Ch1BaseAddr3;
		printk("TY-3012S PciDrv Read CH1FIFO:  %08d!", fifo1c);
		memcpy(dmaptr1, dmaptr2, fifo1c);
		for(i = 0; i < fifo1c; i++) {
			if((i % 16) == 0) printk("\n");
			printk(".%02x", *(pcifpga_card_dma1 + i));
		}
		printk("!\n");
		printk("TY-3012S PciDrv End Read CH1FIFO!\n\n");
	}
	if(fifo2c > 0) {
		dmaptr1 = (unsigned int *)pcifpga_card_dma2;
		dmaptr2 = Ch2BaseAddr4;
		printk("TY-3012S PciDrv Read CH2FIFO:  %08d!", fifo2c);
		memcpy(dmaptr1, dmaptr2, fifo2c);
		for(i = 0; i < fifo2c; i++) {
			if((i % 16) == 0) printk("\n");
			printk(".%02x", *(pcifpga_card_dma2 + i));
		}
		printk("!\n");
		printk("TY-3012S PciDrv End Read CH2FIFO!\n\n");
	}
	#endif
	//注意需要考虑延时
	m_second ++;
	if(m_second > MAX_M_SECOND) {
		m_second = 0;
		second_int ++;
		if(Ch1FifoFull > 0)   printk("TY-3012S PciDrv CH1FIFOFULL:   %08x!\n", Ch1FifoFull);
		Ch1FifoFull = 0;
		if(Ch2FifoFull > 0)   printk("TY-3012S PciDrv CH2FIFOFULL:   %08x!\n", Ch2FifoFull);
		Ch2FifoFull = 0;
		if(Ch1DmaBufFull > 0) printk("TY-3012S PciDrv CH1DMABUFFULL: %08x!\n", Ch1DmaBufFull);
		Ch1DmaBufFull = 0;
		if(Ch2DmaBufFull > 0) printk("TY-3012S PciDrv CH2DMABUFFULL: %08x!\n", Ch2DmaBufFull);
		Ch2DmaBufFull = 0;
		if(Ch1BufFull > 0)    printk("TY-3012S PciDrv CH1BUFFULL:    %08x!\n", Ch1BufFull);
		Ch1BufFull = 0;
		if(Ch2BufFull > 0)    printk("TY-3012S PciDrv CH2BUFFULL:    %08x!\n", Ch2BufFull);
		Ch2BufFull = 0;
		//printk("TY-3012S PciDrv interrupt: %d!\n", second_int);
	}
	return IRQ_HANDLED;
}

//探测PCIFPGA测试卡设备
static int __init pcifpga_probe(struct pci_dev *pcifpga_dev, const struct pci_device_id *pcifpga_id)
{
	u8	config_reg8;
	u16	config_reg16;
	int	retval;

	pcifpga_card_dev = pcifpga_dev;				//我们PCIFPGA卡的变量是pcifpga_card_dev
	//启动PCI设备                                           //pcifpga_card_dev通过Linux系统的PCI程序传入
	if(pci_enable_device(pcifpga_dev)) {
		printk("TY-3012S PciDrv pci_enable_device Err!\n");
		return -EIO;
	}
	//读取PCIFPGA测试卡的配置信息
	ctd_pci_vendor = pcifpga_dev->vendor;			//PCI设备配置空间中的Device ID
	ctd_pci_device = pcifpga_dev->device;			//PCI设备配置空间中的Vendor ID
	printk("TY-3012S PciDrv Device ID: %x!\n", ctd_pci_vendor);
	printk("TY-3012S PciDrv Vendor ID: %x!\n", ctd_pci_device);
	//检查PCIFPGA测试卡的版本号
	if(get_pcifpga_revision(pcifpga_dev) != 0x00) {		//版本应为0x00
		printk("TY-3012S PciDrv get_pcifpga_revision Err!\n");
		return -ENODEV;
	}
	//确定PCIFPGA测试卡的中断IRQ
	pcifpga_irq = pcifpga_dev->irq;				//确定中断号，系统分配给PCIFPGA卡
	printk("PCI TY-3012S PciDrv Interrupt: %d!\n", pcifpga_irq);
	//设定PCIFPGA测试卡的存储地址及其范围
	chx_mem0_base = pci_resource_start(pcifpga_dev, 0);	//确定存储器空间0(公用)的基地址,DMA
	chx_mem0_range = pci_resource_len (pcifpga_dev, 0);	//确定存储器空间0(公用)的范围,DMA
	printk("PCI TY-3012S PciDrv chx_mem0_base: %x, and Range: %x!\n", (unsigned int)chx_mem0_base, chx_mem0_range);
	ch1_mem1_base = pci_resource_start(pcifpga_dev, 1);	//确定存储器空间1(通道1)的基地址,DMA
	ch1_mem1_range = pci_resource_len (pcifpga_dev, 1);	//确定存储器空间1(通道1)的范围,DMA
	printk("PCI TY-3012S PciDrv ch1_mem1_base: %x, and Range: %x!\n", (unsigned int)ch1_mem1_base, ch1_mem1_range);
	ch2_mem2_base = pci_resource_start(pcifpga_dev, 2);	//确定存储器空间2(通道2)的基地址,DMA
	ch2_mem2_range = pci_resource_len (pcifpga_dev, 2);	//确定存储器空间2(通道2)的范围,DMA
	printk("PCI TY-3012S PciDrv ch2_mem2_base: %x, and Range: %x!\n", (unsigned int)ch2_mem2_base, ch2_mem2_range);
	ch1_mem3_base = pci_resource_start(pcifpga_dev, 3);	//确定存储器空间2(通道1)的基地址,DMA
	ch1_mem3_range = pci_resource_len (pcifpga_dev, 3);	//确定存储器空间2(通道1)的范围,DMA
	printk("PCI TY-3012S PciDrv ch1_mem3_base: %x, and Range: %x!\n", (unsigned int)ch1_mem3_base, ch1_mem3_range);
	ch2_mem4_base = pci_resource_start(pcifpga_dev, 4);	//确定存储器空间4(通道2)的基地址,DMA
	ch2_mem4_range = pci_resource_len (pcifpga_dev, 4);	//确定存储器空间4(通道2)的范围,DMA
	printk("PCI TY-3012S PciDrv ch2_mem4_base: %x, and Range: %x!\n", (unsigned int)ch2_mem4_base, ch2_mem4_range);
	ChxBaseAddr0 = pci_iomap(pcifpga_dev, 0, 0);//存储器读写
	printk("PCI TY-3012S PciDrv ChxBaseAddr0:  %x!\n", (unsigned int)ChxBaseAddr0);
	Ch1BaseAddr1 = pci_iomap(pcifpga_dev, 1, 0);//存储器读写
	printk("PCI TY-3012S PciDrv Ch1BaseAddr1:  %x!\n", (unsigned int)Ch1BaseAddr1);
	Ch2BaseAddr2 = pci_iomap(pcifpga_dev, 2, 0);//存储器读写
	printk("PCI TY-3012S PciDrv Ch2BaseAddr2:  %x!\n", (unsigned int)Ch2BaseAddr2);
	Ch1BaseAddr3 = pci_iomap(pcifpga_dev, 3, 0);//存储器读写
	printk("PCI TY-3012S PciDrv Ch1BaseAddr3:  %x!\n", (unsigned int)Ch1BaseAddr3);
	Ch2BaseAddr4 = pci_iomap(pcifpga_dev, 4, 0);//存储器读写
	printk("PCI TY-3012S PciDrv Ch2BaseAddr4:  %x!\n", (unsigned int)Ch2BaseAddr4);
	//申请中断IRQ并设定中断服务子函数
	retval = request_irq(pcifpga_irq, pcifpga_interrupt, 0, CTD_PCI_NAME, NULL);//IRQF_SHARED
	if(retval) {
		printk ("TY-3012S PciDrv Can't get assigned IRQ %d!\n", pcifpga_irq);
 		return -1;
	}
	printk ("TY-3012S PciDrv Request IRQ: %d, and m_second: %d!\n", retval, m_second);
	//使用下述函数读取配置寄存器
	#if 1
	pci_read_config_word(pcifpga_dev, 0x00, &config_reg16);	//读配置寄存器，厂家号
	printk("TY-3012S PciDrv pci_read_config_word00: %x!\n", config_reg16);
	pci_read_config_word(pcifpga_dev, 0x02, &config_reg16);	//读配置寄存器，装置号
	printk("TY-3012S PciDrv pci_read_config_word02: %x!\n", config_reg16);
	pci_read_config_word(pcifpga_dev, 0x04, &config_reg16);	//读配置寄存器，命令寄存器
	printk("TY-3012S PciDrv pci_read_config_word04: %x!\n", config_reg16);
	pci_read_config_word(pcifpga_dev, 0x06, &config_reg16);	//读配置寄存器，状态寄存器
	printk("TY-3012S PciDrv pci_read_config_word06: %x!\n", config_reg16);
	pci_read_config_byte(pcifpga_dev, 0x3c, &config_reg8);	//读配置寄存器，中断线寄存器
	printk("TY-3012S PciDrv pci_read_config_word3c: %x!\n", config_reg8);
	pci_read_config_byte(pcifpga_dev, 0x3d, &config_reg8);	//读配置寄存器，中断脚寄存器
	printk("TY-3012S PciDrv pci_read_config_word3d: %x!\n", config_reg8);
	#endif
	//在内核空间中动态申请内存
	//为消息申请缓存
	MsgBuffer = kmalloc(MSG_BUF_LEN, GFP_DMA | GFP_KERNEL);	//申请一块连续的缓存，64字节，为读取消息数据
	if(!MsgBuffer) {					//MSG_BUF_LEN=0x40Byte
		free_irq(pcifpga_irq, NULL);
		printk("TY-3012S PciDrv kmalloc MsgBuffer Err!\n");
		return -ENOMEM;
	}
	memset(MsgBuffer, 0, MSG_BUF_LEN);
	DrvMsgCommand	= (MsgCommand*)MsgBuffer;
	DrvChInterface	= (MsgChInterface*)MsgBuffer;
	DrvGpsInterface	= (MsgGpsInterface*)MsgBuffer;
	DrvFpgaTime	= (MsgFpgaTime*)MsgBuffer;
	DrvTriggerTime	= (MsgTriggerTime*)MsgBuffer;
	DrvAlarmOut	= (MsgAlarmOut*)MsgBuffer;
	DrvBeepAlarm	= (MsgBeepAlarm*)MsgBuffer;
	DrvWarchDog	= (MsgWarchDog*)MsgBuffer;
	DrvClearFifo	= (MsgClearFifo*)MsgBuffer;
	DrvChFilter	= (MsgChFilter*)MsgBuffer;
	DrvFilterMac	= (MsgFilterMac*)MsgBuffer;
	DrvChStatus	= (MsgChStatus*)MsgBuffer;
	DrvTriggerStatus= (MsgTriggerStatus*)MsgBuffer;
	DrvKeyStatus	= (MsgKeyStatus*)MsgBuffer;
	DrvFrameCounters= (MsgFrameCounters*)MsgBuffer;
	DrvLedStatus	= (MsgLedStatus*)MsgBuffer;
	DrvReadBuffer	= (MsgReadBuffer*)MsgBuffer;
	//为FIFO1申请缓存
	if((pcifpga_card_dma1 = kmalloc(PCIFPGA_DMABUF_BYTELEN, GFP_DMA | GFP_KERNEL)) == NULL) {//申请128K双字
		free_irq(pcifpga_irq, NULL);
		kfree(MsgBuffer);
		printk("TY-3012S PciDrv kmalloc pcifpga_card_dma1 Err!\n");
		return -ENOMEM;
	}
	memset(pcifpga_card_dma1, 0, PCIFPGA_DMABUF_BYTELEN);//DMA缓冲区清零
	ReadCh1Pointer = WriteCh1Pointer = Ch1Cnt = 0;//设置读写指针和双字计数器为0
	Ch1FifoFull = Ch1DmaBufFull = Ch1BufFull = 0;//清除各个缓冲区满标志
	printk("TY-3012S PciDrv pcifpga_card_dma1: %x!\n", (unsigned int)pcifpga_card_dma1);
	//为FIFO2申请缓存
	if((pcifpga_card_dma2 = kmalloc(PCIFPGA_DMABUF_BYTELEN, GFP_DMA | GFP_KERNEL)) == NULL) {//申请128K双字
		free_irq(pcifpga_irq, NULL);
		kfree(MsgBuffer);
		kfree(pcifpga_card_dma1);
		printk("TY-3012S PciDrv kmalloc pcifpga_card_dma2 Err!\n");
		return -ENOMEM;
	}
	memset(pcifpga_card_dma2, 0, PCIFPGA_DMABUF_BYTELEN);//DMA缓冲区清零
	ReadCh2Pointer = WriteCh2Pointer = Ch2Cnt = 0;//设置读写指针和双字计数器为0
	Ch2FifoFull = Ch2DmaBufFull = Ch2BufFull = 0;//清除各个缓冲区满标志
	printk("TY-3012S PciDrv pcifpga_card_dma2: %x!\n", (unsigned int)pcifpga_card_dma2);
	//确定FIFO缓冲区的物理地址
	PciDmaBufPhyAddr1 = pci_map_single(pcifpga_card_dev, pcifpga_card_dma1,  PCIFPGA_DMABUF_BYTELEN, DMA_FROM_DEVICE);
	printk("PCI TY-3012S PciDrv PciDmaBufPhyAddr1: %x!\n", (unsigned int)PciDmaBufPhyAddr1);
	PciDmaBufPhyAddr2 = pci_map_single(pcifpga_card_dev, pcifpga_card_dma2,  PCIFPGA_DMABUF_BYTELEN, DMA_FROM_DEVICE);
	printk("PCI TY-3012S PciDrv PciDmaBufPhyAddr2: %x!\n", (unsigned int)PciDmaBufPhyAddr2);
	//为解帧缓冲区申请内存
	if((pcifpga_card_buf0 = kmalloc(PCIFPGA_PACKET_BYTELEN, GFP_DMA | GFP_KERNEL)) == NULL) {//申请1K双字
		free_irq(pcifpga_irq, NULL);
		kfree(MsgBuffer);
		kfree(pcifpga_card_dma1);
		kfree(pcifpga_card_dma2);
		printk("TY-3012S PciDrv kmalloc pcifpga_card_buf0 Err!\n");
		return -ENOMEM;
	}
	memset(pcifpga_card_buf0, 0, PCIFPGA_PACKET_BYTELEN);//解包缓冲区清零
	printk("TY-3012S PciDrv pcifpga_card_buf0: %x!\n", (unsigned int)pcifpga_card_buf0);
	//为应用缓冲区1申请内存
	if((pcifpga_card_buf1 = kmalloc(PCIFPGA_BUF_BYTELEN, GFP_DMA | GFP_KERNEL)) == NULL) {//申请512K双字
		free_irq(pcifpga_irq, NULL);
		kfree(MsgBuffer);
		kfree(pcifpga_card_dma1);
		kfree(pcifpga_card_dma2);
		kfree(pcifpga_card_buf0);
		printk("TY-3012S PciDrv kmalloc pcifpga_card_buf1 Err!\n");
		return -ENOMEM;
	}
	memset(pcifpga_card_buf1, 0, PCIFPGA_BUF_BYTELEN);//缓冲区1清零
	ReadBuf1Pointer = WriteBuf1Pointer = Ch1PacketCnt = Ch1PacketDw = ReadCh1Dword = 0;//设置读写指针和包计数器为0
	printk("TY-3012S PciDrv pcifpga_card_buf1: %x!\n", (unsigned int)pcifpga_card_buf1);
	//为应用缓冲区2申请内存
	if((pcifpga_card_buf2 = kmalloc(PCIFPGA_BUF_BYTELEN, GFP_DMA | GFP_KERNEL)) == NULL) {//申请512K双字
		free_irq(pcifpga_irq, NULL);
		kfree(MsgBuffer);
		kfree(pcifpga_card_dma1);
		kfree(pcifpga_card_dma2);
		kfree(pcifpga_card_buf0);
		kfree(pcifpga_card_buf1);
		printk("TY-3012S PciDrv kmalloc pcifpga_card_buf2 Err!\n");
		return -ENOMEM;
	}
	memset(pcifpga_card_buf2, 0, PCIFPGA_BUF_BYTELEN);//缓冲区2清零
	ReadBuf2Pointer = WriteBuf2Pointer = Ch2PacketCnt = Ch2PacketDw = ReadCh2Dword = 0;//设置读写指针和包计数器为0
	printk("TY-3012S PciDrv pcifpga_card_buf2: %x!\n", (unsigned int)pcifpga_card_buf2);
	//确定系统IO的逻辑地址和范围
	SysRegaddr = ioremap(SYSREGADDR, SYSREGADDRLEN);//sysreg,1M地址空间
	return 0;
}

//移除PCI设备
static void __devexit pcifpga_remove(struct pci_dev *dev)
{
	/* clean up any allocated resources and stuff here.
	 * like call release_region();
	 */
	free_irq(pcifpga_irq, NULL);
	kfree(MsgBuffer);
	kfree(pcifpga_card_dma1);
	kfree(pcifpga_card_dma2);
	kfree(pcifpga_card_buf0);
	kfree(pcifpga_card_buf1);
	kfree(pcifpga_card_buf2);
	iounmap(SysRegaddr);//释放申请的虚拟地址sysreg
	return;
}

//设备模块信息
static struct pci_driver pcifpga_driver = {
	.name = CTD_PCI_NAME,		//设备模块名称
	.id_table = pcifpga_table,	//驱动设备表
	.probe = pcifpga_probe,		//查找并初始化设备
	.remove = pcifpga_remove,	//卸载设备模块
};

//设备PCIFPGA初始化模块
static int pcifpga_card_init(void)
{
	unsigned int i, j;

	//PCIFPGA测试卡初始化
	iowrite32(0x00000064, ChxBaseAddr0 + 4 * SW1TRIGGERTIME);	//RW01,D7～0开量入消抖时间,缺省1us,最小1us,最大255us(100us)
	iowrite32(0x00000064, ChxBaseAddr0 + 4 * SW2TRIGGERTIME);	//RW02,D7～0开量入消抖时间,缺省1us,最小1us,最大255us(100us)
	iowrite32(0x00000003, ChxBaseAddr0 + 4 * SWALARMOUT);		//RW03,D1～0告警出2和1,缺省为开,1闭合,0断开(缺省)
	iowrite32(0x00000002, ChxBaseAddr0 + 4 * BEEPALARMOUT);		//RW04,D1置1响一声(按键)后清除,D0置1长鸣,缺省不响(响一声)
	iowrite32(0x00000000, ChxBaseAddr0 + 4 * CHINTERFACESEL);	//RW05,D1～0监测通道1和2,缺省电,1光接口,0电接口(1光2电)
	iowrite32(0x00000001, ChxBaseAddr0 + 4 * GPSINTERFACESEL);	//RW06,D1置1选1588时钟,0选IRIGB口,D0置1平衡,0非平衡(缺省)
	iowrite32(0x000000ff, ChxBaseAddr0 + 4 * WARCHDOGOUT);		//RW07,D7～0置全1看门狗脉冲输出1次,然后自动清除D7～0数据
	iowrite32(0x55555555, ChxBaseAddr0 + 4 * LED1DISPLAY);		//RW08,前面板LED显示的低位双字,全部使用
	iowrite32(0x55555555, ChxBaseAddr0 + 4 * LED2DISPLAY);		//RW09,前面板LED显示的低位双字,部分使用
	iowrite32(INTTIMERV,  ChxBaseAddr0 + 4 * INTERRUPTTIMER);	//RW10,定时中断时间,D15～0设置1～65535ms,缺省0不中断(2ms)
	iowrite32(0x0000009f, ChxBaseAddr0 + 4 * INTERRUPTENABLE);	//RW11,D7总,D6/5第2/1路FIFO满,D4/3开入2/1,D2/1按键2/1,0定时
	iowrite32(0x00000000, ChxBaseAddr0 + 4 * IRIGB1588WRSECOND);	//RW12,D31-28秒十,D27-24个,D23-20毫百,D19-16十,D15-12个,D11-8微百,D7-4十,D3-0个
	iowrite32(0x00000000, ChxBaseAddr0 + 4 * IRIGB1588WRDAY);	//RW13,D25-24天百,D23-20十.D19～16个,D13-12时十,D11-8个,D7-4分十,D3～0个
	iowrite32(0x000000ff, ChxBaseAddr0 + 4 * IRIGB1588TIMEPUT);	//RW14,D7～0置全1,把0x22和0x23地址中时间置入计时器,D7～D0写后自清除
	iowrite32(0x000000ff, ChxBaseAddr0 + 4 * CH1FIFORESET);		//OW15,D7～0置全1报文存储FIFO复位,状态和计数器复位,后自动清除
	iowrite32(0x000000ff, ChxBaseAddr0 + 4 * CH2FIFORESET);		//OW16,D7～0置全1报文存储FIFO复位,状态和计数器复位,后自动清除
	iowrite32(0x000000ff, ChxBaseAddr0 + 4 * CH1FIFOCOUNT);		//RW17,取FIFO计数,D29-16为读[13:0],D13-0为写[13:0],读前写0xff,延时1us,再读
	iowrite32(0x000000ff, ChxBaseAddr0 + 4 * CH2FIFOCOUNT);		//RW18,取FIFO计数,D29-16为读[13:0],D13-0为写[13:0],读前写0xff,延时1us,再读
	iowrite32(0x000000ff, ChxBaseAddr0 + 4 * IRIGB1588TIMEGET);	//RW19,D7～0置全1,把当前计时器时间置入0x26和0x27地址,D7～0写后自清除
	i = ioread32(ChxBaseAddr0 + 4 * SW1TRIGGERTIME);
	printk("TY-3012S PciDrv SW1TRIGGERTIME:  %08x!\n", i);//01
	i = ioread32(ChxBaseAddr0 + 4 * SW2TRIGGERTIME);
	printk("TY-3012S PciDrv SW2TRIGGERTIME : %08x!\n", i);//02
	i = ioread32(ChxBaseAddr0 + 4 * SWALARMOUT);
	printk("TY-3012S PciDrv SWALARMOUT:      %08x!\n", i);//03
	i = ioread32(ChxBaseAddr0 + 4 * BEEPALARMOUT);
	printk("TY-3012S PciDrv BEEPALARMOUT:    %08x!\n", i);//04
	i = ioread32(ChxBaseAddr0 + 4 * CHINTERFACESEL);
	printk("TY-3012S PciDrv CHINTERFACESEL:  %08x!\n", i);//05
	i = ioread32(ChxBaseAddr0 + 4 * GPSINTERFACESEL);
	printk("TY-3012S PciDrv GPSINTERFACESEL: %08x!\n", i);//06
	i = ioread32(ChxBaseAddr0 + 4 * SW1STATUS);
	printk("TY-3012S PciDrv SW1STATUS:       %08x!\n", i);//07
	i = ioread32(ChxBaseAddr0 + 4 * SW2STATUS);
	printk("TY-3012S PciDrv SW2STATUS:       %08x!\n", i);//08
	i = ioread32(ChxBaseAddr0 + 4 * KEY1STATUS);
	printk("TY-3012S PciDrv KEY1STATUS:      %08x!\n", i);//09
	i = ioread32(ChxBaseAddr0 + 4 * KEY2STATUS);
	printk("TY-3012S PciDrv KEY2STATUS:      %08x!\n", i);//10
	i = ioread32(ChxBaseAddr0 + 4 * LED1DISPLAY);
	printk("TY-3012S PciDrv LED1DISPLAY:     %08x!\n", i);//11
	i = ioread32(ChxBaseAddr0 + 4 * LED2DISPLAY);
	printk("TY-3012S PciDrv LED2DISPLAY:     %08x!\n", i);//12
	i = ioread32(ChxBaseAddr0 + 4 * INTERRUPTTIMER);
	printk("TY-3012S PciDrv INTERRUPTTIMER:  %08x!\n", i);//13
	i = ioread32(ChxBaseAddr0 + 4 * INTERRUPTENABLE);
	printk("TY-3012S PciDrv INTERRUPTENABLE: %08x!\n", i);//14
	i = ioread32(ChxBaseAddr0 + 4 * INTERRUPTSTATUS);
	printk("TY-3012S PciDrv INTERRUPTSTATUS: %08x!\n", i);//15
	i = ioread32(ChxBaseAddr0 + 4 * IRIGB1588STATUS);
	printk("TY-3012S PciDrv IRIGB1588STATUS: %08x!\n", i);//16
	i = ioread32(ChxBaseAddr0 + 4 * IRIGB1588TIMEERR);
	printk("TY-3012S PciDrv IRIGB1588ERR:    %08x!\n", i);//17
	i = ioread32(ChxBaseAddr0 + 4 * IRIGB1588RDSECOND);
	printk("TY-3012S PciDrv IRIGB1588RDS:    %08x!\n", i);//18
	i = ioread32(ChxBaseAddr0 + 4 * IRIGB1588RDDAY);
	printk("TY-3012S PciDrv IRIGB1588RDDAY:  %08x!\n", i);//19
	i = ioread32(ChxBaseAddr0 + 4 * CH1FIFOSTATUS);
	printk("TY-3012S PciDrv CH1FIFOSTATUS:   %08x!\n", i);//20
	i = ioread32(ChxBaseAddr0 + 4 * CH2FIFOSTATUS);
	printk("TY-3012S PciDrv CH2FIFOSTATUS:   %08x!\n", i);//21
	i = ioread32(ChxBaseAddr0 + 4 * CH1FIFOCOUNT);
	printk("TY-3012S PciDrv CH1FIFOCOUNT:    %08x!\n", i);//22
	i = ioread32(ChxBaseAddr0 + 4 * CH2FIFOCOUNT);
	printk("TY-3012S PciDrv CH2FIFOCOUNT:    %08x!\n", i);//23
	//监测通道1的过滤寄存器写
	iowrite32(0x00000001, Ch1BaseAddr1 + 4 * CHFILTERMODE);		//RW,D0=0Fileter,=1NoFilerer,D1=0ReceiveAllBroadcast,=1RejiectAllBroadcast
	iowrite32(0x0A5A5A5A5,Ch1BaseAddr1 + 4 * CHFILTERRESET);	//RW,写0xA5A5A5A5复位除寄存器模块外的所有模块,配置完之后做1次
	iowrite32(0x00000040, Ch1BaseAddr1 + 4 * CHFILTERMINLEN);	//RW,最短帧长,D15～0写入MinimumEthernetPacket,缺省64
	iowrite32(0x00000600, Ch1BaseAddr1 + 4 * CHFILTERMAXLEN);	//RW,最长帧长,D15～0写入MaximumEthernetPacket,缺省1536
	iowrite32(0x000000c0, Ch1BaseAddr1 + 4 * CHFILTERENABLE);	//RW,1=e,0=d,D0～9:VLAN1,VLAN2,GOOSE,SMV,MMS,类型,MAC源,VLAN过滤,MAC源与类型,VLAN与类型
	iowrite32(0x00000555, Ch1BaseAddr1 + 4 * CHFILTERVID1);		//RW,D11～0,VlanID1
	iowrite32(0x00000aaa, Ch1BaseAddr1 + 4 * CHFILTERVID2);		//RW,D11～0,VlanID1
	iowrite32(0x0000ffff, Ch1BaseAddr1 + 4 * CHFILTERMACEN);	//RW,D31～0,SourceMacAdd,31～00bit
	//for(i = 0; i < 16; i++) {
		iowrite32(0x00001122+0, Ch1BaseAddr1 + 4*(CHFILTERMACH+0));	//RW,D15～0,SourceMacAdd,47～32bit
		iowrite32(0x33445500+0, Ch1BaseAddr1 + 4*(CHFILTERMACL+0));	//RW,D31～0,SourceMacAdd,31～00bit
		iowrite32(0x00001122+0, Ch1BaseAddr1 + 4*(CHFILTERMACH+2));	//RW,D15～0,SourceMacAdd,47～32bit
		iowrite32(0x33445501+0, Ch1BaseAddr1 + 4*(CHFILTERMACL+2));	//RW,D31～0,SourceMacAdd,31～00bit
		iowrite32(0x00001122+0, Ch1BaseAddr1 + 4*(CHFILTERMACH+4));	//RW,D15～0,SourceMacAdd,47～32bit
		iowrite32(0x33445502+0, Ch1BaseAddr1 + 4*(CHFILTERMACL+4));	//RW,D31～0,SourceMacAdd,31～00bit
		iowrite32(0x00001122+0, Ch1BaseAddr1 + 4*(CHFILTERMACH+6));	//RW,D15～0,SourceMacAdd,47～32bit
		iowrite32(0x33445503+0, Ch1BaseAddr1 + 4*(CHFILTERMACL+6));	//RW,D31～0,SourceMacAdd,31～00bit
	//}
	//监测通道2的过滤寄存器写
	iowrite32(0x00000001, Ch2BaseAddr2 + 4 * CHFILTERMODE);		//RW,D0=0Fileter,=1NoFilerer,D1=0ReceiveAllBroadcast,=1RejiectAllBroadcast
	iowrite32(0x0A5A5A5A5,Ch2BaseAddr2 + 4 * CHFILTERRESET);	//RW,写0xA5A5A5A5复位除寄存器模块外的所有模块,配置完之后做1次
	iowrite32(0x00000040, Ch2BaseAddr2 + 4 * CHFILTERMINLEN);	//RW,最短帧长,D15～0写入MinimumEthernetPacket,缺省64
	iowrite32(0x00000600, Ch2BaseAddr2 + 4 * CHFILTERMAXLEN);	//RW,最长帧长,D15～0写入MaximumEthernetPacket,缺省1536
	iowrite32(0x000000c0, Ch2BaseAddr2 + 4 * CHFILTERENABLE);	//RW,1=e,0=d,D0～9:VLAN1,VLAN2,GOOSE,SMV,MMS,类型,MAC源,VLAN过滤,MAC源与类型,VLAN与类型
	iowrite32(0x00000555, Ch2BaseAddr2 + 4 * CHFILTERVID1);		//RW,D11～0,VlanID1
	iowrite32(0x00000aaa, Ch2BaseAddr2 + 4 * CHFILTERVID2);		//RW,D11～0,VlanID1
	iowrite32(0x0000ffff, Ch2BaseAddr2 + 4 * CHFILTERMACEN);	//RW,D31～0,SourceMacAdd,31～00bit
	//for(i = 0; i < 16; i++) {
		iowrite32(0x00006677+0, Ch2BaseAddr2 + 4*(CHFILTERMACH+0));	//RW,D15～0,SourceMacAdd,47～32bit
		iowrite32(0x8899aa00+0, Ch2BaseAddr2 + 4*(CHFILTERMACL+0));	//RW,D31～0,SourceMacAdd,31～00bit
		iowrite32(0x00006677+0, Ch2BaseAddr2 + 4*(CHFILTERMACH+2));	//RW,D15～0,SourceMacAdd,47～32bit
		iowrite32(0x8899aa01+0, Ch2BaseAddr2 + 4*(CHFILTERMACL+2));	//RW,D31～0,SourceMacAdd,31～00bit
		iowrite32(0x00006677+0, Ch2BaseAddr2 + 4*(CHFILTERMACH+4));	//RW,D15～0,SourceMacAdd,47～32bit
		iowrite32(0x8899aa02+0, Ch2BaseAddr2 + 4*(CHFILTERMACL+4));	//RW,D31～0,SourceMacAdd,31～00bit
		iowrite32(0x00006677+0, Ch2BaseAddr2 + 4*(CHFILTERMACH+6));	//RW,D15～0,SourceMacAdd,47～32bit
		iowrite32(0x8899aa03+0, Ch2BaseAddr2 + 4*(CHFILTERMACL+6));	//RW,D31～0,SourceMacAdd,31～00bit
	//}
	//监测通道1的过滤寄存器读
	i = ioread32(Ch1BaseAddr1 + 4 * CHFILTERMODE);
	printk("TY-3012S PciDrv CH1FILTERMODE:   %08x!\n", i);
	i = ioread32(Ch1BaseAddr1 + 4 * CHFILTERRESET);
	printk("TY-3012S PciDrv CH1FILTERRESET:  %08x!\n", i);
	i = ioread32(Ch1BaseAddr1 + 4 * CHFILTERMINLEN);
	printk("TY-3012S PciDrv CH1FILTERMINLEN: %08x!\n", i);
	i = ioread32(Ch1BaseAddr1 + 4 * CHFILTERMAXLEN);
	printk("TY-3012S PciDrv CH1FILTERMAXLEN: %08x!\n", i);
	i = ioread32(Ch1BaseAddr1 + 4 * CHFILTERENABLE);
	printk("TY-3012S PciDrv CH1FILTERENABLE: %08x!\n", i);
	i = ioread32(Ch1BaseAddr1 + 4 * CHFILTERVID1);
	printk("TY-3012S PciDrv CH1FILTERVID1:   %08x!\n", i);
	i = ioread32(Ch1BaseAddr1 + 4 * CHFILTERVID2);
	printk("TY-3012S PciDrv CH1FILTERVID2:   %08x!\n", i);
	i = ioread32(Ch1BaseAddr1 + 4 * CHFILTERMACEN);
	printk("TY-3012S PciDrv CH1FILTERMACEN:  %08x!\n", i);
	//for(i = 0; i < 16; i++) {
		j = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACH+0));
		printk("TY-3012S PciDrv CH1FILTERMAC%02xH: %08x!\n", 0, j);
		j = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACL+0));
		printk("TY-3012S PciDrv CH1FILTERMAC%02xL: %08x!\n", 0, j);
		j = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACH+2));
		printk("TY-3012S PciDrv CH1FILTERMAC%02xH: %08x!\n", 1, j);
		j = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACL+2));
		printk("TY-3012S PciDrv CH1FILTERMAC%02xL: %08x!\n", 1, j);
		j = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACH+4));
		printk("TY-3012S PciDrv CH1FILTERMAC%02xH: %08x!\n", 2, j);
		j = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACL+4));
		printk("TY-3012S PciDrv CH1FILTERMAC%02xL: %08x!\n", 2, j);
		j = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACH+6));
		printk("TY-3012S PciDrv CH1FILTERMAC%02xH: %08x!\n", 3, j);
		j = ioread32(Ch1BaseAddr1 + 4*(CHFILTERMACL+6));
		printk("TY-3012S PciDrv CH1FILTERMAC%02xL: %08x!\n", 3, j);
	//}
	//监测通道2的过滤寄存器读
	i = ioread32(Ch2BaseAddr2 + 4 * CHFILTERMODE);
	printk("TY-3012S PciDrv CH2FILTERMODE:   %08x!\n", i);
	i = ioread32(Ch2BaseAddr2 + 4 * CHFILTERRESET);
	printk("TY-3012S PciDrv CH2FILTERRESET:  %08x!\n", i);
	i = ioread32(Ch2BaseAddr2 + 4 * CHFILTERMINLEN);
	printk("TY-3012S PciDrv CH2FILTERMINLEN: %08x!\n", i);
	i = ioread32(Ch2BaseAddr2 + 4 * CHFILTERMAXLEN);
	printk("TY-3012S PciDrv CH2FILTERMAXLEN: %08x!\n", i);
	i = ioread32(Ch2BaseAddr2 + 4 * CHFILTERENABLE);
	printk("TY-3012S PciDrv CH2FILTERENABLE: %08x!\n", i);
	i = ioread32(Ch2BaseAddr2 + 4 * CHFILTERVID1);
	printk("TY-3012S PciDrv CH2FILTERVID1:   %08x!\n", i);
	i = ioread32(Ch2BaseAddr2 + 4 * CHFILTERVID2);
	printk("TY-3012S PciDrv CH2FILTERVID2:   %08x!\n", i);
	i = ioread32(Ch2BaseAddr2 + 4 * CHFILTERMACEN);
	printk("TY-3012S PciDrv CH2FILTERMACEN:  %08x!\n", i);
	//for(i = 0; i < 16; i++) {
		j = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACH+0));
		printk("TY-3012S PciDrv CH2FILTERMAC%02xH: %08x!\n", 0, j);
		j = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACL+0));
		printk("TY-3012S PciDrv CH2FILTERMAC%02xL: %08x!\n", 0, j);
		j = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACH+2));
		printk("TY-3012S PciDrv CH2FILTERMAC%02xH: %08x!\n", 1, j);
		j = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACL+2));
		printk("TY-3012S PciDrv CH2FILTERMAC%02xL: %08x!\n", 1, j);
		j = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACH+4));
		printk("TY-3012S PciDrv CH2FILTERMAC%02xH: %08x!\n", 2, j);
		j = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACL+4));
		printk("TY-3012S PciDrv CH2FILTERMAC%02xL: %08x!\n", 2, j);
		j = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACH+6));
		printk("TY-3012S PciDrv CH2FILTERMAC%02xH: %08x!\n", 3, j);
		j = ioread32(Ch2BaseAddr2 + 4*(CHFILTERMACL+6));
		printk("TY-3012S PciDrv CH2FILTERMAC%02xL: %08x!\n", 3, j);
	//}
	return 0;
}

/* 初始化并注册cdev */
static void pcifpga_setup_cdev(struct pcifpga_dev *dev, int index)
{
	int err, devno = MKDEV(PCIFPGA_MAJOR, index);

	cdev_init(&dev->cdev, &pcifpga_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &pcifpga_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) printk("TY-3012S pcifpga_setup_cdev err %d adding devno %d!\n", err, index);
}

//注册PCI硬件驱动程序
static int __init pcifpga_init(void)
{
	int status, result;
	dev_t devno;

	printk("TY-3012S PciDrv Ver1.0 Copyright!\n");//注册PCIFPGA硬件驱动程序开始
	//注册CHAR读写函数
	devno = MKDEV(PCIFPGA_MAJOR, 0);
	result = register_chrdev_region(devno, 1, "pcifpga");
	if (result < 0) {
		printk("TY-3012S PciDrv register_chrdev_region fail: %d!\n", PCIFPGA_MAJOR);
		return result;
	}
	printk("TY-3012S register_chrdev_region success: %d!\n", PCIFPGA_MAJOR);
	pcifpga_devp = kmalloc(sizeof(struct pcifpga_dev), GFP_KERNEL);//动态申请设备结构体的内存
	if (!pcifpga_devp) {//申请失败
		printk("TY-3012S PciDrv kmalloc pcifpga_dev fail!\n");
		result = -ENOMEM;
		unregister_chrdev_region(devno, 1);
		return result;
	}
	memset(pcifpga_devp, 0, sizeof(struct pcifpga_dev));
	pcifpga_setup_cdev(pcifpga_devp, 0);
	printk("TY-3012S PciDrv kmalloc pcifpga_dev success!\n");//Setup pcifpga cdev.
	//完成注册CHAR
	/* 注册PCIFPGA硬件驱动程序 */
	printk("TY-3012S PciDrv Ver1.0 Init!\n");//注册PCIFPGA硬件驱动程序开始
	status = pci_register_driver(&pcifpga_driver);//注册驱动程序
	printk("TY-3012S PciDrv pci_register_driver: %x!\n", status);
	if(status) {
		printk("TY-3012S PciDrv pci_register_driver Err!\n");
		pci_unregister_driver(&pcifpga_driver);
		kfree(pcifpga_devp);//处理字符注册
		unregister_chrdev_region(devno, 1);//处理字符注册
		return -ENODEV;
	}
	printk("TY-3012S PciDrv is loaded successfully!\n");//注册驱动程序成功
	mdelay(10);//延时10毫秒
	printk ("TY-3012S PciDrv m_second: %d!\n", m_second);
	pcifpga_card_init();
	return 0;
	/* ... */
	return pci_register_driver(&pcifpga_driver);
}

static void __exit pcifpga_exit(void)
{
	cdev_del(&pcifpga_devp->cdev);/* 注销cdev */
	kfree(pcifpga_devp);/* 释放设备结构体内存 */
	unregister_chrdev_region(MKDEV(PCIFPGA_MAJOR, 0), 1);/* 释放设备号 */
	printk("TY-3012S PciDrv Ver1.0 Exit!\n");
	pci_unregister_driver(&pcifpga_driver);
}

MODULE_AUTHOR("shenxj <shenxj@datanggroup.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Pci Driver for TY-3012S");
module_init(pcifpga_init);
module_exit(pcifpga_exit);
