#include "ChatServer.h"

//TODO ����� 
// ����� �� �ִ� ���� ���� ���� ���� �� �ܺο��� �޵��� �մϴ�
// https://jacking75.github.io/C++_argument_parser_flags/  �� ���̺귯���� ����غ���

//TODO �����
// const�� ���� ����غ��ϴ�

//TODO �����
// �߰�ȣ({)�� �� ����մϴ�. if �� ���

//TODO �����
// ������� �ʴ� �ڵ�� �� ���� ��Ź�մϴ�
// �����δ� ���� �ڵ� ������ ���� ������ �ڵ� ���� �� ��Ź�մϴ�
// ������ ������ �ڵ带 �����ϱ� �ٶ��ϴ�

int main()
{
	ChatServer chatServer;

	Error error = Error::NONE;
	error = chatServer.Init();
	if (Error::NONE != error)
	{
		printf("[ERROR] Error Number: %d, Get Last Error: %d\n", error, WSAGetLastError());
		return static_cast<int>(error);
	}

	chatServer.Run();
	chatServer.Destroy();

	return static_cast<int>(Error::NONE);
}

