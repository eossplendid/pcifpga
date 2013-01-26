#ifndef	__DEBUG_CONSOLE_H__
#define	__DEBUG_CONSOLE_H__

typedef struct
{
	long mtype;			/*消息类型*/
	int   request;			/*命令*/
	struct msqid_ds msgInfo;	/*查询队列状态参数*/
	unsigned long cnt;			/*总包数*/

}MsgDebugConsole;

#define	TYPE_DEBUGCONSOLE_CMD			4		/*telnet调试队列消息类型*/
#define	TYPE_DEBUGCONSOLE_RSP			5		/*回应telnet发来的命令*/

							/*调试线程不加打印,该宏为空*/
#define	DEBUG_CMD_QUERY_DMMSG			1		/*磁盘管理消息队列状态查询*/
#define	DEBUG_CMD_QUERY_APPMSG			2		/*APP读包消息队列状态查询*/
#define	DEBUG_CMD_QUERY_PLHMSG			3		/*告警分析消息队列状态查询*/
#define	DEBUG_CMD_PACKETCNT				4		/*查询当前所收包数总和*/

#endif
