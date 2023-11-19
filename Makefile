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

./bin/server: ./obj/twmailer_pro_server.o ./obj/FileManager.o ./obj/UserVerificationLdap.o ./obj/Blacklist.o
	${CC} ${CFLAGSSERVER} -o bin/server obj/twmailer_pro_server.o obj/FileManager.o obj/UserVerificationLdap.o obj/Blacklist.o ${LIBS}

./bin/client: twmailer_pro_client.cpp
	${CC} ${CFLAGSCLIENT} -o bin/client twmailer_pro_client.cpp

./obj/twmailer_pro_server.o: twmailer_pro_server.cpp
	${CC} ${CFLAGSSERVER} -o obj/twmailer_pro_server.o twmailer_pro_server.cpp -c

./obj/FileManager.o: FileManager.cpp
	${CC} ${CFLAGSSERVER} -o obj/FileManager.o FileManager.cpp -c 

./obj/UserVerificationLdap.o: UserVerificationLdap.cpp
	${CC} ${CFLAGSSERVER} -o obj/UserVerificationLdap.o UserVerificationLdap.cpp -c

./obj/Blacklist.o: Blacklist.cpp
	${CC} ${CFLAGSSERVER} -o obj/Blacklist.o Blacklist.cpp -c	

#Server: ./bin/server 4446 /home/matthias/projects/helloworld/tw-mailer-pro/mailspooldirectory/
#Client: ./bin/client 127.0.0.1 4446