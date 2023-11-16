#############################################################################################
# Makefile
#############################################################################################
CC=g++
CFLAGSSERVER=-g -std=c++17 -pthread
CFLAGSCLIENT=-g -std=c++17
LIBS=-lldap -llber

rebuild: clean all

all: ./bin/server ./bin/client

clean:
	clear
	rm -f bin/*

./bin/server: ./obj/twmailer_pro_server.o ./obj/FileManager.o
	${CC} ${CFLAGSSERVER} -o bin/server obj/twmailer_pro_server.o obj/FileManager.o ${LIBS}

./bin/client: twmailer_pro_client.cpp
	${CC} ${CFLAGSCLIENT} -o bin/client twmailer_pro_client.cpp

./obj/twmailer_pro_server.o: twmailer_pro_server.cpp
	${CC} ${CFLAGSSERVER} -o obj/twmailer_pro_server.o twmailer_pro_server.cpp -c

./obj/FileManager.o: FileManager.cpp
	${CC} ${CFLAGSSERVER} -o obj/FileManager.o FileManager.cpp -c 