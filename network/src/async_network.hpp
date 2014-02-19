#pragma once

/*
 * This module is included as a part of libSprawl
 *
 * Copyright (C) 2013 Jaedyn K. Draper
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//Fix bug in llvm...
#ifndef _WIN32
#	ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_1
#		define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_1
#	endif

#	ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_2
#		define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_2
#	endif

#	ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
#		define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
#	endif

#	ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
#		define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
#	endif
#endif

//C++ includes
#include <string>
#include <thread>
#include <mutex>
#include <sstream>
#include <vector>
#include <memory>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <condition_variable>

//C includes
#ifndef _WIN32
#	include <arpa/inet.h>
#	include <unistd.h>
#	include <sys/socket.h>
#	include <netdb.h>
#	include <sys/time.h>
#else
#	include <WS2tcpip.h>
#	include <Winsock2.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

////////////////////////////////////////////////////////////////////////////////
/// TODO:
/// - Machine ID and Process ID in UDP connections
/// - network byte order on UDP headers
/// - Move function implementations into cpp file
/// - Split up UDP packets that are larger than MTU
/// - Combine UDP packets smaller than MTU
/// - Ordered UDP implementation
/// - Change implementation from select() to epoll()
/// - Windows compatibility
/// - Server socket store connections as a map so we know which connection to Recv() on in UDP
/// - Handle high_id overflow
/// - UDP connection timeout configurable
/// - Remove usages of std::string
////////////////////////////////////////////////////////////////////////////////

namespace sprawl
{
	namespace async_network
	{
#ifndef _WIN32
		typedef int SOCKET;
#endif

		typedef std::function<void(const std::shared_ptr<class Connection>, const char*, int)> ReceiveCallback;
		typedef std::function<void(const std::shared_ptr<class Connection>)> ConnectionCallback;
		typedef std::function<void(void)> SendCallback;
		typedef std::function<int(const char*, int)> PacketValidationCallback;

		typedef std::shared_ptr<Connection> ConnectionPtr;
		typedef std::weak_ptr<Connection> ConnectionWPtr;

		class SockExcept: public std::exception
		{
		public:
			SockExcept(const std::string& arg)
				: m_str(arg)
			{
			}
#ifdef _WIN32
			SockExcept(const wchar_t* arg)
			{
				size_t i;
				char mbBuf[32768];
				char* pmbBuf = mbBuf;
				wcstombs_s( &i, pmbBuf, 32768, arg, _TRUNCATE );
				m_str = pmbBuf;
			}
#endif

			~SockExcept() throw ()
			{
			}
			const char* what() const throw ()
			{
				return m_str.c_str();
			}
		private:
			std::string m_str;
		};

		enum class FailType { resend, ignore, notify };
		enum class ConnectionType { TCP, UDP };

		//Simple connection information.
		class Connection : public std::enable_shared_from_this<Connection>
		{
		public:
			//Information about the connection
			SOCKET GetDescriptor();
			std::string GetHostname();
			int GetPort();

			//Send is asynchronous - data is passed in on the main thread and then sent on the network thread
			virtual void Send(const std::string& data, SendCallback onSendFunction = nullptr);

			virtual void Send(const std::string& str, FailType /*behavior*/, SendCallback callback = nullptr)
			{
				Send(str, callback);
			}

			void Close();

		protected:
			friend class ServerSocket;
			friend class ClientSocket;

			Connection(class ServerSocket* parent, SOCKET desc_, struct sockaddr* addr, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_);
			Connection(class ClientSocket* parent, SOCKET desc_, struct sockaddr* addr, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_);

			//This send is called by the socket and actually sends everything collected by the other send
			//Will call the onSend callback when it finishes
			virtual void Send();
			//Receive data, will call the onReceived callback if there's anything here
			virtual int Recv();

			SOCKET m_desc;
			struct sockaddr m_dest;

			bool m_closeSocket;
			std::string m_partialPacket;

			ReceiveCallback m_onReceive;
			PacketValidationCallback m_validatePacket;

			std::vector< std::pair<std::string, SendCallback> > m_outData;
			std::mutex m_outDataMutex;
			class ServerSocket* m_parentServerSocket;
			class ClientSocket* m_parentClientSocket;
		};

		class UDPConnection : public Connection
		{
		public:
			virtual void Send(const std::string& str, SendCallback callback = nullptr) override final
			{
				Send(str, FailType::ignore, callback);
			}
			virtual void Send(const std::string& str, FailType behavior, SendCallback callback = nullptr) override final;
		protected:
			friend class ServerSocket;
			friend class ClientSocket;

			UDPConnection(class ServerSocket* parent, SOCKET desc_, struct sockaddr* addr, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_);

			UDPConnection(class ServerSocket* parent, SOCKET desc_, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_);

			UDPConnection(class ClientSocket* parent, SOCKET desc_, struct sockaddr* addr, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_);

			UDPConnection(class ClientSocket* parent, SOCKET desc_, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_);

			virtual void Send() override final;
			virtual int Recv() override final;
			bool CheckClosed();
			void SendKeepAlive();
		private:
			struct packet
			{
				packet(uint32_t _id, FailType _behavior, const std::string& _content, const std::string& header_);
				packet(packet&& other);
				packet(const packet& other);
				packet& operator=(const packet& other);
				uint32_t m_ID;
				struct timeval m_sentTime;
				FailType m_behavior;
				std::string m_content;
				std::string m_header;
			};

			std::unordered_map<int32_t, std::pair<packet, SendCallback> > m_outPackets;
			std::map<int32_t, packet> m_packets;
			std::unordered_set<int32_t> m_received;
			std::mutex m_packetMutex;
			int32_t m_highId;
			int32_t m_currentId;
			struct sockaddr m_src;
			struct timeval m_lastRcvd;
			struct timeval m_lastSent;
			socklen_t m_slen;

			void SendPacketWithID(const std::string& str, FailType behavior, int32_t sendid, SendCallback callback);
		};

		//Asynchronous socket class
		class ServerSocket
		{
		public:
			ServerSocket(const ConnectionType connectionType);

			//Set callbacks
			void SetOnReceive( ReceiveCallback c );

			void SetOnConnect( ConnectionCallback c );

			void SetOnClose( ConnectionCallback c );

			void SetPacketValidator( PacketValidationCallback c );

			//listen() not only opens the port, but actually starts a network thread to handle connections on it
			void listen(int port);

			//Close() stops the network thread and closes the port
			void Close();

			~ServerSocket();

			//Accessors to get and close connections.
			//Connections are always returned as weak pointers, if a reference is held by client code it may become invalid later.
			//This ensures the client never has a corrupt pointer to a connection that's been closed
			std::vector< std::weak_ptr<Connection> > GetConnections();

			std::weak_ptr<Connection> GetConnection(int i);

			std::weak_ptr<Connection> GetConnectionByDesc(int d);

			std::weak_ptr<Connection> GetConnectionByPort(int p);

			size_t GetNumConnections();

			void CloseConnection(int i);

			void CloseConnection(std::shared_ptr<Connection> c);

		protected:
			friend class Connection;
			friend class UDPConnection;
			void NotifySend();

		private:
			//Starts the network thread
			void SendThread();
			void RecvThread();

			SOCKET m_inSock;

			ConnectionCallback m_onConnect;
			ConnectionCallback m_onClose;
			ReceiveCallback m_onReceive;
			PacketValidationCallback m_packetValidator;

			bool m_running;

			std::vector< std::shared_ptr<Connection> > m_connections;

			fd_set m_inSet;
			fd_set m_excSet;
			struct addrinfo m_hints;
			struct addrinfo* m_servInfo;
			std::thread m_sendThread;
			std::thread m_recvThread;
			std::condition_variable m_sendNotifier;
			std::mutex m_mtx;
			std::mutex m_sendLock;

			ConnectionType m_connectionType;
		};

		class ClientSocket
		{
		public:
			ClientSocket(ConnectionType connectionType);

			~ClientSocket();

			void Connect(const std::string& addr, int port);

			void Reconnect();

			//Set callbacks
			void SetOnReceive( ReceiveCallback c );

			void SetOnConnect( ConnectionCallback c );

			void SetOnClose( ConnectionCallback c );

			void SetPacketValidator( PacketValidationCallback c );

			std::weak_ptr<Connection> GetConnection()
			{
				return m_con;
			}

			void Send(const std::string& str)
			{
				m_con->Send(str);
			}

			void Send(const std::string& str, FailType behavior)
			{
				m_con->Send(str, behavior);
			}

			void Close();

		protected:
			friend class Connection;
			friend class UDPConnection;
			void NotifySend();

		private:
			//Starts the network thread
			void SendThread();
			void RecvThread();

			ConnectionCallback m_onConnect;
			ConnectionCallback m_onClose;
			ReceiveCallback m_onReceive;
			PacketValidationCallback m_packetValidator;

			std::shared_ptr<Connection> m_con;
			SOCKET m_sock;
			fd_set m_inSet;
			fd_set m_excSet;
			struct addrinfo m_hints;
			struct addrinfo* m_servInfo;

			bool m_running;
			std::thread m_sendThread;
			std::thread m_recvThread;
			std::condition_variable m_sendNotifier;
			std::mutex m_sendLock;

			ConnectionType m_connectionType;
		};
	}
}
