#include "Slave.h"

Slave::Slave(int n)
{
	this->isRunning = true;

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
	std::string addr = slave_addresses[n];
	this->m_server.sin_port = htons(atoi(addr.substr(addr.find(":") + 1).c_str())); // Port number of slave
	this->m_server.sin_addr.s_addr = inet_addr(addr.substr(0, addr.find(":")).c_str()); // IP Address of slave

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
	// Socket Address
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(master_address.substr(0, master_address.find(":")).c_str()); // IP Address of master
	server_addr.sin_port = htons(atoi(master_address.substr(master_address.find(":") + 1).c_str())); // Port number of master

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

			// Convert primes into a string separated by spaces
			std::string prime_string = "";
			for (int i = 0; i < primes.size(); i++)
			{
				prime_string += std::to_string(primes[i]);
				if (i != primes.size() - 1)
				{
					prime_string += " ";
				}
			}

			// Send the message back to the master (TASKID:PRIMES)
			std::string message = std::to_string(request.first) + ":" + prime_string;

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

	}
}
