//
//  serverDemo.cpp
//  serverDemo
//
//  Created by faner on 16/3/9.
//  Copyright © 2016年 faner. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _MSC_VER // #if defined(_OS_WIN_) || defined(WIN32) || defined(WIN64)

#include <WINSOCK.H>
#pragma comment(lib, "WSOCK32.LIB")  // wsock32.dll

// #include <WINSOCK2.H>
// #pragma comment(lib, "WS2_32.LIB") // ws2_32.dll

#define ReportCSError(api)      printf("Error #%d in %s./n", WSAGetLastError(), #api)

#define socklen_t               int

int loadSocketLibrary()
{
    // Initiates use of the wsock32.dll
    WSADATA wsaData;
    WORD sockVersion = MAKEWORD(1, 0);
    if (WSAStartup(sockVersion, &wsaData)) // Initiates failure
    {
        ReportCSError(WSAStartup());
        return -1;
    }
    
    return 0;
}

#define sleep_t Sleep

#else

#include <sys/errno.h>      // errno
#include <sys/socket.h>     // socket
#include <netinet/in.h>     // sockaddr_in
#include <arpa/inet.h>      // inet_ntoa
#include <unistd.h>         // close, sleep
#include <netdb.h>          // getLocalAddr

#define ReportCSError(api)      printf("Error #%d in %s.\n", errno, #api)

#define SOCKET                  int
#define INVALID_SOCKET          -1
#define SOCKET_ERROR            -1
#define closesocket             close

#define loadSocketLibrary()     // just do nothing
#define WSACleanup()        0   // just do nothing

int sleep_t(int milliseconds)
{
    struct timespec 	tswait;   	/* Input for timedwait */
    /* Calculate the end time*/
    tswait.tv_sec = milliseconds/1000;
    tswait.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
    /* High resolution to sleep */
    return nanosleep(&tswait, NULL);
}

#endif



#define LOCAL_ENDPOINT_IP      INADDR_ANY // "127.0.0.1"
#define LOCAL_ENDPOINT_PORT    8888

// 摘自《TCP通信流程解析》中的百度回包报文。
// http://blog.csdn.net/phunxm/article/details/5836034
char httpResponse[] = "HTTP/1.1 200 OK\r\n"
    "Date: Sat, 21 Aug 2010 07:08:30 GMT\r\n"
    "Server: BWS/1.0\r\n"
    "Content-Length: 2139\r\n"
    "Content-Type: text/html;charset=gb2312\r\n"
    "Cache-Control: private\r\n"
    "Expires: Sat, 21 Aug 2010 07:08:30 GMT\r\n"
    "Content-Encoding: gzip\r\n"
    "Set-Cookie: BAIDUID=4CF9E2F821DF77FE6AF9D1A10D080013:FG=1; expires=Sat, 21-Aug-40 07:08:30 GMT; path=/; domain=.baidu.com\r\n"
    "P3P: CP=\" OTI DSP COR IVA OUR IND COM \"\r\n"
    "Connection: Keep-Alive\r\n"
    "\r\n";

int main(int argc, char* argv[])
{
    loadSocketLibrary();
    
    // Create a socket to listen for all incoming client connection
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   
    if(listenSocket == INVALID_SOCKET)
    {
        ReportCSError(socket());
        WSACleanup(); // Terminates use of the wsock32.dll
        return -1;
    }
    
    // Fill the local sockaddr_in structure, which the listenSocket will bind to.
    struct sockaddr_in sin = {0};
#if __APPLE__ // #if	defined(_OS_MAC_)
    sin.sin_len = sizeof(sin);
#endif
    sin.sin_family = AF_INET;
    sin.sin_port = htons(LOCAL_ENDPOINT_PORT);
    sin.sin_addr.s_addr = LOCAL_ENDPOINT_IP; // any local address.
    
    // Associates the listenSocket with a well-known address
    if(bind(listenSocket, (struct sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        ReportCSError(bind());
        WSACleanup(); // Terminates use of the wsock32.dll
        return -1;
    }
    
    // getLocalAddr
    char hname[128];
    struct hostent *hent;
    gethostname(hname, sizeof(hname));
    hent = gethostbyname(hname); // gethostent()
    printf("Listening at %s:%d@%s...\r\n", inet_ntoa((*(struct in_addr*)(hent->h_addr_list[0]))), LOCAL_ENDPOINT_PORT, hent->h_name);
    
    // Put the socket into listening mode to wait for incoming connections
    if(listen(listenSocket, 2) == SOCKET_ERROR)
    {
        ReportCSError(listen());
        WSACleanup(); // Terminates use of the wsock32.dll
        return -1;
    }
    
    // Create a sockaddr_in structure to contain the client's address information
    sockaddr_in peerAddr;
    socklen_t nAddrLen = sizeof(peerAddr);
    
    // The server will create a socket for the session to every client
    SOCKET serveSocket;
    // char echo[] = "Message from the server! /r/n";
    
    while(true) // Listening circulation
    {
        printf("Waiting for connection incoming...\r\n");
        
        serveSocket = accept(listenSocket, (struct sockaddr*)&peerAddr, &nAddrLen);
        if(serveSocket == SOCKET_ERROR)
        {
            ReportCSError(accept());
            continue;
        }
        
        printf("Accept a connection from: %s:%d\r\n", inet_ntoa(peerAddr.sin_addr), peerAddr.sin_port);
        
        // Waiting for request from the client
        char request[512];
        int nRecv = (int)recv(serveSocket, request, 512, 0);
        if(nRecv > 0)
        {
            request[nRecv] = 0; // fill a null-terminated flag of the C string
            printf("Received request:\r\n========================================\r\n%s\r\n", request);

            if (strstr(request, "GET / HTTP/")) // HTTP GET /
            {
                int nTobeSend = (int)strlen(httpResponse);
                int nSend = (int)send(serveSocket, httpResponse, nTobeSend, 0);
                if (nTobeSend == nSend) // simply suppose exactly over
                {
                    sleep_t(1000); // sleep(1)
                    // 紧接着发送 Content-Length=2139 字节的百度首页 HTML 内容。
                }
                closesocket(serveSocket); // shutdown(serveSocket, SD_BOTH);
            }
        }
        
        printf("Press q to exit, or press any key to continue...\r\n");
        char ch = 0;
        if ((ch=getchar()) == 'q')
        {
            break;
        }
    }
    
    // close listenSocket
    if (closesocket(listenSocket) == SOCKET_ERROR)
    {
        ReportCSError(closesocket());
        WSACleanup();
        return -1;
    }
    
    // terminates use of the wsock32.dll
    if (WSACleanup() == SOCKET_ERROR)
    {
        ReportCSError(WSACleanup());
        return -1;
    }
    
    return 0; // normal exit
}