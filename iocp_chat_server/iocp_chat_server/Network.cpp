#include "Network.h"
#include "Define.h"
#include "NetworkConfig.h"
#include <iostream>
#include <string>

using namespace std::chrono;

Error Network::Init(UINT16 SERVER_PORT)
{
	SetMaxThreadCount();

	Error error = Error::NONE;
	error = WinsockStartup();
	if (Error::NONE != error)
		return error;
	
	error = CreateListenSocket();
	if (Error::NONE != error)
		return error;

	error = CreateIOCP();
	if (Error::NONE != error)
		return error;

	CreateClient(MAX_CLIENT);
	
	error = BindandListen(SERVER_PORT);
	if (Error::NONE != error)
		return error;

	error = RegisterListenSocketToIOCP();
	if (Error::NONE != error)
		return error;

	return Error::NONE;
}

Error Network::WinsockStartup()
{
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
	{
		printf("[ERROR] WSAStartup()�Լ� ���� : %d\n", WSAGetLastError());
		return Error::WSASTARTUP;
	}
	return Error::NONE;
}

void Network::SetMaxThreadCount()
{
	//WaingThread Queue�� ��� ���·� ���� ������� ���� ����Ǵ� ���� : (cpu���� * 2) + 1 
	mMaxThreadCount = std::thread::hardware_concurrency() * 2 + 1;
}

Error Network::CreateListenSocket()
{
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == mListenSocket)
	{
		printf("[ERROR] socket()�Լ� ���� : %d\n", WSAGetLastError());
		return Error::SOCKET_CREATE_LISTEN;
	}
	return Error::NONE;
}

Error Network::CreateIOCP()
{
	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mMaxThreadCount);
	if (NULL == mIOCPHandle)
	{
		printf("[ERROR] CreateIoCompletionPort()�Լ� ����: %d\n", GetLastError());
		return Error::IOCP_CREATE;
	}
	return Error::NONE;
}

void Network::CreateClient(const UINT32 maxClientCount)
{
	for (UINT32 i = 0; i < maxClientCount; i++)
	{
		mClientInfos.emplace_back(i);
		mClientInfos[i].SetId(i);
		mIdleClientIds.push(i);
	}
}

Error Network::BindandListen(UINT16 SERVER_PORT)
{
	SOCKADDR_IN stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(SERVER_PORT);		
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
	{
		printf("[ERROR] bind()�Լ� ���� : %d\n", WSAGetLastError());
		return Error::SOCKET_BIND;
	}

	nRet = listen(mListenSocket, 5);
	if (0 != nRet)
	{
		printf("[ERROR] listen()�Լ� ���� : %d\n", WSAGetLastError());
		return Error::SOCKET_LISTEN;
	}

	return Error::NONE;
}

Error Network::RegisterListenSocketToIOCP()
{
	HANDLE handle = CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, NULL, NULL);
	if (handle != mIOCPHandle)
	{
		printf("[ERROR] mListenSocket CreateIoCompletionPort ��� ����: %d\n", GetLastError());
		return Error::SOCKET_REGISTER_IOCP;
	}

	return Error::NONE;
}

//Error Network::SetAsyncAccept()
//{
//	//TODO: �����
//	// ������ ������ ���ڸ��� ���� ������ �߻����� ���� Ŭ���̾�Ʈ ���Ӱ� ������ ����� ��츦 �� ó���ϱ� ���� Accept�� �ִ� ���� ����ŭ �̸� ��û�� �� �ֽ��ϴ�.
//	// ����� 1���� accept�� ��û�� �ߴµ� �ִ� ���� ���� ����ŭ �̸� accept �ϵ��� �մϴ�.
//	//if (SOCKET_ERROR == AsyncAccept(mListenSocket))
//	//{
//	//	printf("[����] AsyncAccept()�Լ� ���� : %d", WSAGetLastError());
//	//	return Error::SOCKET_ASYNC_ACCEPT;
//	//}
//	return Error::NONE;
//}

void Network::Run()
{
	SetWokerThread();
	SetAccepterThread();
	SetSendPacketThread();
}

void Network::SetWokerThread()
{
	for (int i = 0; i < mMaxThreadCount; i++)
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
			ProcSendOperation(pClientInfo, dwIoSize);
		}
		else
		{
			printf("socket(%d)���� ���ܻ�Ȳ\n", (int)pClientInfo->GetClientSocket());
		}
	}
}

void Network::SetSendPacketThread()
{
	mSendPacketThread = std::thread([this]() { SendPacketThread(); });
}

void Network::SendPacketThread()
{
	while (mSendPacketRun)
	{
		if (IsEmptyClientPoolSendPacket())
		{
			Sleep(50);
			continue;
		}

		ClientInfo* clientInfo = GetClientSendingPacket();
		stPacket p = clientInfo->GetSendPacket();

		while (clientInfo->IsSending())
			Sleep(50);

		clientInfo->SetLastSendPacket(p);
		clientInfo->SetSending(true);
		SendData(p);
	}
}

