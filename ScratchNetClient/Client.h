#pragma once
#include "Socket.h"
#include "Snapshot.h"

class Client
{
public:
    void InitSockets(unsigned short int port, bool isBound);
    void ClientProcess();
    void ClientListen(void* recieveBuf);
    
private:
    Snapshot clientSnap;
    Socket clientSock;
};
