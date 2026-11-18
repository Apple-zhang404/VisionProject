#pragma once
#include<WinSock2.h>
#include<string>
#pragma comment(lib,"ws2_32.lib")

//新增：三个回调类型（连接成功 / 收到数据 / 心跳）
typedef void(*heart_callback)(void*);
typedef void(*connect_callback)(void*);
typedef void(*recv_callback)(void*, char[]);

class VisionProject;   //新增：前置声明

class SocketClient {
public:
	SocketClient() {}
	~SocketClient(){}
public:
	bool createSocketClient();
	bool sendData(const char* sendbuf);   //新增：改成const，可以直接传string的c_str()

	//新增：收数据和心跳（这两个将来放进线程里跑）
	static void recvData(void* p);
	static void keepAlive(void* p);

	//新增：设置IP端口、注册三个回调、把主窗口指针存起来
	void setSocketPara(std::string ip, unsigned int port) {
		m_ip = ip;
		m_nport = port;
	}
	void registerRecvCallbackFunc(recv_callback recvfunc) {
		recvCallBack = recvfunc;
	}
	void registerHeartCallbackFunc(heart_callback heartfunc) {
		heartCallBack = heartfunc;
	}
	void registerConnCallbackFunc(connect_callback connfunc) {
		connectCallBack = connfunc;
	}
	void setMainWinPtr(void* p) {
		m_pmainwin = (VisionProject*)p;
	}
private:
	int m_nport = 8888;
	std::string m_ip;
	SOCKET clientsocket = INVALID_SOCKET;
	//新增：三个回调指针、主窗口指针、两个线程退出标志
	recv_callback recvCallBack = nullptr;
	heart_callback heartCallBack = nullptr;
	connect_callback connectCallBack = nullptr;
	VisionProject* m_pmainwin = nullptr;
	bool m_recvExitFlag = false;
	bool m_heartExitFlag = false;
};
