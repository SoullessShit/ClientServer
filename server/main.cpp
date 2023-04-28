#include <iostream>
#include <chrono>
#include <exception>
#include <functional>
#include "server.h"

std::string getHostStr(const server::tcp::TCPServer::Client& client) {
	uint32_t ip = client.getHost();
	return std::string() + std::to_string(int(reinterpret_cast<char*>(&ip)[0])) + '.' +
		std::to_string(int(reinterpret_cast<char*>(&ip)[1])) + '.' +
		std::to_string(int(reinterpret_cast<char*>(&ip)[2])) + '.' +
		std::to_string(int(reinterpret_cast<char*>(&ip)[3])) + ':' +
		std::to_string(client.getPort());
}


int main()
{
	using namespace server::tcp;
	using namespace std;
	TCPServer::client_conn_handler f1 = [](TCPServer::Client& client) {
		std::cout << "Client " << getHostStr(client) << " connected\n";
		return;
	};
	TCPServer::client_conn_handler f2 = [](TCPServer::Client& client) {
		std::cout << "Client " << getHostStr(client) << " disconnected\n";
	};
	TCPServer::client_data_handler f3 =
		[](TCPServer::Client& client, server::DataBuffer data) {
		std::cout << "Client " << getHostStr(client) << " send data [ " << data.size << " bytes ]: " << (char*)data.data_ptr << '\n';
		const char* temp = _strrev((char*)data.data_ptr);
		client.sendData(temp, data.size);
	};

	try
	{
		TCPServer x(f1, f2, f3);
		x.start();
		x.joinThreads();

	}
	catch (std::exception err)
	{
		std::cout << err.what();
	}

	return 0;
}