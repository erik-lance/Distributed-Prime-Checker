#include "Main.h"

int main()
{
	// Ask process type
	std::string process_type;
	std::cout << "Enter process type (0 - MASTER, 1 - SLAVE, 2 - CLIENT): ";
	std::cin >> process_type;

	// Ask n_threads
	int n_threads;
	std::cout << "Enter number of threads: ";
	std::cin >> n_threads;

	// Prepare as master server or slave server
	if (process_type == "0")
	{
		// Master server
		std::string full_address = master_address;
		std::string host = full_address.substr(0, full_address.find(":"));
		int port = atoi(full_address.substr(full_address.find(":") + 1).c_str());

		// Start the master server
		Master master(host, port, n_threads);
		master.start();
	}
	else if (process_type == "2")
	{
		// Client
		std::string full_address = client_address;
		std::string host = full_address.substr(0, full_address.find(":"));
		int port = atoi(full_address.substr(full_address.find(":") + 1).c_str());

		// Start the client
		Client client(host, port);
		client.run();
	}
	else
	{
		// Slave server

		// Ask for server id
		int server_id;
		std::cout << "(0-indexed) Enter server id: ";
		std::cin >> server_id;

		std::string full_address = slave_addresses[server_id];
		std::string host = full_address.substr(0, full_address.find(":"));
		int port = atoi(full_address.substr(full_address.find(":") + 1).c_str());

		// Start the slave server
		Slave slave(server_id, n_threads);
	}

	return 0;
}
