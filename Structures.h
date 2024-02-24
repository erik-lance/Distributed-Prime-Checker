#pragma once
#include <vector>
#include <string>

constexpr auto MAX_SPLITS = 100;

// Range of numbers
typedef std::pair<int, int> range;

// Address and Task ID
typedef std::pair<std::string, int> client_details;

// Client Message (Request: <client_details, range>)
typedef std::pair<client_details, range> client_message;

// Master -> Client (Response: <client_details, result>)
typedef std::pair<client_details, std::vector<int>> response_client;

// Master -> Slave (Request: <address, range>)
typedef std::pair<std::string, range> request_slave;

// Slave -> Master (Response: <task id, result>)
typedef std::pair<int, std::string> response_slave;

// Master Address
const std::string master_address = "127.0.0.1:6379";

// Client Address
const std::string client_address = "127.0.0.1:6378";

// Slave Addresses
const std::vector<std::string> slave_addresses = {
	"127.0.0.1:5000",
	"127.0.0.1:5001",
};