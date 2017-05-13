#ifdef _WIN32
#	define _CRT_SECURE_NO_WARNINGS
#else
#	include <string.h>
#endif

#include "async_network.hpp"
#include <sstream>
#include "../common/logging.hpp"
#include "../common/compat.hpp"

#ifdef _WIN32
#	define close closesocket
#	include <time.h>

#	if SPRAWL_COMPILER_MSVC || defined(_MSC_EXTENSIONS)
#		define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#	else
#		define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#	endif

	namespace
	{
		int gettimeofday(struct timeval* tv, void* unusedArg)
		{
			if(tv == nullptr)
				return 0;

			FILETIME ft;
			unsigned __int64 asInt = 0;

			GetSystemTimeAsFileTime(&ft);

			asInt = ft.dwHighDateTime;
			asInt <<= 32;
			asInt |= ft.dwLowDateTime;
			asInt -= DELTA_EPOCH_IN_MICROSECS;
			asInt /= 10;

			tv->tv_sec = (long)(asInt / 1000000UL);
			tv->tv_usec = (long)(asInt % 1000000UL);
			return 0;
		}
	}

#include <WinBase.h>
#include <Winsock2.h>
#endif

namespace sprawl
{
	namespace async_network
	{
		namespace
		{
			static void PrintLastError(char const* prefixText)
			{
#	ifdef _WIN32
				char buf[512];
				FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 512, NULL);
				SPRAWL_LOG_TRACE( "%s: %s\n", prefixText, buf );
#	else
				SPRAWL_LOG_TRACE("%s: %s\n", prefixText, strerror(errno));
#	endif
			}
		}
		SOCKET Connection::GetDescriptor()
		{
			return m_desc;
		}

		std::string Connection::GetHostname()
		{
			char buf[256];
			inet_ntop( AF_INET, &((sockaddr_in*)&m_dest)->sin_addr, buf, sizeof(buf) );
			return buf;
		}

		int Connection::GetPort()
		{
			return ((sockaddr_in*)&m_dest)->sin_port;
		}

		void Connection::Send(std::string const& data, SendCallback onSendFunction /*= nullptr*/)
		{
			//Store this data to be sent later on the network thread
			std::lock_guard<std::mutex> lock(m_outDataMutex);

			SPRAWL_LOG_TRACE("Request to send %d bytes of data, adding to queue", int(data.length()));

			m_outData.push_back(std::make_pair(data, onSendFunction));

			if( m_parentClientSocket )
			{
				m_parentClientSocket->NotifySend();
			}
			else if( m_parentServerSocket )
			{
				m_parentServerSocket->NotifySend();
			}
		}

		Connection::Connection(ServerSocket* parent, SOCKET desc_, struct sockaddr* addr, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_)
			: m_desc(desc_)
			, m_closeSocket(false)
			, m_partialPacket()
			, m_onReceive(onReceive_)
			, m_validatePacket(validatePacket_)
			, m_parentServerSocket(parent)
			, m_parentClientSocket(nullptr)
		{
			if(addr != nullptr)
			{
				m_dest = *addr;
			}
		}

		Connection::Connection(ClientSocket* parent, SOCKET desc_, struct sockaddr* addr, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_)
			: m_desc(desc_)
			, m_closeSocket(false)
			, m_partialPacket()
			, m_onReceive(onReceive_)
			, m_validatePacket(validatePacket_)
			, m_parentServerSocket(nullptr)
			, m_parentClientSocket(parent)
		{
			if(addr != nullptr)
			{
				m_dest = *addr;
			}
		}

		void Connection::Send()
		{
			//Send all the things
			std::vector<std::pair<std::string, SendCallback>> outData;

			{
				std::lock_guard<std::mutex> lock(m_outDataMutex);
				outData = std::move( m_outData );
			}

			for(auto& data : outData)
			{
				SPRAWL_LOG_TRACE("TCP: Performing send with %d bytes of data", int(data.first.length()));
				int ret = send(m_desc, data.first.c_str(), (int)data.first.length(), 0);
				if(ret == -1)
				{
					PrintLastError("TCP: Send failed");
				}

				if(data.second)
				{
					data.second();
				}
			}
		}

