#ifndef	__MAIN_H__
#define 	__MAIN_H__
//0、本主程序1包括如下部分
//1、和主程序2进行通信部分，通过UDP进行，UDP端口=10002
//2、读取配置文件IDPROM和XML，配置相应的UDP客户端、TCP端口
//3、TCP端口1：53456，实时SV/SMV数据上报通道，（SMV Data Port）
//4、TCP端口2：53457，实时告警数据上报通道（RealTime Alarm Data Port）
//5、TCP端口3：53458，配置数据通道（XML Config Data Port）
//6、TCP端口4：53459，历史文件上传通道（History Data Port）
//7、TCP端口5：53460，TCP备用端口
//8、TCP端口6：53461，系统命令端口（System Command Port）
//9：UDP端口1：10001，装置扫描数据通过该端口传输
//a：UDP端口2：10001，主程序1和主程序2通过该端口传输命令和消息
//b：UDP端口3：30718，UDP备用端口，用于功能扩展
//c：接收主程序2发送过来的系统命令消息，对测试硬件进行查询
//a：建立定时线程，为存盘程序提供定时
//d、定时读取SV/SMV/GOOSE/MMS报文，并存入大容量硬盘
//e、上报SV/SMV数据
//f、上报实时告警信息
	



#endif
