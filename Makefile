all: server client

server: server
	g++ bin/server.cpp -o build/server -lpthread

client: client
	g++ bin/client.cpp -o build/client -lpthread

run: all
	./run_server_client.sh 12345

clean:
	rm -f build/client build/server

.PHONY: all run clean
