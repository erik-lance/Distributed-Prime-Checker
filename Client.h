#pragma once

#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <thread>
#include <chrono>
#include "Structures.h"
#include "PrimeChecker.h"

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

	bool timing = false; // Timer flag
	std::chrono::steady_clock::time_point start; // Start time
	std::chrono::steady_clock::time_point end; // End time
	std::string primesHex; // Hexadecimal string of primes
	int n_primes = 0;
	
	SOCKET m_socket;
	struct sockaddr_in m_server;

	SOCKET master_socket;
	struct sockaddr_in master_server;

	std::queue<std::string> messages;
	std::thread listener;
	bool isRunning;

	void init();
	void receive();
	
};

