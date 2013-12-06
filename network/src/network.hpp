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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <exception>
#include <string>
#include <vector>
#include <iostream>
#include <string.h>
#include <sstream>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <boost/thread/mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <map>
#include <deque>
#include <arpa/inet.h>
#include <unordered_set>

#pragma message("sprawl::network is deprecated. sprawl::async_network provides a better interface and implementation.")

namespace sprawl
{
	namespace network
	{
		class SockExcept: public std::exception
		{
		public:
			SockExcept(const std::string& arg) :
				str(arg)
			{
			}
			~SockExcept() throw ()
			{
			}
			const char* what() const throw ()
			{
				return str.c_str();
			}
		private:
			std::string str;
		};

		enum class FailType { resend, ignore, notify };

		enum class ConnectionType { TCP, UDP };

		template<typename T, ConnectionType C>
		class StatedServerSocket;

		template<typename T, ConnectionType C>
		class StatedClientSocket;

		template<typename T>
		class StatedConnection : public std::enable_shared_from_this<StatedConnection<T>>
		{
		public:
			int GetDescriptor()
			{
				return desc;
			}

			bool DataReady()
			{
				if (m_data_ready)
				{
					m_data_ready = false;
					return true;
				}
				return false;
			}

			virtual void Send(const std::string& str)
			{
				send(desc, str.c_str(), str.length(), 0);
			}

			std::string GetData()
			{
				std::string str;

				std::string str1, str2;

				while(!InData.empty())
				{
					str += InData[0];
					InData.pop_front();
				}
				return str;
			}

			std::string GetLine()
			{
				std::string ret;
				bool bFound = false;
				if(m_telnet)
				{
					for(int i=0; i<InData.size(); i++)
					{
						if(InData[i].find("\n") != std::string::npos)
						{
							bFound = true;
							break;
						}
					}
					if(!bFound)
					{
						return "";
					}
				}
				int i = 0;
				while(!InData.empty())
				{
					i = 0;
					bFound = false;
					for (; i < InData[0].length(); i++)
					{
						if(InData[0][i] == '\n')
						{
							i++;
							bFound = true;
							break;
						}
						if(InData[0][i] == '\r')
						{
							continue;
						}
						ret += InData[0][i];
					}
					while(InData[0][i] == '\n' || InData[0][i] == '\r')
					{
						i++;
					}
					InData[0] = InData[0].substr(i, InData[0].length());
					if(InData[0] == "")
					{
						InData.pop_front();
					}
					//Telnet looks through all packets until it finds \n.
					//Otherwise, we treat the end of a packet as an end of line.
					if(!m_telnet)
					{
						break;
					}
				}
				return ret;
			}

			friend class StatedClientSocket<T, ConnectionType::TCP>;
			friend class StatedClientSocket<T, ConnectionType::UDP>;
			friend class StatedServerSocket<T, ConnectionType::TCP>;
			friend class StatedServerSocket<T, ConnectionType::UDP>;

			void SetState(T s)
			{
				state = s;
			}
			T GetState()
			{
				return state;
			}
			std::string GetPacket()
			{
				std::string ret = InData[0];
				InData.pop_front();
				return ret;
			}

			virtual void Send(const std::string& /*str*/, FailType /*behavior*/)
			{
				throw SockExcept("TCP connections cannot specify failure type.");
			}

			std::string GetHostname()
			{
				char buf[256];
				inet_ntop(AF_INET, &((sockaddr_in*)&dest)->sin_addr, buf, sizeof(buf));
				return buf;
			}

			int GetPort()
			{
				return ((sockaddr_in*)&this->dest)->sin_port;
			}

			virtual ~StatedConnection(){}
		protected:
			StatedConnection(int d, bool b, struct sockaddr* addr) :
				desc(d), m_data_ready(false), m_telnet(b), state((T) 0)
			{
				if(addr != nullptr)
				{
					dest = *addr;
				}
			}
			virtual int Recv()
			{
				int ret;
				char buf[32768];
				ret = recv(desc, &buf, 32768, 0);
				if(ret == -1)
				{
					return ret;
				}
				buf[ret] = '\0';
				m_data_ready = true;
				InData.push_back(std::string(buf, ret));
				return ret;
			}
			void setTelnetMode(bool b)
			{
				m_telnet = b;
			}
			int desc;
			std::deque<std::string> InData;
			bool m_data_ready;
			bool m_telnet;
			T state;
			struct sockaddr dest;
		};

