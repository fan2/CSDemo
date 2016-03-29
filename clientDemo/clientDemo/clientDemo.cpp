//
//  clientDemo.cpp
//  clientDemo
//
//  Created by faner on 16/3/9.
//  Copyright © 2016年 faner. All rights reserved.
//

#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER // #if defined(_OS_WIN_) || defined(WIN32) || defined(WIN64)

#include <WINSOCK.H>
#pragma comment(lib, "WSOCK32.LIB")  // wsock32.dll

// #include <WINSOCK2.H>
// #pragma comment(lib, "WS2_32.LIB") // ws2_32.dll

#define ReportCSError(api)      printf("Error #%d in %s./n", WSAGetLastError(), #api)

// class WSAInitializer is a part of the Socket class (on win32)
// as a static instance - so whenever an application uses a Socket,
// winsock is initialized
class WSAInitializer // Winsock Initializer
{
public:
    WSAInitializer()
    {
        if (WSAStartup(0x101, &m_wsadata))
        {
            exit(-1);
        }
    }
    
    ~WSAInitializer()
    {
        WSACleanup();
    }
    
private:
    WSADATA m_wsadata;
}WsaInitiator;

#else

#include <sys/errno.h>      // errno
#include <sys/socket.h>     // socket
#include <netinet/in.h>     // sockaddr_in
#include <arpa/inet.h>      // inet_ntoa
#include <unistd.h>         // close, sleep

#define ReportCSError(api)      printf("Error #%d in %s.\n", errno, #api)

#define SOCKET                  int
#define INVALID_SOCKET          -1
#define SOCKET_ERROR            -1
#define closesocket             close

#endif



#define SERVER_ENDPOINT_IP      inet_addr("127.0.0.1") // "10.18.84.102"
#define SERVER_ENDPOINT_PORT    8888

// 摘自《TCP通信流程解析》中的请求百度首页报文。
// http://blog.csdn.net/phunxm/article/details/5836034
char httpRequest[] = "GET / HTTP/1.1\r\n"
    "Host: www.baidu.com\r\n"
    "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.8) Gecko/20100722 Firefox/3.6.8\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    "Accept-Language: zh-cn,zh;q=0.5\r\n"
    "Accept-Encoding: gzip,deflate\r\n"
    "Accept-Charset: GB2312,utf-8;q=0.7,*;q=0.7\r\n"
    "Keep-Alive: 115\r\n"
    "Connection: keep-alive\r\n"
    "\r\n";

int main(int argc, char* argv[])
{
    // Create a socket stands for the client process
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(clientSocket == INVALID_SOCKET) // Socket creation failure
    {
        ReportCSError(socket());
        return -1;
    }
    
    // Fill the sockaddr_in structure,
    // which describes the other side(server) of the connection to establish.
    // This demo connect to localhost.
    // Call function bind to associate a specified local address with the socket
    // before connect operation in the case of Multi-NIC.
    struct sockaddr_in servAddr = {0};
#if __APPLE__ // #if	defined(_OS_MAC_)
    servAddr.sin_len = sizeof(servAddr);
#endif
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERVER_ENDPOINT_PORT);
    servAddr.sin_addr.s_addr = SERVER_ENDPOINT_IP;
    
    printf("Connecting to server: %s:%d...\r\n", inet_ntoa(servAddr.sin_addr), SERVER_ENDPOINT_PORT);
    
    // Establishes a connection to the clientSocket.
    if(connect(clientSocket, (sockaddr*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
    {
        ReportCSError(connect());
        return -1;
    }
    
    // Send request to the server
    int nTobeSend = (int)strlen(httpRequest);
    int nSend = (int)send(clientSocket, httpRequest, nTobeSend, 0);
    if (nTobeSend != nSend) // simply suppose exactly over
    {
        ReportCSError(send());
        return -1;
    }
    else
    {
        // Waiting for response from the server
        char response[512];
        int nRecv = (int)recv(clientSocket, response, 512, 0);
        if(nRecv > 0)
        {
            response[nRecv] = 0; // fill a null-terminated flag of the C string
            printf("Received response:\r\n========================================\r\n%s\r\n", response);
            // 紧接着接收 Content-Length=2139 字节的百度首页 HTML 内容。
            // 浏览器边解析边请求 HTML 页面内引用的 logo、favicon、javascript 等资源文件。
        }
    }
    
    // closes client socket
    if (closesocket(clientSocket) == SOCKET_ERROR)
    {
        ReportCSError(closesocket());
        return 1;
    }
    
    return 0; // normal exit
}