//==============================================================
// vision_server.cpp  —— 视觉上位机联调用的 TCP 服务端
//   编译: cl /EHsc /std:c++17 /utf-8 vision_server.cpp
//   运行: vision_server.exe          （默认监听 8888）
//   功能: 1) 收登录包并回执
//         2) 收心跳并打印
//         3) 收到 {"image":"<base64>"} 时解码存成 jpg
//         4) 控制台输入任意文字 + 回车 → 广播给所有客户端
//==============================================================
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>

#pragma comment(lib,"ws2_32.lib")

static std::vector<SOCKET> g_clients;
static std::mutex g_mtx;
static int g_imgIndex = 0;

//---------------- 日志 ----------------
static void logmsg(const std::string& s) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    struct tm lt; localtime_s(&lt, &t);
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", &lt);
    std::cout << "[" << buf << "] " << s << std::endl;
}

//---------------- Base64 解码 ----------------
static std::string base64_decode(const std::string& in) {
    static const std::string table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, bits = -8;
    for (unsigned char c : in) {
        if (c == '=') break;
        size_t pos = table.find(c);
        if (pos == std::string::npos) continue;
        val = (val << 6) + (int)pos;
        bits += 6;
        if (bits >= 0) {
            out.push_back(char((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

//---------------- 从JSON里取字符串字段 ----------------
static std::string getField(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    size_t p = json.find(pat);
    if (p == std::string::npos) return "";
    p = json.find(':', p + pat.size());
    if (p == std::string::npos) return "";
    p = json.find('"', p);
    if (p == std::string::npos) return "";
    size_t e = json.find('"', p + 1);
    if (e == std::string::npos) return "";
    return json.substr(p + 1, e - p - 1);
}

//---------------- 广播 ----------------
static void broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lk(g_mtx);
    std::string line = msg + "\r\n";
    for (SOCKET s : g_clients) {
        send(s, line.c_str(), (int)line.size(), 0);
    }
    logmsg("已广播: " + msg);
}

//---------------- 处理一条报文 ----------------
static void handleLine(SOCKET s, const std::string& line) {
    if (line.empty()) return;
    logmsg("收到: " + line);

    if (line.find("\"acc\"") != std::string::npos) {//登录
        std::string acc = getField(line, "acc");
        logmsg("客户端登录, 账号=" + acc);
        std::string reply = "{\"result\":\"ok\",\"msg\":\"login success\",\"serverPort\":8888}\r\n";
        send(s, reply.c_str(), (int)reply.size(), 0);
    }
    else if (line.find("\"status\"") != std::string::npos) {//心跳
        logmsg("心跳: " + getField(line, "status"));
    }
    else if (line.find("\"image\"") != std::string::npos) {//图片
        std::string b64 = getField(line, "image");
        std::string bin = base64_decode(b64);
        std::filesystem::create_directories("received");
        char name[128];
        snprintf(name, sizeof(name), "received/img_%03d.jpg", ++g_imgIndex);
        std::ofstream ofs(name, std::ios::binary);
        ofs.write(bin.data(), (std::streamsize)bin.size());
        ofs.close();
        logmsg("收到图片 " + std::to_string(bin.size()) + " 字节，已存为 " + name);
        std::string reply = "{\"result\":\"ok\",\"file\":\"" + std::string(name) + "\"}\r\n";
        send(s, reply.c_str(), (int)reply.size(), 0);
    }
    else {
        logmsg("（其它报文，未处理）");
    }
}

//---------------- 每个客户端一个线程 ----------------
static void clientThread(SOCKET s, int id) {
    logmsg("客户端 #" + std::to_string(id) + " 已连接");
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        g_clients.push_back(s);
    }

    std::string buf;
    char tmp[4096];
    while (true) {
        int n = recv(s, tmp, sizeof(tmp), 0);
        if (n <= 0) {
            logmsg("客户端 #" + std::to_string(id) + " 断开");
            break;
        }
        buf.append(tmp, n);
        size_t pos;
        while ((pos = buf.find('\n')) != std::string::npos) {
            std::string line = buf.substr(0, pos);
            buf.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            handleLine(s, line);
        }
    }

    {
        std::lock_guard<std::mutex> lk(g_mtx);
        for (auto it = g_clients.begin(); it != g_clients.end(); ++it) {
            if (*it == s) { g_clients.erase(it); break; }
        }
    }
    closesocket(s);
}

int main(int argc, char** argv) {
    int port = 8888;
    if (argc > 1) port = std::atoi(argv[1]);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cout << "WSAStartup 失败" << std::endl;
        return 1;
    }

    SOCKET srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv == INVALID_SOCKET) {
        std::cout << "创建socket失败" << std::endl;
        return 1;
    }

    //用 EXCLUSIVEADDRUSE，防止两个进程同时占同一个端口（SO_REUSEADDR 在 Windows 上会允许重复绑定）
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cout << "端口 " << port << " 绑定失败（可能已被占用），错误码 "
                  << WSAGetLastError() << std::endl;
        return 1;
    }
    listen(srv, SOMAXCONN);
    logmsg("服务端已启动，监听端口 " + std::to_string(port));
    std::cout << "在下面直接输入文字+回车，会广播给所有客户端（输入 quit 退出）" << std::endl;

    // 控制台输入线程：发指令给客户端
    std::thread([]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "quit") { ExitProcess(0); }
            if (!line.empty()) broadcast(line);
        }
    }).detach();

    int id = 0;
    while (true) {
        SOCKET c = accept(srv, nullptr, nullptr);
        if (c == INVALID_SOCKET) continue;
        std::thread(clientThread, c, ++id).detach();
    }
    return 0;
}
