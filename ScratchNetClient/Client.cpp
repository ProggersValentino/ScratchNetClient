#include "Client.h"
#include "ScratchAck.h"

#include <iostream>
#include <thread>
#include <Payload.h>
#include <PacketSerialization.h>

const int safetyBuffer = 32;
const int packetSize = 55;
const int totalPacketSize = packetSize + safetyBuffer;

Client::Client()
{

}

void Client::InitSockets(unsigned short int port, bool isBound)
{
    clientSock.OpenSock(port, isBound);
}

void Client::ClientProcess()
{
    objectID = generateObjectID();

    clientSock = *RetrieveSocket();
    clientSock.OpenSock(30000, false);

    clientSnap = *CreateFilledSnapShot(objectID, 0.f, 0.f, 0.f);
    
    packetAckMaintence = GenerateScratchAck();

    Snapshot* initSnap = CreateFilledSnapShot(objectID, 0.f, 0.f, 0.f);

    ssRecordKeeper = InitRecordKeeper();
    ssRecordKeeper->InsertNewRecord(0, *initSnap); //inserting default baseline

    delete initSnap;


    //separate buffers to prevent mix ups and ensure there is a clear difference between what gets sent and what is retrieved from server
    char transmitBuf[totalPacketSize] = { 0 };
    char receiveBuf[totalPacketSize] = { 0 };

    
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
        Snapshot* dummySnap = CreateEmptySnapShot();
        dummySnap->objectId = objectID;

        //updated position
        std::cout << "New Position X: " << std::endl;

        float x = 0.f;

        std::cin >> x;

        dummySnap->posX = x;

        std::cout << '\n' << std::endl;

        std::cout << "New Position X: " << std::endl;

        float y = 0.f;

        std::cin >> y;

        dummySnap->posY = y;

        std::cout << '\n' << std::endl;


        std::cout << "New Position X: " << std::endl;

        float z = 0.f;

        std::cin >> z;

        dummySnap->posZ = z;

        std::cout << '\n' << std::endl;

        CompareSnapShot(*ssRecordKeeper->baselineRecord.recordedSnapshot, *dummySnap, changedVariables, changedValues);

        //create header
        ScratchPacketHeader* header = InitPacketHeaderWithoutCRC(11, ssRecordKeeper->baselineRecord.packetSequence, packetAckMaintence->currentPacketSequence, packetAckMaintence->mostRecentRecievedPacket,
            packetAckMaintence->GetAckBits(packetAckMaintence->mostRecentRecievedPacket));

        PacketData data = packetAckMaintence->InsertPacketData(packetAckMaintence->currentPacketSequence);
        data.acked = false; //ensuring that when a packet gets assigned, the acked is always false

        Payload* payloadToSend = CreatePayload(objectID, changedVariables, changedValues);

        ConstructPacket(*header, *payloadToSend, transmitBuf); //combining the header and payload into one transmit buffer to send

        int bytesSent = clientSock.Send(*sendAddress, &transmitBuf, sizeof(transmitBuf));

        if (bytesSent <= 0)
        {
            printf("Error sending message due to this error: %d\n", WSAGetLastError());
        }


        packetAckMaintence->IncrementPacketSequence(); //incrementing the current packet sequence by 1

        //clientSnap = dummySnap;
        //ssRecordKeeper->InsertNewRecord(header->sequence, dummySnap); //keeping record of the most recent snapshot

        changedValues.clear();
        changedVariables.clear();
        delete dummySnap;
        
        //

        if (strcmp(transmitBuf, "q") == 0)
        {
            break;
        }

        //RECIEVING PROCESS 
        int recieveSize = totalPacketSize;
        Address* server = CreateAddress();
        int recievedFromServer = clientSock.Receive(*server, &receiveBuf, recieveSize);

        if (recievedFromServer > 0)
        {
            printf("Recieved information from server: %s \n", receiveBuf);
            ScratchPacketHeader* recvHeader = InitEmptyPacketHeader();

            

            Payload recievedPayload = *CreateEmptyPayload();

            DeconstructPacket(receiveBuf, *recvHeader, recievedPayload);

            /*DeserializePayload(receiveBuf, 30, recievedPayload);*/
            const int tpSize = 17 + safetyBuffer;
            char tempbuf[tpSize] = { 0 };
            SerializePayload(recievedPayload, tempbuf);

            if (!CompareCRC(*recvHeader, tempbuf, tpSize))
            {
                std::cout << "Failed CRC Check" << std::endl;
                continue;
            }

            std::cout << "CRC Check Succeeded" << std::endl;
            //packet maintence
            packetAckMaintence->InsertRecievedSequenceIntoRecvBuffer(recvHeader->sequence); //insert sender's packet sequence into our local recv sequence buf

            packetAckMaintence->OnPacketAcked(recvHeader->ack); //acknowledge the most recent packet that was recieved by the sender

            packetAckMaintence->AcknowledgeAckbits(recvHeader->ack_bits, recvHeader->ack); //acknowledge the previous 32 packets starting from the most recent acknowledged from the sender

            if(recvHeader->sequence < packetAckMaintence->mostRecentRecievedPacket) //is the packet's sequence we just recieved higher than our most recently recieved packet sequence?
            {
                packetAckMaintence->mostRecentRecievedPacket = recvHeader->sequence;
                continue;
            }

            packetAckMaintence->mostRecentRecievedPacket = recvHeader->sequence;
            //apply changes to clients here

            Snapshot* extractedChanges = new Snapshot();
            Snapshot* newBaseline = new Snapshot();

            //how will we deconstruct the packet and who will we apply it to
            switch (recvHeader->packetCode)
            {
            case 11://payload specifically
                extractedChanges = DeconstructRelativePayload(recievedPayload);
                
                if (extractedChanges->objectId == ssRecordKeeper->baselineRecord.recordedSnapshot->objectId) //we dont wnat to change the client's object htta is reserved for the heartbeat sent from server
                {
                    break;
                }

                newBaseline = ApplyChangesToSnapshot(*ssRecordKeeper->baselineRecord.recordedSnapshot, *extractedChanges);

                UpdateNetworkedObject(*newBaseline);
                printf("object: %d has updated to: %f %f %f", newBaseline->objectId, newBaseline->posX, newBaseline->posY, newBaseline->posZ);

                break;
            case 12: //outright apply change to object
                extractedChanges = DeconstructAbsolutePayload(recievedPayload);

                if (extractedChanges->objectId == ssRecordKeeper->baselineRecord.recordedSnapshot->objectId) //we dont wnat to change the client's object htta is reserved for the heartbeat sent from server
                {
                    break;
                }
                UpdateNetworkedObject(*extractedChanges);
                printf("object: %d has updated to: %f %f %f", extractedChanges->objectId, extractedChanges->posX, extractedChanges->posY, extractedChanges->posZ);
                break;
            case 21://ACK specifically

                

                break;
            case 22: //update the client's baseline snapshot which will be the movement 
                extractedChanges = DeconstructAbsolutePayload(recievedPayload);
                ssRecordKeeper->InsertNewRecord(recvHeader->sequence, *extractedChanges);
                UpdateNetworkedObject(*extractedChanges);

                printf("object: %d has updated to: %f %f %f", extractedChanges->objectId, extractedChanges->posX, extractedChanges->posY, extractedChanges->posZ);

                break;
            }

            
            
            delete extractedChanges;
            delete newBaseline;
            delete recvHeader;
            delete payloadToSend; //wipe the payload as we dont need it anymore
            delete header;

        }
        //


        
    }

    clientSock.Close();
    
}

void Client::ClientListen(void* recieveBuf)
{
    int recieveSize = totalPacketSize;
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

void Client::UpdateNetworkedObject(Snapshot changesToBeMade)
{
    
    if (!nom.TryUpdatingNetworkedObject(changesToBeMade))
    {
        nom.TryInsertNewNetworkObject(changesToBeMade);
    }
}

int Client::generateObjectID()
{
    std::random_device rd;

    // seed value designed specifically to be different across app executions
    std::mt19937::result_type seed = rd() ^ (
        (std::mt19937::result_type)
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count() +
        (std::mt19937::result_type)
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count());

    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, INT_MAX);


    return dist(gen); //random number generated
}
