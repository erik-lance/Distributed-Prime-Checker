#include "Master.h"

Master::Master(std::string host_address, int port_number, int threads)
{
	this->host = host_address;
	this->port = port_number;
	this->n_threads = threads;

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
	// Start the message thread
	msg_processor = std::thread(&Master::processor, this);

	// Start the listener threads
	listener = std::thread(&Master::receive, this);

	// Start the sender threads
	client_sender = std::thread(&Master::client_send, this);
	slave_sender = std::thread(&Master::slave_send, this);

	// Wait for the threads to finish
	msg_processor.join();
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
	// Packet Details
	std::string host = client_address.substr(0, client_address.find(":"));
	int port = atoi(client_address.substr(client_address.find(":") + 1).c_str());

	// Address
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	InetPtonA(AF_INET, host.c_str(), &server.sin_addr); // Convert the host address to a usable format

	int batches = 0; // For debugging

	while (running)
	{
		// Check if there are any messages in the queue
		if (!sender_queue.empty())
		{
			// Get the message from the queue
			std::string task = sender_queue.front();
			sender_queue.pop();

			std::cout << "Sending to client" << std::endl;
			std::cout << "Message: " << task << std::endl;

			// Send the message
			int sent = sendto(m_socket, task.c_str(), task.length(), 0, (struct sockaddr*)&server, sizeof(server));

			if (sent < 0)
			{
				// Print full error details
				char error[1024];
				strerror_s(error, sizeof(error), errno);
				std::cerr << "Error sending message: " << error << std::endl;
				exit(1);
			}
			else {
				std::cout << "Sent message to client" << std::endl;
				batches++;

				// Sent details

				std::cout << "Sent: " << sent << " bytes\n" << std::endl;
				
				// If message was DONE, print number of batches done
				if (task == "DONE") { 
					std::cout << "Batches: " << batches << std::endl;
					batches = 0;
				}
			}
		}
	}
}

/**
 * Sends message to slave to split the prime checking.
 */
