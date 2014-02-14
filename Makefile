all: build_all

build_all: build_server build_client

build_server: server.o
	gcc -o server -lzmq server.o

server.o:
	g++ -c server.cc -I /opt/AMDAPP/include/ -I ./include/
        
build_client: interposer.o
	g++ -o client -lzmq -lOpenCl client.o

client.o:
	g++ -c interposer.cc -I /opt/AMDAPP/include/ -I ./include/

clean:
	rm -rf *.o server client
