#include "ChatServer.h"
#include "Network.h"
#include "ServerConfig.h"

void ChatServer::Init()
{
	//��Ʈ��ũ �ʱ�ȭ
	NetworkInstance.Init();

	
}

void ChatServer::Run()
{
	NetworkInstance.Run();
	//��Ʈ��ũ�� ��Ŷ ���ۿ��� ��Ŷ�� �����ͼ�
	//��� ������ ���� �б��Ѵ�.
	

	//������� ���� ����
	while (1)
	{
		if (NetworkInstance.IsPacketPoolEmpty())
			continue;

		stPacket p = NetworkInstance.GetPackget();

		switch (p.mHeader.mPacket_id)
		{
		case 1:
			NetworkInstance.SendData(p.mClientId, p.mBody, strlen(p.mBody));
			break;
			
		}
	}
}
void ChatServer::Destroy()
{
	NetworkInstance.Destroy();
}