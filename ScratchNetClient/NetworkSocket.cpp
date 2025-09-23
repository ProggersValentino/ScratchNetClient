#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include "NetworkSocket.h"
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")


bool NetworkSocket::InitializeSockets()
{
#if PLATFORM == PLATFORM_WINDOWS
    WSADATA wsaData;
    WORD versionRequested;
    
    versionRequested = MAKEWORD(2, 2); 
    bool error = WSAStartup(versionRequested, &wsaData) != NO_ERROR; /*return result of whether an error has occured or not */

    if (error)
    {
        int socketError = WSAGetLastError();
        printf("could not start socket: %d", socketError);
        return false;
    }
    else
    {
        //confirm that WINSOCK2 DLL supports the exact version requesting otherwise cleanup
        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
        {
            WSACleanup();
            return false;
        }
    }

    return true;
#else
    return true;
#endif
    
}

void NetworkSocket::ShutdownSockets()
{
#if PLATFORM == PLATFORM_WINDOWS
    WSACleanup();
    #endif
    
}
