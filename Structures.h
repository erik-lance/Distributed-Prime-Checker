#pragma once
#include <vector>
#include <string>

constexpr auto MAX_BUFFER = 32768;
constexpr auto MAX_SPLITS = 8192;
constexpr auto num_threads = 4;

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
const std::string master_address = "192.168.1.5:6379";

// Client Address
const std::string client_address = "192.168.1.5:6378";

// Slave Addresses
const std::vector<std::string> slave_addresses = {

};