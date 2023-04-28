#include <stdexcept>
#include <iostream>
#include "client.h"

namespace server
{
	string BaseClient::generateErr() const
	{
		throw runtime_error("ERROR - "s + typeid(*this).name() + ": code "s + to_string(WSAGetLastError()));
	}

	namespace tcp
	{

		TCPClient::TCPClient(TCPClient::server_data_handler sdh) : data_handler(sdh), m_wsaData(WSADATA()), state(false), m_CSocket(INVALID_SOCKET) {}

		TCPClient::~TCPClient()
		{
			closesocket(m_CSocket);
			WSACleanup();
		}

		void TCPClient::connectTo(const PCSTR& ip, const PCSTR& port)
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

			err = getaddrinfo(ip, port, &hints, &result);

			if (err != 0)
			{
				WSACleanup();
				generateErr();
			}

			m_addrInfoData = *result;

			m_CSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

			if (m_CSocket == INVALID_SOCKET)
			{
				closesocket(m_CSocket);
				WSACleanup();
				generateErr();
			}

			freeaddrinfo(result);

			if (connect(m_CSocket, m_addrInfoData.ai_addr, m_addrInfoData.ai_addrlen) != 0)
			{
				closesocket(m_CSocket);
				WSACleanup();
				generateErr();
			}
			state = true;

			data_handler_thread = thread([this]() { handlingDataLoop(); });
		}

		bool TCPClient::disconnect()
		{
			if (!state) return true;
			closesocket(m_CSocket);
			WSACleanup();
			return true;
		}
		bool TCPClient::sendData(const void* buffer, const size_t size) const
		{
			if (!state) return false;
			void* send_buffer = ::operator new(size + sizeof(int));
			memcpy(reinterpret_cast<char*>(send_buffer) + sizeof(int), buffer, size);
			*reinterpret_cast<int*>(send_buffer) = size;

			if (send(m_CSocket, reinterpret_cast<char*>(send_buffer), size + sizeof(int), 0) < 0) return false;
			free(send_buffer);
			return true;
		}

		void TCPClient::handlingDataLoop()
		{
			while (state)
			{
				handle_mutex.lock();
				DataBuffer buf = loadData();
				if (!buf.isEmpty())
					data_handler(buf);
				handle_mutex.unlock();
				this_thread::sleep_for(50ms);
			}
		}

		DataBuffer TCPClient::loadData()
		{
			if (!state) return DataBuffer();
			DataBuffer buff;
			u_long t = true;
			WIN(if (SOCKET_ERROR == ioctlsocket(m_CSocket, FIONBIO, &t)) return DataBuffer();)
				int res = recv(m_CSocket, (char*)&buff.size, sizeof(int), WIN(0));

			if (!res)
			{
				disconnect();
				return DataBuffer();
			}

			t = false;
			WIN(if (SOCKET_ERROR == ioctlsocket(m_CSocket, FIONBIO, &t)) return DataBuffer();)

				if (!buff.size) return DataBuffer();
			buff.data_ptr = ::operator new(buff.size);

			recv(m_CSocket, (char*)buff.data_ptr, buff.size, 0);
			return buff;
		}
	}

}