#pragma comment(lib, "Ws2_32.lib")	//AcceptEx
#pragma comment(lib, "mswsock.lib") //AcceptEx

#include "Network.h"
#include "Define.h"
#include "NetworkConfig.h"
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <iostream>
#include <string>

void Network::Init(UINT16 SERVER_PORT)
{
	CreateListenSocket();
	CreateIOCP();
	CreateClient(MAX_CLIENT);
	BindandListen(SERVER_PORT);
	
}
//������ �ʱ�ȭ�ϴ� �Լ�
void Network::CreateListenSocket()
{
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		printf("[ERROR] WSAStartup()�Լ� ���� : %d\n", WSAGetLastError());
		return;
	}

	//���������� TCP , Overlapped I/O ������ ����
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == mListenSocket)
	{
		printf("[����] socket()�Լ� ���� : %d\n", WSAGetLastError());
		return;
	}
}
void Network::BindandListen(UINT16 SERVER_PORT)
{
	SOCKADDR_IN		stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(SERVER_PORT); //���� ��Ʈ�� �����Ѵ�.		
	//� �ּҿ��� ������ �����̶� �޾Ƶ��̰ڴ�.
	//���� ������� �̷��� �����Ѵ�. ���� �� �����ǿ����� ������ �ް� �ʹٸ�
	//�� �ּҸ� inet_addr�Լ��� �̿��� ������ �ȴ�.
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//������ ������ ���� �ּ� ������ cIOCompletionPort ������ �����Ѵ�.
	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
	{
		printf("[����] bind()�Լ� ���� : %d\n", WSAGetLastError());
		return;
	}

	//���� ��û�� �޾Ƶ��̱� ���� cIOCompletionPort������ ����ϰ� 
	//���Ӵ��ť�� 5���� ���� �Ѵ�.
	nRet = listen(mListenSocket, 5);
	if (0 != nRet)
	{
		printf("[����] listen()�Լ� ���� : %d\n", WSAGetLastError());
		return;
	}

	CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, NULL, NULL);
	if (SOCKET_ERROR == AsyncAccept(mListenSocket))
	{
		printf("[����] AsyncAccept()�Լ� ���� : %d", WSAGetLastError());
		return;
	}
}
void Network::CreateIOCP()
{
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (NULL == mIOCPHandle)
	{
		printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
		return;
	}
}
void Network::CreateClient(const UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; i++)
	{
		mClientInfos.emplace_back(i);
		mClientInfos[i].SetId(i);
	}
}
void Network::Run()
{
	SetWokerThread();
	//SetAccepterThread();
}
void Network::SetWokerThread()
{
	//WaingThread Queue�� ��� ���·� ���� ������� ���� ����Ǵ� ���� : (cpu���� * 2) + 1 
	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mIOWorkerThreads.emplace_back([this]() { WokerThread(); });
	}
	printf("WokerThread ����..\n");
}
void Network::WokerThread()
{
	//CompletionKey�� ���� ������ ����
	ClientInfo* pClientInfo = nullptr;
	//�Լ� ȣ�� ���� ����
	BOOL bSuccess = TRUE;
	//Overlapped I/O�۾����� ���۵� ������ ũ��
	DWORD dwIoSize = 0;
	//I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
	LPOVERLAPPED lpOverlapped = NULL;

	while (mIsWorkerRun)
	{
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,					// ������ ���۵� ����Ʈ
			(PULONG_PTR)&pClientInfo,	// CompletionKey
			&lpOverlapped,				// Overlapped IO ��ü
			INFINITE);					// ����� �ð�
 
		if (NULL == lpOverlapped)
			continue;

		//����� ������ ���� �޼��� ó��..
		if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			mIsWorkerRun = false;
			continue;
		}

		auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;
		if (IOOperation::ACCEPT == pOverlappedEx->m_eOperation)
		{
			ProcAcceptOperation(pOverlappedEx);
			continue;
		}
		else if (IOOperation::RECV == pOverlappedEx->m_eOperation)
		{
			//check close client..			
			if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
			{
				printf("socket(%d) ���� ����\n", (int)pClientInfo->GetClientSocket());
				CloseSocket(pClientInfo);
				continue;
			}

			ProcRecvOperation(pClientInfo, dwIoSize);
		}
		else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			//��Ŷ ���� Ȯ��
			//ProcSendOperation(pClientInfo, dwIoSize);
		}
		//����
		else
		{
			printf("socket(%d)���� ���ܻ�Ȳ\n", (int)pClientInfo->GetClientSocket());
		}
	}
}
void Network::ProcAcceptOperation(stOverlappedEx* pOverlappedEx)
{
	PSOCKADDR pRemoteSocketAddr = nullptr;
	PSOCKADDR pLocalSocketAddr = nullptr;
	INT pRemoteSocketAddrLength = 0;
	INT pLocalSocketAddrLength = 0;

	//���� ���
	GetAcceptExSockaddrs(
		pOverlappedEx->m_buffer, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&pLocalSocketAddr, &pLocalSocketAddrLength, &pRemoteSocketAddr, &pRemoteSocketAddrLength);

	SOCKADDR_IN remoteAddr = *(reinterpret_cast<PSOCKADDR_IN>(pRemoteSocketAddr));
	//������ Ŭ���̾�Ʈ IP�� ��Ʈ ���� ���
	char ip[24] = { 0, };
	inet_ntop(AF_INET, &remoteAddr.sin_addr, ip, sizeof(ip));
	printf("Accept New  IP %s PORT: %d \n", ip, ntohs(remoteAddr.sin_port));

	ClientInfo* pClientInfo = GetEmptyClientInfo();
	if (nullptr == pClientInfo)
	{
		return;
	}

	pClientInfo->SetClientSocket(pOverlappedEx->m_clientSocket);
	//I/O Completion Port��ü�� ������ �����Ų��.
	bool bRet = BindIOCompletionPort(pClientInfo);
	if (false == bRet)
	{
		return;
	}

	//Recv Overlapped I/O�۾��� ��û�� ���´�.
	bRet = BindRecv(pClientInfo);
	if (false == bRet)
	{
		return;
	}

	//Ŭ���̾�Ʈ ���� ����
	++mClientCnt;

	//accept �ٽ� ���
	AsyncAccept(mListenSocket);
}
void Network::ProcRecvOperation(ClientInfo* pClientInfo, DWORD dwIoSize)
{
	//����Ľ�
	stPacketHeader header;
	memcpy_s(&header.mSize, sizeof(UINT16), pClientInfo->GetRecvBuf(), sizeof(UINT16));
	memcpy_s(&header.mPacket_id, sizeof(UINT16), &pClientInfo->GetRecvBuf()[2], sizeof(UINT16));

	char body[MAX_SOCKBUF] = { 0, };
	UINT32 bodySize = (UINT32)dwIoSize - PACKET_HEADER_SIZE;
	memcpy_s(body, bodySize, &pClientInfo->GetRecvBuf()[PACKET_HEADER_SIZE], bodySize);

	pClientInfo->AddRecvPacket(stPacket(pClientInfo->GetId(), 0, header, body, bodySize));

	AddToClientPoolRecvPacket(pClientInfo);

	BindRecv(pClientInfo);
}
void Network::ProcSendOperation(ClientInfo* pClientInfo, DWORD dwIoSize)
{
	if (dwIoSize != pClientInfo->GetLastSendPacket().mHeader.mSize)
	{
		//������ �ȵ� ������ �κи� �������� ����
		printf("[ERROR] �۽� ����.. ������ �õ�..\n");
		pClientInfo->AddSendPacket(pClientInfo->GetLastSendPacket());
	}
	else
	{
		pClientInfo->SetSending(false);
		printf("���� %d bytes : %d �۽� �Ϸ�\n", pClientInfo->GetId(), dwIoSize);
	}
}
//������ ������ ���� ��Ų��.
void Network::CloseSocket(ClientInfo* pClientInfo, bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����

// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
	shutdown(pClientInfo->GetClientSocket(), SD_BOTH);

	//���� �ɼ��� �����Ѵ�.
	setsockopt(pClientInfo->GetClientSocket(), SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//���� ������ ���� ��Ų��. 
	closesocket(pClientInfo->GetClientSocket());

	pClientInfo->SetClientSocket(INVALID_SOCKET);
}
//WSASend Overlapped I/O�۾��� ��Ų��.
bool Network::SendMsg(ClientInfo* pClientInfo, char* pMsg, UINT32 len)
{
	DWORD dwRecvNumBytes = 0;

	//���۵� �޼����� ����
	CopyMemory(pClientInfo->GetSendBuf(), pMsg, len);
	pClientInfo->GetSendBuf()[len] = '\0';

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
	pClientInfo->GetSendOverlappedEx()->m_wsaBuf.len = len;
	pClientInfo->GetSendOverlappedEx()->m_wsaBuf.buf = pClientInfo->GetSendBuf();
	pClientInfo->GetSendOverlappedEx()->m_eOperation = IOOperation::SEND;

	int nRet = WSASend(pClientInfo->GetClientSocket(),
		&(pClientInfo->GetSendOverlappedEx()->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED)(pClientInfo->GetSendOverlappedEx()),
		NULL);

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[����] WSASend()�Լ� ���� : %d\n", WSAGetLastError());
		return false;
	}
	return true;
}
//WSARecv Overlapped I/O �۾��� ��Ų��.
bool Network::BindRecv(ClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
	pClientInfo->GetRecvOverlappedEx()->m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->GetRecvOverlappedEx()->m_wsaBuf.buf = pClientInfo->GetRecvBuf();
	pClientInfo->GetRecvOverlappedEx()->m_eOperation = IOOperation::RECV;

	int nRet = WSARecv(pClientInfo->GetClientSocket(),
		&(pClientInfo->GetRecvOverlappedEx()->m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) (pClientInfo->GetRecvOverlappedEx()),
		NULL);

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		printf("[����] WSARecv()�Լ� ���� : %d\n", WSAGetLastError());
		return false;
	}

	return true;
}
void Network::SetAccepterThread()
{
	mAccepterThread = std::thread([this]() { AccepterThread(); });
	printf("AccepterThread ����..\n");
}
void Network::AccepterThread()
{
	SOCKADDR_IN		stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (mIsAccepterRun)
	{
		//������ ���� ����ü�� �ε����� ���´�.
		ClientInfo* pClientInfo = GetEmptyClientInfo();
		if (NULL == pClientInfo)
		{
			printf("[����] Client Full\n");
			return;
		}

		//Ŭ���̾�Ʈ ���� ��û�� ���� ������ ��ٸ���.
		SOCKET socketClient = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
		
		if (INVALID_SOCKET == socketClient)
		{
			continue;
		}

		pClientInfo->SetClientSocket(socketClient);

		//I/O Completion Port��ü�� ������ �����Ų��.
		bool bRet = BindIOCompletionPort(pClientInfo);
		if (false == bRet)
		{
			return;
		}

		//Recv Overlapped I/O�۾��� ��û�� ���´�.
		bRet = BindRecv(pClientInfo);
		if (false == bRet)
		{
			return;
		}

		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		printf("Ŭ���̾�Ʈ ���� : IP(%s) SOCKET(%d)\n", clientIP, (int)pClientInfo->GetClientSocket());

		//Ŭ���̾�Ʈ ���� ����
		++mClientCnt;
	}
}
ClientInfo* Network::GetEmptyClientInfo()
{
	for (auto& client : mClientInfos)
	{
		if (INVALID_SOCKET == client.GetClientSocket())
		{
			return &client;
		}
	}

	return nullptr;
}
ClientInfo* Network::GetClientInfo(UINT32 id)
{
	return id >= MAX_CLIENT ? nullptr : &mClientInfos.at(id);
}
//CompletionPort��ü�� ���ϰ� CompletionKey��
//�����Ű�� ������ �Ѵ�.
bool Network::BindIOCompletionPort(ClientInfo* pClientInfo)
{
	//socket�� pClientInfo�� CompletionPort��ü�� �����Ų��.
	auto hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->GetClientSocket()
		, mIOCPHandle
		, (ULONG_PTR)(pClientInfo), 0);

	if (NULL == hIOCP || mIOCPHandle != hIOCP)
	{
		printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
		return false;
	}

	return true;
}
ClientInfo* Network::GetClientRecvedPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	ClientInfo* front = mClientPoolRecvedPacket.front();
	mClientPoolRecvedPacket.pop();
	return front;
}
bool Network::IsEmptyClientPoolRecvPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	return mClientPoolRecvedPacket.empty();
}
void Network::AddToClientPoolRecvPacket(ClientInfo* c)
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	mClientPoolRecvedPacket.push(c);
}
BOOL Network::AsyncAccept(SOCKET listenSocket)
{
	BOOL ret = false;
	
	ZeroMemory(&mAcceptOverlappedEx->m_wsaOverlapped, sizeof(mAcceptOverlappedEx->m_wsaOverlapped));
	ZeroMemory(mAcceptOverlappedEx->m_buffer, MAX_SOCKBUF);
	mAcceptOverlappedEx->m_eOperation = IOOperation::ACCEPT;
	mAcceptOverlappedEx->m_clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	DWORD bytes = 0;
	ret = AcceptEx(
		listenSocket, 
		mAcceptOverlappedEx->m_clientSocket,
		mAcceptOverlappedEx->m_buffer, 0,
		sizeof(SOCKADDR_IN) + 16, 
		sizeof(SOCKADDR_IN) + 16, 
		&bytes, 
		reinterpret_cast<LPOVERLAPPED>(&mAcceptOverlappedEx->m_wsaOverlapped)
	);

	return ret;
}
ClientInfo* Network::GetClientSendingPacket()
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	ClientInfo* front = mClientPoolSendingPacket.front();
	mClientPoolSendingPacket.pop();
	return front;
}
bool Network::IsEmptyClientPoolSendPacket()
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	return mClientPoolSendingPacket.empty();
}
void Network::AddToClientPoolSendPacket(ClientInfo* c)
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	mClientPoolSendingPacket.push(c);
}
void Network::SendData(stPacket packet)
{
	//char* pMsg = packet.mBody;
	//int nLen = sizeof(packet.mBody);
	UINT16 packetId = packet.mHeader.mPacket_id;

	char buff[MAX_SOCKBUF] = { 0, };

	//��� ���� ä���� ������.
	stPacketHeader header;
	//header.mSize = static_cast<UINT16>(strlen(pMsg) + PACKET_HEADER_SIZE);
	header.mSize = packet.mHeader.mSize;
	// TODO: ���� ���� �ؾ���!
	header.mPacket_id = packetId;
	memcpy_s(buff, sizeof(stPacketHeader), &header, sizeof(stPacketHeader));

	//UINT32 bodySize = strlen(pMsg);
	UINT32 bodySize = packet.mHeader.mSize - PACKET_HEADER_SIZE;
	memcpy_s(&buff[PACKET_HEADER_SIZE], bodySize, packet.mBody, bodySize);

	ClientInfo* c = GetClientInfo(packet.mClientTo);
	SendMsg(c, buff, header.mSize);

}
//�����Ǿ��ִ� �����带 �ı��Ѵ�.
void Network::DestroyThread()
{
	mIsWorkerRun = false;
	CloseHandle(mIOCPHandle);
	
	for (auto& th : mIOWorkerThreads)
	{
		if (th.joinable())
		{
			th.join();
		}
	}

	//Accepter �����带 �����Ѵ�.
	mIsAccepterRun = false;
	closesocket(mListenSocket);

	if (mAccepterThread.joinable())
	{
		mAccepterThread.join();
	}
}
void Network::Destroy()
{
	DestroyThread();
}