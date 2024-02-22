#include "Server.h"

Server::Server(std::string udp_host, int udp_port)
{
	this->host = udp_host;
	this->port = udp_port;
	init();
}

Server::~Server()
{
}

void Server::init()
{
	// Create a socket
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if (m_socket == INVALID_SOCKET)
	{
		throw std::runtime_error("Failed to create socket");
	}

	// Set up the server address
	m_server.sin_family = AF_INET;
	m_server.sin_port = htons(port);
	m_server.sin_addr.s_addr = inet_addr(host.c_str());

	// Bind the socket to the server address
	if (bind(m_socket, (struct sockaddr*)&m_server, sizeof(m_server)) == SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to bind socket");
	}
}
