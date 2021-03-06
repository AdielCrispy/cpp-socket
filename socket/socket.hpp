#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <exception>
#include <utility>
#include <string>
#include <memory>
#include <iostream>
#include <string>
#include <tuple>

#pragma comment(lib, "Ws2_32.lib")

// 🧦
namespace socks{

	enum class sock_type { TCP, UDP };
	enum class ip_type { V4, V6 };

	struct sock_addr_info {
		char addr[INET6_ADDRSTRLEN];
		uint16_t port;
	};

	class socket {
	public:
		socket(const sock_type& sock_type = sock_type::TCP, const ip_type& ip_type = ip_type::V4, SOCKET copy_sock = INVALID_SOCKET);
		void connect(const std::pair<std::string, int>& connection_string);
		void bind(const std::pair<std::string, int>& connection_string);
		void listen(const unsigned int& max_backlog = SOMAXCONN);
		std::pair<socket, sock_addr_info> accept();
		std::pair<std::unique_ptr<char[]>, unsigned int> recv(const unsigned int& buff_size, const int& flags = 0);
		std::tuple<std::unique_ptr<char[]>, unsigned int, sock_addr_info> recvfrom(const unsigned int& buff_size, const int& flags = 0);
		int send(const char* buff, const unsigned int& len, const int& flags = 0);
		int sendto(const char* buff, const unsigned int& len, const sock_addr_info& addr, const int& flags = 0);
		void close();
		operator SOCKET const();

	private:
		sock_type s_type;
		ip_type i_type;
		SOCKET sock;
		struct addrinfo hints;

		std::string get_winsock_error(const int& err_code);

	};
}