#include "processRoutine.h"


bool BeCompeletedSendPacket(BYTE type, BYTE size)
{
	switch (type)
	{
	case PacketType::SetID: 
		if (size != sizeof(Packet::SetID)) 
			return false; 
		break;
	case PacketType::Connect:
		if (size != sizeof(Packet::Connect))
			return false;
		break;
	case PacketType::Disconnect:
		if (size != sizeof(Packet::Disconnect))
			return false;
		break;
	case PacketType::PlayerMove:
		if (size != sizeof(Packet::Player::Move))
			return false;
		break;
	case PacketType::PlayerLight:
		if (size != sizeof(Packet::Player::Light))
			return false;
		break;
	case PacketType::PlayerShout:
		if (size != sizeof(Packet::Player::Shout))
			return false;
		break;
	case PacketType::MonsterSetInfo:
		if (size != sizeof(Packet::Monster::SetInfo))
			return false;
		break;
	case PacketType::MonsterMove:
		if (size != sizeof(Packet::Monster::Move))
			return false;
		break;
	case PacketType::MonsterSetPatrolPos:
		if (size != sizeof(Packet::Monster::SetPatrolPos))
			return false;
		break;
	default:
		break;
	}

	return true;
}

void ProcessPacket(int key, unsigned char *packet)
{
	static int count = 0;
	unsigned char packetType = packet[1];

	std::string debugText = "";

	switch (packetType)
	{
	case (BYTE)PacketType::Connect:
		// Connect 동기화 하는 Packet
		debugText = std::to_string(key) + " ProcessPacket::Connect::Called!!";
		OnReceivePacket::Connect(key, packet);
		SendMonsterSetInfoPacket(key);
		break;
	case (BYTE)PacketType::PlayerMove:
		debugText = std::to_string(key) + " ProcessPacket::PlayerMove::Called!!";
		OnReceivePacket::PlayerMove(key, packet);
		break;
	case (BYTE)PacketType::PlayerLight:
		debugText = std::to_string(key) + " ProcessPacket::PlayerLight::Called!!";
		OnReceivePacket::PlayerLight(key, packet);
		break;
	case (BYTE)PacketType::PlayerShout:
		debugText = std::to_string(key) + " ProcessPacket::PlayerShout::Called!!";
		OnReceivePacket::PlayerShout(key, packet);
		break;
	case (BYTE)PacketType::MonsterMove:
		debugText = std::to_string(key) + " ProcessPacket::MonsterMove::Called!!";
		OnReceivePacket::MonsterMove(key, packet);
		break;
	default:
		debugText = std::to_string(key) + " ProcessPacket::Unknown Packet Type Detected";
		return;
	}
	DisplayDebugText(debugText);

	return;
}

void OnReceivePacket::Connect(int key, unsigned char* packet)
{
	std::string debugText = "";

	// 다른 클라이언트에게 새로운 클라이언트 알림
	for (int ci = 0; ci < MAX_USER; ++ci)
	{
		if (!gClientsList[ci].isConnect) continue;
		if (ci == key) continue;
		SendPacket(ci, packet);

		debugText = "ProcessPacket		:: " + std::to_string(ci) + " client <= " + std::to_string(key) + " client data";
		DisplayDebugText(debugText);
	}

	// 새로 들어온 클라이언트에게 이미 들어와있던 플레이어 정보들을 넘김
	Packet::Connect preOtherPlayerPacket;
	preOtherPlayerPacket.size = sizeof(Packet::Connect);
	preOtherPlayerPacket.type = (BYTE)PacketType::Connect;
	for (int ci = 0; ci < MAX_USER; ++ci)
	{
		if (!gClientsList[ci].isConnect) continue;
		if (ci == key) continue;
		preOtherPlayerPacket.id = ci;
		preOtherPlayerPacket.posX = gClientsList[ci].player.pos.x;
		preOtherPlayerPacket.posY = gClientsList[ci].player.pos.y;
		preOtherPlayerPacket.posZ = gClientsList[ci].player.pos.z;
		SendPacket(key, reinterpret_cast<unsigned char*>(&preOtherPlayerPacket));

		debugText = "ProcessPacket		:: "  + std::to_string(key) + " client <= " + std::to_string(ci) + " client data";
		DisplayDebugText(debugText);
	}

	return;
}

