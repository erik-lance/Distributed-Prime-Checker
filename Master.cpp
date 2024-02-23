#include "Master.h"

Master::Master(std::string host_address, int port_number)
{
	this->host = host_address;
	this->port = port_number;

	init();
}

Master::~Master()
{
}

void Master::init()
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
	m_server.sin_addr.s_addr = inet_addr(host.c_str());

	// Bind the socket to the server address
	if (bind(m_socket, (struct sockaddr*)&m_server, sizeof(m_server)) < 0)
	{
		std::cerr << "Error binding socket" << std::endl;
		exit(1);
	}
}

void Master::loop()
{
		// Start the listener thread
	listener = std::thread(&Master::receive, this);

	// Start the sender thread
	sender = std::thread(&Master::send, this);

	// Wait for the threads to finish
	listener.join();
	sender.join();
}

void Master::start()
{
}

void Master::send(std::string message)
{
}

void Master::receive()
{
}
