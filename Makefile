all: server client

server: twmailer_basic_server.cpp
	g++ -std=c++14 -o server twmailer_basic_server.cpp

client: twmailer_basic_client.cpp
	g++ -std=c++14 -o client twmailer_basic_client.cpp
clean: 
	rm server client