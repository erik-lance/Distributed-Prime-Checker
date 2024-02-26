#pragma once
#include <vector>
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

constexpr auto MAX_BUFFER = 60000;
constexpr auto MAX_SPLITS = 8192;
constexpr auto num_threads = 4;

// Range of numbers
typedef std::pair<int, int> range;

// Address and Task ID
typedef std::pair<std::string, int> client_details;

// Client Message (Request: <client_details, range>)
typedef std::pair<client_details, range> client_message;

// Master -> Client (Response: <client_details, result>)
typedef std::pair<client_details, std::vector<int>> response_client;

// Master -> Slave (Request: <address, range>)
typedef std::pair<std::string, range> request_slave;

// Slave -> Master (Response: <task id, result>)
typedef std::pair<int, std::string> response_slave;

// Socket and message
typedef std::pair <SOCKET, std::string > socket_message;

// Master Address
const std::string master_address = "192.168.1.5:6379";

// Client Address
const std::string client_address = "192.168.1.5:6378";

// Slave Addresses
const std::vector<std::string> slave_addresses = {
	"192.168.1.4:5000"
};

static inline void packetSplitter(std::string &primesHex, std::queue<std::string> &sender_queue) {
	// Once done calculating, split and add to queue
	// until reached end of message. Splits by MAX_SPLITS
	// at a time.
	std::string delimiter = " ";
	size_t pos = 0;
	std::string token;

	// Message to send that can dynamically grow to MAX_BUFFER size
	std::string message = "";

	const int LARGEST_SIZE = MAX_BUFFER - 1;
	// Get list of primes until MAX_BUFFER - 1
	// If last character is not a space, go back until a space.
	// If MAX_BUFFER - 1 is greater than primesHex length, then
	// add the rest of the primes to the message.
	while (primesHex.length() > LARGEST_SIZE) {
		// If last character is not a space, go back until a space
		if (primesHex[LARGEST_SIZE] != ' ') {
			int i = LARGEST_SIZE;
			while (primesHex[i] != ' ') { i--; }
			// Message must be from 0 to SPACE so that space is included
			message = primesHex.substr(0, i + 1);
			primesHex = primesHex.substr(i + 2); // Remove from primesHex
		}
		else {
			// Message must be from 0 to SPACE so that space is included
			message = primesHex.substr(0, LARGEST_SIZE + 1);
			primesHex = primesHex.substr(LARGEST_SIZE + 2); // Remove from primesHex
		}

		// Add to queue
		sender_queue.push(message);
		message = "";
	}

	// Add the rest of the primes to the message
	message = primesHex;
	primesHex = "";
	sender_queue.push(message);

	// Once done sending all primes, send another message
	// confirming that all primes have been sent
	sender_queue.push("DONE");
}