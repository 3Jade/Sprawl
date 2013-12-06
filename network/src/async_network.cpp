#include "async_network.hpp"

namespace sprawl
{
	namespace async_network
	{
		int Connection::GetDescriptor()
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

		void Connection::Send(const std::string& data, SendCallback onSendFunction /*= nullptr*/)
		{
			//Store this data to be sent later on the network thread
			std::lock_guard<std::mutex> lock(m_outDataMutex);

			m_outData.push_back(std::make_pair(data, onSendFunction));
		}

		Connection::Connection(int desc_, struct sockaddr* addr, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_)
			: m_desc(desc_)
			, m_closeSocket(false)
			, m_partialPacket()
			, m_onReceive(onReceive_)
			, m_validatePacket(validatePacket_)
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
				send(m_desc, data.first.c_str(), data.first.length(), 0);
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
			ret = recv(m_desc, &buf, 32768, 0);
			if(ret <= 0)
			{
				return ret;
			}
			if(m_onReceive)
			{
				//If there's no onReceive callback, what can we do? Nothing.
				std::string packet = m_partialPacket + std::string(buf, ret);

				//If we don't have a packet validator, we assume the packet is valid.
				bool packetValid = true;
				std::string leftovers;
				if(m_validatePacket)
				{
					packetValid = m_validatePacket(packet, leftovers);
				}
				if(packetValid)
				{
					m_partialPacket = leftovers;
					m_onReceive(shared_from_this(), packet);
				}
				else
				{
					m_partialPacket = packet;
				}

			}
			return ret;
		}

		/*virtual*/ void UDPConnection::Send(const std::string& str, FailType behavior, SendCallback callback /*= nullptr*/) /*override final*/
		{
			SendPacketWithID(str, behavior, m_currentId, callback);
			m_currentId++;
		}

		UDPConnection::UDPConnection(int desc_, struct sockaddr* addr, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_)
			: Connection(desc_, addr, onReceive_, validatePacket_)
			, m_packets()
			, m_highId(-1)
			, m_currentId(0)
			, m_slen(sizeof(sockaddr_in))
		 {
			gettimeofday(&m_lastRcvd, nullptr);
			m_lastSent.tv_sec = 0;
			m_lastSent.tv_usec = 0;
		 }

		UDPConnection::UDPConnection(int desc_, ReceiveCallback onReceive_, PacketValidationCallback validatePacket_)
			: Connection(desc_, nullptr, onReceive_, validatePacket_)
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
			std::vector< std::pair<packet, SendCallback> > outPackets;

