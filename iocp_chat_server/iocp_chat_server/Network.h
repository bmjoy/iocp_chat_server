#pragma once

#include "Define.h"
#include "ClientInfo.h"
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <optional>
#include <functional>

class Network
{
public:
    Network() = default;
   ~Network() { WSACleanup(); };
    Error           Init(UINT16 serverPort);
    void            Run();
    void            Destroy();
    void            SendData(stPacket packet);
    ClientInfo*     GetClientInfo(UINT32 id);
    std::optional<std::pair<ClientInfo*, size_t>>     GetClientRecvedPacket();
    void            SendPacket(const stPacket& packet);
    std::function<void(stPacket)> GetPacketSender();

private:
    void            SetMaxThreadCount();
    Error           WinsockStartup();
    Error           CreateListenSocket();
    Error           CreateIOCP();
    void            CreateClient(const UINT32 maxClientCount);
    Error           BindandListen(UINT16 port);
    Error           RegisterListenSocketToIOCP();

    void            SetWokerThread();
    void            WokerThread();
    void            SetSendPacketThread();
    void            SendPacketThread();
    void            ProcAcceptOperation(stOverlappedEx* pOverlappedEx);
    void            ProcRecvOperation(ClientInfo* pClientInfo, DWORD dwIoSize);
    //void            ProcSendOperation(ClientInfo* pClientInfo, DWORD dwIoSize);
    bool            PostSendMsg(ClientInfo* pClientInfo, char* pMsg, UINT32 len);
    void            SetAccepterThread();
    void            AccepterThread();
    void            CloseSocket(ClientInfo* pClientInfo, bool bIsForce = false);
    bool            BindIOCompletionPort(ClientInfo* pClientInfo);
    bool            PostRecv(ClientInfo* pClientInfo);
    void            DestroyThread();
    void            AddToClientPoolRecvPacket(ClientInfo* c, size_t size);

private:
    UINT16                      mMaxThreadCount = 0;
    SOCKET                      mListenSocket   = INVALID_SOCKET;
    HANDLE                  	mIOCPHandle     = INVALID_HANDLE_VALUE;
    bool	                   	mIsWorkerRun    = true;
    bool	                   	mIsAccepterRun  = true;
    bool                        mSendPacketRun  = true;
    //���� �Ǿ��ִ� Ŭ���̾�Ʈ ��
    int			                mClientCnt = 0;
    std::vector<std::thread>    mIOWorkerThreads;
    std::thread                 mAccepterThread;
    std::thread                 mSendPacketThread;
    std::vector<ClientInfo>     mClientInfos;

    std::mutex                  mRecvPacketLock;
    std::queue<std::pair<ClientInfo*, size_t>>     mClientPoolRecvedPacket;
};