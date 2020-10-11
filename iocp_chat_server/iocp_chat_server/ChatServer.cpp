#include "ChatServer.h"
#include "PacketProcessor.h"
#include "ServerConfig.h"
#include <string>
#include <iostream>

ChatServer::ChatServer()
{
	mPacketProcessor = new PacketProcessor();
}

ChatServer::~ChatServer()
{
	delete mPacketProcessor;
	mPacketProcessor = nullptr;

	WSACleanup();
}

Error ChatServer::Init(const UINT16 port = SERVER_PORT)
{
	Error error = Error::NONE;

	error = mPacketProcessor->Init(port);

	return error;
}

void ChatServer::Run()
{
	mPacketProcessor->Run();

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

void ChatServer::Destroy()
{
	mPacketProcessor->Destroy();
}