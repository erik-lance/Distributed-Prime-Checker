#include "Master.h"

Master::Master(std::string host_address, int port_number)
{
	this->host = host_address;
	this->port = port_number;
	this->n_threads = num_threads;

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
		WSACleanup();
	#else
		close(m_socket);
	#endif
}

void Master::init()
{
	// Create a socket
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
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

	// Set the socket to be able to use MAX_BUFFER size
	int buffer_size = MAX_BUFFER;
	if (setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&buffer_size, sizeof(buffer_size)) != 0)
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

	// Set the socket to listen for incoming connections
	if (listen(m_socket, 5) != 0)
	{
		// Print full error details
		char error[1024];
		strerror_s(error, sizeof(error), errno);
		std::cerr << "Error listening on socket: " << error << std::endl;
		exit(1);
	}

	// Accept connections from client and slaves
	int n_machines = slave_addresses.size() + 1; // Number of machines (client is the +1)
	while (connected_sockets.size() < n_machines)
	{
		// Accept a connection
		SOCKET server_socket = accept(m_socket, NULL, NULL);
		std::cout << "Accepted: " << server_socket << "\n" << std::endl;

		if (server_socket < 0)
		{
			// Error handling
			#ifdef _WIN32
				int error_code = WSAGetLastError();
				if (error_code != WSAEWOULDBLOCK) {
					char error[1024];
					strerror_s(error, sizeof(error), error_code);
					std::cerr << "Error accepting connection: " << error << std::endl;
					exit(1);
				}
				else {
					continue;
				}
			#else
				if (errno != EWOULDBLOCK && errno != EAGAIN) {
					char error[1024];
					strerror_r(errno, error, sizeof(error));
					std::cerr << "Error accepting connection: " << error << std::endl;
					exit(1);
				}
			#endif
		}

		// If connection is not up, continue
		if (server_socket == INVALID_SOCKET) { continue; }

		// Set to non-blocking
		#ifdef _WIN32
			u_long mode = 1;
			ioctlsocket(server_socket, FIONBIO, &mode);
		#else
			fcntl(server_socket, F_SETFL, O_NONBLOCK);
		#endif

		// Add the socket to the list of connected sockets
		connected_sockets.push_back(server_socket);
	}

	// Set to non-blocking
	#ifdef _WIN32
		u_long mode = 1;
		ioctlsocket(m_socket, FIONBIO, &mode);
	#else
		fcntl(m_socket, F_SETFL, O_NONBLOCK);
	#endif

	std::cout << "All machines connected. Starting receivers and senders." << std::endl;
	start();
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
	int batches = 0; // For debugging

	while (running)
	{
		// Check if there are any messages in the queue
		if (!client_sender_queue.empty())
		{
			// Get the message from the queue
			std::string task = client_sender_queue.front();
			client_sender_queue.pop();

			std::cout << "Sending to client: " << task.length() << " bytes" << std::endl;

			// Send the message
			int sent = send(client_socket, task.c_str(), task.length(), 0);

			if (sent < 0)
			{
				// Print full error details
				#ifdef _WIN32
					int error_code = WSAGetLastError();
					std::cerr << "Error sending message: " << error_code << std::endl;
				#else
					char error[1024];
					strerror_r(errno, error, sizeof(error));
					std::cerr << "Error sending message: " << error << std::endl;
				#endif	
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
			socket_message task = slave_sender_queue.front();
			slave_sender_queue.pop();

			std::cout << "Sending to slave" << std::endl;
			std::cout << "Slave Socket: " << task.first << std::endl;
			std::cout << "Range: " << task.second << std::endl;

			// Packet Details
			std::string message = task.second;

			// Send the message to slave
			int sent = send(task.first, message.c_str(), message.length(), 0);

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
	// Create dynamic buffer of size MAX_BUFFER
	std::vector<char> buffer(MAX_BUFFER);

	// Split work for slaves and master
	int n_machines = slave_addresses.size() + 1; // Number of machines (master is the +1)

	while (running)
	{
		// For each socket connected under connected_sockets, receive the message
		for (int i = 0; i < connected_sockets.size(); i++) {
			SOCKET server_socket = connected_sockets[i];

			// Clear the buffer
			buffer.clear();
			buffer.resize(MAX_BUFFER);

			// Receive the message
			int bytes_received = recv(server_socket, buffer.data(), buffer.size(), 0);

			if (bytes_received < 0)
			{
				// Error handling
				#ifdef _WIN32
					int error_code = WSAGetLastError();
					if (error_code != WSAEWOULDBLOCK) {
						char error[1024];
						strerror_s(error, sizeof(error), error_code);
						std::cerr << "[" << error_code << "] Error receiving message: " << error << std::endl;
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
			std::string message = std::string(buffer.data(), bytes_received);

			// IF message is CLOSE, close the socket
			if (message == "CLOSE")
			{
				// Close the socket
				#ifdef _WIN32
					closesocket(server_socket);
				#else
					close(server_socket);
				#endif

				// Remove the socket from the list of connected sockets
				connected_sockets.erase(connected_sockets.begin() + i);
				continue;
			}

			socket_message msg = std::make_pair(server_socket, message);
			message_queue.push(msg);
		}
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
			socket_message msg = message_queue.front();
			message_queue.pop();

			// If message is empty due to fragmentation, skip
			if (msg.second.empty()) { continue; }

			// Determine the type of message
			if (msg.second[1] == ':')
			{
				std::cout << "\n--------------- CLIENT Received: " << msg.second << " ---------------" << std::endl;
				client_socket = msg.first;

				std::string message = msg.second;

				// Parse client messsage to be range<int, int>
				// Client sends: "C:1,2"
				// Parse to: range<int, int>
				std::string delimiter = ":";
				std::string token = message.substr(0, message.find(delimiter));
				std::string str_range = message.substr(message.find(delimiter) + 1, message.length());

				// Parse the range
				int range_start = std::stoi(str_range.substr(0, str_range.find(",")));
				int range_end = std::stoi(str_range.substr(str_range.find(",") + 1, str_range.length()));

				// Create the range
				range num_range = std::make_pair(range_start, range_end);

				// Calculate the range for each machine
				int range_size = (num_range.second - num_range.first) / n_machines;
				int start = num_range.first;
				int end = start + range_size;

				// Find sockets that are not client_socket
				std::vector<SOCKET> slave_sockets;
				for (int i = 0; i < connected_sockets.size(); i++)
				{
					if (connected_sockets[i] != client_socket)
					{
						slave_sockets.push_back(connected_sockets[i]);
					}
				}

				// Send the message to the slaves
				for (int i = 0; i < n_machines - 1; i++)
				{
					// Create the range
					std::string range_str = std::to_string(start) + "," + std::to_string(end);

					// SOCKET of slave
					SOCKET slave_socket = slave_sockets[i];

					// Create the message
					socket_message message = std::make_pair(slave_socket, range_str);

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
				range thread_range = std::make_pair(new_range.first, new_range.first + range_size_per_thread);

				for (int i = 0; i < n_threads; i++) {
					std::cout << "Range: " << thread_range.first << " - " << thread_range.second << std::endl; 
					// Use `primeCheckerHex` from PrimeChecker.h which takes
					// (range r, std::string& primes, std::mutex& mtx)
					threads.push_back(std::thread(primeCheckerHex, thread_range, std::ref(primesHex), std::ref(mtx)));

					// Update range
					thread_range.first = thread_range.second + 1;
					thread_range.second = thread_range.first + range_size_per_thread;

					// Clamp range
					if (thread_range.second > new_range.second) thread_range.second = new_range.second;
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
				std::cout << "SLAVE Received: " << msg.second.length() << " bytes" << std::endl;

				// Since TCP tends to fragment, the last DONE message might be fragmented
				// We check the last 4 characters of the message to see if it is DONE
				if (msg.second.length() > 4 && msg.second.substr(msg.second.length() - 4, 4) == "DONE")
				{
					machines_done += 1;
					std::cout << "[Fragmented] Finished Slave" << std::endl;

					// Remove last 4 characters from the message
					msg.second = msg.second.substr(0, msg.second.length() - 4);
					
					mtx.lock();
					primesHex += msg.second;
					mtx.unlock();

					// Split the packets if machines_done == n_machines
					if (machines_done == n_machines) { split_packets(); }
				} else if (msg.second[1] == 'O') {
					machines_done += 1; 
					std::cout << "Finished Slave" << std::endl; 

					// Split the packets if machines_done == n_machines
					if (machines_done == n_machines) { split_packets(); }
				} else {
					std::string message = msg.second;
					// No need to parse primes, just store them as a string
					// because we will send them back to the clien
				
					// Add the message to the slave response queue
					// Parse directly into array of integers
					// Slave sends: ":1 2 3 5 7 11"

					// Append to primesHex
					mtx.lock();
					primesHex += message;
					mtx.unlock();
				}
			}
		}
	}
}

void Master::split_packets()
{
	// Count primes before splitting
	int n_primes = countHexPrimes(primesHex);
	std::cout << "Number of primes: " << n_primes << std::endl;

	packetSplitter(primesHex, client_sender_queue);

	// Reset
	machines_done = 0;
	primesHex = "";
}
