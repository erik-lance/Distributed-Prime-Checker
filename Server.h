#pragma once

#include <iostream>
#include <string>
#include <queue>

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

class Server
{
public:
	Server(std::string udp_host, int udp_port);
	~Server();

	void init();
private:
	// Host and port
	std::string host;
	int port;

	// Socket
	int m_socket;
	struct sockaddr_in m_server;

	// Queue
	std::queue<std::string> queue;
};

