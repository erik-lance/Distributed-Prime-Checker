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
	// Start the main loop
	loop();
}

/**
 * Sends message back to the client.
 */
void Master::send(std::string message)
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
		
		// Parse client messsage to be range<int, int>
		// Client sends: "1,2"
		// Parse to: range<int, int>
		int start = 0;
		int end = 0;
		std::string str(buffer);
		std::string delimiter = ",";
		size_t pos = 0;
		std::string token;

		// Parse the string
		while ((pos = str.find(delimiter)) != std::string::npos) {
			token = str.substr(0, pos);
			start = std::stoi(token);
			str.erase(0, pos + delimiter.length());
		}
		end = std::stoi(str);

		// Create the range
		range num_range = std::make_pair(start, end);

		// Hash address and range to task id integer
		int task_id = std::hash<std::string>{}(client_address + std::to_string(start) + std::to_string(end));

		client_details details = std::make_pair(client_address, task_id);
		client_message message = std::make_pair(details, num_range);

		// Add the message to the queue
		queue.push(message);

		// Print the message
		std::cout << "Received: " << buffer << std::endl;
	}
}
