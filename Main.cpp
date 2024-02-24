#include "Main.h"

int main()
{
	// Ask process type
	std::string process_type;
	std::cout << "Enter process type (MASTER, SLAVE, CLIENT): ";
	std::cin >> process_type;

	// Prepare as master server or slave server
	if (process_type == "MASTER")
	{
		// Master server
		std::string full_address = master_address;
		std::string host = full_address.substr(0, full_address.find(":"));
		int port = atoi(full_address.substr(full_address.find(":") + 1).c_str());

		// Start the master server
		Master master(host, port);
		master.start();
	}
	else if (process_type == "CLIENT")
	{
		// Client
		std::string full_address = client_address;
		std::string host = full_address.substr(0, full_address.find(":"));
		int port = atoi(full_address.substr(full_address.find(":") + 1).c_str());
	}
	else
	{
		// Slave server

		// Ask for server id
		int server_id;
		std::cout << "Enter server id: ";
		std::cin >> server_id;

		std::string full_address = slave_addresses[server_id];
		std::string host = full_address.substr(0, full_address.find(":"));
		int port = atoi(full_address.substr(full_address.find(":") + 1).c_str());
	}

	return 0;
}