			{
				std::lock_guard<std::mutex> lock(m_outDataMutex);
				outPackets = std::move( m_outPackets );
			}
			for(auto& kvp: outPackets)
			{
				//Send the packet...
				std::string content = kvp.first.m_header + kvp.first.m_content;
				sendto(m_desc, content.c_str(), content.length(), 0, &m_dest, m_slen);
				if(kvp.second)
				{
					kvp.second();
				}
				if(kvp.first.m_behavior != FailType::ignore)
				{
					m_packets.insert(std::make_pair(kvp.first.m_ID, kvp.first));
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
				return ret;
			}
			if(m_lastRcvd.tv_sec == 0 && m_lastRcvd.tv_usec == 0)
			{
				//New connection. Remember who's on the other end.
				m_dest = m_src;
			}
			//If we're not pulling from the person who's actually on the other end of this connection, ignore the data.
			if(((sockaddr_in*)&m_dest)->sin_addr.s_addr == ((sockaddr_in*)&m_src)->sin_addr.s_addr
					&& ((sockaddr_in*)&m_dest)->sin_port == ((sockaddr_in*)&m_src)->sin_port)
			{
				ret = recvfrom(m_desc, buf, 32768, 0, (sockaddr*)&m_src, &m_slen);
				int id, ack;
				uint32_t bits;
				memcpy(&id, buf, sizeof(uint32_t));

				if(m_received.count(id))
				{
					//We already received this packet, so we can ignore it now.
					return ret;
				}

				buf += sizeof(uint32_t);

				memcpy(&ack, buf, sizeof(uint32_t));
				buf += sizeof(uint32_t);

				memcpy(&bits, buf, sizeof(uint32_t));
				buf += sizeof(uint32_t);

				ret = ret-(sizeof(uint32_t)*3);

				gettimeofday(&m_lastRcvd, nullptr);
				if(id > m_highId)
				{
					m_highId = id;
				}
				m_received.insert(id);
				if(ack >= 0)
				{
					if(m_packets.find(ack) != m_packets.end())
					{
						m_packets.erase(ack);
					}
					for(int i=0; i<32; i++)
					{
						int ackid = ack - i - 1;
						if(ackid < 0)
						{
							break;
						}
						if((bits & (1 << i)) == 1 && m_packets.find(ackid) != m_packets.end())
						{
							m_packets.erase(ackid);
						}
					}
				}
				if(ret <= 0)
				{
					return ret;
				}
				if(m_onReceive)
				{
					//If there's no onReceive callback, what can we do? Nothing.
					std::string packet = m_partialPacket + std::string(buf, ret);

					//If we don't have a packet validator, we assume the packet is valid.
					bool packetValid = true;
					std::string leftovers;
					if(m_validatePacket)
					{
						packetValid = m_validatePacket(packet, leftovers);
					}
					if(packetValid)
					{
						m_partialPacket = leftovers;
						m_onReceive(shared_from_this(), packet);
					}
					else
					{
						m_partialPacket = packet;
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
			if((m_lastRcvd.tv_sec != 0 && m_lastRcvd.tv_usec != 0) && secs >= 5)
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
					m_packets.erase(it++);
				}
				else
				{
					it++;
				}
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

		UDPConnection::packet::packet(uint32_t _id, FailType _behavior, const std::string& _content, const std::string& header_)
			: m_ID(_id)
			, m_behavior(_behavior)
			, m_content(_content)
			, m_header(header_)
		{
			gettimeofday(&m_sentTime, nullptr);
		}

		void UDPConnection::SendPacketWithID(const std::string& str, FailType behavior, int32_t sendid, SendCallback callback)
		{
			char header[3*sizeof(uint32_t)];
			char* ptr = header;

			//Construct header: ID, ACK, Ack bits
			memcpy(ptr, &sendid, sizeof(uint32_t));
			int id = m_highId;
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
			memcpy(ptr+sizeof(uint32_t)*2, &bits, sizeof(uint32_t));

			//Create a packet from header + str...
			std::string headerStr(header, 3*sizeof(uint32_t));

			{
				std::lock_guard<std::mutex> lock(m_outDataMutex);
				m_outPackets.push_back(std::make_pair(packet(sendid, behavior, str, headerStr), callback));
			}
		}

		ServerSocket::ServerSocket(const ConnectionType connectionType)
			: m_inSock(-1)
			, m_inPort(-1)
			, m_tv()
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
			, m_thread()
			, m_mtx()
			, m_connectionType(connectionType)
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

			//Default timeout at .5 seconds
			m_tv.tv_sec = 0;
			m_tv.tv_usec = 500000;
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

		void ServerSocket::listen(int port)
		{
			if( m_inSock != -1 )
			{
				throw SockExcept("Socket already open!");
			}

			//Ports lower than 1024 require root access, just not going to support them
			if( port < 1024 || port > 65535 )
			{
				throw SockExcept("Port out of range.");
			}

			//Get the localhost address info for the requested port
			std::stringstream s;
			s << port;
			int status = getaddrinfo(nullptr, s.str().c_str(), &m_hints, &m_servInfo);

			if( status != 0 )
			{
				throw SockExcept(gai_strerror(status));
			}

			//Open the socket
			m_inSock = socket(m_servInfo->ai_family, m_servInfo->ai_socktype, m_servInfo->ai_protocol);
			if( m_inSock == -1 )
			{
				throw SockExcept("Could not open socket.");
			}

			int yes = 1;
			setsockopt(m_inSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

			//Bind the port
			if( ::bind(m_inSock, m_servInfo->ai_addr, m_servInfo->ai_addrlen) == -1 )
			{
				throw SockExcept("Port already in use.");
			}

			//Open the port for incoming connections
			::listen(m_inSock, 5);

			//And start up the network thread to actually handle them
			m_thread = std::thread(&ServerSocket::RunThread, this );
		}

		void ServerSocket::Close()
		{
			m_running = false;
			m_thread.join();
			close(m_inSock);
			m_inSock = -1;
			freeaddrinfo(m_servInfo);
		}

		ServerSocket::~ServerSocket()
		{
			Close();
		}

		void ServerSocket::setTimeout(int timeout)
		{
			m_tv.tv_sec = timeout/1000;
			m_tv.tv_usec = (timeout%1000) * 1000;
		}

		std::vector<std::weak_ptr<Connection> > ServerSocket::GetConnections()
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			std::vector< std::weak_ptr<Connection> > ret;
			for( auto& connection : m_connections )
			{
				ret.push_back(connection);
			}
			return std::move(ret);
		}

		std::weak_ptr<Connection> ServerSocket::GetConnection(int i)
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			return m_connections[i];
		}

		std::weak_ptr<Connection> ServerSocket::GetConnectionByDesc(int d)
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			for( auto& connection : m_connections )
			{
				if (connection->GetDescriptor() == d)
				{
					return connection;
				}
			}
			return std::weak_ptr<Connection>();
		}

		std::weak_ptr<Connection> ServerSocket::GetConnectionByPort(int p)
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			for( auto& connection : m_connections )
			{
				if (connection->GetPort() == p)
				{
					return connection;
				}
			}
			return std::weak_ptr<Connection>();
		}