void Network::ProcAcceptOperation(stOverlappedEx* pOverlappedEx)
{
	ClientInfo* pClientInfo = GetClientInfo(pOverlappedEx->m_clientId);
	if (nullptr == pClientInfo)
	{
		return;
	}

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

	++mClientCnt;
}

void Network::ProcRecvOperation(ClientInfo* pClientInfo, DWORD dwIoSize)
{
	AddToClientPoolRecvPacket(pClientInfo, dwIoSize);
	BindRecv(pClientInfo);
}

void Network::ProcSendOperation(ClientInfo* pClientInfo, DWORD dwIoSize)
{
	if (dwIoSize != pClientInfo->GetLastSendPacket().mHeader.mSize)
	{
		//����� ������ �ȵƴٸ� Ǯ�� ���� �տ� �߰��Ͽ� �ٽ� �������� �Ѵ�.
		printf("[ERROR] ���� %d �۽� ����.. %d ������ �õ�..\n", pClientInfo->GetId(), pClientInfo->GetLastSendPacket().mHeader.mPacket_id);
		pClientInfo->AddSendPacketAtFront(pClientInfo->GetLastSendPacket());
		AddToClientPoolSendPacket(pClientInfo);
	}
	else
	{
		printf("���� %d bytes : id : %d, size: %d �۽� �Ϸ�\n", pClientInfo->GetId(), pClientInfo->GetLastSendPacket().mHeader.mPacket_id, dwIoSize);
	}
	pClientInfo->SetLastSendPacket(stPacket());
	pClientInfo->SetSending(false);
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

	//TODO �����
	// SO_LINGER�� �ٷ� ��������Ƿ� shutdown ���ص� �������ϴ�.
	//socketClose������ ������ �ۼ����� ��� �ߴ� ��Ų��.
	//shutdown(pClientInfo->GetClientSocket(), SD_BOTH);

	//���� �ɼ��� �����Ѵ�.
	setsockopt(pClientInfo->GetClientSocket(), SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	//���� ������ ���� ��Ų��. 
	pClientInfo->CloseSocket();
	mIdleClientIds.push(pClientInfo->GetId());
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
	while (mIsAccepterRun)
	{
		for (auto& clientInfo : mClientInfos)
		{
			clientInfo.AsyncAccept(mListenSocket);
		}

		Sleep(50);
	}
}

//ClientInfo* Network::GetEmptyClientInfo()
//{
//	//TODO �����
//	// Ŭ���̾�Ʈ ���� ���� ���� ������� �ʴ� ��ü�� ã�� �͵� �� �δ� �� �� �����Ƿ� 
//	//������� �ʴ� ��ü�� �ε��� ��ȣ�� �����ϰ� ������ ���� ������ ã�� �� ���� �� �����ϴ�.
//	if (mIdleClientIds.empty())
//	{
//		return nullptr;
//	}
//	
//	UINT32 idleClientId = mIdleClientIds.front();
//	mIdleClientIds.pop();
//	return &mClientInfos[idleClientId];
//}

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

std::pair<ClientInfo*, size_t> Network::GetClientRecvedPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	std::pair<ClientInfo*, size_t> front = mClientPoolRecvedPacket.front();
	mClientPoolRecvedPacket.pop();
	return front;
}

bool Network::IsEmptyClientPoolRecvPacket()
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	return mClientPoolRecvedPacket.empty();
}

void Network::AddToClientPoolRecvPacket(ClientInfo* c, size_t size)
{
	std::lock_guard<std::mutex> guard(mRecvPacketLock);
	mClientPoolRecvedPacket.push(std::make_pair(c, size));
}

ClientInfo* Network::GetClientSendingPacket()
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	ClientInfo* front = mClientPoolSendingPacket.front();
	mClientPoolSendingPacket.pop();
	return front;
}

void Network::SendPacket(const stPacket& packet)
{
	ClientInfo* clientInfo = GetClientInfo(packet.mClientTo);
	if (nullptr == clientInfo)
	{
		return;
	}

	clientInfo->AddSendPacket(packet);
	AddToClientPoolSendPacket(clientInfo);
}

bool Network::IsEmptyClientPoolSendPacket()
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	return mClientPoolSendingPacket.empty();
}

void Network::AddToClientPoolSendPacket(ClientInfo* clientInfo)
{
	std::lock_guard<std::mutex> guard(mSendPacketLock);
	mClientPoolSendingPacket.push(clientInfo);
}

void Network::SendData(stPacket packet)
{
	UINT16 packetId = packet.mHeader.mPacket_id;
	char buff[MAX_SOCKBUF] = { 0, };

	stPacketHeader header;
	header.mSize = packet.mHeader.mSize;
	header.mPacket_id = packetId;
	memcpy_s(buff, sizeof(stPacketHeader), &header, sizeof(stPacketHeader));

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

	mSendPacketRun = false;
	if (mSendPacketThread.joinable())
	{
		mSendPacketThread.join();
	}
	  
	mIsAccepterRun = false;
	if (mAccepterThread.joinable())
	{
		mAccepterThread.join();
	}

	closesocket(mListenSocket);
}

void Network::Destroy()
{
	DestroyThread();
}