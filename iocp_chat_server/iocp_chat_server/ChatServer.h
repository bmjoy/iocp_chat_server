#pragma once

#include "Define.h"

#include <vector>
#include <thread>
#include <memory>
#include <unordered_map>

class PacketProcessor;

class ChatServer
{
public:
    ChatServer();

    ~ChatServer();

	Error Init(const UINT16 port);

    void Run();

    void Destroy();

private:
    void Waiting();

private:
    PacketProcessor*            mPacketProcessor = nullptr;

};
