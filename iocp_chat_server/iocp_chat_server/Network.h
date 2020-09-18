#pragma once

#include "Define.h"
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
    std::thread	                mAccepterThread;
    std::vector<std::thread>    mIOWorkerThreads;
    std::vector<stClientInfo>   mClientInfos;
    //Ʈ���Ű� �� �������� ����Ǯ�� ť���Ѵ�.
    std::queue<stClientInfo*>    mClientPoolRecvPacket;
    std::queue<stClientInfo*>    mClientPoolSendPacket;
    
    
    stPacket                    mSendingPacket;     //���� �������� ��Ŷ

private:
    void CreateSocket();
    void BindandListen();
    void CreateIOCP();
    void CreateClient(const UINT32 maxClientCount);
    void SetWokerThread();
    void WokerThread();
    bool SendMsg(stClientInfo * pClientInfo, char* pMsg, int nLen);
    void SetAccepterThread();
    void AccepterThread();
    void CloseSocket(stClientInfo* pClientInfo, bool bIsForce = false);
    stClientInfo* GetEmptyClientInfo();
    bool BindIOCompletionPort(stClientInfo * pClientInfo);
    bool BindRecv(stClientInfo * pClientInfo);
    void DestroyThread();

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
    stClientInfo* GetClientReceivedPacket();
    
    bool IsEmptyClientPoolSendPacket();
    stClientInfo* GetClientSendPacket();
    
    void SendData(stPacket packet);
    void AddClient(stClientInfo* c);

    // 10. getter/setter ����Լ��� ����
public:

};