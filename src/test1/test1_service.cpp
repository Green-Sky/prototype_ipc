#include "./service_interface.hpp"

#include <zed_net.h>

#include <atomic>
#include <iostream>
#include <type_traits>
#include <variant>
#include <charconv>
#include <cassert>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** args) {
	using namespace zpp::bits::literals;

	if (argc < 2) {
		std::cerr << "S: missing argument: port of host\n";
		return -1;
	}

	uint16_t port {0};

	std::string_view port_sv{args[1]};
	auto [ptr, ec] = std::from_chars(port_sv.data(), port_sv.data()+port_sv.size(), port);
	assert(ec == std::errc());

	// open socket
	if (zed_net_init() != 0) {
		std::cerr << "S: zed_net_init failed: " << zed_net_get_error() << "\n";
		return -1;
	}

	std::cout << "S: initialized zed_net\n";

	// connecting back to host
	zed_net_socket_t socket;
	if (zed_net_tcp_socket_open(&socket, 0, 0, 0) != 0) {
		std::cerr << "S: zed_net_tcp_socket_open failed: " << zed_net_get_error() << "\n";
		zed_net_shutdown();
		return -1;
	}

	zed_net_address_t host_addr;
	if (zed_net_get_address(&host_addr, "127.0.0.1", port) != 0) {
		std::cerr << "S: zed_net_get_address failed: " << zed_net_get_error() << "\n";
		zed_net_socket_close(&socket);
		zed_net_shutdown();
		return -1;
	}
	zed_net_tcp_connect(&socket, host_addr);

	ServiceInterface service;

	// 100MiB
	auto buffer = std::make_unique<std::array<uint8_t, 1024*1024*100>>();

	std::atomic_bool quit {false};

	while (!quit) {
		// we start our loop by listening
		int64_t bytes_received {0};
		bytes_received = zed_net_tcp_socket_receive(&socket, buffer->data(), buffer->size());
		if (bytes_received < 0) {
			quit = true;
			std::cerr << "S: zed_net_tcp_socket_receive failed: " << zed_net_get_error() << "\n";
			zed_net_socket_close(&socket);
			zed_net_shutdown();
			return -1;
		} else if (bytes_received == 0) {
			std::cout << "S: got 0 bytes?\n";
			break; // connection closed
		}

		std::cout << "S: got " << bytes_received << " bytes\n";

		zpp::bits::in in{std::span{buffer->data(), static_cast<size_t>(bytes_received)}};

		auto [data_out, out] = zpp::bits::data_out();

		ServiceInterface::rpc::server rpc_service_server{in, out, service};

		rpc_service_server.serve().or_throw();

		// send data back
		if (zed_net_tcp_socket_send(&socket, data_out.data(), data_out.size()) != 0) {
			quit = true;
			std::cerr << "S: zed_net_tcp_socket_send failed: " << zed_net_get_error() << "\n";
			zed_net_socket_close(&socket);
			zed_net_shutdown();
			return -1;
		}
	}

	zed_net_socket_close(&socket);
	zed_net_shutdown();

	return 0;
}

std::optional<ServiceInterface::Command> ServiceInterface::poll(void) {
	std::cout << "S: called poll\n";
	return TextMessage{"A", "haha"};
}

void ServiceInterface::push(const Command& cmd) {
	std::cout << "S: got ";

	std::visit([](auto&& arg) {
		std::cout << arg.user;

		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, TextMessage>) {
			std::cout << " '" << arg.msg << "'\n";
		} else if constexpr (std::is_same_v<T, UserJoined>) {
			std::cout << " joined\n";
		} else if constexpr (std::is_same_v<T, UserLeft>) {
			std::cout << " left\n";
		}
	}, cmd);
}

