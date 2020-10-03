#pragma once

#include <basetsd.h>
#include <string>

class ChatUser
{
public:
	ChatUser() = default;
	//TODO �����
	// �ڵ忡 �ϰ����� �־�� �մϴ�. �ڵ� ��Ģ�� ���� �ʽ��ϴ�.
	ChatUser(std::string userId, UINT32 clientId);
	~ChatUser() = default;

	std::string			GetUserId()		const { return mUserId; };
	UINT32				GetClientId()	const { return mClientId; }
	UINT32				GetRoomNumber() const { return mRoomNumber; };

	void				SetRoomNumber(UINT32 roomNumber);
		
private:
	std::string			mUserId;
	//TODO �����
	// Ŭ���� ������ ���鿡�� ChatUser�� ClientInfo�� Room ��ü�� �����Ͽ� ������谡 ��������µ� ���ΰ��� �������� �����־����� �մϴ�.
	UINT32				mClientId = 0;
	UINT32				mRoomNumber = 0;
};

