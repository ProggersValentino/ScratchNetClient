#pragma once
#include "Socket.h"
#include "Snapshot.h"
#include "ScratchAck.h"
#include "SnaphotRecordKeeper.h"
#include "NetworkObjectManagement.h"
#include <iostream>
#include <random>
#include <chrono>
#include <unordered_map>

class Client
{
public:
    void InitSockets(unsigned short int port, bool isBound);
    void ClientProcess();
    void ClientListen(void* recieveBuf);
    
    void UpdateNetworkedObject(Snapshot changesToBeMade);

    int generateObjectID();
    
    
private:
    //std::unordered_map<int, Snapshot> networkedObjects;

    int objectID = 0;

    NetworkObjectManagement nom;
    SnapshotRecordKeeper* ssRecordKeeper;
    ScratchAck* packetAckMaintence;
    Snapshot clientSnap;
    Socket clientSock;
};
