ocl
===

Build server :  make 
Build client: make build_client

Dependencies:
GPUPerfAPI
OpenCV
OpenCL

On the client machine, replace the libOpenCl.so.1 with the interposer shared library.

For the sample application, copy the facedetect.cpp and the controller.py file into opencv/samples/ocl and build.

Set the correct ipaddress in server.cc and interposer.cc files

This project is licensed under BSD
