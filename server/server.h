#pragma once

#include<winsock2.h>
#include<MSWSock.h>
#include<iostream>
#include <winnt.h>
#include<list>
#include<vector>
#include<string>
#include<mutex>
#pragma comment(lib,"ws2_32")
#pragma comment(lib,"atls.lib")

#include<tchar.h>
//#include <atlstr.h>
//#include <atltime.h>
#include <locale.h>
//使用系统安全字符串函数支持
#include <strsafe.h>
//使用ATL的字符集转换支持
//#include <atlconv.h>

using std::vector;
using std::list;
using std::wstring;

#define BUFF_SIZE (1024*4)						// I/O 请求的缓冲区大小
#define WORKER_THREADS_PER_PROCESSOR (2)		// 每个处理器上的线程数
#define MAX_POST_ACCEPT (10)					// 同时投递的Accept数量
#define INIT_IOCONTEXT_NUM (100)				// IOContextPool中的初始数量
#define EXIT_CODE	(-1)						// 传递给Worker线程的退出信号
#define DEFAULT_IP	(L"127.0.0.1")				// 默认IP地址
#define DEFAULT_PORT	(10240)					// 默认端口




// 释放指针的宏
#define RELEASE(x)			{if(x != NULL) {delete x; x = NULL;}}
// 释放句柄的宏
#define RELEASE_HANDLE(x)	{if(x != NULL && x != INVALID_HANDLE_VALUE) { CloseHandle(x); x = INVALID_HANDLE_VALUE; }}
// 释放Socket的宏
#define RELEASE_SOCKET(x)	{if(x != INVALID_SOCKET) { closesocket(x); x = INVALID_SOCKET; }}

enum IO_OPERATION_TYPE
{
	NULL_POSTED,		// 用于初始化，无意义
	ACCEPT_POSTED,		// 投递Accept操作
	SEND_POSTED,		// 投递Send操作
	RECV_POSTED,		// 投递Recv操作
};

class IOContext
{
public:
	WSAOVERLAPPED		overLapped;		// 每个socket的每一个IO操作都需要一个重叠结构
	SOCKET				ioSocket;		// 此IO操作对应的socket
	WSABUF				wsaBuf;			// 数据缓冲
	IO_OPERATION_TYPE	ioType;			// IO操作类型
	UINT				connectID;		// 连接ID

	IOContext()
	{
		ZeroMemory(&overLapped, sizeof(overLapped));
		ioSocket = INVALID_SOCKET;
		wsaBuf.buf = (char *)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUFF_SIZE);
		wsaBuf.len = BUFF_SIZE;
		ioType = NULL_POSTED;
		connectID = 0;
	}

	~IOContext()
	{
		RELEASE_SOCKET(ioSocket);

		if (wsaBuf.buf != NULL)
			::HeapFree(::GetProcessHeap(), 0, wsaBuf.buf);
	}

	void Reset()
	{
		if (wsaBuf.buf != NULL)
			//ZeroMemory(wsaBuf.buf, BUFF_SIZE);
			//::HeapFree(::GetProcessHeap(), 0, wsaBuf.buf);
			wsaBuf.buf = (char *)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUFF_SIZE);
		else
			wsaBuf.buf = (char *)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUFF_SIZE);
		ZeroMemory(&overLapped, sizeof(overLapped));
		ioType = NULL_POSTED;
		connectID = 0;
	}
};

// 空闲的IOContext管理类(IOContext池)
class IOContextPool
{
private:
	list<IOContext *> contextList;
	CRITICAL_SECTION csLock;

public:
	IOContextPool()
	{
		InitializeCriticalSection(&csLock);
		contextList.clear();

		EnterCriticalSection(&csLock);
		for (size_t i = 0; i < INIT_IOCONTEXT_NUM; i++)
		{
			IOContext *context = new IOContext;
			contextList.push_back(context);
		}
		LeaveCriticalSection(&csLock);
		
	}

	~IOContextPool()
	{
		EnterCriticalSection(&csLock);
		for (list<IOContext *>::iterator it = contextList.begin(); it != contextList.end(); it++)
		{
			delete (*it);
		}
		contextList.clear();
		LeaveCriticalSection(&csLock);

		DeleteCriticalSection(&csLock);
	}

	// 分配一个IOContxt
	IOContext *AllocateIoContext()
	{
		IOContext *context = NULL;

		EnterCriticalSection(&csLock);
		if (contextList.size() > 0) //list不为空，从list中取一个
		{
			context = contextList.back();
			contextList.pop_back();
		}
		else	//list为空，新建一个
		{
			context = new IOContext;
		}
		LeaveCriticalSection(&csLock);

		return context;
	}

	// 回收一个IOContxt
	void ReleaseIOContext(IOContext *pContext)
	{
		pContext->Reset();
		EnterCriticalSection(&csLock);
		contextList.push_front(pContext);
		LeaveCriticalSection(&csLock);
	}
};

