#pragma once

#include <zpp_bits/zpp_bits.h>

#include <variant>

using namespace zpp::bits::literals;

// service_impl calls methods on host
struct ServiceInterface {
	struct TextMessage {
		std::string user;
		std::string msg;
	};

	struct UserJoined {
		std::string user;
	};

	struct UserLeft {
		std::string user;
	};

	using Command = std::variant<TextMessage, UserJoined, UserLeft>;

	std::optional<Command> poll(void);

	void push(const Command& cmd);

	using rpc = zpp::bits::rpc<
		zpp::bits::bind<&ServiceInterface::poll, "ServiceInterface::poll"_sha256_int>,
		zpp::bits::bind<&ServiceInterface::push, "ServiceInterface::push"_sha256_int>
	>;
};

