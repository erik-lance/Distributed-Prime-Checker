#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <iostream>
#include "PrimeChecker.h"
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
 * Receives messages from master. Processes range (start,end) of numbers 
 * and returns a list of prime numbers in that range.
 */
class Slave
{
public:
	Slave(int n);
	~Slave();
private:
	int m_socket; // Socket
	struct sockaddr_in m_server; // Server Address

	std::queue<request_slave> requests;
	std::vector<int> primes;
	std::thread listener;
	bool isRunning;

	void init();
	void processor();
	void listen();
};

