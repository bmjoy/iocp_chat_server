#pragma once

#include <basetsd.h>
#include <string>

class ClientInfo;
class Room;

class ChatUser
{
public:
	ChatUser() = default;
	//TODO �����
	// �ڵ忡 �ϰ����� �־�� �մϴ�. �ڵ� ��Ģ�� ���� �ʽ��ϴ�.
	ChatUser(std::string userId, ClientInfo* clientInfo);
	~ChatUser();

	std::string			GetUserId() const;
	UINT32				GetClientId();
	ClientInfo*			GetClientInfo() const { return mClientInfo; };
	Room*				GetRoom() const { return mRoom; };

	void				SetRoom(Room* room);

private:
	std::string			mUserId;
	//TODO �����
	// Ŭ���� ������ ���鿡�� ChatUser�� ClientInfo�� Room ��ü�� �����Ͽ� ������谡 ��������µ� ���ΰ��� �������� �����־����� �մϴ�.
	ClientInfo*			mClientInfo = nullptr;
	Room*				mRoom = nullptr;
};

