
#include "Client.h"
#include "NetSockets.h"

int main(int argc, char* argv[])
{
    InitializeSockets(); // init sockets

    //WSADATA wsaData;
    //WORD versionRequested;
    //
    //versionRequested = MAKEWORD(2, 2); 
    //bool error = WSAStartup(versionRequested, &wsaData) != NO_ERROR; /*return result of whether an error has occured or not */

    //if (error)
    //{
    //    int socketError = WSAGetLastError();
    //    printf("could not start socket: %d", socketError);
    //    return 1;
    //}
    //else
    //{
    //    //confirm that WINSOCK2 DLL supports the exact version requesting otherwise cleanup
    //    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    //    {
    //        WSACleanup();
    //        return 1;
    //    }
    //}

    
    Client client;

    /*client.InitSockets(30000, false);*/

    client.ClientProcess();

    
    return 0;
}
