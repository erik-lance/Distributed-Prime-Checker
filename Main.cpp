#include "Main.h"

int main()
{
	// Check process type using env
	if (getenv("PROCESS_TYPE") == NULL)
	{
		std::cout << "Please set the PROCESS_TYPE environment variable" << std::endl;
		return 1;
	}

	std::string process_type = getenv("PROCESS_TYPE");

	// Prepare as master server or slave server
	if (process_type == "MASTER")
	{
		// Master server
		std::string host = "127.0.0.1";
		int port = getenv("PORT") == NULL ? 6379 : atoi(getenv("PORT"));
	}
	else if (process_type == "CLIENT")
	{
		// Client
		std::string host = "127.0.0.1";
		int port = getenv("PORT") == NULL ? 6379 : atoi(getenv("PORT"));
	}
	else
	{
		// Slave server

	}

	return 0;
}
