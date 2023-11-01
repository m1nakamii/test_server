all: server client

build:
	mkdir -p build

server: server
	g++ bin/server.cpp -o build/server

client: build
	g++ bin/client.cpp -o build/client

run: all
	./run_server_client.sh 12345

clean:
	rm -f build/client build/server

.PHONY: all run clean
