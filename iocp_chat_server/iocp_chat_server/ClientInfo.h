#pragma once
#include "Define.h"
#include <mutex>

class ClientInfo
{
public:
	ClientInfo() = default;
	ClientInfo(const ClientInfo&);
	ClientInfo(UINT32 id);
	
	UINT32			GetId() const { return mId;  };
	SOCKET			GetClientSocket() { return mClientSocket; }
	stPacket		GetRecvPacket();
	stPacket		GetSendPacket();
	bool			IsSending();
	
	char*			GetRecvBuf() { return mRecvBuf; };
	char*			GetSendBuf() { return mSendBuf; };
	bool			IsConnecting();
	void			CloseSocket();
	void			AsyncAccept(SOCKET listenSocket);

	stPacket		GetLastSendPacket();
	void			SetLastSendPacket(const stPacket& packet);

	stOverlappedEx*	GetRecvOverlappedEx() { return &mRecvOverlappedEx; };
	stOverlappedEx*	GetSendOverlappedEx() { return &mSendOverlappedEx; };

	UINT64			GetLatestClosedTimeSec();

	void			SetId(UINT32 id) { mId = id; };
	void			SetClientSocket(SOCKET clientSocket);
	void			AddRecvPacket(stPacket p);
	void			AddSendPacket(stPacket p);
	void			AddSendPacketAtFront(stPacket p);
	void			SetRecvOverlappedEx(stOverlappedEx overlappedEx);
	void			SetSendOverlappedEx(const stOverlappedEx& overlappedEx);
	void			SetSending(bool bSending);
	void			SetIsConnecting(bool isConnecting);
	
private:
	void			SetLatestClosedTimeSec(UINT64 latestClosedTimeSec);
	bool			PostAccept(SOCKET listenSock_, const UINT64 curTimeSec_);

private:
	INT32						mId = 0;
	//TODO: stOverlappedEx�� �ִ°ɷ� ��ü�ϱ�
	SOCKET						mClientSocket = INVALID_SOCKET;
	stOverlappedEx				mAcceptOverlappedEx;
	stOverlappedEx				mRecvOverlappedEx;
	stOverlappedEx				mSendOverlappedEx;

	std::queue<stPacket>        mRecvPacketPool;
	std::deque<stPacket>        mSendPacketPool;

	std::mutex                  mRecvPacketPoolLock;
	std::mutex                  mSendPacketPoolLock;

	stPacket					mLastSendPacket;
	std::mutex                  mLastSendPacketLock;

	//TODO: stOverlappedEx�� �ִ°ɷ� ��ü�ϱ�
	char						mRecvBuf[MAX_SOCKBUF]	= { 0, };
	char						mSendBuf[MAX_SOCKBUF]	= { 0, };
	char						mAcceptBuf[64]			= { 0, };

	bool						m_bSending = false;
	std::mutex                  mSendingLock;

	bool						mIsConnecting = false;
	std::mutex                  mIsConnectingLick;

	UINT64						mLatestClosedTimeSec = 0;;
};