#include "Master.h"

Master::Master(std::string host_address, int port_number)
{
	this->host = host_address;
	this->port = port_number;

	// If Windows, initialize Winsock
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

Master::~Master()
{
	// Close the socket
	#ifdef _WIN32
		closesocket(m_socket);
	#else
		close(m_socket);
	#endif
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

void Master::loop()
{
	// Start the listener threads
	listener = std::thread(&Master::receive, this);

	// Start the sender threads
	client_sender = std::thread(&Master::client_send, this);
	slave_sender = std::thread(&Master::slave_send, this);

	// Wait for the threads to finish
	listener.join();
	client_sender.join();
	slave_sender.join();
}

void Master::start()
{
	// Start the main loop
	loop();
}

/**
 * Sends message back to the client.
 */
void Master::client_send()
{
	while (running)
	{
		// Check if there are any messages in the queue
		if (!sender_queue.empty())
		{
			// Get the message from the queue
			response_client task = sender_queue.front();
			sender_queue.pop();

			std::cout << "Sending to client" << std::endl;
			std::cout << "Task ID: " << task.first.second << std::endl;
			std::cout << "Address: " << task.first.first << std::endl;
			std::cout << "Primes: " << task.second.size() << std::endl;
		}
	}
}

/**
 * Sends message to the slave.
 */
void Master::slave_send()
{
	while (running)
	{
		// Check if there are any messages from client
		if (!queue.empty())
		{
			// Get the message from the queue
			client_message task = queue.front();
			queue.pop();

			std::cout << "Sending to slave" << std::endl;
			std::cout << "Task ID: " << task.first.second << std::endl;
			std::cout << "Address: " << task.first.first << std::endl;
			std::cout << "Range: " << task.second.first << " - " << task.second.second << std::endl;

		}
	}
}

/**
 * Receive a message from client or slave. Store the message in the appropriate queue.
 */
void Master::receive()
{
	// Receive a message
	char buffer[1024];

	// Address
	struct sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = htonl(INADDR_ANY); // Address (Can be any address)
	client.sin_port = htons(6378); // Port

	int client_len = sizeof(client);

	while (running)
	{
		int bytes_received = recvfrom(m_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client, &client_len);

		if (bytes_received < 0)
		{
			// Error handling
			#ifdef _WIN32
				int error_code = WSAGetLastError();
				if (error_code != WSAEWOULDBLOCK) {
					char error[1024];
					strerror_s(error, sizeof(error), error_code);
					std::cerr << "Error receiving message: " << error << std::endl;
					exit(1);
				}
				else {
					continue;
				}
			#else
				if (errno != EWOULDBLOCK && errno != EAGAIN) {
					char error[1024];
					strerror_r(errno, error, sizeof(error));
					std::cerr << "Error receiving message: " << error << std::endl;
					exit(1);
				}
			#endif
		}

		// Add the message to the queue with address
		std::string client_address;
		inet_ntop(AF_INET, &client.sin_addr, &client_address[0], client_address.size());

		// Client: "C:1,2"
		// Slave: "ID:1 2 3 5 7 11"

		// Determine the type of message
		if (buffer[0] == 'C')
		{
			std::cout << "CLIENT Received: " << buffer << std::endl;

			// Parse client messsage to be range<int, int>
			// Client sends: "C:1,2"
			// Parse to: range<int, int>
			std::string str_msg = buffer;
			std::string delimiter = ":";
			std::string token = str_msg.substr(0, str_msg.find(delimiter));
			std::string str_range = str_msg.substr(str_msg.find(delimiter) + 1, str_msg.length());

			// Parse the range
			int start = std::stoi(str_range.substr(0, str_range.find(",")));
			int end = std::stoi(str_range.substr(str_range.find(",") + 1, str_range.length()));
			
			// Create the range
			range num_range = std::make_pair(start, end);

			// Hash address and range to task id integer
			int task_id = std::hash<std::string>{}(client_address + std::to_string(start) + std::to_string(end));

			client_details details = std::make_pair(client_address, task_id);
			client_message message = std::make_pair(details, num_range);

			// Add the message to the queue
			queue.push(message);
		}
		else
		{
			std::cout << "SLAVE Received: " << buffer << std::endl;

			// Add the message to the slave response queue
			// Parse directly into array of integers
			// Slave sends: "id123879872:1 2 3 5 7 11"
			std::string str_msg = buffer;
			std::string delimiter = ":";
			std::string token = str_msg.substr(0, str_msg.find(delimiter)); // Get the task id
			std::string str_primes = str_msg.substr(str_msg.find(delimiter) + 1, str_msg.length());

			// No need to parse primes, just store them as a string
			// because we will send them back to the client

			// Get task id from token
			int task_id = std::stoi(token);

			response_slave response = std::make_pair(task_id, str_primes);
			slave_queue.push(response);
		}
	}
}