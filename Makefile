all: server client

server: twmailer_pro_server.cpp
	g++ -std=c++17 -o server twmailer_pro_server.cpp

client: twmailer_pro_client.cpp
	g++ -std=c++17 -o client twmailer_pro_client.cpp
clean: 
	rm server client