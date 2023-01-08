#include "./service_interface.hpp"

#include <zed_net.h>

#include <span>
#include <filesystem>
#include <atomic>
#include <thread>
#include <iostream>
#include <cstdlib>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** args) {
	using namespace zpp::bits::literals;
	using std::chrono::operator""ms;

	// open socket, save port
	if (zed_net_init() != 0) {
		std::cerr << "H: zed_net_init failed: " << zed_net_get_error() << "\n";
		return -1;
	}

	std::cout << "H: initialized zed_net\n";

	const uint16_t port_start {1337};
	const uint16_t port_end {port_start+100};
	uint16_t port = port_start;
	zed_net_socket_t listen_socket;
	bool found_free_port {false};
	for (; port <= port_end; port++) {
		if (zed_net_tcp_socket_open(
			&listen_socket,
			port, // port
			0, // non blocking
			1 // listen
		) == 0) {
			found_free_port = true;
			break;
		}
	}

	if (!found_free_port) {
		std::cerr << "H: zed_net_tcp_socket_open failed: " << zed_net_get_error() << "\n";
		zed_net_shutdown();
		return -1;
	}

	std::cout << "H: listening on " << port << "\n";

	// TODO: config this path
	auto service_exe_path {std::filesystem::current_path().string() + "/bin/test1_service"};
	std::string command {service_exe_path + " " + std::to_string(port)};
	std::atomic_bool thread_has_quit {false};

	// start service with port (and secret?) as arg
	std::thread call_thread([command, &thread_has_quit](void) {
		// TODO: put in loop AND report repeated crashes or whatever
		std::system(command.c_str());

		thread_has_quit = true;
	});

	std::cout << "H: started service\n";

	// wait for service to connect back
	zed_net_socket_t remote_socket;
	zed_net_address_t remote_address;
	// TODO: timeout
	if (zed_net_tcp_accept(&listen_socket, &remote_socket, &remote_address) != 0) {
		std::cerr << "H: zed_net_tcp_accept failed: " << zed_net_get_error() << "\n";
		zed_net_socket_close(&listen_socket);
		zed_net_shutdown();
		call_thread.join();
		return -1;
	}

	// TODO: check and allow local only connections or change zed_net api

	std::cout << "H: got connection from " << zed_net_host_to_str(remote_address.host) << ":" << remote_address.port << "\n";

	std::atomic_bool quit {false};

	// 100MiB
	auto buffer = std::make_unique<std::array<uint8_t, 1024*1024*100>>();

	while (!quit) {
		zpp::bits::in in{std::span{buffer->data(), 0}}; // tmp, rpc needs it
		auto [data_out, out] = zpp::bits::data_out();

		ServiceInterface::rpc::client rpc_service_client{in, out}; // ephimeral, should be cheap

		// start poll
		rpc_service_client.request<"ServiceInterface::poll"_sha256_int>().or_throw();
		if (zed_net_tcp_socket_send(&remote_socket, data_out.data(), data_out.size()) != 0) {
			quit = true;
			std::cerr << "H: zed_net_tcp_socket_send failed: " << zed_net_get_error() << "\n";
			zed_net_socket_close(&remote_socket);
			zed_net_socket_close(&listen_socket);
			zed_net_shutdown();
			call_thread.join(); // wait for thread
			return -1;
		}

		int64_t bytes_received {0};
		bytes_received = zed_net_tcp_socket_receive(&remote_socket, buffer->data(), buffer->size());
		if (bytes_received < 0) {
			quit = true;
			std::cerr << "H: zed_net_tcp_socket_receive failed: " << zed_net_get_error() << "\n";
			zed_net_socket_close(&remote_socket);
			zed_net_socket_close(&listen_socket);
			zed_net_shutdown();
			call_thread.join(); // wait for thread
			return -1;
		} else if (bytes_received == 0) {
			std::cout << "H: got 0 bytes?\n";
			break; // connection closed
		}

		std::cout << "H: got " << bytes_received << " bytes\n";

		in = zpp::bits::in{std::span{buffer->data(), static_cast<size_t>(bytes_received)}};

		auto cmd = rpc_service_client.response<"ServiceInterface::poll"_sha256_int>().or_throw();
		if (cmd.has_value()) {
			std::cout << "H: has_value\n";
			std::visit([](auto&& arg) {
				std::cout << arg.user;

				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, ServiceInterface::TextMessage>) {
					std::cout << " '" << arg.msg << "'\n";
				} else if constexpr (std::is_same_v<T, ServiceInterface::UserJoined>) {
					std::cout << " joined\n";
				} else if constexpr (std::is_same_v<T, ServiceInterface::UserLeft>) {
					std::cout << " left\n";
				}
			}, cmd.value());
		}

		// end poll

		// TODO: push
		// if have something to push, do the same for push

		std::this_thread::sleep_for(100ms);
	}

	zed_net_shutdown();

	call_thread.join();

	return 0;
}

