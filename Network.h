#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

class Network
{
public:
	Network();
	~Network();

	bool init();
	bool createServer(int port);
	bool connectToServer(const char* ip, int port);
	bool sendData(const char* data, int length);
	bool receiveData(char* data, int length);
	void closeConnection();

private:
	int serverSocket;
	int clientSocket;

};

