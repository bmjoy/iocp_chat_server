#include "ChatServer.h"
#include "ServerConfig.h"
#include "RedisConfig.h"
#include "Packet.h"
#include "RedisPacket.h"
#include "RedisTask.h"
#include "Packet.h"
#include <string>
#include <iostream>

Error ChatServer::Init()
{
	Error error = Error::NONE;

	mNetwork = std::make_unique<Network>();
	error = mNetwork->Init(SERVER_PORT);
	if (Error::NONE != error)
		return error;
	
	mRedis = std::make_unique<Redis>();
	error = mRedis->Connect(REDIS_IP, REDIS_PORT);
	if (Error::NONE != error)
		return error;

	RegisterRecvProc();

	return error;
}

void ChatServer::RegisterRecvProc()
{
	mRecvPacketProcDict[PacketID::DEV_ECHO] = &ChatServer::ProcEcho;
	mRecvPacketProcDict[PacketID::LOGIN_REQ] = &ChatServer::ProcLogin;
	mRecvPacketProcDict[PacketID::ROOM_ENTER_REQ] = &ChatServer::ProcRoomEnter;
	mRecvPacketProcDict[PacketID::ROOM_CHAT_REQ] = &ChatServer::ProcRoomChat;
	mRecvPacketProcDict[PacketID::ROOM_LEAVE_REQ] = &ChatServer::ProcRoomLeave;
}

void ChatServer::ProcEcho(stPacket packet)
{
	ClientInfo* clientInfo = mNetwork->GetClientInfo(packet.mClientFrom);
	if (nullptr == clientInfo)
		return;

	packet.mClientTo = packet.mClientFrom;
	clientInfo->AddSendPacket(packet);
	mNetwork->AddToClientPoolSendPacket(clientInfo);
}

void ChatServer::ProcLogin(stPacket packet)
{
	LoginReqPacket loginReqPacket;
	loginReqPacket.SetPacket(packet.mBody);
	printf("Login User Id : %s passwd : %s \n", loginReqPacket.GetUserId(), loginReqPacket.GetUserPw());

	char buf[MAX_USER_ID_BYTE_LENGTH + MAX_USER_PW_BYTE_LENGTH] = { 0, };
	memcpy_s(buf, strlen(loginReqPacket.GetUserId()), loginReqPacket.GetUserId(), strlen(loginReqPacket.GetUserId()));
	memcpy_s(&buf[MAX_USER_ID_BYTE_LENGTH], strlen(loginReqPacket.GetUserPw()), loginReqPacket.GetUserPw(), strlen(loginReqPacket.GetUserPw()));
	LoginReqRedisPacket redisReqPacket(packet.mClientFrom, REDIS_TASK_ID::REQUEST_LOGIN, buf, MAX_USER_ID_BYTE_LENGTH + MAX_USER_PW_BYTE_LENGTH);
	mRedis->RequestTask(redisReqPacket.GetTask());
}

