#pragma once

#include "Define.h"
#include <vector>
#include <thread>

class ChatServer
{
    // 1. �ʿ�� static_assert

    // 2. ��ũ�� ����

    // 3. friend Ŭ������ �ִٸ� ����

    // 4. �ش� class�� �������� Ÿ�Ժ�Ī�� �ʿ��ϴٸ�, ���� ���� �ռ� �̸� ����

    // 5. ������� ����
private:
    std::thread	                mReceivePacketThread;
    bool                        mReceivePacketRun = true;

private:
    ChatServer() = default;

    void SetReceivePacketThread();
    void ReceivePacketThread();
    void Waiting();

    // 6. ������/�Ҹ��� ����
public:
    ~ChatServer() { WSACleanup(); };
    ChatServer(const ChatServer&) = delete;
    ChatServer(ChatServer&&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;
    ChatServer& operator=(ChatServer&&) = delete;

    // 7. ���� ����Լ��� ����
public:
    static ChatServer& Instance()
    {
        static ChatServer instance;
        return instance;
    }

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

#define ChatServerInstance ChatServer::Instance()
