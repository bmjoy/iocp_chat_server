#include "ChatServer.h"

#include "ServerConfig.h"
#include <string>
#include <iostream>

void ChatServer::Init()
{
	mNetwork = std::make_unique<Network>();
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
		{
			//sleep
			continue;
		}
		
		ClientInfo* clientInfo = mNetwork->GetClientRecvedPacket();

		stPacket p = clientInfo->GetRecvPacket();
		if (0 == p.mHeader.mSize)
			continue;

		PacketID packetId = static_cast<PacketID>(p.mHeader.mPacket_id);
		switch (packetId)
		{
		case PacketID::DEV_ECHO:
			p.mClientTo = p.mClientFrom;
			clientInfo->AddSendPacket(p);
			mNetwork->AddToClientPoolSendPacket(clientInfo);
			break;
		}
	}
}
void ChatServer::SetSendPacketThread()
{
	//mSendPacketThread = std::thread([this]() { SendPacketThread(); });
	//������ ������ ����?
	for (int i = 0; i < 2; i++)
	{
		mSendPacketThreads.emplace_back([this]() { SendPacketThread(); });
	}
}
void ChatServer::SendPacketThread()
{
	while (mSendPacketRun)
	{
		if (mNetwork->IsEmptyClientPoolSendPacket())
		{
			//sleep
			continue;
		}
			
		ClientInfo* c = mNetwork->GetClientSendingPacket();
		
		stPacket p = c->GetSendPacket();
		if (0 == p.mHeader.mSize)
		{
			continue;
		}

		//TODO: �������� ��Ŷ�� �ִ��� Ȯ��
		//Sleep ���� �ٸ� ������� �������� ��������
		while (c->IsSending())
		{
			Sleep(50);
		};

		//TODO: �̰� ��ġ �ű���
		c->SetLastSendPacket(p);
		mNetwork->SendData(p);
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
	//if (mSendPacketThread.joinable())
	//{
	//	mSendPacketThread.join();
	//}
	for (auto& th : mSendPacketThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}

	mNetwork->Destroy();
}