		int Connection::Recv()
		{
			if(m_closeSocket)
			{
				close(m_desc);
				return 0;
			}
			int ret;
			char buf[32768];
			char* pbuf = buf;

			SPRAWL_LOG_TRACE("TCP: Told there was data on socket %d, receiving.", m_desc);
			ret = recv(m_desc, pbuf, 32768, 0);
			SPRAWL_LOG_TRACE("TCP: Received %d bytes of data on socket %d.", ret, m_desc);
			if(ret <= 0)
			{
				if(ret == -1)
				{
					PrintLastError("TCP: Recv failed");
				}
				return ret;
			}

			if(m_onReceive)
			{
				//If there's no onReceive callback, what can we do? Nothing.
				m_partialPacket.append(buf, ret);
				const char* cPacket = m_partialPacket.c_str();
				int packetEnd = (int)m_partialPacket.length();

				while(packetEnd > 0)
				{
					int newEnd = packetEnd;
					//if we don't have a validator we'll just give them the full packet.
					if(m_validatePacket)
					{
						int totalLength = -1;
						newEnd = m_validatePacket(cPacket, packetEnd, totalLength);
						if(totalLength > 0 && totalLength < (int)m_partialPacket.capacity())
						{
							m_partialPacket.reserve(totalLength);
						}
					}
					if(newEnd > 0)
					{
						SPRAWL_LOG_TRACE("TCP: Client code informs of complete packet, calling receive callback.");
						m_onReceive(shared_from_this(), cPacket, newEnd);
						cPacket += newEnd;
						packetEnd -= newEnd;
						m_partialPacket = std::string(cPacket, packetEnd);
						cPacket = m_partialPacket.c_str();
					}
					else
					{
						SPRAWL_LOG_TRACE("TCP: Client code informs packet is incomplete, returning and waiting for more.");
						break;
					}
				}
			}
			return ret;
		}

		void Connection::Close()
		{
			SPRAWL_LOG_TRACE("Socket close: %d", m_desc);
			if( m_parentClientSocket )
			{
				m_parentClientSocket->Close();
			}
			else if( m_parentServerSocket )
			{
				m_parentServerSocket->CloseConnection( shared_from_this() );
			}
		}

		/*virtual*/ void UDPConnection::Send(std::string const& str, FailType behavior, SendCallback callback /*= nullptr*/) /*override final*/
		{
			SPRAWL_LOG_TRACE("UDP: Request to send %d bytes of data, adding to queue", int(str.length()));
			SendPacketWithID(str, behavior, ++m_currentId, callback);

			if( m_parentClientSocket )
			{
				m_parentClientSocket->NotifySend();
			}
			else if( m_parentServerSocket )
			{
				m_parentServerSocket->NotifySend();
			}
		}

		UDPConnection::UDPConnection(ServerSocket* parent, SOCKET desc_, struct sockaddr* addr, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_)
			: Connection(parent, desc_, addr, onReceive_, validatePacket_)
			, m_packets()
			, m_highId(-1)
			, m_currentId(0)
			, m_slen(sizeof(sockaddr_in))
		 {
			gettimeofday(&m_lastRcvd, nullptr);
			m_lastSent.tv_sec = 0;
			m_lastSent.tv_usec = 0;
		 }

		UDPConnection::UDPConnection(ServerSocket* parent, SOCKET desc_, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_)
			: Connection(parent, desc_, nullptr, onReceive_, validatePacket_)
			, m_packets()
			, m_highId(-1)
			, m_currentId(0)
			, m_slen(sizeof(sockaddr_in))
		{
			m_lastRcvd.tv_sec = 0;
			m_lastRcvd.tv_usec = 0;
			m_lastSent.tv_sec = 0;
			m_lastSent.tv_usec = 0;
		}

		UDPConnection::UDPConnection(ClientSocket* parent, SOCKET desc_, struct sockaddr* addr, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_)
			: Connection(parent, desc_, addr, onReceive_, validatePacket_)
			, m_packets()
			, m_highId(-1)
			, m_currentId(0)
			, m_slen(sizeof(sockaddr_in))
		 {
			gettimeofday(&m_lastRcvd, nullptr);
			m_lastSent.tv_sec = 0;
			m_lastSent.tv_usec = 0;
		 }

		UDPConnection::UDPConnection(ClientSocket* parent, SOCKET desc_, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_)
			: Connection(parent, desc_, nullptr, onReceive_, validatePacket_)
			, m_packets()
			, m_highId(-1)
			, m_currentId(0)
			, m_slen(sizeof(sockaddr_in))
		{
			m_lastRcvd.tv_sec = 0;
			m_lastRcvd.tv_usec = 0;
			m_lastSent.tv_sec = 0;
			m_lastSent.tv_usec = 0;
		}

