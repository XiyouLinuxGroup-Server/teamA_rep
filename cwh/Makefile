all: client server
client:client.cpp
	g++ client.cpp -lpthread -std=c++11 -o client
server:server.cpp
	g++ server.cpp -lpthread -std=c++11 -o server

clear:
	rm ./client ./server