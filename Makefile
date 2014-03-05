all: build_all

build_all: build_server build_client


build_server:
	g++ -ggdb -I /opt/AMDAPP/include -I ./include -o server server.cc -L/usr/lib64 -L/usr/local/lib -lOpenCL -lzmq


build_client:
	g++ -c interposer.cc -I /opt/AMDAPP/include/ -I ./include/ -fPIC
	g++ -shared -Wl,-soname,libOpenCL.so.1 -o libCLInterposerClient.so.1.0.1  interposer.o -lzmq

clean:
	rm -rf *.o server client libCLInterposerClient.so.1.0.1
