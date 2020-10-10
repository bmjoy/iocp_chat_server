#pragma once

#include "RedisDefine.h"

//TODO �����. ���� �� �۾��� �ƴϰ� �巡�� �� ������ ���� ���� �� ���Դϴ�.
// ����ȭ �۾��� �Ѵ�.
// �½�ũ�� �����͸� ���Ǻ� ���ۿ� �����ϰ�, �ش� �����͸� �����Ѵ�.

class RedisTask
{
public:
	RedisTask() = default;
	~RedisTask() = default;

	UINT32 GetClientId() const { return mClientId; };
	RedisTaskID GetTaskId() const { return mTaskID; };
	const char* GetData() const { return mData; };
	size_t GetDataSize() const { return mDataSize; };

	void SetClientId(UINT32 clientId);
	void SetTaskId(RedisTaskID id);
	void SetData(const char* data, size_t size);

private:
	UINT32 mClientId = 0;
	RedisTaskID mTaskID = RedisTaskID::INVALID;
	size_t mDataSize = 0;

	char mData[MAX_REDIS_BUF_SIZE] = { 0, };
};