void OnReceivePacket::PlayerMove(int key, unsigned char* packet)
{
	int id = 0;
	Vector3 pos;

	Packet::Player::Move *data = reinterpret_cast<Packet::Player::Move*>(packet);

	id = data->id;
	pos.x = data->posX;
	pos.y = data->posY;
	pos.z = data->posZ;

	if (gClientsList[id].isConnect)
	{
		gClientsList[id].player.pos.x = pos.x;
		gClientsList[id].player.pos.y = pos.y;
		gClientsList[id].player.pos.z = pos.z;
	}
	
	for (int ci = 0; ci < MAX_USER; ++ci)
	{
		if (!gClientsList[ci].isConnect) continue;
		if (gClientsList[ci].id == key) continue;
		SendPacket(ci, packet);
	}

	return;
}

void OnReceivePacket::PlayerLight(int key, unsigned char* packet)
{
	for (int ci = 0; ci < MAX_USER; ++ci)
	{
		if (!gClientsList[ci].isConnect) continue;
		if (gClientsList[ci].id == key) continue;
		SendPacket(ci, packet);
	}

	return;
}

void OnReceivePacket::PlayerShout(int key, unsigned char* packet)
{
	for (int ci = 0; ci < MAX_USER; ++ci)
	{
		if (!gClientsList[ci].isConnect) continue;
		if (gClientsList[ci].id == key) continue;
		SendPacket(ci, packet);
	}
	return;
}

void OnReceivePacket::MonsterMove(int key, unsigned char *packet)
{
	Packet::Monster::Move *data = reinterpret_cast<Packet::Monster::Move*>(packet);

	gLock.lock();
	gMonster.pos.x = data->posX;
	gMonster.pos.y = data->posY;
	gMonster.pos.z = data->posZ;
	gLock.unlock();

	return;
}

void InitializeMonster(void)
{
	gLock.lock();
	gMonster.pos = { 160, 0, -83 };
	gMonster.patrolPos = { tmpMonsterPath.x, tmpMonsterPath.y, tmpMonsterPath.z };
	gLock.unlock();
	return;
}

void SendMonsterSetInfoPacket(int key)
{
	// 새로 들어온 클라이언트에게 몬스터 정보 동기화
	Packet::Monster::SetInfo monsterSetInfoPacket;
	monsterSetInfoPacket.size = sizeof(Packet::Monster::SetInfo);
	monsterSetInfoPacket.type = (BYTE)PacketType::MonsterSetInfo;
	gLock.lock();
	monsterSetInfoPacket.posX = gMonster.pos.x;
	monsterSetInfoPacket.posY = gMonster.pos.y;
	monsterSetInfoPacket.posZ = gMonster.pos.z;
	monsterSetInfoPacket.patrolPosX = gMonster.patrolPos.x;
	monsterSetInfoPacket.patrolPosY = gMonster.patrolPos.y;
	monsterSetInfoPacket.patrolPosZ = gMonster.patrolPos.z;
	gLock.unlock();

	SendPacket(key, reinterpret_cast<unsigned char*>(&monsterSetInfoPacket));

	std::string debugText = "";
	gLock.lock();
	
	debugText = "ProcessPacket		:: " + std::to_string(key) + " client <= gMonster Pos(" 
		+ std::to_string((int)gMonster.pos.x) + ", " + std::to_string((int)gMonster.pos.y) + ", " + std::to_string((int)gMonster.pos.z) + ")";
	DisplayDebugText(debugText);
	
	debugText = "ProcessPacket		:: " + std::to_string(key) + " client <= gMonster Patrol Pos("
		+ std::to_string((int)gMonster.patrolPos.x) + ", " + std::to_string((int)gMonster.patrolPos.y) + ", " + std::to_string((int)gMonster.patrolPos.z) + ")";
	DisplayDebugText(debugText);

	gLock.unlock();

	return;
}
