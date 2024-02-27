#pragma once

#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <vector>
#include <errno.h>
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
	void start();
private:
	std::string host; // Host Address
	int port; // Port Number
	int n_threads;
	bool running = true; // Running flag
	int machines_done = 0; // Number of machines done
	std::mutex mtx;

	// Socket
	SOCKET m_socket;
	struct sockaddr_in m_server; // Server Address

	// Connections
	std::vector<SOCKET> connected_sockets; // Connected Sockets
	std::vector<bool> socket_done; // Socket Done

	SOCKET client_socket; // Client Socket

	// Queue for messages to process
	std::queue<socket_message> message_queue;

	// Queue for messages to the client
	std::queue<std::string> client_sender_queue;

	// Queue for messages to the slave servers
	std::queue<socket_message> slave_sender_queue;

	std::string primesHex;

	// Threads
	std::thread listener;
	std::thread msg_processor;
	std::thread client_sender;
	std::thread slave_sender;

	// Functions
	void init();
	void loop();
	void client_send();
	void slave_send();
	void receive();
	void processor();
	void split_packets();
};