		/*virtual*/ void UDPConnection::Send() /*override final*/
		{
			std::unordered_map<int32_t, std::pair<packet, SendCallback> > outPackets;

			{
				std::lock_guard<std::mutex> lock(m_outDataMutex);
				outPackets = std::move( m_outPackets );
			}

			std::lock_guard<std::mutex> lock(m_packetMutex);
			for(auto& kvp: outPackets)
			{
				SPRAWL_LOG_TRACE("UDP: Performing send with %d bytes of data", int(kvp.second.first.m_content.length()));
				//Send the packet...
				std::string content = kvp.second.first.m_header + kvp.second.first.m_content;
				int ret = sendto(m_desc, content.c_str(), (int)content.length(), 0, &m_dest, m_slen);
				if(ret == -1)
				{
					PrintLastError("UDP: Send failed");
				}
				if(kvp.second.second)
				{
					kvp.second.second();
				}
				if(kvp.second.first.m_behavior != FailType::ignore)
				{
					if(m_packets.count(kvp.first))
					{
						m_packets.at(kvp.first) = kvp.second.first;
					}
					else
					{
						m_packets.insert(std::make_pair(kvp.first, kvp.second.first));
					}
				}
			}
			if(!outPackets.empty())
			{
				gettimeofday(&m_lastSent, nullptr);
			}
		}

		/*virtual*/ int UDPConnection::Recv() /*override final*/
		{
			int ret;
			char abuf[32768];
			char* buf = abuf;
			//Check the data to see where it's from. Don't pull it yet.
			ret = recvfrom(m_desc, buf, 32768, MSG_PEEK, (sockaddr*)&m_src, &m_slen);
			if(ret == -1)
			{
				PrintLastError("UDP: recv failed");
				return ret;
			}
			if(m_lastRcvd.tv_sec == 0 && m_lastRcvd.tv_usec == 0)
			{
				//New connection. Remember who's on the other end.
				m_dest = m_src;
			}
			SPRAWL_LOG_TRACE("UDP: Receiving %d bytes of data on socket %d.", ret, m_dest);
			//If we're not pulling from the person who's actually on the other end of this connection, ignore the data.
			if(((sockaddr_in*)&m_dest)->sin_addr.s_addr == ((sockaddr_in*)&m_src)->sin_addr.s_addr
					&& ((sockaddr_in*)&m_dest)->sin_port == ((sockaddr_in*)&m_src)->sin_port)
			{
				ret = recvfrom(m_desc, buf, 32768, 0, (sockaddr*)&m_src, &m_slen);
				int32_t id, ack;
				uint32_t bits;

				const int32_t* header = reinterpret_cast<const int32_t*>(buf);
				id = header[0];
				ack = header[1];
				bits = header[2];

				gettimeofday(&m_lastRcvd, nullptr);

				buf += (3 * sizeof(uint32_t));
				ret -= (3 * sizeof(uint32_t));

				if(id > m_highId)
				{
					m_highId = id;
				}
				if(ack >= 0)
				{
					std::lock_guard<std::mutex> dataLock(m_outDataMutex);
					std::lock_guard<std::mutex> packetLock(m_packetMutex);
					if(m_packets.count(ack))
					{
						m_packets.erase(ack);
					}
					if(m_outPackets.count(ack))
					{
						m_outPackets.erase(ack);
					}
					for(int i=0; i<32; i++)
					{
						int ackid = ack - i - 1;
						if(ackid < 0)
						{
							break;
						}
						if((bits & (1 << i)) != 0)
						{
							if(m_packets.count(ackid))
							{
								m_packets.erase(ackid);
							}
							if(m_outPackets.count(ackid))
							{
								m_outPackets.erase(ackid);
							}
						}
					}
					fflush(stdout);
				}
				if(ret <= 0)
				{
					return ret;
				}

				if(m_received.count(id))
				{
					//We already received this packet, so we can ignore it now.
					return ret;
				}

				m_received.insert(id);

				if(m_onReceive)
				{
					//If there's no onReceive callback, what can we do? Nothing.
					m_partialPacket.append(buf, ret);
					const char* cPacket = m_partialPacket.c_str();
					int packetEnd = (int)m_partialPacket.length();

					while(packetEnd > 0)
					{
						int newEnd = packetEnd;
						//if we don't have a validator we'll just give them the full packet.
						if(m_validatePacket)
						{
							int totalLength = -1;
							newEnd = m_validatePacket(cPacket, packetEnd, totalLength);
							if(totalLength > 0 && totalLength < (int)m_partialPacket.capacity())
							{
								m_partialPacket.reserve(totalLength);
							}
						}
						if(newEnd > 0)
						{
							m_onReceive(shared_from_this(), cPacket, newEnd);
							cPacket += newEnd;
							packetEnd -= newEnd;
							m_partialPacket = std::string(cPacket, packetEnd);
							cPacket = m_partialPacket.c_str();
						}
						else
						{
							break;
						}
					}
				}
				return ret;
			}
			//-2 indicates a valid request, but invalid for this connection. Move on and try the next connection.
			return -2;
		}
		bool UDPConnection::CheckClosed()
		{
			struct timeval now;
			gettimeofday(&now, nullptr);
			int secs = now.tv_sec - m_lastRcvd.tv_sec;
			int usecs = now.tv_usec - m_lastRcvd.tv_usec;
			while(usecs < 0)
			{
				usecs += 1000000;
				secs -= 1;
			}
			if((m_lastRcvd.tv_sec != 0 || m_lastRcvd.tv_usec != 0) && secs >= 5)
			{
				return true;
			}
			return false;
		}
		void UDPConnection::SendKeepAlive()
		{
			struct timeval now;
			gettimeofday(&now, nullptr);
			int secs, usecs;

			std::lock_guard<std::mutex> lock(m_packetMutex);
			auto it = m_packets.begin();
			while( it != m_packets.end() )
			{
				secs = now.tv_sec - it->second.m_sentTime.tv_sec;
				int usecs = now.tv_usec - it->second.m_sentTime.tv_usec;
				while(usecs < 0)
				{
					usecs += 1000000;
					secs -= 1;
				}
				//Resend any packet that hasn't been ACKed for a full second or more.
				if(secs >= 1)
				{
					SendPacketWithID(it->second.m_content, FailType::resend, it->first, nullptr);
				}
				++it;
			}
			secs = now.tv_sec - m_lastSent.tv_sec;
			usecs = now.tv_usec - m_lastSent.tv_usec;
			while(usecs < 0)
			{
				usecs += 1000000;
				secs -= 1;
			}
			//Pulse four times a second.
			if(secs >= 1 || usecs >= 250000)
			{
				Send("", FailType::ignore);
			}
		}

