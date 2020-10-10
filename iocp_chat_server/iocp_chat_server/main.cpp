#include "ChatServer.h"
#include "../thirdparty/flags.h"
#include <iostream>

//TODO ����� 
// ����� �� �ִ� ���� ���� ���� ���� �� �ܺο��� �޵��� �մϴ�
// https://jacking75.github.io/C++_argument_parser_flags/  �� ���̺귯���� ����غ���

//TODO �����
// const�� ���� ����غ��ϴ�

//TODO �����
// ������� �ʴ� �ڵ�� �� ���� ��Ź�մϴ�
// �����δ� ���� �ڵ� ������ ���� ������ �ڵ� ���� �� ��Ź�մϴ�
// ������ ������ �ڵ带 �����ϱ� �ٶ��ϴ�

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

