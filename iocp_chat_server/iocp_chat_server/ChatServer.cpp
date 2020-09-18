#include "ChatServer.h"

#include "ServerConfig.h"
#include <string>
#include <iostream>

void ChatServer::Init()
{
	mNetwork = std::make_unique<Network>();
	//��Ʈ��ũ �ʱ�ȭ
	mNetwork->Init();
}
void ChatServer::Run()
{
	mNetwork->Run();
	SetReceivePacketThread();
	SetSendPacketThread();
	Waiting();
}
void ChatServer::Waiting()
{
	printf("�ƹ� Ű�� ���� ������ ����մϴ�\n");
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
void ChatServer::SetReceivePacketThread()
{
	mReceivePacketThread = std::thread([this]() { ReceivePacketThread(); });
}
void ChatServer::ReceivePacketThread()
{
	while (mReceivePacketRun)
	{
		if (mNetwork->IsEmptyClientPoolRecvPacket())
			continue;

		stClientInfo* clientInfo = mNetwork->GetClientReceivedPacket();

		if (clientInfo->mReceivePacketPool.empty())
			continue;

		stPacket p = clientInfo->mReceivePacketPool.front();
		clientInfo->mReceivePacketPool.pop();

		if (0 == p.mHeader.mSize)
			continue;

		PacketID packetId = static_cast<PacketID>(p.mHeader.mPacket_id);
		switch (packetId)
		{
		case PacketID::DEV_ECHO:
			clientInfo->mSendPacketPool.push(p);
			mNetwork->AddClient(clientInfo);
			break;

		}
	}
}
void ChatServer::SetSendPacketThread()
{
	mSendPacketThread = std::thread([this]() { SendPacketThread(); });
}
void ChatServer::SendPacketThread()
{
	while (mSendPacketRun)
	{
		if (mNetwork->IsEmptyClientPoolSendPacket())
			continue;

		stClientInfo* c = mNetwork->GetClientSendPacket();
		stPacket p = c->mSendPacketPool.front();
		mNetwork->SendData(p);
		//p ���� �����ؼ� �۽� �Ϸ� �� ���� �ϱ�
		//c->mSendPacketPool.pop();
		//mNetwork->SendData(p);
	}
}
void ChatServer::Destroy()
{
	//���� ���� �߿��Ѱ�?
	mReceivePacketRun = false;
	if (mReceivePacketThread.joinable())
	{
		mReceivePacketThread.join();
	}

	mSendPacketRun = false;
	if (mSendPacketThread.joinable())
	{
		mSendPacketThread.join();
	}

	mNetwork->Destroy();
}