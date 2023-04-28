#ifndef CLIENT_H_
#define CLEINT_H_
#include ""

#include <string>
#include <exception>
#include <typeinfo>
#include <mutex>
#include <functional>

#ifdef _WIN32
#define WIN(exp) exp
#define NIX(exp)
#define WINIX(win_exp, nix_exp) win_exp

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

namespace server
{
	using namespace std;

	class BaseClient
	{
	public:
		virtual string generateErr() const;
		virtual DataBuffer loadData() = 0;
		virtual bool sendData(const void* buffer, const size_t size) const = 0;
		virtual void connectTo(const PCSTR& ip, const PCSTR& port = "9100") = 0;
		virtual bool getState() const = 0;
		virtual bool disconnect() = 0;
		virtual ~BaseClient() = 0 {}

	};

	namespace tcp
	{

		class TCPClient : public BaseClient
		{
		public:
			typedef function<void(DataBuffer)> server_data_handler;

			TCPClient(TCPClient::server_data_handler sdh);
			void connectTo(const PCSTR& ip, const PCSTR& port = "9100");
			DataBuffer loadData();
			void joinThreads() { data_handler_thread.join(); };
			bool sendData(const void* buffer, const size_t size) const;
			virtual bool disconnect();
			bool getState() const { return state; }
			~TCPClient();

		private:
			thread data_handler_thread;

			server_data_handler data_handler;

			WSADATA m_wsaData;
			SOCKET m_CSocket;
			mutex handle_mutex{};
			addrinfo m_addrInfoData{};
			bool state;

			void handlingDataLoop();
		};

	}

}

#endif // _WIN32


#endif // !CLIENT_H_
