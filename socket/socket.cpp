#include "socket.hpp"

namespace socks {

	/**
	* Socket constructor.
	* 
	* @param sock_type Communication protocol i.e. UDP/TCP.
	* @param ip_type IPV4/IPV6.
	* @param copy_sock Optional socket to use instead of creating a new one.
	* 
	*/
	socket::socket(const sock_type& sock_type, const ip_type& ip_type, SOCKET copy_sock) {
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

		if (copy_sock == INVALID_SOCKET) {
			sock = ::socket(hints.ai_family,      // IP family.
				hints.ai_socktype,                // Socket type.
				hints.ai_protocol                 // Socket protocol.
			);

			if (sock == INVALID_SOCKET) {
				int err = WSAGetLastError();
				throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
			}
		}
		else {
			sock = copy_sock;
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
			int err = WSAGetLastError();
			freeaddrinfo(result);
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
			int err = WSAGetLastError();
			close();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}
	}
	
	/**
	* 
	* Bind a server socket to a given host and port.
	* 
	* @param connection_string An std::pair where the first element is the IP
	* and the second element is the port.
	* 
	*/
	void socket::bind(const std::pair<std::string, int>& connection_string) {
		std::string ip = connection_string.first;
		int port = connection_string.second;

		struct addrinfo* result = NULL;

		int addr_result = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);
		if (addr_result != 0) {
			int err = WSAGetLastError();
			freeaddrinfo(result);
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
			int err = WSAGetLastError();
			close();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}
	}

	/**
	* 
	* Setup server socket to listen after bind. Set maximum backlog.
	* 
	* @param max_backlog The maximum number of connections to keep in connection queue.
	* 
	*/
	void socket::listen(const unsigned int& max_backlog) {
		if (::listen(sock, max_backlog) == SOCKET_ERROR) {
			int err = WSAGetLastError();
			close();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}
	}

	/**
	* 
	* Accept a new connection.
	* 
	* @return std::pair containing client's socket object, and information about client's address.
	* 
	*/
	std::pair<socket, sock_addr_info> socket::accept() {
		SOCKET client_sock = INVALID_SOCKET;
		struct sockaddr client_addr;
		int addr_size = sizeof(client_addr);
		sock_addr_info client_addr_info;

		memset(&client_addr_info, NULL, sizeof(sock_addr_info));

		// Accept a client socket.
		client_sock = ::accept(sock, &client_addr, &addr_size);
		if (client_sock == INVALID_SOCKET) {
			int err = WSAGetLastError();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}
		
		if (addr_size != 0) {
			if (client_addr.sa_family == AF_INET) {
				struct sockaddr_in* client_info_ipv4 = (struct sockaddr_in*)&client_addr;
				inet_ntop(AF_INET, &client_info_ipv4->sin_addr, client_addr_info.addr, INET_ADDRSTRLEN);
				client_addr_info.port = ntohs(client_info_ipv4->sin_port);
			}
			else if (client_addr.sa_family == AF_INET6) {
				struct sockaddr_in6* client_info_ipv6 = (struct sockaddr_in6*)&client_addr;
				inet_ntop(AF_INET6, &client_info_ipv6->sin6_addr, client_addr_info.addr, INET6_ADDRSTRLEN);
				client_addr_info.port = ntohs(client_info_ipv6->sin6_port);
			}
		}

		// Building client socket from WinAPI socket.
		return { 
				socket(hints.ai_protocol == IPPROTO_TCP ? sock_type::TCP : sock_type::UDP,
				hints.ai_family == AF_INET ? ip_type::V4 : ip_type::V6,
				client_sock),
				client_addr_info
		};
	}

	/**
	* Receive information from socket.
	* 
	* @param buff_size Size of buffer to receive data into.
	* @param flags Flags supplied to WinAPI recv function. Default value is 0.
	* 
	* @return std::pair containing unique_ptr pointing to data received from socket, and the amount of bytes received.
	*
	*/
	std::pair<std::unique_ptr<char[]>, unsigned int> socket::recv(const unsigned int& buff_size, const int& flags) {
		std::unique_ptr<char[]> buff = std::make_unique<char[]>(buff_size);
		unsigned int bytes_received = ::recv(sock, buff.get(), buff_size, NULL);
		return { std::move(buff), bytes_received };
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
		int bytes_sent = 0;
		if ((bytes_sent = ::send(sock, buff, len, flags)) == SOCKET_ERROR) {
			int err = WSAGetLastError();
			throw std::runtime_error("[WinError " + std::to_string(err) + "]" + ": " + get_winsock_error(err));
		}
		return bytes_sent;
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
	* 
	* Implicit conversion operator between WinAPI socket and socks::socket.
	* 
	* @return socket handle stored in socket object.
	* 
	*/
	socket::operator SOCKET const() {
		return sock;
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