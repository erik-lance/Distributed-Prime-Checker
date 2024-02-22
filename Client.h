#pragma once

#include <iostream>
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
	Client(char* host, int port);
	~Client();

	void run();
private:
	// Host and port of the master server
	const char* host;
	int port;
};

