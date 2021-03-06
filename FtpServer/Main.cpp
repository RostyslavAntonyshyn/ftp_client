// FTP-Server

#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

using boost::asio::ip::tcp;

const int max_length = 1030;

struct Executor
{
	static void ChangeDir(tcp::socket &sock)
	{
		boost::system::error_code error;
		std::vector<char> data(max_length);
		size_t length = sock.read_some(boost::asio::buffer(data), error);
		data[length] = '\0';

		std::cout << "Dispathed change dir" << std::endl;
		std::cout << &data[0] << std::endl;
	}

	static void DownloadFile(tcp::socket &sock)
	{
		boost::system::error_code error;
		std::vector<char> data(max_length);
		size_t length = sock.read_some(boost::asio::buffer(data), error);
		data[length] = '\0';

		std::cout << "Dispathed download file" << std::endl;
		std::cout << &data[0] << std::endl;
	}

	static void UploadFile(tcp::socket &sock)
	{
		boost::system::error_code error;
		std::vector<char> directory(max_length);

		size_t length = sock.read_some(boost::asio::buffer(directory), error);

		std::string path(&directory[4]);
		boost::filesystem::path filePath(path);
		if (exists(filePath))
		{
			std::cout << "Dispatched directory path: " << filePath << std::endl;
		}
		else
		{
			std::cout << "Wrong directory! Change directory please." << std::endl;
			return;
		}

		int fileSize = file_size(filePath);

		std::vector<char> data(fileSize);
		size_t length1 = sock.read_some(boost::asio::buffer(data), error);
		
		std::cout << "Dispathed upload file: Uploading..." << std::endl;

		std::ofstream file("UploadFile", std::ios::binary);

		if (file.write(&data[0], length1))
		{
			std::cout << "Upload successfull!" << std::endl;
		}
		else std::cout << "Upload error! " << std::endl;
		
	}	
};

struct Dispather
{
	Dispather(tcp::socket &socket) : _socket(socket)
	{
	}
	void Dispatch(short op)
	{
		if (op < 0 || op > 2)
		{
			std::cout << "Unknown operation" << std::endl;
			return;
		}

		_operations[static_cast<MessageT>(op)](_socket);
	}

	enum class MessageT { DownloadFile = 0, ChangeDir, UploadFile };
	typedef std::function<void(tcp::socket&)> Action;
	static std::map < MessageT, Action > _operations;

	tcp::socket &_socket;
};

std::map<Dispather::MessageT, Dispather::Action> Dispather::_operations { 
	{ MessageT::ChangeDir, Executor::ChangeDir },
	{ MessageT::DownloadFile, Executor::DownloadFile },
	{ MessageT::UploadFile, Executor::UploadFile }
};

void session(tcp::socket sock)
{
	try
	{
		auto dispatcher = std::make_unique<Dispather>(sock);
		for (;;)
		{
			char data[2];

			boost::system::error_code error;
			size_t length = sock.read_some(boost::asio::buffer(data), error);
			if (error == boost::asio::error::eof)
				break; // Connection closed cleanly by peer.
			else if (error)
				throw boost::system::system_error(error); // Some other error.

			
			short op = *reinterpret_cast<short*>(data);
			dispatcher->Dispatch(op);
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception in thread: " << e.what() << "\n";
	}
}

void server(boost::asio::io_service& io_service, unsigned short port)
{
	tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
	for (;;)
	{
		tcp::socket sock(io_service);
		a.accept(sock);
		std::thread(session, std::move(sock)).detach();

		std::cout << "Session started on port: " << port << std::endl << std::endl;
	}
}

int main(int argc, char* argv[])
{
	try
	{
		boost::asio::io_service io_service;
		server(io_service, 1488);
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	system("pause");
	return 0;
}