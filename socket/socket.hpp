#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <exception>
#include <stdio.h>
#include <utility>
#include <string>
#include <memory>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

enum class sock_type { TCP, UDP };
enum class ip_type { V4, V6, UNSPEC };

namespace socks{
	class socket {
	public:
		socket(const sock_type& sock_type = sock_type::TCP, const ip_type& ip_type = ip_type::V4);
		void connect(const std::pair<std::string, int>& connection_string);
		std::unique_ptr<char[]> recv(const unsigned int& buff_size, const int& flags = 0);
		int send(const char* buff, const unsigned int& len, const int& flags = 0);
		void close();

	private:
		sock_type s_type;
		ip_type i_type;
		SOCKET sock;

		std::string get_winsock_error(const int& err_code);

	};
}