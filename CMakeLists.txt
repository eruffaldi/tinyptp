
if(WIN32)
link_libraries(ws2_32)
endif()

add_executable(master_socket  master_socket.c master.c common_socket.c)
add_executable(slave_socket   slave_socket.c  slave.c  common_socket.c)