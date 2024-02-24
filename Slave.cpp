#include "Slave.h"

Slave::Slave(int n)
{
	this->slave_id = n;
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
		if (!requests.empty())
		{
			// Get the message
			request_slave request = requests.front();
			requests.pop();

			// Process by calculating primes
			range n_range = request.second;
			std::vector<int> primes = getPrimes(n_range); // Single threaded for now

			// Format the response
			std::string message = format_response(request, primes);

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
			std::cerr << "Error receiving message" << std::endl;
			exit(1);
		}

		// Add message to queue
		std::string message = std::string(buffer);

		std::cout << "Slave " << slave_id << " received message: " << message << std::endl;

		// Split message into task id and range (Received: TASKID:RANGE)
		request_slave request = split_request(message);

		// Add to queue
		requests.push(request);
	}
}

/**
 * Splits the message into task id and range (TASKID:RANGE)
 * @param request - the message to split (e.g.: 1579344:100000,200000)
 */
request_slave Slave::split_request(std::string request)
{
	// Split the request into task id and range
	int task_id = std::stoi(request.substr(0, request.find(":")));
	std::string str_range = request.substr(request.find(":") + 1);
	int start = std::stoi(str_range.substr(0, str_range.find(",")));
	int end = std::stoi(str_range.substr(str_range.find(",") + 1));
	range n_range = std::make_pair(start, end);

	return std::make_pair(task_id, n_range);
}

/**
 * Gets the task id and primes in the range
 * @param request - the message to grab id (e.g.: 1579344:100000,200000)
 * @return - the task id and range
 */
std::string Slave::format_response(request_slave request, std::vector<int> primes)
{
	std::string response = std::to_string(request.first) + ":";
	for (int i = 0; i < primes.size(); i++)
	{
		response += std::to_string(primes[i]);
		if (i != primes.size() - 1)
		{
			response += " ";
		}
	}
	return response;
}
