#include "Client.h"

Client::Client(std::string host, int port)
{
	this->host = host;
	this->port = port;

	// Setup WinSock
	#ifdef _WIN32
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
		{
			std::cerr << "Error initializing Winsock" << std::endl;
			exit(1);
		}
	#endif

	init();
}

Client::~Client()
{
}

void Client::init()
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
	InetPtonA(AF_INET, host.c_str(), &m_server.sin_addr); // Convert the host address to a usable format

	// Set the socket to non-blocking
	#ifdef _WIN32
		u_long mode = 1;
		ioctlsocket(m_socket, FIONBIO, &mode);
	#else
		fcntl(m_socket, F_SETFL, O_NONBLOCK);
	#endif

	// Set the socket to reuse the address
	int opt = 1;
	if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) != 0)
	{
		// Print full error details
		char error[1024];
		strerror_s(error, sizeof(error), errno);
		std::cerr << "Error setting socket options: " << error << std::endl;
		exit(1);
	}

	// Bind the socket to the server address
	if (bind(m_socket, (struct sockaddr*)&m_server, sizeof(m_server)) != 0)
	{
		// Print full error details
		char error[1024];
		strerror_s(error, sizeof(error), errno);
		std::cerr << "Error binding socket: " << error << std::endl;
		exit(1);
	}
}

/**
 * Run the client. Asks input from user and sends it to the master server.
 * User input is N which is the last number to check for prime numbers.
 */
void Client::run()
{
	bool running = true;

	// Master address
	std::string master_host = master_address.substr(0, master_address.find(":"));
	int master_port = atoi(master_address.substr(master_address.find(":") + 1).c_str());

	// Set up the master server address
	struct sockaddr_in master_server;
	memset((char*)&master_server, 0, sizeof(master_server));
	master_server.sin_family = AF_INET;
	master_server.sin_port = htons(master_port);
	InetPtonA(AF_INET, master_host.c_str(), &master_server.sin_addr); // Convert the host address to a usable format

	while (running)
	{
		int n; // Last number to check for prime numbers
		std::cout << "Enter how many numbers to check for prime numbers: ";
		std::cin >> n;

		if (n <= 0)
		{
			std::cerr << "Stopping ..." << std::endl;
			running = false;
			break;
		}

		// Sends the range to the master server
		int sent = sendto(m_socket, std::to_string(n).c_str(), std::to_string(n).length(), 0, (struct sockaddr*)&master_server, sizeof(master_server));
		if (sent < 0)
		{
			// Print full error details
			char error[1024];
			strerror_s(error, sizeof(error), errno);
			std::cerr << "Error sending message: " << error << std::endl;
			exit(1);
		}
		else {
			std::cout << "Sent message to master server" << std::endl;
		}
	}
}
	