		template<typename T>
		class StatedUDPConnection : public StatedConnection<T>
		{
		public:
			virtual void Send(const std::string& str) override final
			{
				Send(str, FailType::ignore);
			}

			struct packet
			{
				packet(uint32_t _id, FailType _behavior, const std::string& _content) : ID(_id), behavior(_behavior), content(_content)
				{
					gettimeofday(&sent_time, nullptr);
				}
				uint32_t ID;
				struct timeval sent_time;
				FailType behavior;
				const std::string content;
			};

			virtual void Send(const std::string& str, FailType behavior) override final
			{
				SendPacketWithID(str, behavior, current_id);
				current_id++;
			}
			virtual ~StatedUDPConnection(){}
		protected:
			virtual void SendPacketWithID(const std::string& str, FailType behavior, int32_t sendid)
			{
			
				boost::interprocess::scoped_lock<boost::mutex> locker;
				if(parent)
				{
					locker = boost::interprocess::scoped_lock<boost::mutex>(parent->recvlock);
				}

				char header[3*sizeof(uint32_t)];
				char* ptr = header;

				//Construct header: ID, ACK, Ack bits
				memcpy(ptr, &sendid, sizeof(uint32_t));
				int id = high_id;
				memcpy(ptr+sizeof(uint32_t), &id, sizeof(uint32_t));
				uint32_t bits = 0;
				if(id >= 0)
				{
					std::vector<int> ids_to_erase;
					for(auto& rcvd : received)
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
						received.erase(rcvd);
					}
				}
				memcpy(ptr+sizeof(uint32_t)*2, &bits, sizeof(uint32_t));

				//Create a packet from header + str...
				std::string content(header, 3*sizeof(uint32_t));
				content += str;

				//Send the packet...
				sendto(this->desc, content.c_str(), content.length(), 0, &this->dest, slen);
				if(behavior != FailType::ignore)
				{
					packets.insert(std::make_pair(sendid, packet(sendid, behavior, str)));
				}
				gettimeofday(&lastsent, nullptr);
			}
			StatedUDPConnection(int d, struct sockaddr* addr)
			: StatedConnection<T>(d, false, addr), packets(), high_id(-1), current_id(0), slen(sizeof(sockaddr_in)), parent(nullptr)
			 {
				gettimeofday(&lastrcvd, nullptr);
				lastsent.tv_sec = 0;
				lastsent.tv_usec = 0;
			 }
			StatedUDPConnection(int d, StatedServerSocket<T, ConnectionType::UDP>* pServ)
			: StatedConnection<T>(d, false, nullptr), packets(), high_id(-1), current_id(0), slen(sizeof(sockaddr_in)), parent(pServ)
			{
				lastrcvd.tv_sec = 0;
				lastrcvd.tv_usec = 0;
				lastsent.tv_sec = 0;
				lastsent.tv_usec = 0;
			}
			StatedUDPConnection(int d, StatedServerSocket<T, ConnectionType::TCP>* /*pServ*/)
			: StatedConnection<T>(d, false, nullptr), packets(), high_id(-1), current_id(0), slen(sizeof(sockaddr_in)), parent(nullptr)
			 {
				throw SockExcept("UDP connection opened on TCP server!");
			 }
			friend class StatedClientSocket<T, ConnectionType::TCP>;
			friend class StatedClientSocket<T, ConnectionType::UDP>;
			friend class StatedServerSocket<T, ConnectionType::TCP>;
			friend class StatedServerSocket<T, ConnectionType::UDP>;
			virtual int Recv() override final
			{
				int ret;
				char abuf[32768];
				char* buf = abuf;
				boost::interprocess::scoped_lock<boost::mutex> locker;
				if(parent)
				{
					locker = boost::interprocess::scoped_lock<boost::mutex>(parent->recvlock);
				}
				//Check the data to see where it's from. Don't pull it yet.
				ret = recvfrom(this->desc, buf, 32768, MSG_PEEK, (sockaddr*)&src, &slen);
				if(ret == -1)
				{
					return ret;
				}
				if(lastrcvd.tv_sec == 0 && lastrcvd.tv_usec == 0)
				{
					//New connection. Remember who's on the other end.
					this->dest = src;
				}
				//If we're not pulling from the person who's actually on the other end of this connection, ignore the data.
				if(((sockaddr_in*)&this->dest)->sin_addr.s_addr == ((sockaddr_in*)&src)->sin_addr.s_addr && ((sockaddr_in*)&this->dest)->sin_port == ((sockaddr_in*)&src)->sin_port)
				{
					ret = recvfrom(this->desc, buf, 32768, 0, (sockaddr*)&src, &slen);
					int id, ack;
					uint32_t bits;
					memcpy(&id, buf, sizeof(uint32_t));
				
					if(received.count(id))
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

					gettimeofday(&lastrcvd, nullptr);
					if(id > high_id)
					{
						high_id = id;
					}
					received.insert(id);
					if(ack >= 0)
					{
						if(packets.find(ack) != packets.end())
						{
							packets.erase(ack);
						}
						for(int i=0; i<32; i++)
						{
							int ackid = ack - i - 1;
							if(ackid < 0)
							{
								break;
							}
							if((bits & (1 << i)) == 1 && packets.find(ackid) != packets.end())
							{
								packets.erase(ackid);
							}
						}
					}
					if(ret <= 0)
					{
						return ret;
					}
					this->m_data_ready = true;
					this->InData.push_back(std::string(buf, ret));
					return ret;
				}
				//-2 indicates a valid request, but invalid for this connection. Move on and try the next connection.
				return -2;
			}
			bool CheckClosed()
			{
				struct timeval now;
				gettimeofday(&now, nullptr);
				int secs = now.tv_sec - lastrcvd.tv_sec;
				int usecs = now.tv_usec - lastrcvd.tv_usec;
				while(usecs < 0)
				{
					usecs += 1000000;
					secs -= 1;
				}
				if((lastrcvd.tv_sec != 0 && lastrcvd.tv_usec != 0) && secs >= 5)
				{
					return true;
				}
				return false;
			}
			void SendKeepAlive()
			{
				struct timeval now;
				gettimeofday(&now, nullptr);
				int secs, usecs;

				auto it = packets.begin();
				while( it != packets.end() )
				{
					secs = now.tv_sec - it->second.sent_time.tv_sec;
					int usecs = now.tv_usec - it->second.sent_time.tv_usec;
					while(usecs < 0)
					{
						usecs += 1000000;
						secs -= 1;
					}
					//Resend any packet that hasn't been ACKed for a full second or more.
					if(secs >= 1)
					{
						SendPacketWithID(it->second.content, FailType::resend, it->first);
						packets.erase(it++);
					}
					else
					{
						it++;
					}
				}
				secs = now.tv_sec - lastsent.tv_sec;
				usecs = now.tv_usec - lastsent.tv_usec;
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
		protected:
			std::map<int, packet> packets;
			std::unordered_set<int> received;
			int high_id;
			uint32_t current_id;
			struct sockaddr src;
			struct timeval lastrcvd;
			struct timeval lastsent;
			socklen_t slen;
			StatedServerSocket<T, ConnectionType::UDP>* parent;
		};

