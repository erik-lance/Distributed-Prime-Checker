#pragma once

#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <vector>
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

// Range of numbers
typedef std::pair<int, int> range;

// Client message
typedef std::pair<std::string, std::string> udp_task;

// Slave Sender message (task id, range)
typedef std::pair<std::string, range> slave_task;

// Slave Receiver message (task id, result)
typedef std::pair<std::string, int> slave_result;

// Client and Task Id (client addr, task id)
typedef std::pair<std::string, std::string> client_task;



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
	std::queue<udp_task> queue;

	// Queue for messages from the slave servers
	std::queue<udp_task> slave_queue;

	// Queue for messages to the slave servers
	std::queue<slave_task> slave_sender_queue;

	// Queue for task id resolvement
	std::queue<client_task> client_task_queue;

	// Threads
	std::thread listener;
	std::thread sender;

	// Functions
	void init();
	void loop();
	void start();
	void send(std::string message);
	void receive();
	void check_slave_msgs();
};

