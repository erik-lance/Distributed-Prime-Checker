#include "Client.h"

Client::Client(char* ip, int port)
{
	this->host = ip;
	this->port = port;
	run();
}

Client::~Client()
{
}

void Client::run()
{
	// Create a socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cerr << "Can't create a socket! Quitting" << std::endl;
		return;
	}

	// Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, host, &hint.sin_addr);

	// Connect to the server

}