		typedef StatedConnection<int> Connection;
		typedef StatedUDPConnection<int> UDPConnection;

		template<typename T, ConnectionType C = ConnectionType::TCP>
		class StatedClientSocket
		{
		public:
			StatedClientSocket()
			{
				memset(&hints, 0, sizeof hints);
				hints.ai_family = AF_UNSPEC;
				if(C == ConnectionType::TCP)
				{
					hints.ai_socktype = SOCK_STREAM;
				}
				else
				{
					hints.ai_socktype = SOCK_DGRAM;
				}
				tv.tv_sec = 0;
				tv.tv_usec = 1000;
			}
			~StatedClientSocket()
			{
				freeaddrinfo(servinfo);
			}
			void Connect(const std::string& addr, int port)
			{
				struct addrinfo* p;
				if(port < 1 || port > 65535)
					throw SockExcept("Port out of range.");
				std::stringstream s;
				s << port;
				getaddrinfo(addr.c_str(), s.str().c_str(), &hints, &servinfo);

				for(p = servinfo; p != nullptr; p = p->ai_next) {
				    if ((Sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
				    	throw SockExcept("Could not open socket.");

				    if (connect(Sock, p->ai_addr, p->ai_addrlen) == -1) {
				        close(Sock);
				        continue;
				    }

				    break;
				}

				if(p == nullptr)
					throw SockExcept("Connection failure.");

				if(C == ConnectionType::TCP)
				{
					con.reset(new StatedConnection<T>(Sock, false, (sockaddr*)p->ai_addr));
				}
				else
				{
					con.reset(new StatedUDPConnection<T>(Sock,  (sockaddr*)p->ai_addr));
				}
			}

			void Reconnect()
			{
				struct addrinfo* p;

				if(con != nullptr)
				{
					throw SockExcept("Already connected.");
				}
				for(p = servinfo; p != nullptr; p = p->ai_next) {
				    if ((Sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
				    	throw SockExcept("Could not open socket.");

				    if (connect(Sock, p->ai_addr, p->ai_addrlen) == -1) {
				        close(Sock);
				        continue;
				    }

				    break;
				}

				if(p == nullptr)
					throw SockExcept("Connection failure.");

				if(C == ConnectionType::TCP)
				{
					con.reset(new StatedConnection<T>(Sock, false, (sockaddr*)p->ai_addr));
				}
				else
				{
					con.reset(new StatedUDPConnection<T>(Sock,  (sockaddr*)p->ai_addr));
				}
			}

			void HandleIO()
			{
				if(con == NULL)
				{
					throw SockExcept("No active connection.");
				}
				FD_ZERO(&InSet);
				FD_ZERO(&OutSet);
				FD_ZERO(&ExcSet);
				FD_SET( Sock, &InSet );
				FD_SET( Sock, &OutSet );
				FD_SET( Sock, &ExcSet );
				if( select( Sock+1, &InSet, &OutSet, &ExcSet, &tv ) == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
					throw SockExcept(std::string("HandleIO poll failure: ") + strerror(errno));
				if( FD_ISSET( Sock, &InSet ) )
				{
					if( con->Recv() == 0 && C == ConnectionType::TCP )
					{
						con.reset();
						throw SockExcept("Connection closed by foreign host");
					}
				}
				if(C == ConnectionType::UDP)
				{
					if(std::static_pointer_cast<StatedUDPConnection<T>>(con)->CheckClosed())
					{
						con.reset();
						throw SockExcept("Connection closed by foreign host");
					}
					std::static_pointer_cast<StatedUDPConnection<T>>(con)->SendKeepAlive();
				}
			}
			std::string GetData()
			{
				return con->GetData();
			}
			std::string GetLine()
			{
				return con->GetLine();
			}
			std::string GetPacket()
			{
				return con->GetPacket();
			}
			void Send(const std::string& str)
			{
				con->Send(str);
			}
			void Send(const std::string& str, FailType behavior)
			{
				if(C == ConnectionType::TCP)
				{
					throw SockExcept("TCP connections cannot specify failure type.");
				}
				std::static_pointer_cast<StatedUDPConnection<T>>(con)->Send(str, behavior);
			}
			void SetConnection(std::shared_ptr<Connection> c)
			{
				con = c;
				Sock = c->GetDescriptor();
			}
			std::weak_ptr<Connection> GetConnection()
			{
				return con;
			}
			void SetState(T s)
			{
				con->SetState(s);
			}
			T GetState()
			{
				return con->GetState();
			}
		protected:
			std::shared_ptr<Connection> con;
			int Sock;
			fd_set InSet, OutSet, ExcSet;
			struct addrinfo hints;
			struct addrinfo* servinfo;
			struct timeval tv;
		};

		template<typename T, ConnectionType C = ConnectionType::TCP>
		class StatedServerSocket
		{
		public:
			StatedServerSocket(bool b = false) :
				InSock(-1), InPort(-1), m_no_timeout(false), m_telnet(b)
			{
				memset(&hints, 0, sizeof hints);
				hints.ai_family = AF_UNSPEC;
				if(C == ConnectionType::TCP)
				{
					hints.ai_socktype = SOCK_STREAM;
				}
				else
				{
					hints.ai_socktype = SOCK_DGRAM;
				}
				hints.ai_flags = AI_PASSIVE;
				tv.tv_sec = 0;
				tv.tv_usec = 1000;
				telnet_tv.tv_sec = 0;
				telnet_tv.tv_usec = 0;
			}
			virtual ~StatedServerSocket()
			{
				freeaddrinfo(servinfo);
			}
			void setTelnetMode(bool b)
			{
				std::shared_ptr<StatedConnection<T>> c;
				unsigned int size = Connections.size();
				for (unsigned int i = 0; i < size; i++)
				{
					c = Connections[i];
					c->setTelnetMode(b);
				}
				m_telnet = b;
			}

			void setTimeout(int timeout)
			{
				if(timeout < 0)
				{
					m_no_timeout = true;
					return;
				}
				m_no_timeout = false;
				if(m_telnet)
				{
					telnet_tv.tv_sec = timeout/1000;
					telnet_tv.tv_usec = timeout%1000;
				}
				else
				{
					tv.tv_sec = timeout/1000;
					tv.tv_usec = (timeout%1000) * 1000;
				}
			}

			void listen(int port, bool block=false)
			{
				if (InSock != -1)
					throw SockExcept("Socket already open!");
				if (port < 1024 || port > 65535)
					throw SockExcept("Port out of range.");
				std::stringstream s;
				s << port;
				int status = getaddrinfo(nullptr, s.str().c_str(), &hints, &servinfo);
				if (status != 0)
					throw SockExcept(gai_strerror(status));
				if ((InSock = socket(servinfo->ai_family, servinfo->ai_socktype,
						servinfo->ai_protocol)) == -1)
					throw SockExcept("Could not open socket.");
				if(!block)
					fcntl(InSock, F_SETFL, O_NONBLOCK);
				int yes = 1;
				setsockopt(InSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
				if (::bind(InSock, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
					throw SockExcept("Port already in use.");
				::listen(InSock, 5);
			}
			void HandleIO()
			{
				struct sockaddr_storage addr;
				socklen_t addr_size = sizeof(addr);
				std::shared_ptr<StatedConnection<T>> c;
				struct timeval t;

				if(m_telnet)
					t = telnet_tv;
				else
					t = tv;

				FD_ZERO(&InSet);
				//FD_ZERO(&OutSet);
				FD_ZERO(&ExcSet);
				FD_SET(InSock, &InSet);
				int newcon, max = InSock;
				for (unsigned int i = 0; i < Descriptors.size(); i++)
				{
					if (Descriptors[i] > max)
						max = Descriptors[i];
					FD_SET( Descriptors[i], &InSet );
					//FD_SET( Descriptors[i], &OutSet );
					FD_SET( Descriptors[i], &ExcSet );
				}
				if (select(max + 1, &InSet, NULL, &ExcSet, &t) == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
					throw SockExcept(std::string("HandleIO poll failure: ") + strerror(errno));
				if (FD_ISSET( InSock, &ExcSet ))
				{
					std::cerr << "Exception on listen port " << InSock << "."
							<< std::endl;
					FD_CLR( InSock, &InSet );
					//FD_CLR( InSock, &OutSet );
				}
				else if(FD_ISSET( InSock, &InSet ))
				{
					if (C == ConnectionType::TCP)
					{
						if((newcon = accept(InSock, (struct sockaddr *)& addr, &addr_size)) != -1)
						{
							CloseMutex.lock();
							c.reset(new StatedConnection<T> (newcon, m_telnet, ((sockaddr*)&addr)));
							Connections.push_back(c);
							NewConnections.push_back(c);
							Descriptors.push_back(newcon);
							CloseMutex.unlock();
						}
						else if(errno != EWOULDBLOCK && errno != EAGAIN)
							throw SockExcept("Error accepting new connection.");
					}
					else
					{
						bool bFound = false;
						for (auto it = Connections.begin(); it != Connections.end(); it++)
						{
							if((*it)->Recv() != -2)
							{
								bFound = true;
							}
						}
						if(!bFound)
						{
							CloseMutex.lock();
							c.reset(new StatedUDPConnection<T> (InSock, this));
							Connections.push_back(c);
							NewConnections.push_back(c);
							CloseMutex.unlock();
							c->Recv();
						}
					}
				}
				if(C == ConnectionType::TCP)
				{
					for (unsigned int i = 0; i < Connections.size(); i++)
					{
						if (FD_ISSET( Descriptors[i], &InSet ))
						{
							if(Connections[i]->Recv() == 0 && C == ConnectionType::TCP)
							{
								CloseMutex.lock();
								c = Connections[i];
								ClosedConnections.push_back(Connections[i]->GetDescriptor());
								for (unsigned int j = 0; j < NewConnections.size(); j++)
								{
									if (!NewConnections[j].lock() || NewConnections[j].lock() == Connections[i])
									{
										NewConnections.erase(NewConnections.begin() + j);
										j--;
									}
								}
								Connections.erase(Connections.begin() + i);
								Descriptors.erase(Descriptors.begin() + i);
								i--;
								CloseMutex.unlock();
								continue;
							}
						}
					}
				}
				else
				{
					for (unsigned int i = 0; i < Connections.size(); i++)
					{
						c = Connections[i];
						if(std::static_pointer_cast<StatedUDPConnection<T>>(Connections[i])->CheckClosed())
						{
							CloseMutex.lock();
							ClosedConnections.push_back(c->GetPort());
							for (unsigned int j = 0; j < NewConnections.size(); j++)
							{
								if (!NewConnections[j].lock() || NewConnections[j].lock() == Connections[i])
								{
									NewConnections.erase(NewConnections.begin() + j);
									j--;
								}
							}
							Connections.erase(Connections.begin() + i);
							i--;
							CloseMutex.unlock();
							continue;
						}
						std::static_pointer_cast<StatedUDPConnection<T>>(c)->SendKeepAlive();
					}
				}
			}
			/* Return all currently active connections */
			std::vector<std::weak_ptr<StatedConnection<T>>> GetConnections()
			{
				std::vector<std::weak_ptr<StatedConnection<T>>> ret;
				for( auto& connection : Connections )
				{
					ret.push_back(connection);
				}
				return ret;
			}
			std::weak_ptr<StatedConnection<T>> GetConnection(int i)
			{
				return Connections[i];
			}

			int GetNumConnections()
			{
				return Connections.size();
			}

			/* Return all connections that have been opened since the last time this was called */
			std::vector<std::weak_ptr<StatedConnection<T>>> GetNew()
			{
				std::vector<std::weak_ptr<StatedConnection<T>>> ret = std::move( NewConnections );
				return ret;
			}

			void CloseConnection(int i)
			{
				std::shared_ptr<StatedConnection<T>> c = Connections[i];
				close(c->GetDescriptor());
				for (unsigned int j = 0; j < NewConnections.size(); j++)
				{
					if (!NewConnections[j].lock() || NewConnections[j].lock() == Connections[i])
						NewConnections.erase(NewConnections.begin() + j);
				}
				Connections.erase(Connections.begin() + i);
				if(C == ConnectionType::TCP)
				{
					Descriptors.erase(Descriptors.begin() + i);
				}
			}

			void CloseConnection(std::shared_ptr<StatedConnection<T>> c)
			{
				if(!c)
				{
					return;
				}

				CloseMutex.lock();
				for (unsigned int i = 0; i < Connections.size(); i++)
				{
					if(Connections[i] == c)
					{
						Connections.erase(Connections.begin() + i);
						if(C == ConnectionType::TCP)
						{
							close(c->GetDescriptor());
							Descriptors.erase(Descriptors.begin() + i);
						}
					}
				}
				for (unsigned int i = 0; i < NewConnections.size(); i++)
				{
					if (!NewConnections[i].lock() || NewConnections[i].lock() == c)
						NewConnections.erase(NewConnections.begin() + i);
				}
				CloseMutex.unlock();
			}

			std::weak_ptr<StatedConnection<T>> GetConnectionByDesc(int d)
			{
				for (unsigned int i = 0; i < Connections.size(); i++)
				{
					if (Connections[i]->GetDescriptor() == d)
						return Connections[i];
				}
				return std::weak_ptr<StatedConnection<T>>();
			}

			std::weak_ptr<StatedConnection<T>> GetConnectionByPort(int d)
			{
				for (unsigned int i = 0; i < Connections.size(); i++)
				{
					if (Connections[i]->GetPort() == d)
						return Connections[i];
				}
				return std::weak_ptr<StatedConnection<T>>();
			}

			/* Return a list of descriptors that have received close signatures from the client since the last time this was called */
			std::vector<int> GetClosed()
			{
				std::vector<int> ret = ClosedConnections;
				ClosedConnections.clear();
				return ret;
			}
		protected:
			int InSock, InPort;

			//To make some of the code simpler, I'm storing both a list of connection objects *and*
			//a list of actual file descriptors. Connections[i] should always have the same descriptor as
			//Descriptors[i].
			std::vector<std::shared_ptr<StatedConnection<T>>> Connections;
			std::vector<std::weak_ptr<StatedConnection<T>>> NewConnections;
			std::vector<int> ClosedConnections;
			std::vector<int> Descriptors;
			std::vector<int> CloseQueue;

			fd_set InSet, ExcSet;
			struct addrinfo hints;
			struct addrinfo* servinfo;
			struct timeval tv;
			struct timeval telnet_tv;
			bool m_no_timeout;
			bool m_telnet;

			boost::mutex CloseMutex;

			friend class StatedUDPConnection<T>;
			boost::mutex recvlock;
		};

		typedef StatedClientSocket<int, ConnectionType::TCP> ClientSocket;
		typedef StatedClientSocket<int, ConnectionType::UDP> UDPClientSocket;
		typedef StatedServerSocket<int, ConnectionType::TCP> ServerSocket;
		typedef StatedServerSocket<int, ConnectionType::UDP> UDPServerSocket;
	}
}
