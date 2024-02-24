#pragma once

#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <thread>
#include <chrono>
#include "Structures.h"

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

/**
 * The client takes a start and end point from the user and sends it to the master server.
 */
class Client
{
public:
	Client(std::string host, int port);
	~Client();

	void run();
private:
	std ::string host;
	int port;
	
	SOCKET m_socket;
	struct sockaddr_in m_server;

	void init();
};

