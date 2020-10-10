#include "ChatServer.h"
#include "../thirdparty/flags.h"
#include <iostream>

//TODO 최흥배
// const를 적극 사용해봅니다

//TODO 최흥배
// 사용하지 않는 코드는 꼭 삭제 부탁합니다
// 앞으로는 제가 코드 리뷰할 때는 그전에 코드 정리 꼭 부탁합니다
// 언제나 깨끗한 코드를 유지하기 바랍니다

int main(int argc, char* argv[])
{
	const flags::args args(argc, argv);

	const auto port = args.get<int>("port");
	if (!port) {
		std::cerr << "No Port." << std::endl;
		return static_cast<int>(Error::PORT);

	}
	std::cout << "Port: " << *port << std::endl;

	ChatServer chatServer;

	Error error = Error::NONE;
	error = chatServer.Init(*port);
	if (Error::NONE != error)
	{
		printf("[ERROR] Error Number: %d, Get Last Error: %d\n", error, WSAGetLastError());
		return static_cast<int>(error);
	}

	chatServer.Run();
	chatServer.Destroy();

	return static_cast<int>(Error::NONE);
}

