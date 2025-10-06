#include "Client.h"
#include "ScratchAck.h"

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
    
    packetAckMaintence = GenerateScratchAck();

    //separate buffers to prevent mix ups and ensure there is a clear difference between what gets sent and what is retrieved from server
    char transmitBuf[30];
    char receiveBuf[30];

    
    strcpy_s(transmitBuf, "");
    strcpy_s(receiveBuf, "");

    Address* sendAddress = CreateAddressIndiIPPort(127, 0, 0, 1, 30000);

    std::vector<char> changedValues; //the changes to the data to be sent over

    std::vector<EVariablesToChange> changedVariables;

    ///this method uses the main thread ONLY to send and recieve packets which does mean the receive will be delayed by a tick in the loop
    ///this is a common approach and can lead into threadpooling methods 
    while (true)
    {
        //SENDING PROCESS
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

        ScratchPacketHeader* header = InitPacketHeaderWithoutCRC(packetAckMaintence->currentPacketSequence, packetAckMaintence->mostRecentRecievedPacket, 
            packetAckMaintence->GetAckBits(packetAckMaintence->mostRecentRecievedPacket));

        PacketData data = packetAckMaintence->InsertPacketData(packetAckMaintence->currentPacketSequence);
        data.acked = false; //ensuring that when a packet gets assigned, the acked is always false

        Payload* payloadToSend = CreatePayload(changedVariables, changedValues);

        ConstructPacket(*header, *payloadToSend, transmitBuf); //combining the header and payload into one transmit buffer to send

        int bytesSent = clientSock.Send(*sendAddress, &transmitBuf, sizeof(transmitBuf));

        if (bytesSent <= 0)
        {
            printf("Error sending message due to this error: %d\n", WSAGetLastError());
        }


        packetAckMaintence->IncrementPacketSequence(); //incrementing the current packet sequence by 1

        clientSnap = dummySnap;

        changedValues.clear();
        changedVariables.clear();
        
        delete payloadToSend; //wipe the payload as we dont need it anymore
        delete header;
        //

        if (strcmp(transmitBuf, "q") == 0)
        {
            break;
        }

        //RECIEVING PROCESS 
        int recieveSize = 30;
        Address* server = CreateAddress();
        int recievedFromServer = clientSock.Receive(*server, &receiveBuf, recieveSize);

        if (recievedFromServer > 0)
        {
            printf("Recieved information from server: %s \n", receiveBuf);
            ScratchPacketHeader* recvHeader = InitEmptyPacketHeader();

            

            Payload recievedPayload = *CreateEmptyPayload();

            DeconstructPacket(receiveBuf, *recvHeader, recievedPayload);

            /*DeserializePayload(receiveBuf, 30, recievedPayload);*/

            char tempbuf[15] = { 0 };
            SerializePayload(recievedPayload, tempbuf);

            if (!CompareCRC(*recvHeader, tempbuf))
            {
                return;
            }

            //packet maintence
            packetAckMaintence->InsertRecievedSequenceIntoRecvBuffer(header->sequence); //insert sender's packet sequence into our local recv sequence buf

            packetAckMaintence->OnPacketAcked(recvHeader->ack); //acknowledge the most recent packet that was recieved by the sender

            packetAckMaintence->AcknowledgeAckbits(header->ack_bits, header->ack); //acknowledge the previous 32 packets starting from the most recent acknowledged from the sender

            if(header->sequence < packetAckMaintence->mostRecentRecievedPacket) //is the packet's sequence we just recieved higher than our most recently recieved packet sequence?
            {
                packetAckMaintence->mostRecentRecievedPacket = header->sequence;
                return;
            }

            //apply changes to clients here
            printf("deserialized float: %f", DeserializeFloat(recievedPayload.setChanges, 0));
        }
        //


        
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
