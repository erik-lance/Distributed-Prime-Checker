#pragma once
#include <vector>
#include <string>

// Range of numbers
typedef std::pair<int, int> range;

// Address and Task ID
typedef std::pair<std::string, int> client_details;

// Client Message
typedef std::pair<client_details, range> client_message;

// Master -> Client (Response: <client_details, result>)
typedef std::pair<client_details, std::vector<int>> response_client;

// Master -> Slave (Request: <task id, range>)
typedef std::pair<int, range> request_slave;

// Slave -> Master (Response: <task id, result>)
typedef std::pair<int, std::vector<int>> response_slave;