void ChatServer::ProcRoomEnter(stPacket packet)
{
	ChatUser* chatUser = mChatUserManager.GetUser(packet.mClientFrom);
	if (nullptr == chatUser)
		return;

	RoomEnterReqPacket reqPacket(packet.mBody, packet.GetBodySize());
	UINT32 roomNumber = 0;
	if (mRoomManager.IsExistRoom(reqPacket.GetRoomNumber()))
	{
		roomNumber = reqPacket.GetRoomNumber();
	}
	else
	{
		roomNumber = mRoomManager.CreateRoom();
	}

	chatUser->SetRoomNumber(roomNumber);
	mRoomManager.EnterRoom(roomNumber, chatUser);

	ERROR_CODE result = ERROR_CODE::NONE;
	size_t resBodySize = sizeof(result);
	char resBody[MAX_SOCKBUF] = { 0, };
	memcpy_s(resBody, resBodySize, &result, resBodySize);
	
	SendPacket(
		packet.mClientFrom, 
		packet.mClientFrom, 
		static_cast<UINT16>(PacketID::ROOM_ENTER_RES),
		resBody,
		resBodySize
	);

	//방 유저 리스트 내려주기
	Room* room = mRoomManager.GetRoom(roomNumber);
	auto userList = room->GetUserList();
	UINT16 userCount = static_cast<UINT16>(userList->size()) -1; //자신은 제외
	if (0 < userCount)
	{
		RoomUserListNTFPacket roomUserListNTFPacket(chatUser->GetClientId(), *userList);
		SendPacket(
			packet.mClientFrom,
			packet.mClientFrom,
			static_cast<UINT16>(PacketID::ROOM_USER_LIST_NTF),
			roomUserListNTFPacket.GetBody(),
			roomUserListNTFPacket.GetBodySize()
		);
	}

	//방 유저들에게 노티
	RoomEnterNTFPacket roomEnterNTFPacket(chatUser->GetClientId(), chatUser->GetUserId());
	room->Notify(
		chatUser->GetClientId(),
		static_cast<UINT16>(PacketID::ROOM_NEW_USER_NTF),
		roomEnterNTFPacket.GetBody(),
		roomEnterNTFPacket.GetBodySize(),
		mNetwork.get()
	);
}

void ChatServer::ProcRoomChat(stPacket packet)
{
	const ChatUser* chatUser = mChatUserManager.GetUser(packet.mClientFrom);
	Room* room = mRoomManager.GetRoom(chatUser->GetRoomNumber());
	if (nullptr == chatUser || nullptr == room)
	{
		return;
	}

	char msg[MAX_CHAT_MSG_SIZE] = { 0, };
	size_t msgLenth = strlen(packet.mBody);
	memcpy_s(&msg, msgLenth, packet.mBody, msgLenth);
	const size_t ntfBodySize = MAX_USER_ID_BYTE_LENGTH + MAX_CHAT_MSG_SIZE;
	char ntfBody[ntfBodySize] = { 0, };
	memcpy_s(ntfBody, chatUser->GetUserId().length(), chatUser->GetUserId().c_str(), chatUser->GetUserId().length());
	memcpy_s(&ntfBody[MAX_USER_ID_BYTE_LENGTH], msgLenth, msg, msgLenth);

	room->Notify(
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_CHAT_NOTIFY),
		ntfBody,
		ntfBodySize,
		mNetwork.get()
	);


	ERROR_CODE result = ERROR_CODE::NONE;
	const size_t resBodySize = sizeof(result);
	char resBody[resBodySize] = {0,};
	memcpy_s(resBody, resBodySize, &result, resBodySize);

	SendPacket(
		packet.mClientFrom,
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_CHAT_RES),
		resBody,
		resBodySize
	);
}

void ChatServer::ProcRoomLeave(stPacket packet)
{
	ChatUser* chatUser = mChatUserManager.GetUser(packet.mClientFrom);
	Room* room = mRoomManager.GetRoom(chatUser->GetRoomNumber());
	if (nullptr == chatUser || nullptr == room)
	{
		return;
	}

	UINT64 userUniqueId = chatUser->GetClientId();
	const size_t ntfBodySize = sizeof(userUniqueId);
	char ntfBody[ntfBodySize] = { 0, };
	memcpy_s(ntfBody, ntfBodySize, &userUniqueId, ntfBodySize);
	room->Notify(
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_LEAVE_USER_NTF),
		ntfBody,
		ntfBodySize,
		mNetwork.get()
	);

	room->RemoveUser(chatUser);
	
	ERROR_CODE result = ERROR_CODE::NONE;
	const size_t resBodySize = sizeof(result);
	char resBody[resBodySize] = { 0, };
	memcpy_s(resBody, resBodySize, &result, resBodySize);

	SendPacket(
		packet.mClientFrom,
		packet.mClientFrom,
		static_cast<UINT16>(PacketID::ROOM_LEAVE_RES),
		resBody,
		resBodySize
	);
}