		UDPConnection::packet::packet(uint32_t _id, FailType _behavior, std::string const& _content, std::string const& header_)
			: m_ID(_id)
			, m_behavior(_behavior)
			, m_content(_content)
			, m_header(header_)
		{
			gettimeofday(&m_sentTime, nullptr);
		}

		UDPConnection::packet::packet(UDPConnection::packet&& other)
			: m_ID(std::move(other.m_ID))
			, m_behavior(std::move(other.m_behavior))
			, m_content(std::move(other.m_content))
			, m_header(std::move(other.m_header))
		{
		}

		UDPConnection::packet::packet(UDPConnection::packet const& other)
			: m_ID(other.m_ID)
			, m_behavior(other.m_behavior)
			, m_content(other.m_content)
			, m_header(other.m_header)
		{
		}

		UDPConnection::packet& UDPConnection::packet::operator=(UDPConnection::packet const& other)
		{
			m_ID = other.m_ID;
			m_behavior = other.m_behavior;
			m_content = other.m_content;
			m_header = other.m_header;
			return *this;
		}

		void UDPConnection::SendPacketWithID(std::string const& str, FailType behavior, int32_t sendid, SendCallback callback)
		{
			char header[3*sizeof(uint32_t)];
			char* ptr = header;

			//Construct header: ID, ACK, Ack bits
			memcpy(ptr, &sendid, sizeof(uint32_t));
			int32_t id = m_highId;
			memcpy(ptr+sizeof(uint32_t), &id, sizeof(uint32_t));
			uint32_t bits = 0;
			if(id >= 0)
			{
				std::vector<int> ids_to_erase;
				for(auto& rcvd : m_received)
				{
					if(rcvd == id)
					{
						continue;
					}
					int bit = id - rcvd - 1;
					//Old bits we don't care about.
					if(bit > 31)
					{
						ids_to_erase.push_back(rcvd);
						continue;
					}
					bits |= (1 << bit);
				}
				for(auto& rcvd : ids_to_erase)
				{
					m_received.erase(rcvd);
				}
			}
			memcpy(ptr+(sizeof(uint32_t)*2), &bits, sizeof(uint32_t));

			//Create a packet from header + str...
			std::string headerStr(header, 3*sizeof(uint32_t));

			{
				std::lock_guard<std::mutex> lock(m_outDataMutex);
				m_outPackets.insert(std::make_pair(sendid, std::make_pair(packet(sendid, behavior, str, headerStr), callback)));
			}
		}

#define SOCK_ERROR(errorStr) do{ m_lastError = errorStr; return false; }while(false)

