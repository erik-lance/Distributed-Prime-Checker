#pragma once

#include <iostream>
#include <thread>
#include <string>
#include <queue>
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
 * This is the master server. It is responsible for waiting for a request from a client, which is the range
 * of numbers to check prime numbers, and then sends a message to slave servers to check the prime numbers.
 */
class Master
{
public:
	Master(std::string host_address, int port_number);
	~Master();

private:
	std::string host; // Host Address
	int port; // Port Number

	// Socket
	int m_socket;
	struct sockaddr_in m_server; // Server Address

	// Queue for messages
	std::queue<std::string> queue;

	// Threads
	std::thread listener;
	std::thread sender;

	// Functions
	void init();
	void loop();
	void start();
	void send(std::string message);
	void receive();
};