void Master::slave_send()
{
	while (running)
	{
		// Check if there are any messages from client
		if (!slave_sender_queue.empty())
		{
			// Get the message from the queue
			request_slave task = slave_sender_queue.front();
			slave_sender_queue.pop();

			std::cout << "Sending to slave" << std::endl;
			std::cout << "Slave Addr: " << task.first << std::endl;
			std::cout << "Range: " << task.second.first << " - " << task.second.second << std::endl;

			// Packet Details
			std::string host = task.first.substr(0, task.first.find(":"));
			int port = atoi(task.first.substr(task.first.find(":") + 1).c_str());
			std::string message = std::to_string(task.second.first) + "," + std::to_string(task.second.second);

			// Address
			struct sockaddr_in server;
			server.sin_family = AF_INET;
			server.sin_port = htons(port);
			InetPtonA(AF_INET, host.c_str(), &server.sin_addr); // Convert the host address to a usable format

			// Send the message
			int sent = sendto(m_socket, message.c_str(), message.length(), 0, (struct sockaddr*)&server, sizeof(server));

			if (sent < 0)
			{
				// Print full error details
				char error[1024];
				strerror_s(error, sizeof(error), errno);
				std::cerr << "Error sending message: " << error << std::endl;
				exit(1);
			}
			else {
				std::cout << "Sent message to slave" << std::endl;
			}
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

	// Split work for slaves and master
	int n_machines = slave_addresses.size() + 1; // Number of machines (master is the +1)

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

		// Client: "C:1,2"
		// Slave: "1 2 3 5 7 11"

		// Add message to queue
		std::string message = std::string(buffer);
		std::cout << "Received message: " << message << std::endl;

		message_queue.push(message);

		// Reset the bufferq
		memset(buffer, 0, sizeof(buffer));
	}
}

void Master::processor()
{
	int n_machines = slave_addresses.size() + 1; // Number of machines (master is the +1)

	while (running)
	{
		// Check if there are any messages in the queue
		if (!message_queue.empty())
		{
			// Get the message from the queue
			std::string msg = message_queue.front();
			message_queue.pop();

			// Process the message
			std::cout << "Processing message: " << msg << std::endl;

			// Determine the type of message
			if (msg[1] == ':')
			{
				std::cout << "\n--------------- CLIENT Received: " << msg << " ---------------" << std::endl;

				// Parse client messsage to be range<int, int>
				// Client sends: "C:1,2"
				// Parse to: range<int, int>
				std::string delimiter = ":";
				std::string token = msg.substr(0, msg.find(delimiter));
				std::string str_range = msg.substr(msg.find(delimiter) + 1, msg.length());

				// Parse the range
				int range_start = std::stoi(str_range.substr(0, str_range.find(",")));
				int range_end = std::stoi(str_range.substr(str_range.find(",") + 1, str_range.length()));

				// Create the range
				range num_range = std::make_pair(range_start, range_end);

				// Calculate the range for each machine
				int range_size = (num_range.second - num_range.first) / n_machines;
				int start = num_range.first;
				int end = start + range_size;

				// Send the message to the slaves
				for (int i = 0; i < n_machines - 1; i++)
				{
					// Create the range
					range new_range = std::make_pair(start, end);

					// Address of slave
					std::string slave_addr = slave_addresses[i];

					// Create the message
					request_slave message = std::make_pair(slave_addr, new_range);

					// Add the message to the queue
					slave_sender_queue.push(message);

					// Update the range
					start = end + 1;
					end = start + range_size;

					// Clamp the end
					if (end > num_range.second) end = num_range.second;
				}

				// Process the remaining primes
				range new_range = std::make_pair(start, num_range.second);
				int range_size_per_thread = getSizePerThread(new_range.first, new_range.second, n_threads);

				// Prepare threads
				std::vector<std::thread> threads;

				for (int i = 0; i < n_threads; i++)
				{
					// Use `primeCheckerHex` from PrimeChecker.h which takes
					// (range r, std::string& primes, std::mutex& mtx)
					threads.push_back(std::thread(primeCheckerHex, new_range, std::ref(primesHex), std::ref(mtx)));

					// Update range
					new_range.first = new_range.second + 1;
					new_range.second = new_range.first + range_size_per_thread;

					// Clamp range
					if (new_range.second > range_end) new_range.second = range_end;
				}

				// Join threads
				for (int i = 0; i < n_threads; i++)
				{
					threads[i].join();
				}

				machines_done += 1;

				// Split the packets if machines_done == n_machines
				if (machines_done == n_machines) { split_packets(); }
			}
			else
			{
				std::cout << "SLAVE Received: " << msg << std::endl;

				// Add the message to the slave response queue
				// Parse directly into array of integers
				// Slave sends: ":1 2 3 5 7 11"
				std::string delimiter = ":";
				std::string token = msg.substr(0, msg.find(delimiter)); // Get the task id
				std::string str_primes = msg.substr(msg.find(delimiter) + 1, msg.length());

				// No need to parse primes, just store them as a string
				// because we will send them back to the client

				// Check if the message is "DONE"
				if (str_primes == "DONE") { machines_done += 1; }
				else
				{
					// Append to primesHex
					primesHex += str_primes;
				}

				// Split the packets if machines_done == n_machines
				if (machines_done == n_machines) { split_packets(); }
			}
		}
	}
}

void Master::split_packets()
{
	// Once done calculating, split and add to queue
	// until reached end of message. Splits by MAX_SPLITS
	// at a time.
	std::string delimiter = " ";
	size_t pos = 0;
	std::string token;
	std::string message = "";
	int count = 0;

	while ((pos = primesHex.find(delimiter)) != std::string::npos) {
		token = primesHex.substr(0, pos);
		primesHex.erase(0, pos + delimiter.length()); // Erase the token until the delimiter (including the delimiter)

		// If primesHex is empty or count is MAX_SPLITS, add to queue
		if (primesHex.empty() || count == MAX_SPLITS)
		{
			// Add to queue
			sender_queue.push(message);
			count = 0;
			message = "";
		}
		else {
			message += token + " ";
			count++;
		}
	}

	// Once done sending all primes, send another message
	// confirming that all primes have been sent
	sender_queue.push("DONE");
	

	// Reset
	machines_done = 0;
	primesHex = "";
}
