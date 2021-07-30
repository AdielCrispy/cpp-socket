#include "socket.hpp"
#include <iostream>

int main() {
	socks::socket s(sock_type::TCP, ip_type::UNSPEC);
	s.connect({ "google.com", "80" });
	s.send("HELLO", 5);
	std::unique_ptr<char[]> b = s.recv(1024);
	if (b) {
		std::cout << b << '\n';
	}
	else {
		std::cout << "Pointer is invalid!\n";
	}
	s.close();

	return 0;
}