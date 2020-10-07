#include "RoomManager.h"

RoomManager::RoomManager()
{

}

RoomManager::~RoomManager()
{

}

Room* RoomManager::GetRoom(UINT32 roomNumber)
{
	if (false == IsExistRoom(roomNumber))
	{
		return nullptr;
	}

	return &mRoomDict[roomNumber];
}

bool RoomManager::IsExistRoom(UINT32 roomNumber)
{
	auto item = mRoomDict.find(roomNumber);
	return (item != mRoomDict.end()) ? true : false;
}

//TODO �����
// ���� ������ �ִ� ���� ������ �����Ƿ� �̰͵� ClientInfoó�� ��üǮ�� ����ϴ� ���� �����ϴ�.
// �� ��ȣ�� ��üǮ������ �ε��� ��ġ�� ������ �� ���� ��ȣ�� �˸� �� ��ȣ�� ��ġ�� ���� ã�� �� �ս��ϴ�.
UINT32 RoomManager::CreateRoom()
{
	Room room;
	room.SetRoomNumber(mRoomCount);
	mRoomDict[mRoomCount] = room;
	UINT32 roomNumber = mRoomCount;
	mRoomCount++;
	return roomNumber;
}

void RoomManager::EnterRoom(UINT32 roomNumber, ChatUser* chatUser)
{
	//TODO �����
	// map�� �̷��� ����ϴ� ���� ���� �ʽ��ϴ�. �Ƹ� EnterRoom �Լ��� ȣ������ ��ü�� �ִ� �������� ���̶�� ���������� 
	// ���� ���簡 �ȵǾ����� ���⼭ ũ���� �߻��մϴ�. ����� �ڵ� ����鿡�� ���� �ʽ��ϴ�.
	mRoomDict[roomNumber].AddUser(chatUser);
}