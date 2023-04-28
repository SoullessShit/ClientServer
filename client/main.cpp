#include <iostream>
#include <thread>
#include "client.h"



int main()
{
	using namespace std;
	using namespace server::tcp;
	TCPClient::server_data_handler f = [](server::DataBuffer data) {
		cout << "Server send data [ " << data.size << " bytes ]: " << (char*)data.data_ptr << endl;
	};
	try
	{
		char ip[20]{};
		cout << "Ip: ";
		cin >> ip;
		TCPClient x(f);
		x.connectTo(ip, "9100");
		cout << "You have been conected\n";
		std::string data;
		while (getline(cin, data) && x.getState())
		{
			if (data.size() != 0) data += '\0';
			x.sendData(data.c_str(), data.size());

		}


	}
	catch (const std::exception& err)
	{
		cout << err.what() << endl;
	}


}