#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <memory>
#include "server.h"

namespace server
{

	string BaseServer::generateErr() const
	{
		throw runtime_error("ERROR - "s + typeid(*this).name() + ": code "s + to_string(WSAGetLastError()));
	}

	namespace tcp
	{

		DataBuffer TCPServer::Client::loadData()
		{
			if (!state) return DataBuffer();
			using namespace std::chrono_literals;

			DataBuffer buff;
			u_long t = true;
			WIN(if (SOCKET_ERROR == ioctlsocket(socket, FIONBIO, &t)) return DataBuffer();)
				int res = recv(socket, (char*)&buff.size, sizeof(int), WIN(0));

			if (!res)
			{
				disconnect();
				return DataBuffer();
			}

			t = false;
			WIN(if (SOCKET_ERROR == ioctlsocket(socket, FIONBIO, &t)) return DataBuffer();)

				if (!buff.size) return DataBuffer();
			buff.data_ptr = ::operator new(buff.size);

			recv(socket, (char*)buff.data_ptr, buff.size, 0);
			return buff;
		}
		bool TCPServer::Client::sendData(const void* buffer, const size_t size) const
		{
			if (!state) return false;
			void* send_buffer = ::operator new(size + sizeof(int));
			memcpy(reinterpret_cast<char*>(send_buffer) + sizeof(int), buffer, size);
			*reinterpret_cast<int*>(send_buffer) = size;

			if (send(socket, reinterpret_cast<char*>(send_buffer), size + sizeof(int), 0) < 0) return false;

			delete send_buffer;
			return true;
		}
		bool TCPServer::Client::disconnect()
		{
			state = false;
			closesocket(socket);
			socket = INVALID_SOCKET;
			return state;
		}

		TCPServer::TCPServer(client_conn_handler cch, client_conn_handler dh, client_data_handler cdh, const PCSTR& port)
			: m_port(port), data_handler(cdh), conn_handler(cch), disconnect_handler(dh), m_wsaData(WSADATA()),
			m_LSocket(INVALID_SOCKET), m_state(false) {}

		TCPServer::~TCPServer() { if (m_state) stop(); }

		void TCPServer::handlingAcceptLoop(int maxConn)
		{
			int addrLen = sizeof(sockaddr_in);

			while (m_state)
			{
				sockaddr_in clientAddr;

				SOCKET CSocket = accept(m_LSocket, (sockaddr*)&clientAddr, &addrLen);


				if (CSocket == INVALID_SOCKET)
				{
					closesocket(CSocket);
					continue;
				}

				if (!enableKeepAlive(CSocket))
				{
					closesocket(CSocket);
					continue;
				}

				unique_ptr<Client> client(new Client(CSocket, clientAddr));
				conn_handler(*client);
				client_mutex.lock();
				clients.emplace_back(move(client));
				client_mutex.unlock();
			}
		}

		void TCPServer::handlingDataLoop()
		{
			while (m_state)
			{
				client_mutex.lock();
				for (list<unique_ptr<Client>>::iterator it = clients.begin(); it != clients.end(); ++it)
				{
					auto& client = *it;
					if (client)
					{
						DataBuffer data = client->loadData();
						if (data.size)
						{
							std::thread([this, _data = std::move(data), &client] {
								client->access_mutex.lock();
								data_handler(*client, _data);
								client->access_mutex.unlock();
								}).detach();
						}
						else if (!client->state)
						{
							std::thread([this, &client, it] {
								client->access_mutex.lock();
								Client* pointer = client.release();
								client = nullptr;
								pointer->access_mutex.unlock();
								disconnect_handler(*pointer);
								clients.erase(it);
								delete pointer;
								}).detach();
						}
					}
				}
				client_mutex.unlock();
				std::this_thread::sleep_for(50ms);
			}
		}

		bool TCPServer::enableKeepAlive(SOCKET socket)
		{
			int flags = 1;
			if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&flags, sizeof(flags)) != 0) return false;
			return true;
		}

		void TCPServer::start(int maxConn)
		{
			int err = WSAStartup(MAKEWORD(2, 2), &m_wsaData);
			if (err != 0)
			{
				WSACleanup();
				generateErr();
			}

			addrinfo* result = nullptr, hints{};
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_flags = AI_PASSIVE;

			if ((err = getaddrinfo(NULL, m_port, &hints, &result)) != 0)
			{
				WSACleanup();
				generateErr();
			}

			if ((m_LSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == INVALID_SOCKET)
			{
				freeaddrinfo(result);
				WSACleanup();
				generateErr();
			}

			if ((err = ::bind(m_LSocket, result->ai_addr, (int)result->ai_addrlen)) != 0)
			{
				closesocket(m_LSocket);
				WSACleanup();
				generateErr();
			}

			freeaddrinfo(result);

			if (listen(m_LSocket, maxConn) == SOCKET_ERROR)
			{
				m_state = false;
				closesocket(m_LSocket);
				WSACleanup();
				generateErr();
			}

			m_state = true;

			connection_handler_thread = thread([this, &maxConn](int) {handlingAcceptLoop(maxConn); }, maxConn);
			data_handler_thread = thread([this]() {handlingDataLoop(); });

		}

		void TCPServer::stop()
		{
			m_state = false;
			joinThreads();
			closesocket(m_LSocket);
			WSACleanup();
			clients.clear();
		}

	}

}