		ServerSocket::ServerSocket(const ConnectionType connectionType)
			: m_inSock(-1)
			, m_onConnect(nullptr)
			, m_onClose(nullptr)
			, m_onReceive(nullptr)
			, m_packetValidator(nullptr)
			, m_running(false)
			, m_connections()
			, m_inSet()
			, m_excSet()
			, m_hints()
			, m_servInfo(nullptr)
			, m_sendThread()
			, m_recvThread()
			, m_mtx()
			, m_sendLock()
			, m_connectionType(connectionType)
			, m_lastError(nullptr)
			, m_sendReady(false)
		{
			memset(&m_hints, 0, sizeof m_hints);
			m_hints.ai_family = AF_UNSPEC;
			if(m_connectionType == ConnectionType::TCP)
			{
				m_hints.ai_socktype = SOCK_STREAM;
			}
			else
			{
				m_hints.ai_socktype = SOCK_DGRAM;
			}
			m_hints.ai_flags = AI_PASSIVE;
		}

		void ServerSocket::SetOnReceive(ReceiveCallback c)
		{
			m_onReceive = c;
		}

		void ServerSocket::SetOnConnect(ConnectionCallback c)
		{
			m_onConnect = c;
		}

		void ServerSocket::SetOnClose(ConnectionCallback c)
		{
			m_onClose = c;
		}

		void ServerSocket::SetPacketValidator(PacketValidationCallback c)
		{
			m_packetValidator = c;
		}

		bool ServerSocket::listen(int port)
		{
			if( m_inSock != -1 )
			{
				SOCK_ERROR("Socket already open!");
			}

			//Ports lower than 1024 require root access, just not going to support them
			if( port < 1024 || port > 65535 )
			{
				SOCK_ERROR("Port out of range.");
			}

			//Get the localhost address info for the requested port
			std::stringstream s;
			s << port;
			int status = getaddrinfo(nullptr, s.str().c_str(), &m_hints, &m_servInfo);

			if( status != 0 )
			{
				SOCK_ERROR(gai_strerror(status));
			}

			//Open the socket
			m_inSock = socket(m_servInfo->ai_family, m_servInfo->ai_socktype, m_servInfo->ai_protocol);
			if( m_inSock == -1 )
			{
				PrintLastError("Could not open socket");
				return false;
			}

			int yes = 1;
#ifndef _WIN32
			setsockopt(m_inSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
#else
			int no = 0;
			setsockopt(m_inSock, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(int));
			setsockopt(m_inSock, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &no, sizeof(int));
#endif

			//Bind the port
			if( ::bind(m_inSock, m_servInfo->ai_addr, (int)m_servInfo->ai_addrlen) == -1 )
			{
				SOCK_ERROR("Port already in use.");
			}

			//Open the port for incoming connections
			::listen(m_inSock, 5);

			m_running = true;

			//And start up the network thread to actually handle them
			m_sendThread = std::thread(&ServerSocket::SendThread, this );
			m_recvThread = std::thread(&ServerSocket::RecvThread, this );
			return true;
		}

		void ServerSocket::Close()
		{
			if(m_running)
			{
				m_running = false;
				m_sendNotifier.notify_one();
				m_sendThread.join();
				close(m_inSock);
				m_recvThread.join();
				m_inSock = -1;
				freeaddrinfo(m_servInfo);

			}
		}

		ServerSocket::~ServerSocket()
		{
			Close();
		}

		std::vector<ConnectionWPtr > ServerSocket::GetConnections()
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			std::vector< ConnectionWPtr > ret;
			for( auto& connection : m_connections )
			{
				ret.push_back(connection);
			}
			return ret;
		}

