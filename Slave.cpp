#include "Slave.h"

Slave::Slave(int n)
{
	this->slave_id = n;
	this->n_threads = num_threads;
	this->isRunning = true;

	// If windows, initialize winsock
	#ifdef _WIN32
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		{
			std::cerr << "Error initializing Winsock" << std::endl;
			exit(1);
		}
	#endif

	init();

	// Activate thread
	this->listener = std::thread(&Slave::listen, this);
	processor();
}

Slave::~Slave()
{
	this->isRunning = false;
	this->listener.join();
}

void Slave::init()
{
	// Prepare socket
	this->m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->m_socket < 0)
	{
		std::cerr << "Error creating socket" << std::endl;
		exit(1);
	}

	// Set up the address
	this->m_server.sin_family = AF_INET;

	// Get address of master
	int port = atoi(master_address.substr(master_address.find(":") + 1).c_str());
	std::string host = master_address.substr(0, master_address.find(":"));

	this->m_server.sin_port = htons(port);
	InetPtonA(AF_INET, host.c_str(), &this->m_server.sin_addr); // Convert the host address to a usable format

	// Set the socket to reuse the address
	int opt = 1;
	if (setsockopt(this->m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) != 0)
	{
		// Print full error details
		char error[1024];
		strerror_s(error, sizeof(error), errno);
		std::cerr << "Error setting socket options: " << error << std::endl;
		exit(1);
	}

	// Set socket to be able to use MAX_BUFFER size
	int buffer_size = MAX_BUFFER;
	if (setsockopt(this->m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&buffer_size, sizeof(buffer_size)) != 0)
	{
		// Print full error details
		char error[1024];
		strerror_s(error, sizeof(error), errno);
		std::cerr << "Error setting socket buffer size: " << error << std::endl;
		exit(1);
	}

	// Connect to the master
	if (connect(this->m_socket, (struct sockaddr*)&this->m_server, sizeof(this->m_server)) < 0)
	{
		// Print full error details
		char error[1024];
		strerror_s(error, sizeof(error), errno);
		std::cerr << "Error connecting to master: " << error << std::endl;
		exit(1);
	}

	std::cout << "Connected to master!: " << master_address << std::endl;
}

/**
 * Reads through messages and processes them
 */
void Slave::processor()
{
	while (isRunning)
	{
		// If there are messages in the queue
		if (!messages.empty())
		{
			// Get the message
			std::string message = messages.front();
			messages.pop();

			int sent = send(this->m_socket, message.c_str(), message.length(), 0);

			if (sent < 0)
			{
				// Print full error details
				char error[1024];
				strerror_s(error, sizeof(error), errno);
				std::cerr << "Error sending message: " << error << std::endl;
				exit(1);
			}

			// Log the message
			std::cout << "Slave " << slave_id << " sent message " << message.length() << " bytes" << std::endl;

			if (message == "DONE")
			{
				std::cout << "Slave " << slave_id << " is done" << std::endl;
			}
		}
	}
}

/**
 * Listens for messages from the master and adds to message queue
 */
void Slave::listen()
{
	while (isRunning)
	{
		// Buffer for message
		std::vector<char> buffer(MAX_BUFFER);

		// Receive message
		int n = recv(this->m_socket, buffer.data(), MAX_BUFFER, 0);

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

		// Add message to queue
		std::string message = std::string(buffer.data(), n);

		std::cout << "Slave " << slave_id << " received message: " << message << std::endl;

		// Parse message to range (NUM1,NUM2)
		int start = atoi(message.substr(0, message.find(",")).c_str());
		int end = atoi(message.substr(message.find(",") + 1).c_str());
		

		// Split the range into n_threads
		int range_size_per_thread = getSizePerThread(start, end, n_threads);
		range n_range = std::make_pair(start, start + range_size_per_thread);

		// Prepare threads
		std::vector<std::thread> threads;
		std::string primesHex;

		for (int i = 0; i < n_threads; i++)
		{
			// Use `primeCheckerHex` from PrimeChecker.h which takes
			// (range r, std::string& primes, std::mutex& mtx)
			threads.push_back(std::thread(primeCheckerHex, n_range, std::ref(primesHex), std::ref(mtx)));

			// Update range
			n_range.first = n_range.second + 1;
			n_range.second = n_range.first + range_size_per_thread;

			// Clamp range
			if (n_range.second > end) n_range.second = end;
		}

		// Join threads
		for (int i = 0; i < n_threads; i++)
		{
			threads[i].join();
		}
		
		// Print num of primes before splitting
		//int n_primes = countHexPrimes(primesHex);
		//std::cout << "Number of primes: " << n_primes << std::endl;

		// Split packets
		packetSplitter(primesHex, messages);

		// Reset everything
		primesHex = "";
		buffer.clear();
	}
}