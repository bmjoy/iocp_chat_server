#pragma once

#include "Define.h"
#include "ClientInfo.h"
#include <thread>
#include <vector>
#include <queue>
#include <mutex>

class Network
{
    // 1. �ʿ�� static_assert

    // 2. ��ũ�� ����

    // 3. friend Ŭ������ �ִٸ� ����

    // 4. �ش� class�� �������� Ÿ�Ժ�Ī�� �ʿ��ϴٸ�, ���� ���� �ռ� �̸� ����

    // 5. ������� ����
private:
    SOCKET      mListenSocket   = INVALID_SOCKET;
    HANDLE		mIOCPHandle     = INVALID_HANDLE_VALUE;
    bool		mIsWorkerRun    = true;
    bool		mIsAccepterRun  = true;
    //���� �Ǿ��ִ� Ŭ���̾�Ʈ ��
    int			mClientCnt = 0;
    std::thread                 mAccepterThread;
    std::vector<std::thread>    mIOWorkerThreads;
    std::vector<ClientInfo>     mClientInfos;
 
    std::queue<ClientInfo*>     mClientPoolRecvedPacket;
    std::queue<ClientInfo*>     mClientPoolSendingPacket;

    std::mutex                  mRecvPacketLock;
    std::mutex                  mSendPacketLock;

    std::unique_ptr<stOverlappedEx> mAcceptOverlappedEx = std::make_unique<stOverlappedEx>();
  
private:
    void CreateListenSocket();
    void BindandListen();
    void CreateIOCP();
    void CreateClient(const UINT32 maxClientCount);

    void SetWokerThread();
    void WokerThread();
    void ProcAcceptOperation(stOverlappedEx* pOverlappedEx);
    void ProcRecvOperation(ClientInfo* pClientInfo, DWORD dwIoSize);
    void ProcSendOperation(ClientInfo* pClientInfo, DWORD dwIoSize);
    bool SendMsg(ClientInfo* pClientInfo, char* pMsg, UINT32 len);
    void SetAccepterThread();
    void AccepterThread();
    void CloseSocket(ClientInfo* pClientInfo, bool bIsForce = false);
    ClientInfo* GetEmptyClientInfo();
    ClientInfo* GetClientInfo(UINT32 id);
    bool BindIOCompletionPort(ClientInfo* pClientInfo);
    bool BindRecv(ClientInfo* pClientInfo);
    void DestroyThread();
    void AddToClientPoolRecvPacket(ClientInfo* c);
    BOOL AsyncAccept(SOCKET listenSocket);

    // 6. ������/�Ҹ��� ����
public:
    Network() = default;
    ~Network() { WSACleanup(); };
    Network(const Network&) = delete;
    Network(Network&&) = delete;
    Network& operator=(const Network&) = delete;
    Network& operator=(Network&&) = delete;

    // 7. ���� ����Լ��� ����
public:

    // 8. ���� ����Լ��� ����
public:

    // 9. �Ϲ� ����Լ��� ����
public:
    void Init();
    void Run();
    void Destroy();

    bool IsEmptyClientPoolRecvPacket();
    ClientInfo* GetClientRecvedPacket();
    
    bool IsEmptyClientPoolSendPacket();
    ClientInfo* GetClientSendingPacket();
    
    void SendData(stPacket packet);
    void AddToClientPoolSendPacket(ClientInfo* c);

    // 10. getter/setter ����Լ��� ����
public:

};