		int ServerSocket::GetNumConnections()
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			return m_connections.size();
		}

		void ServerSocket::CloseConnection(std::shared_ptr<Connection> c)
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

		void ServerSocket::RunThread()
		{
			m_running = true;
			while(m_running)
			{
				HandleIO();
			}
		}

		void ServerSocket::HandleIO()
		{
			struct sockaddr_storage addr;
			socklen_t addr_size = sizeof(addr);
			std::shared_ptr<Connection> c;
			struct timeval t = m_tv;

			FD_ZERO(&m_inSet);
			FD_ZERO(&m_excSet);
			FD_SET(m_inSock, &m_inSet);
			int newcon = -1;
			int max = m_inSock;

			for( auto& connection : m_connections )
			{
				int desc = connection->GetDescriptor();
				if (desc > max)
					max = desc;
				FD_SET( desc, &m_inSet );
				FD_SET( desc, &m_excSet );
			}

			int ret = select(max + 1, &m_inSet, NULL, &m_excSet, &t);

			std::lock_guard<std::mutex> lock(m_mtx);

			if( ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK )
			{
				throw SockExcept(std::string("HandleIO poll failure: ") + strerror(errno));
			}

			if( FD_ISSET( m_inSock, &m_excSet ) )
			{
				throw SockExcept("Exception on listen port.");
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
						setsockopt(newcon, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int));
						c.reset(new Connection(newcon, ((sockaddr*)&addr), m_onReceive, m_packetValidator));
						if(m_onConnect)
						{
							m_onConnect(c);
						}
						m_connections.push_back(c);
					}
					else if(errno != EWOULDBLOCK && errno != EAGAIN)
					{
						throw SockExcept("Error accepting new connection.");
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
						c.reset(new UDPConnection(m_inSock, m_onReceive, m_packetValidator));
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
				for( auto it = m_connections.begin(); it != m_connections.end(); )
				{
					auto& connection = *it;
					connection->Send();
					if( FD_ISSET( connection->GetDescriptor(), &m_inSet ) )
					{
						if(connection->Recv() == 0)
						{
							if(m_onClose)
							{
								m_onClose(connection);
							}
							//Don't increment it here, erasing will increment it automatically.
							m_connections.erase(it);
							continue;
						}
					}
					it++;
				}
			}
			else
			{
				for( auto it = m_connections.begin(); it != m_connections.end(); )
				{
					auto& connection = *it;
					connection->Send();
					if(std::static_pointer_cast<UDPConnection>(connection)->CheckClosed())
					{
						if(m_onClose)
						{
							m_onClose(connection);
						}
						//Don't increment it here, erasing will increment it automatically.
						m_connections.erase(it);
						continue;
					}
					std::static_pointer_cast<UDPConnection>(connection)->SendKeepAlive();
					it++;
				}
			}

		}

		void ServerSocket::CloseConnection(int i)
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			std::shared_ptr<Connection> c = m_connections[i];
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
			, m_tv()
			, m_running(false)
			, m_thread()
			, m_connectionType(connectionType)
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
			//Default timeout at .5 seconds
			m_tv.tv_sec = 0;
			m_tv.tv_usec = 500000;
		}

		ClientSocket::~ClientSocket()
		{
			freeaddrinfo(m_servInfo);
		}

		void ClientSocket::setTimeout(int timeout)
		{
			m_tv.tv_sec = timeout/1000;
			m_tv.tv_usec = (timeout%1000) * 1000;
		}

		void ClientSocket::Connect(const std::string& addr, int port)
		{
			struct addrinfo* p;
			if(port < 1 || port > 65535)
				throw SockExcept("Port out of range.");
			std::stringstream s;
			s << port;
			getaddrinfo(addr.c_str(), s.str().c_str(), &m_hints, &m_servInfo);

			for(p = m_servInfo; p != nullptr; p = p->ai_next) {
				if ((m_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
					throw SockExcept("Could not open socket.");

				if (connect(m_sock, p->ai_addr, p->ai_addrlen) == -1) {
					close(m_sock);
					continue;
				}

				break;
			}

			if(p == nullptr)
				throw SockExcept("Connection failure.");

			if(m_connectionType == ConnectionType::TCP)
			{
				m_con.reset(new Connection(m_sock, (sockaddr*)p->ai_addr, m_onReceive, m_packetValidator));
			}
			else
			{
				m_con.reset(new UDPConnection(m_sock, (sockaddr*)p->ai_addr, m_onReceive, m_packetValidator));
			}

			m_thread = std::thread(&ClientSocket::RunThread, this );
		}

		void ClientSocket::Reconnect()
		{
			struct addrinfo* p;

			if(m_con != nullptr)
			{
				throw SockExcept("Already connected.");
			}
			for(p = m_servInfo; p != nullptr; p = p->ai_next) {
				if ((m_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
					throw SockExcept("Could not open socket.");

				if (connect(m_sock, p->ai_addr, p->ai_addrlen) == -1) {
					close(m_sock);
					continue;
				}

				break;
			}

			if(p == nullptr)
				throw SockExcept("Connection failure.");

			if(m_connectionType == ConnectionType::TCP)
			{
				m_con.reset(new Connection(m_sock, (sockaddr*)p->ai_addr, m_onReceive, m_packetValidator));
			}
			else
			{
				m_con.reset(new UDPConnection(m_sock, (sockaddr*)p->ai_addr, m_onReceive, m_packetValidator));
			}

			m_thread = std::thread(&ClientSocket::RunThread, this );
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
			if(std::this_thread::get_id() != m_thread.get_id())
			{
				m_thread.join();
			}
			if(m_onClose)
			{
				m_onClose(m_con);
			}
			m_con.reset();
		}

		void ClientSocket::RunThread()
		{
			m_running = true;

			if(m_onConnect)
			{
				m_onConnect(m_con);
			}

			while(m_running)
			{
				HandleIO();
			}
		}

		void ClientSocket::HandleIO()
		{
			if(m_con == nullptr)
			{
				throw SockExcept("No active connection.");
			}
			FD_ZERO(&m_inSet);
			FD_ZERO(&m_excSet);
			FD_SET( m_sock, &m_inSet );
			FD_SET( m_sock, &m_excSet );

			struct timeval t = m_tv;
			int ret = select(m_sock + 1, &m_inSet, NULL, &m_excSet, &t);

			if( ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK )
			{
				throw SockExcept(std::string("HandleIO poll failure: ") + strerror(errno));
			}

			if( FD_ISSET( m_sock, &m_inSet ) )
			{
				if( m_con->Recv() == 0 && m_connectionType == ConnectionType::TCP )
				{
					Close();
					return;
				}
			}
			m_con->Send();
			if(m_connectionType == ConnectionType::UDP)
			{
				if(std::static_pointer_cast<UDPConnection>(m_con)->CheckClosed())
				{
					Close();
					return;
				}
				std::static_pointer_cast<UDPConnection>(m_con)->SendKeepAlive();
			}
		}


	}
}
