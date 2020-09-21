#pragma once

#include "Define.h"
#include <vector>
#include <thread>
#include <memory>
#include "Network.h"

class ChatServer
{
    // 1. �ʿ�� static_assert

    // 2. ��ũ�� ����

    // 3. friend Ŭ������ �ִٸ� ����

    // 4. �ش� class�� �������� Ÿ�Ժ�Ī�� �ʿ��ϴٸ�, ���� ���� �ռ� �̸� ����

    // 5. ������� ����
private:
    std::thread	                mReceivePacketThread;
    //std::thread	                mSendPacketThread;
    std::vector<std::thread>    mSendPacketThreads;
    bool                        mReceivePacketRun = true;
    bool                        mSendPacketRun = true;
    std::unique_ptr<Network>    mNetwork;

private:
    void SetReceivePacketThread();
    void ReceivePacketThread();
    void SetSendPacketThread();
    void SendPacketThread();
    void Waiting();

    // 6. ������/�Ҹ��� ����
public:
    ChatServer() = default;
    ~ChatServer() { WSACleanup(); };
    ChatServer(const ChatServer&) = delete;
    ChatServer(ChatServer&&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;
    ChatServer& operator=(ChatServer&&) = delete;

    // 7. ���� ����Լ��� ����
public:

    // 8. ���� ����Լ��� ����
public:

    // 9. �Ϲ� ����Լ��� ����
public:
    void Init();
    void Run();
    void Destroy();

    // 10. getter/setter ����Լ��� ����
public:

};