void ChatServer::SendPacket(UINT32 from, UINT32 to, UINT16 packetId, char* body, size_t bodySize)
{
	stPacketHeader header;
	header.mSize = static_cast<UINT16>(bodySize + PACKET_HEADER_SIZE);
	header.mPacket_id = packetId;

	mNetwork->SendPacket(
		stPacket(
			from,
			to,
			header,
			body,
			bodySize
		)
	);
}

void ChatServer::Run()
{
	mNetwork->Run();
	mRedis->Run();
	
	SetReceivePacketThread();
	SetRedisResponseThread();
	Waiting();
}

void ChatServer::Waiting()
{
	printf("아무 키나 누를 때까지 대기합니다\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}
}
void ChatServer::SetRedisResponseThread()
{
	mRedisResponseThread = std::thread([this]() { RedisResponseThread(); });
}

void ChatServer::RedisResponseThread()
{
	while (mRedisResponseRun)
	{
		RedisTask task = mRedis->GetResponseTask();

		if (REDIS_TASK_ID::INVALID == task.GetTaskId())
		{
			Sleep(50);
			continue;
		}

		if (REDIS_TASK_ID::RESPONSE_LOGIN == task.GetTaskId())
		{
			LoginResRedisPacket loginResRedisPacket(task.GetClientId(), task.GetTaskId(), task.GetData(), task.GetDataSize());
			ERROR_CODE loginResult = loginResRedisPacket.GetResult();
			UINT16 bodySize = sizeof(loginResult);

			SendPacket(
				task.GetClientId(),
				task.GetClientId(),
				static_cast<UINT16>(PacketID::LOGIN_RES),
				reinterpret_cast<char*>(&loginResult),
				bodySize
			);

			if (ERROR_CODE::NONE == loginResRedisPacket.GetResult())
			{
				ChatUser chatUser(loginResRedisPacket.GetUserId(), task.GetClientId());
				mChatUserManager.AddUser(chatUser);
			}
		}
	}
}

void ChatServer::SetReceivePacketThread()
{
	mReceivePacketThread = std::thread([this]() { ReceivePacketThread(); });
}

void ChatServer::ReceivePacketThread()
{
	while (mReceivePacketRun)
	{
		if (mNetwork->IsEmptyClientPoolRecvPacket())
		{
			Sleep(50);
			continue;
		}
		
		std::pair<ClientInfo*, size_t> recvedPacketInfo = mNetwork->GetClientRecvedPacket();
		ClientInfo* pClientInfo = recvedPacketInfo.first;
		size_t dwIoSize = recvedPacketInfo.second;

		stPacketHeader header;
		memcpy_s(&header.mSize, sizeof(UINT16), pClientInfo->GetRecvBuf(), sizeof(UINT16));
		memcpy_s(&header.mPacket_id, sizeof(UINT16), &pClientInfo->GetRecvBuf()[2], sizeof(UINT16));

		char body[MAX_SOCKBUF] = { 0, };
		UINT32 bodySize = (UINT32)dwIoSize - PACKET_HEADER_SIZE;
		memcpy_s(body, bodySize, &pClientInfo->GetRecvBuf()[PACKET_HEADER_SIZE], bodySize);

		ProcessPacket(stPacket(pClientInfo->GetId(), 0, header, body, bodySize));
	}
}

void ChatServer::ProcessPacket(stPacket p)
{
	PacketID packetId = static_cast<PacketID>(p.mHeader.mPacket_id);
	auto iter = mRecvPacketProcDict.find(packetId);
	if (iter != mRecvPacketProcDict.end())
	{
		(this->*(iter->second))(p);
	}
}

void ChatServer::Destroy()
{
	mReceivePacketRun = false;
	if (mReceivePacketThread.joinable())
	{
		mReceivePacketThread.join();
	}

	mNetwork->Destroy();
}