class SocketContext
{
public:
	SOCKET connSocket;								// 连接的socket
	SOCKADDR_IN clientAddr;							// 连接的远程地址

private:
	vector<IOContext*> arrIoContext;				// 同一个socket上的多个IO请求
	static IOContextPool ioContextPool;				// 空闲的IOContext池
	CRITICAL_SECTION csLock;

public:
	SocketContext()
	{
		InitializeCriticalSection(&csLock);
		arrIoContext.clear();
		connSocket = INVALID_SOCKET;
		ZeroMemory(&clientAddr, sizeof(clientAddr));
	}

	~SocketContext()
	{
		RELEASE_SOCKET(connSocket);

		// 回收所有的IOContext
		for (vector<IOContext*>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
		{
			ioContextPool.ReleaseIOContext(*it);
		}

		EnterCriticalSection(&csLock);
		arrIoContext.clear();
		LeaveCriticalSection(&csLock);

		DeleteCriticalSection(&csLock);
	}

	// 获取一个新的IoContext
	IOContext *GetNewIOContext()
	{
		IOContext *context = ioContextPool.AllocateIoContext();
		if (context != NULL)
		{
			EnterCriticalSection(&csLock);
			arrIoContext.push_back(context);
			LeaveCriticalSection(&csLock);
		}
		return context;
	}

	// 从数组中移除一个指定的IoContext
	void RemoveContext(IOContext* pContext)
	{
		for (vector<IOContext*>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
		{
			if (pContext == *it)
			{
				ioContextPool.ReleaseIOContext(*it);

				EnterCriticalSection(&csLock);
				arrIoContext.erase(it);
				LeaveCriticalSection(&csLock);

				break;
			}
		}
	}

	// 
};

// IOCP基类
class IOCPBase
{
public:
	IOCPBase();
	~IOCPBase();

	// 开始服务
	BOOL Start(int port = 10240, int maxConn = 2000, int maxIOContextInPool = 256, 
		int maxSocketContextInPool = 200);
	// 停止服务
	void Stop();
	// 向指定客户端发送数据
	BOOL SendData(SocketContext *socketContext, char *data, int size);

	// 获取本机的IP
	wstring GetLocalIP();

	// 获取当前连接数
	ULONG GetConnectCnt() { return connectCnt;  }

	// 获取当前连接数
	UINT GetPort() { return port; }

	// 事件通知函数(派生类重载此族函数)
	// 新连接
	virtual void OnConnectionEstablished(SocketContext *sockContext) = 0;
	// 连接关闭
	virtual void OnConnectionClosed(SocketContext *sockContext) = 0;
	// 连接上发生错误
	virtual void OnConnectionError(SocketContext *sockContext, int error) = 0;
	// 读操作完成
	virtual void OnRecvCompleted(SocketContext *sockContext, IOContext *ioContext) = 0;
	// 写操作完成
	virtual void OnSendCompleted(SocketContext *sockContext, IOContext *ioContext) = 0;

protected:
	HANDLE					stopEvent;				// 通知线程退出的时间
	HANDLE					completionPort;			// 完成端口
	HANDLE					*workerThreads;			// 工作者线程的句柄指针
	int						workerThreadNum;		// 工作者线程的数量
	wstring					IP;						// 本地IP
	int						port;					// 监听端口
	SocketContext			*listenSockContext;		// 监听socket的Context
	LONG					connectCnt;				// 当前的连接数量
	LONG					acceptPostCnt;			// 当前投递的的Accept数量

	LPFN_ACCEPTEX			fnAcceptEx;				//AcceptEx函数指针
	//GetAcceptExSockAddrs;函数指针
	LPFN_GETACCEPTEXSOCKADDRS	fnGetAcceptExSockAddrs;

private:
	static DWORD WINAPI WorkerThreadProc(LPVOID lpParam); // 工作线程函数

	// 初始化IOCP
	BOOL InitializeIOCP();
	// 初始化Socket
	BOOL InitializeListenSocket();
	// 释放资源
	void DeInitialize();
	// socket是否存活
	BOOL IsSocketAlive(SOCKET sock);
	// 获取本机CPU核心数
	int GetNumOfProcessors();
	// 将句柄(Socket)绑定到完成端口中
	BOOL AssociateWithIOCP(SocketContext *sockContext);
	// 投递IO请求
	BOOL PostAccept(SocketContext *sockContext, IOContext *ioContext);
	BOOL PostRecv(SocketContext *sockContext, IOContext *ioContext);
	BOOL PostSend(SocketContext *sockContext, IOContext *ioContext);

	// IO处理函数
	BOOL DoAccpet(SocketContext *sockContext, IOContext *ioContext);
	BOOL DoRecv(SocketContext *sockContext, IOContext *ioContext);
	BOOL DoSend(SocketContext *sockContext, IOContext *ioContext);
	BOOL DoClose(SocketContext *sockContext);
};
