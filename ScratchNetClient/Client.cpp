#include "Client.h"

#include <iostream>
#include <thread>
#include <Payload.h>
#include <PacketSerialization.h>

void Client::InitSockets(unsigned short int port, bool isBound)
{
    clientSock.OpenSock(port, isBound);
}

void Client::ClientProcess()
{
    clientSock = *RetrieveSocket();
    clientSock.OpenSock(30000, false);

    clientSnap = *CreateFilledSnapShot(62266, 0.f, 0.f, 0.f);
    
    //separate buffers to prevent mix ups and ensure there is a clear difference between what gets sent and what is retrieved from server
    char transmitBuf[13];
    char receiveBuf[13];

    
    strcpy_s(transmitBuf, "");
    strcpy_s(receiveBuf, "");

    Address* sendAddress = CreateAddressIndiIPPort(127, 0, 0, 1, 30000);

    std::vector<char> changedValues; //the changes to the data to be sent over

    std::vector<EVariablesToChange> changedVariables;

    ///this method uses the main thread ONLY to send and recieve packets which does mean the receive will be delayed by a tick in the loop
    ///this is a common approach and can lead into threadpooling methods 
    while (true)
    {
        Snapshot dummySnap = *CreateEmptySnapShot();

        //updated position
        std::cout << "New Position X: " << std::endl;

        float x = 0.f;

        std::cin >> x;

        dummySnap.posX = x;

        std::cout << '\n' << std::endl;

        std::cout << "New Position X: " << std::endl;

        float y = 0.f;

        std::cin >> y;

        dummySnap.posY = y;

        std::cout << '\n' << std::endl;


        std::cout << "New Position X: " << std::endl;

        float z = 0.f;

        std::cin >> z;

        dummySnap.posZ = z;

        std::cout << '\n' << std::endl;

        CompareSnapShot(clientSnap, dummySnap, changedVariables, changedValues);

        Payload payloadToSend = *CreatePayload(changedVariables, changedValues);

        SerializePayload(payloadToSend, transmitBuf);

        int bytesSent = clientSock.Send(*sendAddress, &transmitBuf, sizeof(transmitBuf));

        if (bytesSent <= 0)
        {
            printf("Error sending message due to this error: %d\n", WSAGetLastError());
        }

        /*if (std::cin.getline(transmitBuf, sizeof(transmitBuf)))
        {
            

            int bytesSent = clientSock.Send(*sendAddress, transmitBuf, sizeof(transmitBuf));

            if (bytesSent <= 0)
            {
                printf("Error sending message due to this error: %d\n", WSAGetLastError());
            }
        }*/

        clientSnap = dummySnap;

        changedValues.clear();
        changedVariables.clear();

        if (strcmp(transmitBuf, "q") == 0)
        {
            break;
        }

        int recieveSize = 40;
        Address* server = CreateAddress();
        int recievedFromServer = clientSock.Receive(*server, &receiveBuf, recieveSize);

        if (recievedFromServer > 0)
        {
            printf("Recieved information from server: %s \n", receiveBuf);

            Payload recievedPayload = *CreateEmptyPayload();

            DeserializePayload(receiveBuf, 40, recievedPayload);

            printf("deserialized float: %f", DeserializeFloat(recievedPayload.setChanges, 0));
        }
        
    }

    clientSock.Close();
    
}

void Client::ClientListen(void* recieveBuf)
{
    int recieveSize = 40;
    Address* server = CreateAddress();
    int recievedFromServer = clientSock.Receive(*server, recieveBuf, recieveSize);

    if (recievedFromServer > 0)
    {
        printf("Recieved information from server: %s \n", (char*)recieveBuf);

        Payload recievedPayload = *CreateEmptyPayload();

        DeserializePayload((char*)recieveBuf, recieveSize, recievedPayload);

        printf("deserialized float: %f", DeserializeFloat(recievedPayload.setChanges, 0));
    }
   
}
