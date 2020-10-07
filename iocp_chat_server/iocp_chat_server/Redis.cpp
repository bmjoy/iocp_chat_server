#include "Redis.h"
#include "RedisPacket.h"
#include "Define.h"
//include ���� �߿�! CRedisConn�� ���� �ö󰡸� �ߺ� ���� ���� �߻�
//Define.h ���� winsock2 �� include CRedisConn.h ���� winsock�� include �ϱ� ������
//�ߺ� ���� ���� �߻�
#include "../thirdparty/CRedisConn.h"
#include <sstream>


Redis::Redis()
	:mConn(nullptr)
{
	mConn = new RedisCpp::CRedisConn;
}

Redis::~Redis()
{
	delete mConn;
}

Error Redis::Connect(const char* ip, unsigned port)
{
	bool ret = mConn->connect(ip, port);
	if (false == ret)
		return Error::REDIS_CONNECT;

	return Error::NONE;
}

void Redis::Run()
{
	mThread = std::thread([this]() { RedisThread(); });
	mIsThreadRun = true;
}

void Redis::Destroy()
{
	mIsThreadRun = false;
	if (mThread.joinable())
	{
		mThread.join();
	}
}

void Redis::RedisThread()
{
	while (mIsThreadRun)
	{
		auto reqTaskOpt = GetRequestTask();
		//TODO �����
		// std::nullopt �� ���ؾ� ���� �� �����ϴ�. GetRequestTask() ���� std::nullopt�� ��ȯ�ϴ� 
		if (false == reqTaskOpt.has_value())
		{
			Sleep(1);
			continue;
		}

		RedisTask reqTask = reqTaskOpt.value();

		//TODO �����
		//�巡�� ������ case������ ó���� �� �ֵ��� ����ȭ �ϱ� �ٶ��ϴ�. ��Ŷ ó����ó��
		if (REDIS_TASK_ID::REQUEST_LOGIN == reqTask.GetTaskId())
		{
			ERROR_CODE error_code = ERROR_CODE::LOGIN_USER_INVALID_PW;

			LoginReqRedisPacket reqPacket(reqTask);
			const size_t bufSize = MAX_USER_ID_BYTE_LENGTH + sizeof(ERROR_CODE);
			char buf[bufSize] = { 0, };
			std::string pw;
			if (mConn->get(reqPacket.GetUserId(), pw))
			{
				//TODO �����
				// �Ʒ� 2���� if���� �ϳ��� ��ġ��
				if (pw.compare(reqPacket.GetUserPw()) == 0)
				{
					error_code = ERROR_CODE::NONE;
				}

				if (ERROR_CODE::NONE == error_code)
				{
					memcpy_s(buf, strlen(reqPacket.GetUserId()), reqPacket.GetUserId(), strlen(reqPacket.GetUserId()));
				}
				
			}
			memcpy_s(&buf[MAX_USER_ID_BYTE_LENGTH], sizeof(error_code), &error_code, sizeof(error_code));
			LoginResRedisPacket resPacket(reqPacket.GetClientId(), REDIS_TASK_ID::RESPONSE_LOGIN, buf);
			ResponseTask(resPacket.GetTask());
		}
	}
}

void Redis::ProcLogin(const RedisTask& task)
{
	ERROR_CODE error_code = ERROR_CODE::LOGIN_USER_INVALID_PW;

	LoginReqRedisPacket reqPacket(task);
	const size_t bufSize = MAX_USER_ID_BYTE_LENGTH + sizeof(ERROR_CODE);
	char buf[bufSize] = { 0, };
	std::string pw;
	if (mConn->get(reqPacket.GetUserId(), pw))
	{
		if (pw.compare(reqPacket.GetUserPw()) == 0)
		{
			error_code = ERROR_CODE::NONE;
		}

		if (ERROR_CODE::NONE == error_code)
		{
			//TODO �����
			// �������� ���� strlen�� ������� ���� ������ �Լ��� ����մϴ�.
			memcpy_s(buf, strlen(reqPacket.GetUserId()), reqPacket.GetUserId(), strlen(reqPacket.GetUserId()));
		}

	}
	memcpy_s(&buf[MAX_USER_ID_BYTE_LENGTH], sizeof(error_code), &error_code, sizeof(error_code));
	LoginResRedisPacket resPacket(reqPacket.GetClientId(), REDIS_TASK_ID::RESPONSE_LOGIN, buf);
	ResponseTask(resPacket.GetTask());
}

void Redis::RequestTask(const RedisTask& task)
{
	std::lock_guard<std::mutex> guard(mRequestTaskLock);
	mRequestTaskPool.push(task);
}

void Redis::ResponseTask(const RedisTask& task)
{
	std::lock_guard<std::mutex> guard(mResponseTaskLock);
	mResponseTaskPool.push(task);
}

std::optional<RedisTask> Redis::GetRequestTask()
{
	std::lock_guard<std::mutex> guard(mRequestTaskLock);
	if (mRequestTaskPool.empty())
	{
		return std::nullopt;
	}
	
	RedisTask task = mRequestTaskPool.front();
	mRequestTaskPool.pop();
	return task;
}

std::optional<RedisTask> Redis::GetResponseTask()
{
	std::lock_guard<std::mutex> guard(mResponseTaskLock);
	if (mResponseTaskPool.empty())
	{
		return std::nullopt;
	}
	
	RedisTask task = mResponseTaskPool.front();
	mResponseTaskPool.pop();
	return task;
}
