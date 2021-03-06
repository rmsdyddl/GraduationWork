#include "stdafx.h"
#include "protocol.h"
#include "defaultInit.h"
#include "iocpNetwork.h"
#include "processRoutine.h"
#include "lock-free_synchronization.h"

bool gShutdown = false;
HANDLE ghIOCP;

LFList *gClientInfoSet;
LFNode *gClientInfo_DelList = nullptr;
std::mutex gClientInfo_DelList_Lock;

Monster gMonster;
Vector3 monsterPath[NUM_OF_MONSTER_PATH];
std::mutex gLock;

int main(int argc, char *argv[])
{
	std::vector<std::thread*> workerThreads;
	std::thread acceptThread;
	gClientInfoSet = new LFList();
	gClientInfo_DelList = new LFNode(MIN_INT);

	gClientInfoSet->Initialize();
	InitializeServer();
	InitializeMonster();

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
		LFNode *temp = gClientInfo_DelList;
		gClientInfo_DelList = gClientInfo_DelList->GetNext();
		delete temp;
	}

	delete gClientInfo_DelList;
	delete gClientInfoSet;

	return 0;
}