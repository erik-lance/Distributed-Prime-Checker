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
	// Send a message to the master server to close the connection
	send(m_socket, "CLOSE", 5, 0);

	// Close the socket
	#ifdef _WIN32
		closesocket(m_socket);
		WSACleanup();
	#else
		close(m_socket);
	#endif
}

void Client::init()
{
	// Create a socket
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket < 0)
	{
		std::cerr << "Error creating socket" << std::endl;
		exit(1);
	}

	// Set up the server address (master's address)
	memset((char*)&m_server, 0, sizeof(m_server));
	m_server.sin_family = AF_INET;
	m_server.sin_port = htons(atoi(master_address.substr(master_address.find(":") + 1).c_str()));
	InetPtonA(AF_INET, master_address.substr(0, master_address.find(":")).c_str(), &m_server.sin_addr);

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

	// Set socket to be able to use MAX_BUFFER size
	int buffer_size = MAX_BUFFER;
	if (setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&buffer_size, sizeof(buffer_size)) != 0)
	{
		// Print full error details
		char error[1024];
		strerror_s(error, sizeof(error), errno);
		std::cerr << "Error setting socket buffer size: " << error << std::endl;
		exit(1);
	}

	// Don't fragment packets
	int opt2 = 1;
	if (setsockopt(m_socket, IPPROTO_TCP, IP_DONTFRAGMENT, (char*)&opt2, sizeof(opt2)) != 0)
	{
		// Print full error details
		char error[1024];
		strerror_s(error, sizeof(error), errno);
		std::cerr << "Error setting socket options: " << error << std::endl;
		exit(1);
	}

	// Start the listener thread
	this->isRunning = true;
}

/**
 * Run the client. Asks input from user and sends it to the master server.
 */
void Client::run()
{
	bool running = true;

	std::cout << "Connecting to master server... : " << master_address << std::endl;

	// Connect to the master server
	if (connect(m_socket, (struct sockaddr*)&m_server, sizeof(m_server)) < 0)
	{
		// Print full error details
		#ifdef _WIN32
			int error_code = WSAGetLastError();
			char error[1024];
			strerror_s(error, sizeof(error), error_code);
			std::cerr << "[" << error_code << "] Error connecting to master server: " << error << std::endl;
			exit(1);
		#else
			char error[1024];
			strerror_r(errno, error, sizeof(error));
			std::cerr << "Error connecting to master server: " << error << std::endl;
			exit(1);
		#endif
	}

	std::cout << "Connected to master server" << std::endl;

	while (running)
	{
		int start_n, end_n;
		std::cout << "Enter start: ";
		std::cin >> start_n;
		std::cout << "Enter end: ";
		std::cin >> end_n;


		// Format message to ("C:NUM1,NUM2")
		std::string message = "C:" + std::to_string(start_n) + "," + std::to_string(end_n);

		// Sends the range to the master server
		int sent = send(m_socket, message.c_str(), message.size(), 0);
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

			// Block until done
			while (timing)
			{
				// Receive message
				std::vector<char> buffer(MAX_BUFFER);
				int n = recv(m_socket, buffer.data(), buffer.size(), 0);

				if (n < 0)
				{
					// Error handling
					#ifdef _WIN32
						int error_code = WSAGetLastError();
						if (error_code != WSAEWOULDBLOCK) {
							char error[1024];
							strerror_s(error, sizeof(error), error_code);
							std::cerr << "[" << error_code << "] Error receiving message: " << error << std::endl;
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

				std::string message = std::string(buffer.data(), n);

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
					primesHex += message + " ";
					
					// Reset the buffer
					buffer.clear();
				}
			}

			std::cout << "Number of primes: " << n_primes << std::endl;

			// Ask user if they want to print primes
			std::string print;
			std::cout << "Print primes? (y/n): ";
			std::cin >> print;

			if (print == "y")
			{
				// Print primes
				std::vector<int> primes = convertHexPrimes(primesHex);

				// Reorder
				std::cout << "Primes: ";
				for (int i = 0; i < primes.size(); i++)
				{
					std::cout << primes[i] << " ";
				}
				std::cout << std::endl;
			}

			// Clear primesHex
			primesHex = "";

		}
	}
}
	