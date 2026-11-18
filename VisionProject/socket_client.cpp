#define _WINSOCK_DEPRECATED_NO_WARNINGS   //新增：允许用 inet_addr（老师代码用的是它）
#include "socket_client.h"
#include<iostream>
#include<cstring>
#include<windows.h>
#include"VisionProject.h"

bool SocketClient::createSocketClient()
{
	//初始化socket库
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		std::cout<<"初始化socket库失败"<<std::endl;
		return false;
	}
	//建立客户端socket
	clientsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientsocket == INVALID_SOCKET) {
		std::cout << "创建套接字失败" << std::endl;
		closesocket(clientsocket);
		clientsocket = 0;
		return false;
	}

	//定义要连接的服务器地址
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_nport);   //新增：端口从成员取（默认8888）
	addr.sin_addr.S_un.S_addr = inet_addr(m_ip.c_str());
	
	int ret = connect(clientsocket, (SOCKADDR*)&addr, sizeof(addr));
	if (ret == SOCKET_ERROR) {
		std::cout << "客户端建立链接失败" << std::endl;
		closesocket(clientsocket);
		clientsocket = 0;
		return false;
	}
	std::cout << "客户端建立链接成功" << std::endl;

	//新增：连上了通知主窗口（主窗口会把账号密码发过去）
	if (connectCallBack != nullptr) {
		connectCallBack(m_pmainwin);
	}
	return true;   //新增：原来这里漏了返回值
}

bool SocketClient::sendData(const char* sendbuf)
{
	int sendRes = send(clientsocket, sendbuf, (int)strlen(sendbuf), 0);
	if (sendRes == SOCKET_ERROR) {
		std::cout << "客户端send()出现错误: " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}

//新增：收数据。连上 -> 一直recv -> 每收到一条就回调主窗口（将来放进线程里跑）
void SocketClient::recvData(void* p)
{
	int recv_len;
	char recv_buf[1024];
	SocketClient* ptr = (SocketClient*)p;
	while (1) {
		if (ptr->m_recvExitFlag) {
			return;
		}
		if (ptr->createSocketClient()) {
			while (1) {
				recv_len = recv(ptr->clientsocket, recv_buf, sizeof(recv_buf) - 1, 0);
				if (recv_len == SOCKET_ERROR) {
					std::perror("recv fail");
					break;
				}
				if (recv_len == 0) {
					std::perror("offline");
					closesocket(ptr->clientsocket);
					break;
				}
				recv_buf[recv_len] = '\0';   //补上结束符，避免打印越界
				if (ptr->recvCallBack != nullptr) {
					ptr->recvCallBack(ptr->m_pmainwin, recv_buf);
					memset(recv_buf, 0, sizeof(recv_buf));
				}
			}
		}
		else {
			Sleep(500);   //连不上就等半秒再试
		}
	}
}

//新增：心跳，每10秒发一次（将来放进线程里跑）
void SocketClient::keepAlive(void* p)
{
	SocketClient* ptr = (SocketClient*)p;
	while (true) {
		if (ptr->m_heartExitFlag) {
			std::cout << "keepAlive thread func exit" << std::endl;
			break;
		}
		Sleep(10000);
		if (ptr->heartCallBack != nullptr) {
			ptr->heartCallBack(ptr->m_pmainwin);
		}
	}
}
