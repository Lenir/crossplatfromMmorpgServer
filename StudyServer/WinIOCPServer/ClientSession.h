#pragma once

//#include <WinSock2.h>	move stdafx.h

class ClientSession;

struct OverlappedIO : public OVERLAPPED
{
	OverlappedIO():Object(nullptr)
	{}

	ClientSession* Object;
};

class ClientSession
{
public:
	void OnTick();
	bool OnConnected(SOCKET socket);
	bool PostReceive();

	bool IsConnected();
private:
	SOCKET socket_ = INVALID_SOCKET;
	bool isConnected_ = false;

	SOCKADDR_IN		clientAddr_;	//������ ���� ����ų� �ϸ� ��ȿ���� ������ ������, ���� ǥ�ÿ����� �����صд�

	OverlappedIO overlappedSend_;
	OverlappedIO overlappedRecv_;
};


void CALLBACK RecvCompletion(
	IN DWORD dwError,
	IN DWORD cbTransferred,
	IN LPWSAOVERLAPPED lpOverlapped,
	IN DWORD dwFlags
);
void CALLBACK SendCompletion(
	IN DWORD dwError,
	IN DWORD cbTransferred,
	IN LPWSAOVERLAPPED lpOverlapped,
	IN DWORD dwFlags
);
