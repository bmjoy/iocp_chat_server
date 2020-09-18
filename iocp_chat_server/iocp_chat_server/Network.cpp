#pragma comment(lib, "ws2_32")

#include "Network.h"
#include "Define.h"
#include "NetworkConfig.h"

#include <iostream>
#include <string>


std::unique_ptr<Network> Network::mInstance = nullptr;
std::once_flag Network::mflag;

Network& Network::Instance()
{
	std::call_once(Network::mflag, []() {
		Network::mInstance.reset(new Network);
		});

	return *Network::mInstance;
}
void Network::Init()
{
	CreateSocket();
	BindandListen();
	CreateIOCP();
	CreateClient(MAX_CLIENT);
}
//������ �ʱ�ȭ�ϴ� �Լ�
void Network::CreateSocket()
{
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		printf("[ERROR] WSAStartup()�Լ� ���� : %d\n", WSAGetLastError());
		throw Error::WSASTARTUP;
	}

	//���������� TCP , Overlapped I/O ������ ����
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == mListenSocket)
	{
		printf("[����] socket()�Լ� ���� : %d\n", WSAGetLastError());
		throw Error::SOCKET_CREATE;
	}
}
void Network::BindandListen()
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
		throw Error::SOCKET_BIND;
	}

	//���� ��û�� �޾Ƶ��̱� ���� cIOCompletionPort������ ����ϰ� 
	//���Ӵ��ť�� 5���� ���� �Ѵ�.
	nRet = listen(mListenSocket, 5);
	if (0 != nRet)
	{
		printf("[����] listen()�Լ� ���� : %d\n", WSAGetLastError());
		throw Error::SOCKET_LISTEN;
	}
}
void Network::CreateIOCP()
{
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKERTHREAD);
	if (NULL == mIOCPHandle)
	{
		printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
		throw Error::IOCP_CREATE;
	}
}
void Network::CreateClient(const UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; ++i)
	{
		mClientInfos.emplace_back();
	}
}
void Network::Run()
{
	SetWokerThread();

	SetAccepterThread();
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
	stClientInfo* pClientInfo = nullptr;
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
			(PULONG_PTR)&pClientInfo,		// CompletionKey
			&lpOverlapped,				// Overlapped IO ��ü
			INFINITE);					// ����� �ð�

		//����� ������ ���� �޼��� ó��..
		if (TRUE == bSuccess && 0 == dwIoSize && NULL == lpOverlapped)
		{
			mIsWorkerRun = false;
			continue;
		}

		if (NULL == lpOverlapped)
		{
			continue;
		}

		//client�� ������ ��������..			
		if (FALSE == bSuccess || (0 == dwIoSize && TRUE == bSuccess))
		{
			printf("socket(%d) ���� ����\n", (int)pClientInfo->m_socketClient);

			stPacketHeader header;
			header.mSize = 0;
			char body[MAX_SOCKBUF] = { 0, };
			mReceivePacketPool.push(stPacket(pClientInfo->m_socketClient, header, body, MAX_SOCKBUF));

			CloseSocket(pClientInfo);
			continue;
		}

		//��Ŷ Ǯ�� �����.

		auto pOverlappedEx = (stOverlappedEx*)lpOverlapped;

		//Overlapped I/O Recv�۾� ��� �� ó��
		if (IOOperation::RECV == pOverlappedEx->m_eOperation)
		{
			pClientInfo->mRecvBuf[dwIoSize] = '\0';

			//����Ľ�
			stPacketHeader header;
			memcpy_s(&header.mSize, sizeof(UINT16), pClientInfo->mRecvBuf, sizeof(UINT16));
			memcpy_s(&header.mPacket_id, sizeof(UINT16), &pClientInfo->mRecvBuf[2], sizeof(UINT16));

			char body[MAX_SOCKBUF] = {0,};
			UINT32 bodySize = (UINT32)dwIoSize - PACKET_HEADER_SIZE;
			memcpy_s(body, bodySize, &pClientInfo->mRecvBuf[PACKET_HEADER_SIZE], bodySize);
		
			mReceivePacketPool.push(stPacket(pClientInfo->m_socketClient, header, body, bodySize));

			BindRecv(pClientInfo);
		}
		//Overlapped I/O Send�۾� ��� �� ó��
		else if (IOOperation::SEND == pOverlappedEx->m_eOperation)
		{
			pClientInfo->mSendBuf[dwIoSize] = '\0';
			//����Ľ�
			stPacketHeader header;
			memcpy_s(&header.mSize, sizeof(UINT16), pClientInfo->mSendBuf, sizeof(UINT16));
			memcpy_s(&header.mPacket_id, sizeof(UINT16), &pClientInfo->mSendBuf[2], sizeof(UINT16));

			char body[MAX_SOCKBUF] = { 0, };
			UINT32 bodySize = (UINT32)dwIoSize - PACKET_HEADER_SIZE;
			memcpy_s(body, bodySize, &pClientInfo->mRecvBuf[PACKET_HEADER_SIZE], bodySize);

			printf("[�۽�] bytes : %d , msg : %s\n", dwIoSize, body);
		}
		//���� ��Ȳ
		else
		{
			printf("socket(%d)���� ���ܻ�Ȳ\n", (int)pClientInfo->m_socketClient);
		}
	}
}
//������ ������ ���� ��Ų��.
void Network::CloseSocket(stClientInfo* pClientInfo, bool bIsForce)
{
	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����

// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
	if (true == bIsForce)
	{
		stLinger.l_onoff = 1;
	}

	//socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
	shutdown(pClientInfo->m_socketClient, SD_BOTH);

	//���� �ɼ��� �����Ѵ�.
	setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//���� ������ ���� ��Ų��. 
	closesocket(pClientInfo->m_socketClient);

	pClientInfo->m_socketClient = INVALID_SOCKET;
}
//WSASend Overlapped I/O�۾��� ��Ų��.
bool Network::SendMsg(stClientInfo* pClientInfo, char* pMsg, int nLen)
{
	DWORD dwRecvNumBytes = 0;

	//���۵� �޼����� ����
	CopyMemory(pClientInfo->mSendBuf, pMsg, nLen);
	pClientInfo->mSendBuf[nLen] = '\0';

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.len = nLen;
	pClientInfo->m_stSendOverlappedEx.m_wsaBuf.buf = pClientInfo->mSendBuf;
	pClientInfo->m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;

	int nRet = WSASend(pClientInfo->m_socketClient,
		&(pClientInfo->m_stSendOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED) & (pClientInfo->m_stSendOverlappedEx),
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
bool Network::BindRecv(stClientInfo* pClientInfo)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	//Overlapped I/O�� ���� �� ������ ������ �ش�.
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	pClientInfo->m_stRecvOverlappedEx.m_wsaBuf.buf = pClientInfo->mRecvBuf;
	pClientInfo->m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

	int nRet = WSARecv(pClientInfo->m_socketClient,
		&(pClientInfo->m_stRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) & (pClientInfo->m_stRecvOverlappedEx),
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
		stClientInfo* pClientInfo = GetEmptyClientInfo();
		if (NULL == pClientInfo)
		{
			printf("[����] Client Full\n");
			return;
		}

		//Ŭ���̾�Ʈ ���� ��û�� ���� ������ ��ٸ���.
		pClientInfo->m_socketClient = accept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen);
		if (INVALID_SOCKET == pClientInfo->m_socketClient)
		{
			continue;
		}

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
		printf("Ŭ���̾�Ʈ ���� : IP(%s) SOCKET(%d)\n", clientIP, (int)pClientInfo->m_socketClient);

		//Ŭ���̾�Ʈ ���� ����
		++mClientCnt;
	}
}
stClientInfo* Network::GetEmptyClientInfo()
{
	for (auto& client : mClientInfos)
	{
		if (INVALID_SOCKET == client.m_socketClient)
		{
			return &client;
		}
	}

	return nullptr;
}
//CompletionPort��ü�� ���ϰ� CompletionKey��
//�����Ű�� ������ �Ѵ�.
bool Network::BindIOCompletionPort(stClientInfo* pClientInfo)
{
	//socket�� pClientInfo�� CompletionPort��ü�� �����Ų��.
	auto hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->m_socketClient
		, mIOCPHandle
		, (ULONG_PTR)(pClientInfo), 0);

	if (NULL == hIOCP || mIOCPHandle != hIOCP)
	{
		printf("[����] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
		return false;
	}

	return true;
}
stPacket Network::ReceivePacket()
{
	stPacket front = mReceivePacketPool.front();
	mReceivePacketPool.pop();
	return front;
}
bool Network::IsReceivePacketPoolEmpty()
{
	return mReceivePacketPool.empty();
}
void Network::AddPacket(const stPacket& p)
{
	mSendPacketPool.push(p);
}
bool Network::IsSendPacketPoolEmpty()
{
	return mSendPacketPool.empty();
}
stPacket Network::GetSendPacket()
{
	stPacket front = mSendPacketPool.front();
	mSendPacketPool.pop();
	return front;
}
void Network::SendData(UINT32 userId, char* pMsg, int nLen)
{
	std::vector<stClientInfo>::iterator ptr;
	for (ptr = mClientInfos.begin(); ptr != mClientInfos.end(); ++ptr)
	{
		if (ptr->m_socketClient != userId)
			continue;

		char buff[MAX_SOCKBUF] = { 0, };

		//��� ���� ä���� ������.
		stPacketHeader header;
		header.mSize = static_cast<UINT16>(strlen(pMsg) + PACKET_HEADER_SIZE);
		header.mPacket_id = 1;	
		memcpy_s(buff, sizeof(stPacketHeader), &header, sizeof(stPacketHeader));

		UINT32 bodySize = strlen(pMsg);
		memcpy_s(&buff[PACKET_HEADER_SIZE], bodySize, pMsg, bodySize);

		SendMsg(&(*ptr), buff, header.mSize);
		break;
	}
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