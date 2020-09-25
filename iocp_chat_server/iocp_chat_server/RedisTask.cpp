#include "RedisTask.h"
#include <memory>

void RedisTask::SetClientId(UINT32 clientId)
{
	mClientId = clientId;
}
void RedisTask::SetTaskId(REDIS_TASK_ID id)
{
	mTaskID = id;
};
void RedisTask::SetData(const char* data, size_t size)
{
	memcpy_s(mData, size, data, size);
	mDataSize = size;
};