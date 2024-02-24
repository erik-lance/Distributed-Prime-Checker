#include "Master.h"

Master::Master(std::string host_address, int port_number)
{
	this->host = host_address;
	this->port = port_number;

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
	// Start the listener threads
	listener = std::thread(&Master::receive, this);

	// Start the sender threads
	client_sender = std::thread(&Master::client_send, this);

	// Wait for the threads to finish
	listener.join();
	client_sender.join();
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
		if (!queue.empty())
		{
			// Get the message from the queue
			response_client task = sender_queue.front();
			sender_queue.pop();
		}
	}
}

/**
 * Receive a message from clients. Queue the message for processing.
 */
void Master::receive()
{
	// Receive a message
	char buffer[1024];

	// Client Address
	struct sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = htonl(INADDR_ANY);
	client.sin_port = htons(atoi(getenv("PORT"))); // Port

	int client_len = sizeof(client);

	while (running)
	{
		int bytes_received = recvfrom(m_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client, &client_len);

		if (bytes_received < 0)
		{
			std::cerr << "Error receiving message" << std::endl;
			exit(1);
		}

		// Add the message to the queue with address
		std::string client_address = inet_ntoa(client.sin_addr);

		// Client: "C:1,2"
		// Slave: "1 2 3 5 7 11"

		// Determine the type of message
		if (buffer[0] == 'C')
		{
			// Parse client messsage to be range<int, int>
			// Client sends: "C:1,2"
			// Parse to: range<int, int>
			std::string str_msg = buffer;
			std::string delimiter = ":";
			std::string token = str_msg.substr(2, str_msg.find(delimiter));
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
		else if (buffer[0] == 'S')
		{
			// Add the message to the queue
		}

		// Print the message
		std::cout << "Received: " << buffer << std::endl;
	}
}

void Master::slave_send()
{
}