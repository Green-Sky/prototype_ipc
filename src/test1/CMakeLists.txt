cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

find_package(Threads REQUIRED)

add_executable(test1_host
	test1_host.cpp
)

target_link_libraries(test1_host PUBLIC
	zpp_bits
	zed_net
	Threads::Threads
)

add_executable(test1_service
	test1_service.cpp
)

target_link_libraries(test1_service PUBLIC
	zpp_bits
	zed_net
	Threads::Threads
)

