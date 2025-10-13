#pragma once
#include "Socket.h"
#include "Snapshot.h"
#include "ScratchAck.h"
#include "SnaphotRecordKeeper.h"
#include <unordered_map>

class Client
{
public:
    void InitSockets(unsigned short int port, bool isBound);
    void ClientProcess();
    void ClientListen(void* recieveBuf);
    
    
private:
    std::unordered_map<int, Snapshot> networkedObjects;

    int objectID = 6224;

    SnapshotRecordKeeper* ssRecordKeeper;
    ScratchAck* packetAckMaintence;
    Snapshot clientSnap;
    Socket clientSock;
};
