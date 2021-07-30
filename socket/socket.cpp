#include "socket.hpp"

namespace socks {

	/**
	* Socket constructor.
	* 
	* @param sock_type Communication protocol i.e. UDP/TCP.
	* @param ip_type IPV4/IPV6.
	* 
	*/
	socket::socket(const sock_type& sock_type, const ip_type& ip_type) {
		WSADATA wsaData;
		int result;
		s_type = sock_type;
		i_type = ip_type;

		// Initialize Winsock.
		// Winsock will increment a counter if initialized more than once.
		// WSACleanup will check the counter, and cleanup when it gets to 0.
		result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0) {
			int err = WSAGetLastError();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}

		ZeroMemory(&hints, sizeof(hints));

		// Checking IP family.
		switch (i_type) {
			case ip_type::V6:
				hints.ai_family = AF_INET6;
				break;
			default:
				hints.ai_family = AF_INET;
				break;
		}

		hints.ai_socktype = (s_type == sock_type::TCP) ? SOCK_STREAM : SOCK_DGRAM;
		hints.ai_protocol = (s_type == sock_type::TCP) ? IPPROTO_TCP : IPPROTO_UDP;

		sock = ::socket(hints.ai_family,      // IP family.
						hints.ai_socktype,    // Socket type.
						hints.ai_protocol     // Socket protocol.
				);

		if (sock == INVALID_SOCKET) {
			int err = WSAGetLastError();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}
	}

	/**
	* Connect socket to a given IP and port.
	* 
	* @param connection_string An std::pair where the first element is the IP
	* and the second element is the port.
	* 
	*/
	void socket::connect(const std::pair<std::string, int>& connection_string) {
		std::string ip = connection_string.first;
		int port = connection_string.second;

		struct addrinfo* result = NULL;

		int addr_result = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);
		if (addr_result != 0) {
			freeaddrinfo(result);
			int err = WSAGetLastError();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}

		struct addrinfo* runner;
		bool success = false;

		// Testing all addresses returned by getaddrinfo until one connects.
		for (runner = result; runner; runner = runner->ai_next) {
			if (::connect(sock, runner->ai_addr, (int)runner->ai_addrlen) != SOCKET_ERROR) {
				success = true;
				break;
			}
		}

		freeaddrinfo(result);
		
		if (!success) {
			close();
			int err = WSAGetLastError();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}
	}
	
	void socket::bind(const std::pair<std::string, int>& connection_string) {
		std::string ip = connection_string.first;
		int port = connection_string.second;

		struct addrinfo* result = NULL;

		int addr_result = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);
		if (addr_result != 0) {
			freeaddrinfo(result);
			int err = WSAGetLastError();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}

		struct addrinfo* runner;
		bool success = false;

		// Testing all addresses returned by getaddrinfo until one binds.
		for (runner = result; runner; runner = runner->ai_next) {
			if (::bind(sock, runner->ai_addr, (int)runner->ai_addrlen) != SOCKET_ERROR) {
				success = true;
				break;
			}
			else {
				int err = WSAGetLastError();
				std::cout << "[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err) << '\n';
			}
		}

		freeaddrinfo(result);

		if (!success) {
			close();
			int err = WSAGetLastError();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}
	}

	/**
	* Receive information from socket.
	* 
	* @param buff_size Size of buffer to receive data into.
	* @param flags Flags supplied to WinAPI recv function. Default value is 0.
	* 
	* @return unique_ptr pointing to data received from socket.
	*
	*/
	std::unique_ptr<char[]> socket::recv(const unsigned int& buff_size, const int& flags) {
		std::unique_ptr<char[]> buff = std::make_unique<char[]>(buff_size);
		::recv(sock, buff.get(), buff_size, NULL);
		return buff;
	}

	/**
	* Send information through socket.
	* 
	* @param buff The data to send through socket.
	* @param len The length of the supplied data.
	* @param flags Flags supplied to WinAPI send function. Default value is 0.
	* 
	* @return number of bytes sent.
	* 
	*/
	int socket::send(const char* buff, const unsigned int& len, const int& flags) {
		return ::send(sock, buff, len, flags);
	}

	/**
	* Close the current socket.
	* 
	* WSACleanup will also be called if the WinAPI internal counter gets to 0.
	* 
	*/
	void socket::close() {
		closesocket(sock);
		WSACleanup();
	}

	/**
	* Get WinSock error message from error code.
	* 
	* @param err_code Error code to get message from.
	* 
	* @return std::string containing error message or an empty string if no message can be retrieved.
	* 
	*/
	std::string socket::get_winsock_error(const int& err_code) {
		char* msg = NULL;
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err_code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&msg,
			0,
			NULL
		);
		if (msg) {
			std::string ret(msg);
			LocalFree(msg);
			return ret;
		}
		return {};
	}
}