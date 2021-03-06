#include "TcpChatClient.h"

#include <iostream>
#include<boost/bind.hpp>

#include "PlatformDefine.h"
#include "../AsyncTestChattingServer/Protocol.h"
#include "Logger.h"

TcpChatClient::TcpChatClient(boost::asio::io_service& ios,
	const boost::asio::ip::tcp::endpoint&endpoint)
	:io_service_(ios), socket_(ios), endpoint_(endpoint)
{		
	packetBufferMark_ = 0;
	isLogin_ = false;	
	isConnectedServer_ = false;

	debugSendCount_ = 0;
}

void TcpChatClient::Connect()
{		
	socket_.async_connect(endpoint_, [this](boost::system::error_code errorcode)->void
	{
		if (errorcode)
		{			
			isConnectedServer_ = false;
			Logger::Log(Logger::LogType::Normal, errorcode.message());
		}
		else
		{			
			isConnectedServer_ = true;
			Logger::Log(Logger::LogType::Normal, "connected");
			
			std::cout << "succeed connect server" << std::endl;
			std::cout << "Enter Nickname" << std::endl;
			
			PostReceive();
		}
	});
}

void TcpChatClient::Close()
{
	if (socket_.is_open())
	{
		socket_.close();
	}
}

//call by main thread. so. important to managing share resource.
void TcpChatClient::PostSend(const int packetSize, char* pPacket)
{	
	std::cout << "call : PostSend : " << ++debugSendCount_ << std::endl;			
	//std::lock_guard<std::recursive_mutex> guard(mutex_);
	std::unique_lock<std::recursive_mutex> guard(mutex_);


	char* pSendpacket = new char[packetSize];
	memcpy(pSendpacket, pPacket, packetSize);

	sendPacketQ_.push_back(pSendpacket);	


	boost::asio::async_write(socket_,
		boost::asio::buffer(pSendpacket, packetSize),
		boost::bind(&TcpChatClient::HandleWrite, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));		
	
}

void TcpChatClient::PostReceive()
{	
	memset(receiveBuffer_.data(), 0, receiveBuffer_.size());	

	socket_.async_read_some(boost::asio::buffer(receiveBuffer_),
		boost::bind(&TcpChatClient::HandleRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

TcpChatClient::~TcpChatClient()
{	
	//std::lock_guard<std::recursive_mutex> guard(mutex_);
	std::unique_lock<std::recursive_mutex> guard(mutex_);
	while (!sendPacketQ_.empty())
	{
		delete[] sendPacketQ_.front();
		sendPacketQ_.pop_front();
	}			
}


//call by walker thread. so. important to managing share resource.
void TcpChatClient::HandleWrite(boost::system::error_code error_code, size_t bytes_transferred)
{
	//std::lock_guard<std::recursive_mutex> guard(mutex_);
	std::unique_lock<std::recursive_mutex> guard(mutex_);

	if (error_code)
	{
		if (error_code == boost::asio::error::eof)
		{
			isLogin_ = false;
		}
	}
	else
	{		
		delete[] sendPacketQ_.front();
		sendPacketQ_.pop_front();

		if (!sendPacketQ_.empty())
		{
			char* pPakcet = sendPacketQ_.front();
			PacketHeader* pHeader = reinterpret_cast<PacketHeader*>(pPakcet);
			PostSend(pHeader);
		}		
	}	
}

void TcpChatClient::HandleRead(boost::system::error_code error_code, size_t bytes_transferred)
{
	if (error_code)
	{
		if (error_code == boost::asio::error::eof)
		{
			std::cout << error_code.message() << std::endl;
			std::cout << "disconnected client" << std::endl;

			isLogin_ = false;
		}
		else
		{
			std::cout << "error id : " << error_code.value() << " error message : " << error_code.message() << std::endl;
		}		
	}
	else
	{
		//server can receive the packet more than once at time.	so..It stores the received data in a buffer. And decompose.
		memcpy(&packetBuffer[packetBufferMark_], receiveBuffer_.data(), bytes_transferred);


		size_t packetTotalDataSize = packetBufferMark_ + bytes_transferred;
		int readDataSize = 0;

		while (packetTotalDataSize > 0)
		{
			if (packetTotalDataSize < sizeof(PacketHeader))
			{
				break;
			}

			PacketHeader* pHeader = reinterpret_cast<PacketHeader*>(&packetBuffer[readDataSize]);
			if (pHeader)
			{
				if (pHeader->Size <= packetTotalDataSize)
				{
					ProcessPacket(&packetBuffer[readDataSize]);

					packetTotalDataSize -= pHeader->Size;
					readDataSize += pHeader->Size;
				}
				else
				{
					break;
				}
			}
		}

		if (packetTotalDataSize > 0)
		{			
			char tempBuff[MAX_RECEIVE_BUFFER_SIZE] = { 0, };
			memcpy(&tempBuff[0], &packetBuffer[readDataSize], packetTotalDataSize);
			memcpy(&packetBuffer[0], &tempBuff[0], packetTotalDataSize);
		}

		packetBufferMark_ = packetTotalDataSize;

		PostReceive();
	}
}


void TcpChatClient::ProcessPacket(const char* pData)
{
	const PacketHeader* pPacket = reinterpret_cast<const PacketHeader*>(pData);
	if (pPacket == nullptr)
		return;

	switch (pPacket->Id)
	{
		case PacketCode::LoginRes: 
		{
			const PacketLoginRes* pLoginRes = reinterpret_cast<const PacketLoginRes*>(pPacket);
			if (pLoginRes->isSuccess)
			{
				std::cout << "Login Success" << std::endl;
				isLogin_ = true;
			}
			else
			{
				std::cout << "Login Fail" << std::endl;
				isLogin_ = false;
			}
		}
		break;

		case PacketCode::ChatRes: 
		{
			const PacketChatRes* pLoginRes = reinterpret_cast<const PacketChatRes*>(pPacket);
			if (!pLoginRes->isSuccess)
			{
				std::cout << "fail to send" << std::endl;
			}
			else
			{
				std::cout << pLoginRes->NickName << ":" << pLoginRes->Message << std::endl;
			}
		}
		break;

		case PacketCode::ChatNfy:
		{
			const PacketChatNfy* pLoginNfy = reinterpret_cast<const PacketChatNfy*>(pPacket);
						
			std::cout << pLoginNfy->NickName  << ":" << pLoginNfy->Message << std::endl;
		}
		break;
	}
}