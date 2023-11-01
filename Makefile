all: server client

server: bin/server.cpp
	g++ bin/server.cpp -o build/server -lpthread

client: bin/client.cpp
	g++ bin/client.cpp -o build/client -lpthread

run_server: server
	./build/server 12345

run_client: client
	./build/client 127.0.0.1 12345

clean:
	rm -f build/client build/server

.PHONY: all run_server run_client clean