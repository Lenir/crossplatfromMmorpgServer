// SyncTestEchoClient.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include "boost/asio.hpp"

using boost::asio::ip::tcp;

const std::string SERVER_IP = "127.0.0.1";
const unsigned short SERVER_PORT = 7777;

int main()
{	
	boost::asio::io_service ios;
			
	tcp::endpoint endpoint(boost::asio::ip::address::from_string(SERVER_IP), SERVER_PORT);	
	tcp::socket socket(ios);

	boost::system::error_code error_code;
	socket.connect(endpoint, error_code);

	if (error_code)
	{
		std::cout << "failed to connect : " << error_code.message() << error_code.value() << std::endl;
		//10061 : ���� �� ���־ ���� �ȵǴ� ��� ��� : ��� ��ǻ�Ϳ��� ������ �ź��������� �������� ���߽��ϴ�		
		getchar();
	}
	else
	{
		std::cout << "succed to connect" << std::endl;
	}

		
	for (;;)
	{
		std::string msg;
		std::cin >> msg;

		socket.write_some(boost::asio::buffer(msg), error_code);
		if (error_code)
		{
			if (error_code == boost::asio::error::eof)
			{
				std::cout << error_code.message() << std::endl;
				std::cout << "disconnected" << std::endl;
				//10054 : ������ ����Ǿ��, ������ ���⳪ �ٿ�Ȱ��, 
				//r/w�� �Ҽ� ��� " ���� ������ ���� ȣ��Ʈ�� ���� ������ ������ϴ�. �޽����� ���Ե�"
			}
			else
			{
				std::cout << "msg id : " << error_code.value() << " error message : " << error_code.message() << std::endl;
			}
		}
		
		std::array<char, 128> readBuff;
		readBuff.assign(0);		
		socket.read_some(boost::asio::buffer(readBuff), error_code);

		if (error_code)
		{
			if (error_code == boost::asio::error::eof)
			{
				std::cout << error_code.message() << std::endl;
				std::cout << "disconnected" << std::endl;
			}
			else
			{
				std::cout << "msg id : " << error_code.value() << " error message : " << error_code.message() << std::endl;
			}
		}

		std::cout << "Msg : " << readBuff.data() << std::endl;;


		if (socket.is_open())
		{
			socket.close();
		}

	}
    return 0;
}

