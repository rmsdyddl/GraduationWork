	#include "stdafx.h"
#include "protocol.h"
#include "defaultInit.h"
#include "iocpNetwork.h"
#include "processRoutine.h"
#include "lock-free-SET_ClientInfo.h"
#include "lock-free-SET_RoomInfo.h"

bool gShutdown = false;
HANDLE ghIOCP;

ClientList *gClientInfoSet;
ClientNode *gClientInfo_DelList = nullptr;
std::mutex gClientInfo_DelList_Lock;

RoomList *gRoomInfoSet;
RoomNode *gRoomInfo_DelList = nullptr;
std::mutex gRoomInfo_DelList_Lock;

/**********************변 경 사 항**********************/
int gItemArr[NUM_OF_ITEM];
/**********************변 경 사 항**********************/

int main(int argc, char *argv[])
{
	std::vector<std::thread*> workerThreads;
	std::thread acceptThread;
	gClientInfoSet = new ClientList();
	gClientInfo_DelList = new ClientNode(MIN_INT);

	gRoomInfoSet = new RoomList();
	gRoomInfo_DelList = new RoomNode(MIN_INT);

	gClientInfoSet->Initialize();
	gRoomInfoSet->Initialize();
	InitializeServer();

	/**********************변 경 사 항**********************/
	InitializeItem();
	/**********************변 경 사 항**********************/

	acceptThread = std::thread(AcceptThreadFunc);
	for (int i = 0; i < NUM_THREADS; ++i)
		workerThreads.push_back(new std::thread(WorkerThreadFunc));

	while (!gShutdown) Sleep(1000);
	//  다음과 같이 Main Thread를 1초마다 잠들게 하여
	// Processor가 Main Thread를 점유하는 일을 최대한 저해하도록 한다.

	acceptThread.join();
	for (auto t : workerThreads)
	{
		t->join();
		delete t;
	}

	StopServer();

	gClientInfoSet->Rearrangement();
	while (0 != gClientInfo_DelList->next)
	{
		ClientNode *temp = gClientInfo_DelList;
		gClientInfo_DelList = gClientInfo_DelList->GetNext();
		delete temp;
	}

	gRoomInfoSet->Rearrangement();
	while (0 != gRoomInfo_DelList->next)
	{
		RoomNode *temp = gRoomInfo_DelList;
		gRoomInfo_DelList = gRoomInfo_DelList->GetNext();
		delete temp;
	}

	delete gClientInfo_DelList;
	delete gClientInfoSet;

	delete gRoomInfo_DelList;
	delete gRoomInfoSet;

	return 0;
}