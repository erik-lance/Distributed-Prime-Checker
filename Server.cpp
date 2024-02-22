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

	// Set the socket to non-blocking mode
	#ifdef _WIN32
	u_long mode = 1;
	ioctlsocket(m_socket, FIONBIO, &mode);
	#else
	fcntl(m_socket, F_SETFL, O_NONBLOCK);
	#endif

	// Print the server address
	std::cout << "Server listening on " << host << ":" << port << std::endl;

	// Start listening for incoming messages
	start();
}

/**
 * Main server loop that listens for incoming messages
 */
void Server::loop()
{
while (true)
	{
		// Receive a message
		char buffer[1024];

		struct sockaddr_in client;
		// For now client is 127.0.0.1
		client = m_server;

		int client_len = sizeof(client);
		int bytes_received = recvfrom(m_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client, &client_len);

		if (bytes_received == SOCKET_ERROR)
		{
			throw std::runtime_error("Failed to receive message");
		}

		// Print the message
		std::cout << "Received: " << buffer << std::endl;

		// Send a response
		std::string response = "Hello from server";
		int bytes_sent = sendto(m_socket, response.c_str(), response.length(), 0, (struct sockaddr*)&client, client_len);

		if (bytes_sent == SOCKET_ERROR)
		{
			throw std::runtime_error("Failed to send response");
		}
	}
}

/**
 * Starts listening for incoming messages
 */
void Server::start()
{
	if (listen(m_socket, 5) == SOCKET_ERROR)
	{
		throw std::runtime_error("Failed to listen on socket");
	}

	// Start the server loop. Provide a listener for each client
	listener = std::thread(&Server::loop, this);
	listener.detach();

	// Print a message
	std::cout << "Server started" << std::endl;
}
