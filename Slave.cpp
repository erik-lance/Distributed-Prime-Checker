#include "Slave.h"

Slave::Slave()
{
	this->isRunning = true;

	// Activate thread
	this->listener = std::thread(&Slave::listen, this);
	processor();
}

Slave::~Slave()
{
	this->isRunning = false;
	this->listener.join();
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
