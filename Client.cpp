#include "Client.h"

Client::Client(std::string host, int port)
{
	this->host = host;
	this->port = port;

	// Setup WinSock
	#ifdef _WIN32
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		{
			std::cerr << "Error initializing Winsock" << std::endl;
			exit(1);
		}
	#endif

	init();
}

Client::~Client()
{
}

void Client::init()
{
	// Create a socket
	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_socket < 0)
	{
		std::cerr << "Error creating socket" << std::endl;
		exit(1);
	}

	// Set up the server address
	memset((char*)&m_server, 0, sizeof(m_server));
	m_server.sin_family = AF_INET;
	m_server.sin_port = htons(port);
	InetPtonA(AF_INET, host.c_str(), &m_server.sin_addr); // Convert the host address to a usable format

	// Set the socket to non-blocking
	#ifdef _WIN32
		u_long mode = 1;
		ioctlsocket(m_socket, FIONBIO, &mode);
	#else
		fcntl(m_socket, F_SETFL, O_NONBLOCK);
	#endif

	// Set the socket to reuse the address
	int opt = 1;
	if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) != 0)
	{
		// Print full error details
		char error[1024];
		strerror_s(error, sizeof(error), errno);
		std::cerr << "Error setting socket options: " << error << std::endl;
		exit(1);
	}

	// Bind the socket to the server address
	if (bind(m_socket, (struct sockaddr*)&m_server, sizeof(m_server)) != 0)
	{
		// Print full error details
		char error[1024];
		strerror_s(error, sizeof(error), errno);
		std::cerr << "Error binding socket: " << error << std::endl;
		exit(1);
	}

	// Start the listener thread
	this->isRunning = true;
	this->listener = std::thread(&Client::listen, this);
}

void Client::listen()
{
	std::cout << "Now listening for messages" << std::endl;

	// Master address
	std::string master_host = master_address.substr(0, master_address.find(":"));
	int master_port = atoi(master_address.substr(master_address.find(":") + 1).c_str());

	// Set up the master server address
	struct sockaddr_in master_server;
	memset((char*)&master_server, 0, sizeof(master_server));
	master_server.sin_family = AF_INET;
	master_server.sin_port = htons(master_port);
	InetPtonA(AF_INET, master_host.c_str(), &master_server.sin_addr); // Convert the host address to a usable format

	// Buffer for the message
	char buffer[1024];
	int len = sizeof(master_server);

	while (isRunning)
	{
		// Receive message
		int n = recvfrom(this->m_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&master_server, (socklen_t*)&len);

		if (n < 0)
		{
			// Error handling
			#ifdef _WIN32
				int error_code = WSAGetLastError();
				if (error_code != WSAEWOULDBLOCK) {
					char error[1024];
					strerror_s(error, sizeof(error), error_code);
					std::cerr << "Error receiving message: " << error << std::endl;
					exit(1);
				}
				else {
					continue;
				}
			#else
			if (errno != EWOULDBLOCK && errno != EAGAIN) {
					char error[1024];
					strerror_r(errno, error, sizeof(error));
					std::cerr << "Error receiving message: " << error << std::endl;
					exit(1);
				}
			#endif
		}

		std::string message = std::string(buffer);
		// std::cout << "Received message: " << message << std::endl;
		
		// If message is DONE, stop and print the number of primes
		if (message == "DONE")
		{
			end = std::chrono::steady_clock::now();
			std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

			// Convert primesHex to get the number of primes
			std::vector<int> primes = convertHexPrimes(primesHex);
			n_primes = primes.size();

			timing = false;
		}
		else {
			// Add message to primesHex
			primesHex += message;

			// Reset buffer
			memset(buffer, 0, sizeof(buffer));
		}
	}

}

/**
 * Run the client. Asks input from user and sends it to the master server.
 */
void Client::run()
{
	bool running = true;

	// Master address
	std::string master_host = master_address.substr(0, master_address.find(":"));
	int master_port = atoi(master_address.substr(master_address.find(":") + 1).c_str());

	// Set up the master server address
	struct sockaddr_in master_server;
	memset((char*)&master_server, 0, sizeof(master_server));
	master_server.sin_family = AF_INET;
	master_server.sin_port = htons(master_port);
	InetPtonA(AF_INET, master_host.c_str(), &master_server.sin_addr); // Convert the host address to a usable format

	while (running)
	{
		int start, end;
		std::cout << "Enter start: ";
		std::cin >> start;
		std::cout << "Enter end: ";
		std::cin >> end;


		// Format message to ("C:NUM1,NUM2")
		std::string message = "C:" + std::to_string(start) + "," + std::to_string(end);

		// Sends the range to the master server
		int sent = sendto(m_socket, message.c_str(), message.size(), 0, (struct sockaddr*)&master_server, sizeof(master_server));
		if (sent < 0)
		{
			// Print full error details
			char error[1024];
			strerror_s(error, sizeof(error), errno);
			std::cerr << "Error sending message: " << error << std::endl;
			exit(1);
		}
		else {
			std::cout << "Sent message to master server" << std::endl;

			// Time and wait for response
			this->start = std::chrono::steady_clock::now();
			this->timing = true;

			while (timing)
			{
				// Wait for response
			}

			std::cout << "Number of primes: " << n_primes << std::endl;

		}
	}
}
	