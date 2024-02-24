#include "Slave.h"

Slave::Slave(int n, int n_threads)
{
	this->slave_id = n;
	this->n_threads = n_threads;
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
	this->m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (this->m_socket < 0)
	{
		std::cerr << "Error creating socket" << std::endl;
		exit(1);
	}

	// Set up the address
	this->m_server.sin_family = AF_INET;

	// Get address of slave
	std::string addr = slave_addresses[slave_id];
	int port = atoi(addr.substr(addr.find(":") + 1).c_str());
	std::string host = addr.substr(0, addr.find(":"));

	this->m_server.sin_port = htons(port);
	InetPtonA(AF_INET, host.c_str(), &this->m_server.sin_addr); // Convert the host address to a usable format


	// Bind the socket
	if (bind(this->m_socket, (struct sockaddr*)&this->m_server, sizeof(this->m_server)) < 0)
	{
		std::cerr << "Error binding socket" << std::endl;
		exit(1);
	}
}

/**
 * Reads through messages and processes them
 */
void Slave::processor()
{
	int port = atoi(master_address.substr(master_address.find(":") + 1).c_str());
	std::string host = master_address.substr(0, master_address.find(":"));

	// Socket Address
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port); // Port number of master
	InetPtonA(AF_INET, host.c_str(), &server_addr.sin_addr); // Convert the host address to a usable format

	while (isRunning)
	{
		// If there are messages in the queue
		if (!messages.empty())
		{
			// Get the message
			std::string request = messages.front();
			messages.pop();

			// Format if needed
			std::string message = request;

			sendto(this->m_socket, message.c_str(), message.length(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

			// Log the message
			std::cout << "Slave " << slave_id << " sent message: " << message << std::endl;
		}
	}
}

/**
 * Listens for messages from the master and adds to message queue
 */
void Slave::listen()
{
	int port = atoi(master_address.substr(master_address.find(":") + 1).c_str());
	std::string host = master_address.substr(0, master_address.find(":"));

	// Socket Address
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port); // Port number of master
	InetPtonA(AF_INET, host.c_str(), &server_addr.sin_addr); // Convert the host address to a usable format

	int len = sizeof(server_addr);

	while (isRunning)
	{
		// Buffer for message
		char buffer[1024];
		memset(buffer, 0, 1024);

		// Receive message
		int n = recvfrom(this->m_socket, buffer, 1024, 0, (struct sockaddr*)&server_addr, (socklen_t*)&len);

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
		std::string message = std::string(buffer);

		std::cout << "Slave " << slave_id << " received message: " << message << std::endl;

		// Parse message to range (NUM1,NUM2)
		int start = atoi(message.substr(0, message.find(",")).c_str());
		int end = atoi(message.substr(message.find(",") + 1).c_str());
		

		// Split the range into n_threads
		int range_size_per_thread = (end - start) / n_threads;
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

		// Once done calculating, split and add to queue
		// until reached end of message. Splits by MAX_SPLITS
		// at a time.
		std::string delimiter = " ";
		size_t pos = 0;
		std::string token;
		std::string message = "";
		int count = 0;
		while ((pos = primesHex.find(delimiter)) != std::string::npos) {
			token = primesHex.substr(0, pos);
			primesHex.erase(0, pos + delimiter.length());

			// If primesHex is empty or count is MAX_SPLITS, add to queue
			if (primesHex.empty() || count == MAX_SPLITS)
			{
				// Add to queue
				messages.push(message);
				start += range_size_per_thread;
				count = 0;
				message = "";
			}
			else { 
				message += token + " ";
				count++; 
			}
		}
	}
}