		ConnectionWPtr ServerSocket::GetConnection(int i)
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			return m_connections[i];
		}

		ConnectionWPtr ServerSocket::GetConnectionByDesc(int d)
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			for( auto& connection : m_connections )
			{
				if (connection->GetDescriptor() == d)
				{
					return connection;
				}
			}
			return ConnectionWPtr();
		}

		ConnectionWPtr ServerSocket::GetConnectionByPort(int p)
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			for( auto& connection : m_connections )
			{
				if (connection->GetPort() == p)
				{
					return connection;
				}
			}
			return ConnectionWPtr();
		}

		size_t ServerSocket::GetNumConnections()
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			return m_connections.size();
		}

		void ServerSocket::CloseConnection(ConnectionPtr c)
		{
			if(!c)
			{
				return;
			}

			std::lock_guard<std::mutex> lock(m_mtx);
			for( auto it = m_connections.begin(); it != m_connections.end(); it++ )
			{
				if( *it == c )
				{
					if(m_connectionType == ConnectionType::TCP)
					{
						close(c->GetDescriptor());
					}
					if(m_onClose)
					{
						m_onClose(c);
					}
					m_connections.erase(it);

					return;
				}
			}
		}

		void ServerSocket::SendThread()
		{
			while(m_running)
			{
				{
					std::unique_lock<std::mutex> lock(m_sendLock);
					while(!m_sendReady)
					{
						if(m_connectionType == ConnectionType::UDP)
						{
							m_sendNotifier.wait_for( lock, std::chrono::milliseconds(250) );
						}
						else
						{
							m_sendNotifier.wait( lock );
						}
					}
					m_sendReady = false;
				}

				std::lock_guard<std::mutex> lock(m_mtx);

				if(m_connectionType == ConnectionType::TCP)
				{
					for( size_t i = 0; i < m_connections.size(); i++ )
					{
						m_connections[i]->Send();
					}
				}
				else
				{
					std::vector<size_t> indexes_to_erase;
					for( size_t i = 0; i < m_connections.size(); i++ )
					{
						auto& connection = m_connections[i];
						connection->Send();
						if(std::static_pointer_cast<UDPConnection>(connection)->CheckClosed())
						{
							if(m_onClose)
							{
								m_onClose(connection);
							}
							indexes_to_erase.push_back(i);
							continue;
						}
						std::static_pointer_cast<UDPConnection>(connection)->SendKeepAlive();
					}
					for( int i = (int)indexes_to_erase.size() - 1; i >= 0; --i )
					{
						m_connections.erase(m_connections.begin() + indexes_to_erase[i]);
					}
				}
			}
		}

		void ServerSocket::RecvThread()
		{
			while(m_running)
			{
				struct sockaddr_storage addr;
				socklen_t addr_size = sizeof(addr);
				ConnectionPtr c;

				FD_ZERO(&m_inSet);
				FD_ZERO(&m_excSet);
				FD_SET(m_inSock, &m_inSet);
				SOCKET newcon = -1;
				SOCKET max = m_inSock;

				{
					std::lock_guard<std::mutex> lock(m_mtx);
					for( auto& connection : m_connections )
					{
						SOCKET desc = connection->GetDescriptor();
						if (desc > max)
							max = desc;
						FD_SET( desc, &m_inSet );
						FD_SET( desc, &m_excSet );
					}
				}

				int ret = select((int)(max + 1), &m_inSet, NULL, &m_excSet, nullptr);

				SPRAWL_LOG_TRACE("Select informs of %d sockets with data ready to receive.", ret);

				std::lock_guard<std::mutex> lock(m_mtx);

				if( ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK )
				{
					m_lastError = strerror(errno);
					break;
				}

				if( FD_ISSET( m_inSock, &m_excSet ) )
				{
					continue;
					FD_CLR( m_inSock, &m_inSet );
				}
				else if( FD_ISSET( m_inSock, &m_inSet ) )
				{
					if (m_connectionType == ConnectionType::TCP)
					{
						newcon = accept(m_inSock, (struct sockaddr *)& addr, &addr_size);
						if(newcon != -1)
						{
							int yes = 1;
#ifndef _WIN32
							setsockopt(newcon, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int));
#else
							setsockopt(newcon, SOL_SOCKET, SO_KEEPALIVE, (char*) &yes, sizeof(int));
#endif
							c.reset(new Connection(this, newcon, ((sockaddr*)&addr), m_onReceive, m_packetValidator));
							if(m_onConnect)
							{
								m_onConnect(c);
							}
							m_connections.push_back(c);
						}
						else if(errno != EWOULDBLOCK && errno != EAGAIN)
						{
							m_lastError = strerror(errno);
							continue;
						}
						else
						{
							PrintLastError("Could not accept connection");
						}
					}
					else
					{
						bool bFound = false;
						for (auto it = m_connections.begin(); it != m_connections.end(); it++)
						{
							if((*it)->Recv() != -2)
							{
								bFound = true;
							}
						}
						if(!bFound)
						{
							c.reset(new UDPConnection(this, m_inSock, m_onReceive, m_packetValidator));
							if(m_onConnect)
							{
								m_onConnect(c);
							}
							m_connections.push_back(c);
							c->Recv();
						}
					}
				}

				if(m_connectionType == ConnectionType::TCP)
				{
					std::vector<size_t> indexes_to_erase;
					for( size_t i = 0; i < m_connections.size(); i++ )
					{
						auto& connection = m_connections[i];
						if( FD_ISSET( connection->GetDescriptor(), &m_inSet ) )
						{
							if(connection->Recv() <= 0)
							{
								if(m_onClose)
								{
									m_onClose(connection);
								}
								indexes_to_erase.push_back(i);
							}
						}
					}
					for( int i = (int)indexes_to_erase.size() - 1; i >= 0; --i )
					{
						m_connections.erase(m_connections.begin() + indexes_to_erase[i]);
					}
				}
			}
		}

		void ServerSocket::CloseConnection(int i)
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			ConnectionPtr c = m_connections[i];
			if(m_connectionType == ConnectionType::TCP)
			{
				close(c->GetDescriptor());
			}
			if(m_onClose)
			{
				m_onClose(c);
			}
			m_connections.erase(m_connections.begin() + i);
		}

		void ServerSocket::NotifySend()
		{
			std::lock_guard<std::mutex> lock(m_sendLock);
			m_sendReady = true;
			m_sendNotifier.notify_one();
		}

		ClientSocket::ClientSocket(ConnectionType connectionType)
			: m_onConnect(nullptr)
			, m_onClose(nullptr)
			, m_onReceive(nullptr)
			, m_packetValidator(nullptr)
			, m_con()
			, m_sock(-1)
			, m_inSet()
			, m_excSet()
			, m_hints()
			, m_servInfo(nullptr)
			, m_running(false)
			, m_sendThread()
			, m_recvThread()
			, m_sendLock()
			, m_connectionType(connectionType)
			, m_lastError(nullptr)
			, m_sendReady(false)
		{
			memset(&m_hints, 0, sizeof m_hints);
			m_hints.ai_family = AF_UNSPEC;
			if(m_connectionType == ConnectionType::TCP)
			{
				m_hints.ai_socktype = SOCK_STREAM;
			}
			else
			{
				m_hints.ai_socktype = SOCK_DGRAM;
			}
		}

		ClientSocket::~ClientSocket()
		{
			Close();
			freeaddrinfo(m_servInfo);
		}

		bool ClientSocket::Connect(std::string const& addr, int port)
		{
			struct addrinfo* p;
			if(port < 1 || port > 65535)
			{
				SOCK_ERROR("Port out of range.");
			}
			std::stringstream s;
			s << port;
			getaddrinfo(addr.c_str(), s.str().c_str(), &m_hints, &m_servInfo);

			for(p = m_servInfo; p != nullptr; p = p->ai_next)
			{
				if ((m_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
				{
					PrintLastError("Could not open socket.");
					return false;
				}

				if (connect(m_sock, p->ai_addr, (int)p->ai_addrlen) == -1)
				{
					PrintLastError("Socket connect attempt failed (non-fatal)");
					close(m_sock);
					continue;
				}

				break;
			}

			if(p == nullptr)
			{
				SOCK_ERROR("All socket connect attempts failed. Could not establish a connection.");
			}

			if(m_connectionType == ConnectionType::TCP)
			{
				m_con.reset(new Connection(this, m_sock, (sockaddr*)p->ai_addr, m_onReceive, m_packetValidator));
			}
			else
			{
				m_con.reset(new UDPConnection(this, m_sock, (sockaddr*)p->ai_addr, m_onReceive, m_packetValidator));
			}

			m_running = true;

			m_sendThread = std::thread(&ClientSocket::SendThread, this );
			m_recvThread = std::thread(&ClientSocket::RecvThread, this );

			if(m_onConnect)
			{
				m_onConnect(m_con);
			}
			return true;
		}

		bool ClientSocket::Reconnect()
		{
			struct addrinfo* p;

			if(m_con != nullptr)
			{
				SOCK_ERROR("Already connected.");
			}

			for (p = m_servInfo; p != nullptr; p = p->ai_next)
			{
				if ((m_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
				{
					PrintLastError("Could not open socket.");
					return false;
				}

				if (connect(m_sock, p->ai_addr, (int)p->ai_addrlen) == -1)
				{
					PrintLastError("Socket connect attempt failed (non-fatal)");
					close(m_sock);
					continue;
				}

				break;
			}

			if (p == nullptr)
			{
				SOCK_ERROR("All socket connect attempts failed. Could not establish a connection.");
			}

			if(m_connectionType == ConnectionType::TCP)
			{
				m_con.reset(new Connection(this, m_sock, (sockaddr*)p->ai_addr, m_onReceive, m_packetValidator));
			}
			else
			{
				m_con.reset(new UDPConnection(this, m_sock, (sockaddr*)p->ai_addr, m_onReceive, m_packetValidator));
			}

			m_running = true;

			m_sendThread = std::thread(&ClientSocket::SendThread, this );
			m_recvThread = std::thread(&ClientSocket::RecvThread, this );

			if(m_onConnect)
			{
				m_onConnect(m_con);
			}
			return true;
		}

		void ClientSocket::SetOnReceive(ReceiveCallback c)
		{
			m_onReceive = c;
		}

		void ClientSocket::SetOnConnect(ConnectionCallback c)
		{
			m_onConnect = c;
		}

		void ClientSocket::SetOnClose(ConnectionCallback c)
		{
			m_onClose = c;
		}

		void ClientSocket::SetPacketValidator(PacketValidationCallback c)
		{
			m_packetValidator = c;
		}

		void ClientSocket::Close()
		{
			m_running = false;
			if (m_sendThread.joinable())
			{
				if (m_connectionType == ConnectionType::UDP)
				{
					close(m_con->GetDescriptor());
				}
				m_sendNotifier.notify_one();
				m_sendThread.join();
			}
			if(m_recvThread.joinable())
			{
				if (m_connectionType == ConnectionType::TCP)
				{
					close(m_con->GetDescriptor());
				}
				m_recvThread.join();
			}
			m_con.reset();
		}

		void ClientSocket::SendThread()
		{
			while(m_running)
			{
				{
					std::unique_lock<std::mutex> lock(m_sendLock);
					while(!m_sendReady)
					{
						if(m_connectionType == ConnectionType::UDP)
						{
							m_sendNotifier.wait_for( lock, std::chrono::milliseconds(250) );
						}
						else
						{
							m_sendNotifier.wait( lock );
						}
					}
					m_sendReady = false;
				}
				if (!m_running)
				{
					return;
				}
				m_con->Send();
				if(m_connectionType == ConnectionType::UDP)
				{
					if(std::static_pointer_cast<UDPConnection>(m_con)->CheckClosed())
					{
						SPRAWL_LOG_TRACE("Received disconnect signal for socket %d.", m_con->GetDescriptor());
						close(m_con->GetDescriptor());
						if (m_onClose)
						{
							m_onClose(m_con);
						}
						m_running = false;
						return;
					}
					std::static_pointer_cast<UDPConnection>(m_con)->SendKeepAlive();
				}
			}
		}

		void ClientSocket::RecvThread()
		{
			while(m_running)
			{
				if(m_con == nullptr)
				{
					m_running = false;
					break;
				}
				FD_ZERO(&m_inSet);
				FD_ZERO(&m_excSet);
				FD_SET( m_sock, &m_inSet );
				FD_SET( m_sock, &m_excSet );

				int ret = select((int)(m_sock + 1), &m_inSet, NULL, &m_excSet, nullptr);

				if (!m_running)
				{
					return;
				}

				SPRAWL_LOG_TRACE("Select informs client socket has data ready to receive.");

				if( ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK )
				{
					m_lastError = strerror(errno);
				}

				if( FD_ISSET( m_sock, &m_inSet ) )
				{
					if( m_con->Recv() <= 0 && m_connectionType == ConnectionType::TCP )
					{
						SPRAWL_LOG_TRACE("Received disconnect signal for socket %d.", m_con->GetDescriptor());
						close(m_con->GetDescriptor());
						if (m_onClose)
						{
							m_onClose(m_con);
						}
						m_running = false;
						m_sendNotifier.notify_one();
						return;
					}
				}
			}
		}

		void ClientSocket::NotifySend()
		{
			std::lock_guard<std::mutex> lock(m_sendLock);
			m_sendReady = true;
			m_sendNotifier.notify_one();
		}
	}
}
