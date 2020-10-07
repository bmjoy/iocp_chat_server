#pragma once

#include "Network.h"
#include <basetsd.h>
#include <list>
#include <functional>

class ChatUser;

//TODO �����
// �ڵ� �ϰ����� ���� GetRoomNumber() ó�� 1�������� ������ �Լ��� ��� ���Ͽ� ���Ǹ� ���� �ƴϸ� �ٸ� Ŭ����ó�� �Լ� ���Ǵ� cpp�� ���� �ϰ��� �ְ� ������ ���ڽ��ϴ�.
// ���� 1�������� ������ ���� ��� ���Ͽ� �����ϴ� ���� ��õ�մϴ�.
class Room
{
public:
	UINT32 GetRoomNumber() { return mRoomNumber; };
	std::list<ChatUser*>* GetUserList() { return &mUserList; };
	void SetRoomNumber(UINT32 roomNumber);
	void AddUser(ChatUser* chatUser);
	void RemoveUser(ChatUser* chatUser);

	//TODO �����
	// �� �Լ��� ȣ���ϴ� ������ �Ź� std::function<void(stPacket)>�� ���� �ѱ�� �ֽ��ϴ�.
	// ȣ���ϴ� ������ �ѹ��� ���� ������ �����ϰų� Ȥ�� ���⿡ std::function<void(stPacket)>�� static���� ���� �Ź� ��ü�� �������� �ʵ��� �մϴ�.
	void Notify(UINT32 clientFrom, UINT16 packetId, const char* body, size_t bodySize, std::function<void(stPacket)> packetSender);

private:
	UINT32						mRoomNumber = 0;

	//TODO �����
	//�濡 ���� �������� ���� ���� �ʴٸ� vector ����� ��õ�մϴ�. 
	// �ڷᱸ�� ���鿡���� list�� ������ ������� ���� �ʴٸ� vector�� ����ϱ� ���ϰ� ������ ������ ������ �����ϴ�.
	// ������ �޸� �Ҵ��� ���̰� ĳ�ÿ� ���Ƽ� �� ���� ���� �ֽ��ϴ�.
	std::list<ChatUser*>		mUserList;
};

