#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_
#include "dependencies.h"

#include <string>
#include <typeinfo>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <list>

#ifdef _WIN32
#define WIN(exp) exp
#define NIX(exp)

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

namespace server
{
	using namespace std;

	class BaseServer
	{
	public:
		virtual void start(int maxConn) = 0;
		virtual void stop() = 0;
		virtual void joinThreads() = 0;
		virtual ~BaseServer() = 0 {}
		virtual bool getState() const = 0;
		virtual string generateErr() const;

	private:
		virtual void handlingAcceptLoop(int maxConn) = 0;
		virtual void handlingDataLoop() = 0;
	};

	namespace tcp
	{
		class TCPServer : public BaseServer
		{
		public:
			class Client
			{
				friend TCPServer;
			public:
				SOCKET socket = INVALID_SOCKET;
				sockaddr_in address{};
				mutex access_mutex{};
				bool state = true;

				Client(SOCKET sock, sockaddr_in sockaddr) : socket(sock), address(sockaddr) {}
				DataBuffer loadData();
				bool sendData(const void* buffer, const size_t size) const;
				bool disconnect();

				uint16_t getHost() const { return address.sin_addr.S_un.S_addr; }
				uint16_t getPort() const { return address.sin_port; }
			};

			typedef function<void(Client&, DataBuffer)> client_data_handler;
			typedef function<void(Client&)> client_conn_handler;

			TCPServer(client_conn_handler cch, client_conn_handler dh,
				client_data_handler cdh, const PCSTR& port = "9100");
			void joinThreads() { connection_handler_thread.join(); data_handler_thread.join(); }
			void start(int maxConn = SOMAXCONN);
			void stop();
			const WSADATA& getWsaData() const { return m_wsaData; }
			bool getState() const { return m_state; }
			~TCPServer();

		private:
			client_conn_handler conn_handler;
			client_conn_handler disconnect_handler;
			client_data_handler data_handler;

			thread data_handler_thread;
			thread connection_handler_thread;

			mutex client_mutex;

			PCSTR m_port;
			WSADATA m_wsaData;
			SOCKET m_LSocket;
			bool m_state;
			list<unique_ptr<Client>> clients;

			void handlingAcceptLoop(int maxConn);
			void handlingDataLoop();
			bool enableKeepAlive(SOCKET socket);
		};

	}

}

#endif // _WIN32

#endif // !TCPCLIENT_H_
