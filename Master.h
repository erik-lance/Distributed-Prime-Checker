#pragma once

#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <vector>
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
	bool running = true; // Running flag

	// Socket
	int m_socket;
	struct sockaddr_in m_server; // Server Address

	// Queue for messages from the client
	std::queue<client_message> queue;

	// Queue for messages to the client
	std::queue<response_client> sender_queue;

	// Queue for messages from the slave servers
	std::queue<response_slave> slave_queue;

	// Queue for messages to the slave servers
	std::queue<request_slave> slave_sender_queue;

	// Threads
	std::thread client_listener;
	std::thread client_sender;
	std::thread slave_listener;
	std::thread slave_sender;

	// Functions
	void init();
	void loop();
	void start();
	void client_send();
	void client_receive();
	void slave_send();
	void slave_receive();
	void check_slave_msgs();
};

