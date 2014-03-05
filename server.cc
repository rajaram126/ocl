#include <map>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "interposer.h"

void clGetPlatformIDs_server(get_platform_ids_ *argp, get_platform_ids_ *retp){

	cl_int err = CL_SUCCESS;
	retp->err = err;

        cl_uint num_platforms = 0;

        err = clGetPlatformIDs(0, NULL, &num_platforms);

        if(err != CL_SUCCESS){
                //fprintf(stderr,"clGetPlatformIDs failed with err %d\n", err);
                exit(-1);
        }

	retp->err |= err;

        //fprintf(stderr,"[clGetPlatformIDs_server] Num OpenCL platforms found %d\n", num_platforms);
	retp->num_platforms_found = num_platforms;

        if(num_platforms > 0){

		cl_platform_id *platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)* num_platforms);

		clGetPlatformIDs(num_platforms, platforms, NULL);

                if(err != CL_SUCCESS){
                        //fprintf(stderr,"clGetPlatformIDs failed with err %d\n", err);
                        exit(-1);
                }

		retp->err |= err;

		for(int i=0; i<num_platforms; i++){
			//fprintf(stderr,"[clGetPlatformIDs_server] platforms[%d]=%p\n",i, platforms[i]);
		}

		retp->platforms.buff_ptr = (char *)platforms;
		retp->platforms.buff_len = num_platforms * sizeof(cl_platform_id);
	}else{

		retp->platforms.buff_ptr = "\0";
		retp->platforms.buff_len = sizeof(char);	
	}
}

void clGetPlatformInfo_server(get_platform_info_ *argp, get_platform_info_ *retp){
	retp->err = CL_SUCCESS;
	size_t size = 0;
	char * profile = NULL;
	fprintf(stderr,"clGetPlatformInfo_server platform id = %d param name = %d \n",argp->platform,argp->param_name);
	if(argp->is_buff_null) {
		clGetPlatformInfo(argp->platform, argp->param_name, NULL, NULL, &size);
	} else {
		profile = (char * ) malloc(argp->param_value_size);
		clGetPlatformInfo(argp->platform, argp->param_name, argp->param_value_size, profile, NULL);
	}
	if(profile) {
		retp->param_value.buff_ptr = profile;
		retp->param_value.buff_len = argp->param_value_size;
		retp->param_value_size = argp->param_value_size;
	} else {
		retp->param_value.buff_ptr = "\0";
		retp->param_value.buff_len = sizeof(char);
		retp->param_value_size = size;
	}
}

void clGetProgramInfo_server(get_program_info_ *argp, get_program_info_ *retp){
	retp->err = CL_SUCCESS;
	size_t size = 0;
	 char * profile = NULL;
	 char ** binaries = NULL;
int i;
	fprintf(stderr,"clGetProgramInfo_server program = %d param name = %d size = %d \n",argp->program,argp->param_name,argp->param_value_size);
	if(argp->is_buff_null) {
		fprintf(stderr,"clGetProgramInfo first case\n");
		clGetProgramInfo(argp->program, argp->param_name, NULL, NULL, &size);
	} else {
		if(argp->param_name == 4454) {
		fprintf(stderr,"clGetProgramInfo second case\n");
		binaries = new  char*[argp->param_value_size];
		for ( i=0;i<(int)argp->param_value_size;++i) {
    			binaries[i] = new  char[4096];
		}
		clGetProgramInfo(argp->platform, argp->param_name, argp->param_value_size, binaries, NULL);
		} else {
			profile = (char * ) malloc(argp->param_value_size);
		clGetPlatformInfo(argp->platform, argp->param_name, argp->param_value_size, profile, NULL);
		}	
	}
	if(profile) {
		retp->param_value.buff_ptr = profile;
		retp->param_value.buff_len = argp->param_value_size;
		retp->param_value_size = argp->param_value_size;
	} else if(binaries) {
		retp->param_value.buff_ptr = *binaries;
		retp->param_value.buff_len = argp->param_value_size;
		retp->param_value_size = argp->param_value_size;
	} else
	 {
		retp->param_value.buff_ptr = "\0";
		retp->param_value.buff_len = sizeof(char);
		retp->param_value_size = size;
	}
}

void clGetDeviceInfo_server(get_device_info_ *argp, get_device_info_ *retp){
	retp->err = CL_SUCCESS;
	size_t size = 0;
	char * profile = NULL;
	printf("clGetDeviceInfo_server device id = %d param name = %d \n",argp->device,argp->param_name);
	if(argp->is_buff_null) {
		clGetDeviceInfo(argp->device, argp->param_name, NULL, NULL, &size);
	} else {
		profile = (char * ) malloc(argp->param_value_size);
		clGetDeviceInfo(argp->device, argp->param_name, argp->param_value_size, profile, NULL);
	}
	if(profile) {
		retp->param_value.buff_ptr = profile;
		retp->param_value.buff_len = argp->param_value_size;
		retp->param_value_size = argp->param_value_size;
	} else {
		retp->param_value.buff_ptr = "\0";
		retp->param_value.buff_len = sizeof(char);
		retp->param_value_size = size;
	}
}
void clGetDeviceIDs_server(get_device_ids_ *argp, get_device_ids_ *retp){

	cl_int err = CL_SUCCESS;
	retp->err = err;

        cl_uint num_devices = 0;

	//fprintf(stderr,"[clGetDeviceIDs_server] platform %p\n", (cl_platform_id)(argp->platform));
	//fprintf(stderr,"[clGetDeviceIDs_server] device_type %d\n", (cl_device_type)(argp->device_type));


        err = clGetDeviceIDs((cl_platform_id)(argp->platform), (cl_device_type)(argp->device_type), 0, NULL, &num_devices);

        if(err != CL_SUCCESS){
                //fprintf(stderr,"clGetDeviceIDs failed with err %d\n", err);
//                exit(-1);
        }
	retp->err |= err;

        //fprintf(stderr,"[clGetDeviceIDs_server] Num OpenCL devices found %d\n", num_devices);
	retp->num_devices_found = num_devices;

        if(num_devices > 0){

		cl_device_id *devices = (cl_device_id *)malloc(sizeof(cl_device_id)* num_devices);

		clGetDeviceIDs((cl_platform_id)(argp->platform), (cl_device_type)(argp->device_type), num_devices, devices, NULL);

                if(err != CL_SUCCESS){
                        //fprintf(stderr,"clGetDeviceIDs failed with err %d\n", err);
                        exit(-1);
                }
		retp->err |= err;

		for(int i=0; i<num_devices; i++){
			//fprintf(stderr,"[clGetDeviceIDs_server] devices[%d]=%p\n",i, devices[i]);
		}

		retp->devices.buff_ptr = (char *)devices;
		retp->devices.buff_len = num_devices * sizeof(cl_device_id);
	}else{

		retp->devices.buff_ptr = "\0";
		retp->devices.buff_len = sizeof(char);	
	}
}


void clCreateContext_server(create_context_ *argp, create_context_ *retp){

	cl_int err = CL_SUCCESS;
	retp->err = err;

        cl_context context = 0;

        //fprintf(stderr,"[clCreateContext_server] Num devices received %d\n", argp->num_devices);

	cl_device_id *devices = (cl_device_id*)(argp->devices.buff_ptr);

	for(int i=0; i<argp->num_devices; i++){
		//fprintf(stderr,"[clCreateContext_server] devices[%d] = %p\n", i, devices[i]);
	}

        context  = clCreateContext(NULL, (cl_uint)(argp->num_devices), devices, NULL, NULL, &err);

        if(err != CL_SUCCESS){
                //fprintf(stderr,"clCreateContext failed with err %d\n", err);
                exit(-1);
        }
	retp->err |= err;

	retp->context = (unsigned long)context;
        //fprintf(stderr,"[clCreateContext_server] context created %p\n", retp->context);

	retp->devices.buff_ptr = "\0";
	retp->devices.buff_len = sizeof(char);	
}

void clCreateCommandQueue_server(create_command_queue_ *argp, create_command_queue_ *retp){

	cl_int err = CL_SUCCESS;

        cl_command_queue command_queue = 0;

        //fprintf(stderr,"[clCreateCommandQueue_server] context %p\n", argp->context);
        //fprintf(stderr,"[clCreateCommandQueue_server] device %p\n", argp->device);

        command_queue  = clCreateCommandQueue((cl_context)(argp->context), (cl_device_id)(argp->device), 0, &err);

        if(err != CL_SUCCESS){
                //fprintf(stderr,"clCreateCommandQueue failed with err %d\n", err);
                exit(-1);
        }
	retp->err = err;

	retp->command_queue = (unsigned long)command_queue;
        //fprintf(stderr,"[clCreateCommandQueue_server] command_queue created %p\n", retp->command_queue);

}


void clCreateBuffer_server(create_buffer_ *argp, create_buffer_ *retp){

	cl_int err = CL_SUCCESS;

        cl_mem mem = 0;

        //fprintf(stderr,"[clCreateBuffer_server] context %p\n", argp->context);

	cl_mem_flags flags = (cl_mem_flags)(argp->flags);
	size_t size = (size_t)(argp->size);
	void *host_ptr = NULL;

	bool use_host_ptr = flags & CL_MEM_USE_HOST_PTR;

	bool copy_host_ptr = flags & CL_MEM_COPY_HOST_PTR;

	if(use_host_ptr){
		host_ptr = malloc(size);
	} else if (copy_host_ptr) {
		host_ptr = (void *)(argp->data.buff_ptr);
	}
        mem  = clCreateBuffer((cl_context)(argp->context), flags, size, host_ptr, &err);

        if(err != CL_SUCCESS){
                //fprintf(stderr,"clCreateBuffer failed with err %d\n", err);
                exit(-1);
        }
	retp->err = err;

	retp->data.buff_ptr = "\0";
	retp->data.buff_len = sizeof(char);	

	retp->mem = (unsigned long)mem;

        //fprintf(stderr,"[clCreateBuffer_server] mem created %p\n", retp->mem);

}


void clCreateProgramWithSource_server(create_program_with_source_ *argp, create_program_with_source_ *retp){

	cl_int err = CL_SUCCESS;

        cl_program program = 0;

        fprintf(stderr,"[clCreateProgramWithSource_server] context %p program %s\n", argp->context,argp->program_str.buff_ptr);

        program  = clCreateProgramWithSource((cl_context)(argp->context), 1, (const char **)&(argp->program_str.buff_ptr), NULL, &err);

        if(err != CL_SUCCESS){
                //fprintf(stderr,"clCreateProgramWithSource failed with err %d\n", err);
                exit(-1);
        }
	retp->err = err;

	retp->program_str.buff_ptr = "\0";
	retp->program_str.buff_len = sizeof(char);	

	retp->program = (unsigned long)program;

        //fprintf(stderr,"[clCreateProgramWithSource_server] program created %p\n", retp->program);

}


void clBuildProgram_server(build_program_ *argp, build_program_ *retp){

	cl_int err = CL_SUCCESS;

        //fprintf(stderr,"[clBuildProgram_server] program %p\n", argp->program);
	//fprintf(stderr,"[clBuildProgram_server] options %s\n", argp->options.buff_ptr);
	 //fprintf(stderr,"[clBuildProgram_server] options length %d\n", argp->options.buff_len);
	if(!argp->options.buff_len) {
	argp->options.buff_ptr = "";
	}

	if(argp->all_devices){
		err  = clBuildProgram((cl_program)(argp->program), 0, NULL, (const char *)(argp->options.buff_ptr), NULL, NULL);
	} else {
		err  = clBuildProgram((cl_program)(argp->program), (argp->num_devices), (cl_device_id *)(argp->devices.buff_ptr), (const char *)(argp->options.buff_ptr), NULL, NULL);
	}
        if(err != CL_SUCCESS){
                //fprintf(stderr,"clBuildProgram failed with err %d\n", err);
                exit(-1);
        }
	retp->err = err;

	retp->devices.buff_ptr = "\0";
	retp->devices.buff_len = sizeof(char);	

	retp->options.buff_ptr = "\0";
	retp->options.buff_len = sizeof(char);	

}


void clCreateKernel_server(create_kernel_ *argp, create_kernel_ *retp){

	cl_int err = CL_SUCCESS;

        cl_kernel kernel = 0;

        fprintf(stderr,"[clCreateKernel_server] program %p\n", argp->program);
        fprintf(stderr,"[clCreateKernel_server] kernel_name %s\n", argp->kernel_name.buff_ptr);
        fprintf(stderr,"[clCreateKernel_server] kernel_name length %d\n", argp->kernel_name.buff_len);

	char *kernel_name = (char *)calloc(argp->kernel_name.buff_len + 1, sizeof(char));
	for(int i=0; i<argp->kernel_name.buff_len; i++){
		kernel_name[i] = *((argp->kernel_name.buff_ptr) + i);
	}
	kernel_name[argp->kernel_name.buff_len] = '\0'; 

        //kernel  = clCreateKernel((cl_program)(argp->program), (const char *)(argp->kernel_name.buff_ptr), &err);
        //if(err != CL_SUCCESS){
        //        //fprintf(stderr,"clCreateKernel failed with err %d\n", err);
        //        exit(-1);
        //}

	err = CL_SUCCESS;
        kernel  = clCreateKernel((cl_program)(argp->program), kernel_name, &err);
        if(err != CL_SUCCESS){
                //fprintf(stderr,"clCreateKernel failed with err %d\n", err);
                exit(-1);
        }

	retp->err = err;

	retp->kernel_name.buff_ptr = "\0";
	retp->kernel_name.buff_len = sizeof(char);	

	retp->kernel = (unsigned long)kernel;

        //fprintf(stderr,"[clCreateKernel_server] kernel created %p\n", retp->kernel);

}

void clSetKernelArg_server(set_kernel_arg_ *argp, set_kernel_arg_ *retp){

	cl_int err = CL_SUCCESS;

        cl_kernel kernel = 0;

        fprintf(stderr,"[clSetKernelArg_server] kernel %p\n", argp->kernel);

	if(argp->is_null_arg){
		err  = clSetKernelArg((cl_kernel)(argp->kernel), argp->arg_index, argp->arg_size, NULL);
	} else if (argp->is_clobj) {
		if(argp->is_mem){
			cl_mem mem = (cl_mem)(argp->mem);
			fprintf(stderr,"[clSetKernelArg_server] mem %p\n", mem);
			assert(argp->arg_size == sizeof(cl_mem));
			err = clSetKernelArg((cl_kernel)(argp->kernel), argp->arg_index, sizeof(cl_mem), (void *)&mem);
		} else if (argp->is_image){
			//cl_image not supported in the runtime version installed on shiva
			//cl_image image = (cl_image)(argp->image);
			//assert(argp->arg_size == sizeof(cl_image));
			//err = clSetKernelArg((cl_kernel)(argp->kernel), argp->arg_index, sizeof(cl_image), (void *)&image);
		} else if (argp->is_sampler) {
			cl_sampler sampler = (cl_sampler)(argp->sampler);
			assert(argp->arg_size == sizeof(cl_sampler));
			err = clSetKernelArg((cl_kernel)(argp->kernel), argp->arg_index, sizeof(cl_sampler), (void *)&sampler);
		} else {
			assert(0); //should not be here
		}

	} else {
		err  = clSetKernelArg((cl_kernel)(argp->kernel), argp->arg_index, argp->arg_size, (void *)(argp->plain_old_data.buff_ptr));
	}

        if(err != CL_SUCCESS){
                //fprintf(stderr,"clCreateKernel failed with err %d\n", err);
                exit(-1);
        }
	retp->err = err;

	retp->plain_old_data.buff_ptr = "\0";
	retp->plain_old_data.buff_len = sizeof(char);	

}

void clEnqueueWriteBuffer_server(enqueue_write_buffer_ *argp, enqueue_write_buffer_ *retp){

	cl_int err = CL_SUCCESS;

        //fprintf(stderr,"[clEnqueueWriteBuffer_server] mem %p\n", argp->mem);
        //fprintf(stderr,"[clEnqueueWriteBuffer_server] command queue %p\n", argp->command_queue);

        err  = clEnqueueWriteBuffer((cl_command_queue)(argp->command_queue), (cl_mem)(argp->mem), argp->blocking, argp->offset, argp->size, (void *)(argp->data.buff_ptr), 0, NULL, NULL);

        if(err != CL_SUCCESS){
                //fprintf(stderr,"clEnqueueWriteBuffer failed with err %d\n", err);
                exit(-1);
        }
	retp->err = err;

	retp->data.buff_ptr = "\0";
	retp->data.buff_len = sizeof(char);

        //fprintf(stderr,"[clEnqueueWriteBuffer_server]err returned %d\n", retp->err);

}



void clEnqueueNDRangeKernel_server(enqueue_ndrange_kernel_ *argp, enqueue_ndrange_kernel_ *retp){

	cl_int err = CL_SUCCESS;

        //fprintf(stderr,"[clEnqueueNDRangeKernel_server] kernel %p\n", argp->kernel);
        //fprintf(stderr,"[clEnqueueNDRangeKernel_server] command queue %p\n", argp->command_queue);

	size_t *global_work_offset=NULL, *local_work_size=NULL;

	if(strcmp(argp->global_offset.buff_ptr, "\0")){
		global_work_offset = (size_t *)argp->global_offset.buff_ptr;
	}

	if(strcmp(argp->local_size.buff_ptr, "\0")){
		local_work_size = (size_t *)argp->local_size.buff_ptr;
	}

        //err  = clEnqueueNDRangeKernel((cl_command_queue)(argp->command_queue), (cl_kernel)(argp->kernel), argp->work_dim, (const size_t *)(argp->global_offset.buff_ptr), (const size_t *)(argp->global_size.buff_ptr), (const size_t *)(argp->local_size.buff_ptr), 0, NULL, NULL);
        err  = clEnqueueNDRangeKernel((cl_command_queue)(argp->command_queue), (cl_kernel)(argp->kernel), argp->work_dim, (const size_t *)global_work_offset, (const size_t *)(argp->global_size.buff_ptr), (const size_t *)local_work_size, 0, NULL, NULL);

        if(err != CL_SUCCESS){
                //fprintf(stderr,"clEnqueueNDRangeKernel failed with err %d\n", err);
                exit(-1);
        }
	retp->err = err;

	retp->global_offset.buff_ptr = "\0";
	retp->global_offset.buff_len = sizeof(char);

	retp->global_size.buff_ptr = "\0";
	retp->global_size.buff_len = sizeof(char);

	retp->local_size.buff_ptr = "\0";
	retp->local_size.buff_len = sizeof(char);

        //fprintf(stderr,"[clEnqueueNDRangeKernel_server]err returned %d\n", retp->err);

}

void clEnqueueReadBuffer_server(enqueue_read_buffer_ *argp, enqueue_read_buffer_ *retp){

	cl_int err = CL_SUCCESS;

        //fprintf(stderr,"[clEnqueueReadBuffer_server] mem %p\n", argp->mem);
        //fprintf(stderr,"[clEnqueueReadBuffer_server] command queue %p\n", argp->command_queue);

	void *ptr = malloc(argp->size);
        err  = clEnqueueReadBuffer((cl_command_queue)(argp->command_queue), (cl_mem)(argp->mem), argp->blocking, argp->offset, argp->size, ptr, 0, NULL, NULL);

        if(err != CL_SUCCESS){
                //fprintf(stderr,"clEnqueueReadBuffer failed with err %d\n", err);
                exit(-1);
        }
	retp->err = err;

	retp->data.buff_ptr = (char *)ptr;
	retp->data.buff_len = argp->size;
	//fprintf(stderr,"[clEnqueueReadBuffer_server] output %s\n",retp->data.buff_ptr);
        //fprintf(stderr,"[clEnqueueReadBuffer_server]err returned %d\n", retp->err);

}


main() {
//fprintf(stderr,"reached here");
    void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);

//TODO cleanup
    int rc = zmq_bind (responder, "tcp://127.0.0.1:5555");
    assert (rc == 0);

	while (1) {
 		zmq_msg_t message_header;
		invocation_header * header;
		zmq_msg_init(&message_header);
		zmq_msg_recv(&message_header, responder, 0);
		header = (invocation_header *) zmq_msg_data(&message_header);
		fprintf(stderr,"\ngot %d\n",header->api_id);
		
		switch(header->api_id) {
			case GET_PLATFORM_IDS: {
						get_platform_ids_  arg_pkt,ret_pkt;
						
						zmq_msg_t message,message_buffer,reply,reply_buffer;
						zmq_msg_init(&message);
						zmq_msg_init(&message_buffer);
						zmq_msg_recv(&message, responder, 0);
						arg_pkt = * (get_platform_ids_*) zmq_msg_data(&message);
						zmq_msg_recv(&message_buffer, responder, 0);
						arg_pkt.platforms.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						clGetPlatformIDs_server(&arg_pkt, &ret_pkt);
						zmq_msg_init_size(&reply, sizeof(ret_pkt));
						zmq_msg_init_size(&reply_buffer,ret_pkt.platforms.buff_len);
						memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
						memcpy(zmq_msg_data(&reply_buffer), ret_pkt.platforms.buff_ptr,ret_pkt.platforms.buff_len);
						zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
						zmq_msg_send(&reply_buffer, responder, 0);
						zmq_msg_close(&message);
						zmq_msg_close(&message_buffer);
						zmq_msg_close(&reply);
						break;
						}
			case GET_PLATFORM_INFO: {
						get_platform_info_  arg_pkt,ret_pkt;
						
						zmq_msg_t message,message_buffer,reply,reply_buffer;
						zmq_msg_init(&message);
						zmq_msg_init(&message_buffer);
						zmq_msg_recv(&message, responder, 0);
						arg_pkt = * (get_platform_info_*) zmq_msg_data(&message);
						zmq_msg_recv(&message_buffer, responder, 0);
						arg_pkt.param_value.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						clGetPlatformInfo_server(&arg_pkt, &ret_pkt);
						zmq_msg_init_size(&reply, sizeof(ret_pkt));
						
						zmq_msg_init_size(&reply_buffer,ret_pkt.param_value.buff_len);
	
						memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
						memcpy(zmq_msg_data(&reply_buffer), ret_pkt.param_value.buff_ptr,ret_pkt.param_value.buff_len);
						zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
						zmq_msg_send(&reply_buffer, responder, 0);
						zmq_msg_close(&message);
						zmq_msg_close(&message_buffer);
						zmq_msg_close(&reply);
						break;
						}
			case GET_DEVICE_INFO: {
						get_device_info_  arg_pkt,ret_pkt;
						
						zmq_msg_t message,message_buffer,reply,reply_buffer;
						zmq_msg_init(&message);
						zmq_msg_init(&message_buffer);
						zmq_msg_recv(&message, responder, 0);
						arg_pkt = * (get_device_info_*) zmq_msg_data(&message);
						zmq_msg_recv(&message_buffer, responder, 0);
						arg_pkt.param_value.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						clGetDeviceInfo_server(&arg_pkt, &ret_pkt);
						zmq_msg_init_size(&reply, sizeof(ret_pkt));
						
						zmq_msg_init_size(&reply_buffer,ret_pkt.param_value.buff_len);
	
						memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
						memcpy(zmq_msg_data(&reply_buffer), ret_pkt.param_value.buff_ptr,ret_pkt.param_value.buff_len);
						zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
						zmq_msg_send(&reply_buffer, responder, 0);
						zmq_msg_close(&message);
						zmq_msg_close(&message_buffer);
						zmq_msg_close(&reply);
						break;
						}
			case GET_PROGRAM_INFO: {
						get_program_info_  arg_pkt,ret_pkt;
						
						zmq_msg_t message,message_buffer,reply,reply_buffer;
						zmq_msg_init(&message);
						zmq_msg_init(&message_buffer);
						zmq_msg_recv(&message, responder, 0);
						arg_pkt = * (get_program_info_*) zmq_msg_data(&message);
						zmq_msg_recv(&message_buffer, responder, 0);
						arg_pkt.param_value.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						clGetProgramInfo_server(&arg_pkt, &ret_pkt);
						zmq_msg_init_size(&reply, sizeof(ret_pkt));
						
						zmq_msg_init_size(&reply_buffer,ret_pkt.param_value.buff_len);
	
						memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
						memcpy(zmq_msg_data(&reply_buffer), ret_pkt.param_value.buff_ptr,ret_pkt.param_value.buff_len);
						zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
						zmq_msg_send(&reply_buffer, responder, 0);
						zmq_msg_close(&message);
						zmq_msg_close(&message_buffer);
						zmq_msg_close(&reply);
						break;
						}
			case GET_DEVICE_IDS : {
						get_device_ids_  arg_pkt,ret_pkt;
						zmq_msg_t message,message_buffer,reply,reply_buffer;
						zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
						zmq_msg_recv(&message, responder, 0);
						arg_pkt = * (get_device_ids_*) zmq_msg_data(&message);
						zmq_msg_recv(&message_buffer, responder, 0);
						arg_pkt.devices.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						clGetDeviceIDs_server(&arg_pkt, &ret_pkt);
						zmq_msg_init_size(&reply, sizeof(ret_pkt));
						zmq_msg_init_size(&reply_buffer,ret_pkt.devices.buff_len);
						memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
						memcpy(zmq_msg_data(&reply_buffer), ret_pkt.devices.buff_ptr,ret_pkt.devices.buff_len);
						zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
						zmq_msg_send(&reply_buffer, responder, 0);
						zmq_msg_close(&message);
						zmq_msg_close(&message_buffer);
						zmq_msg_close(&reply);
						break;
						}
			case CREATE_CONTEXT : {
						create_context_  arg_pkt,ret_pkt;
						zmq_msg_t message,message_buffer,reply,reply_buffer;
						zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
						zmq_msg_recv(&message, responder, 0);
						arg_pkt = * (create_context_*) zmq_msg_data(&message);
						zmq_msg_recv(&message_buffer, responder, 0);
						arg_pkt.devices.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						 clCreateContext_server(&arg_pkt, &ret_pkt);
						zmq_msg_init_size(&reply, sizeof(ret_pkt));
						zmq_msg_init_size(&reply_buffer,ret_pkt.devices.buff_len);
						memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
						memcpy(zmq_msg_data(&reply_buffer), ret_pkt.devices.buff_ptr,ret_pkt.devices.buff_len);
						zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
						zmq_msg_send(&reply_buffer, responder, 0);
						zmq_msg_close(&message);
						zmq_msg_close(&message_buffer);
						zmq_msg_close(&reply);
						break;
						}
			case CREATE_COMMAND_QUEUE: {
							create_command_queue_ arg_pkt,ret_pkt;
							zmq_msg_t message,message_buffer,reply,reply_buffer;
							zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
							zmq_msg_recv(&message, responder, 0);
							arg_pkt = * (create_command_queue_*) zmq_msg_data(&message);
							zmq_msg_recv(&message_buffer, responder, 0);
							
							clCreateCommandQueue_server(&arg_pkt, &ret_pkt);
							zmq_msg_init_size(&reply, sizeof(ret_pkt));
							zmq_msg_init_size(&reply_buffer,sizeof(char));
							memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
							memcpy(zmq_msg_data(&reply_buffer), "\0",sizeof(char));
							zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
							zmq_msg_send(&reply_buffer, responder, 0);
							zmq_msg_close(&message);
							zmq_msg_close(&message_buffer);
							zmq_msg_close(&reply);
							break;
						}
			case CREATE_BUFFER: {
							create_buffer_ arg_pkt,ret_pkt;
							zmq_msg_t message,message_buffer,reply,reply_buffer;
							zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
							zmq_msg_recv(&message, responder, 0);
							arg_pkt = * (create_buffer_*) zmq_msg_data(&message);
							zmq_msg_recv(&message_buffer, responder, 0);
							arg_pkt.data.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						 	clCreateBuffer_server(&arg_pkt, &ret_pkt);
							zmq_msg_init_size(&reply, sizeof(ret_pkt));
							zmq_msg_init_size(&reply_buffer,ret_pkt.data.buff_len);
							memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
							memcpy(zmq_msg_data(&reply_buffer), ret_pkt.data.buff_ptr,ret_pkt.data.buff_len);
							zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
							zmq_msg_send(&reply_buffer, responder, 0);
							zmq_msg_close(&message);
							zmq_msg_close(&message_buffer);
							zmq_msg_close(&reply);
							break;
						}
			case CREATE_PROGRAM_WITH_SOURCE: {
							create_program_with_source_ arg_pkt,ret_pkt;
							zmq_msg_t message,message_buffer,reply,reply_buffer;
							zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
							zmq_msg_recv(&message, responder, 0);
							arg_pkt = * (create_program_with_source_*) zmq_msg_data(&message);
							zmq_msg_recv(&message_buffer, responder, 0);
							arg_pkt.program_str.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						 	clCreateProgramWithSource_server(&arg_pkt, &ret_pkt);
							zmq_msg_init_size(&reply, sizeof(ret_pkt));
							zmq_msg_init_size(&reply_buffer,ret_pkt.program_str.buff_len);
							memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
							memcpy(zmq_msg_data(&reply_buffer), ret_pkt.program_str.buff_ptr,ret_pkt.program_str.buff_len);
							zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
							zmq_msg_send(&reply_buffer, responder, 0);
							zmq_msg_close(&message);
							zmq_msg_close(&message_buffer);
							zmq_msg_close(&reply);
							break;
						}
			case BUILD_PROGRAM: 
			case BUILD_PROGRAM_AUX:		{
							build_program_ arg_pkt,ret_pkt;
							zmq_msg_t message,message_buffer,message_buffer_aux,reply,reply_buffer;
							zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
                                                zmq_msg_init(&message_buffer_aux);
							zmq_msg_recv(&message, responder, 0);
							arg_pkt = * (build_program_*) zmq_msg_data(&message);
							zmq_msg_recv(&message_buffer, responder, 0);
							zmq_msg_recv(&message_buffer_aux, responder, 0);
							arg_pkt.devices.buff_ptr = (char *) zmq_msg_data(&message_buffer);
							arg_pkt.options.buff_ptr = (char *) zmq_msg_data(&message_buffer_aux);
						 	clBuildProgram_server(&arg_pkt, &ret_pkt);
							zmq_msg_init_size(&reply, sizeof(ret_pkt));
							zmq_msg_init_size(&reply_buffer,ret_pkt.options.buff_len);
							memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
							memcpy(zmq_msg_data(&reply_buffer), ret_pkt.options.buff_ptr,ret_pkt.options.buff_len);
							zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
							zmq_msg_send(&reply_buffer, responder, 0);
							zmq_msg_close(&message);
							zmq_msg_close(&message_buffer);
							zmq_msg_close(&reply);
							break;
						}
			case CREATE_KERNEL: 		{
							create_kernel_ arg_pkt,ret_pkt;
							zmq_msg_t message,message_buffer,reply,reply_buffer;
							zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
							zmq_msg_recv(&message, responder, 0);
							arg_pkt = * (create_kernel_*) zmq_msg_data(&message);
							zmq_msg_recv(&message_buffer, responder, 0);
							arg_pkt.kernel_name.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						 	clCreateKernel_server(&arg_pkt, &ret_pkt);
							zmq_msg_init_size(&reply, sizeof(ret_pkt));
							zmq_msg_init_size(&reply_buffer,ret_pkt.kernel_name.buff_len);
							memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
							memcpy(zmq_msg_data(&reply_buffer), ret_pkt.kernel_name.buff_ptr,ret_pkt.kernel_name.buff_len);
							zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
							zmq_msg_send(&reply_buffer, responder, 0);
							zmq_msg_close(&message);
							zmq_msg_close(&message_buffer);
							zmq_msg_close(&reply);
							break;
						}
			case SET_KERNEL_ARG:  		{
							 set_kernel_arg_ arg_pkt,ret_pkt;
							zmq_msg_t message,message_buffer,reply,reply_buffer;
							zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
							zmq_msg_recv(&message, responder, 0);
							arg_pkt = * (set_kernel_arg_*) zmq_msg_data(&message);
							zmq_msg_recv(&message_buffer, responder, 0);
							arg_pkt.plain_old_data.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						 	clSetKernelArg_server(&arg_pkt, &ret_pkt);
							zmq_msg_init_size(&reply, sizeof(ret_pkt));
							zmq_msg_init_size(&reply_buffer,ret_pkt.plain_old_data.buff_len);
							memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
							memcpy(zmq_msg_data(&reply_buffer), ret_pkt.plain_old_data.buff_ptr,ret_pkt.plain_old_data.buff_len);
							zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
							zmq_msg_send(&reply_buffer, responder, 0);
							zmq_msg_close(&message);
							zmq_msg_close(&message_buffer);
							zmq_msg_close(&reply);
							break;
						}
			case ENQUEUE_WRITE_BUFFER: 	{
							enqueue_write_buffer_ arg_pkt,ret_pkt;
							zmq_msg_t message,message_buffer,reply,reply_buffer;
							zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
							zmq_msg_recv(&message, responder, 0);
							arg_pkt = * (enqueue_write_buffer_*) zmq_msg_data(&message);
							zmq_msg_recv(&message_buffer, responder, 0);
							arg_pkt.data.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						 	clEnqueueWriteBuffer_server(&arg_pkt, &ret_pkt);
							zmq_msg_init_size(&reply, sizeof(ret_pkt));
							zmq_msg_init_size(&reply_buffer,ret_pkt.data.buff_len);
							memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
							memcpy(zmq_msg_data(&reply_buffer), ret_pkt.data.buff_ptr,ret_pkt.data.buff_len);
							zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
							zmq_msg_send(&reply_buffer, responder, 0);
							zmq_msg_close(&message);
							zmq_msg_close(&message_buffer);
							zmq_msg_close(&reply);
							break;
						}
			case ENQUEUE_READ_BUFFER:	{
						 	enqueue_read_buffer arg_pkt,ret_pkt;
							zmq_msg_t message,message_buffer,reply,reply_buffer;
							zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
							zmq_msg_recv(&message, responder, 0);
							arg_pkt = * (enqueue_read_buffer*) zmq_msg_data(&message);
							zmq_msg_recv(&message_buffer, responder, 0);
							arg_pkt.data.buff_ptr = (char *) zmq_msg_data(&message_buffer);
						 	clEnqueueReadBuffer_server(&arg_pkt, &ret_pkt);
							zmq_msg_init_size(&reply, sizeof(ret_pkt));
							zmq_msg_init_size(&reply_buffer,ret_pkt.data.buff_len);
							memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
							memcpy(zmq_msg_data(&reply_buffer), ret_pkt.data.buff_ptr,ret_pkt.data.buff_len);
							zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
							zmq_msg_send(&reply_buffer, responder, 0);
							zmq_msg_close(&message);
							zmq_msg_close(&message_buffer);
							zmq_msg_close(&reply);
							break;
						}
			case ENQUEUE_NDRANGE_KERNEL:	{
							enqueue_ndrange_kernel_ arg_pkt,ret_pkt;
							zmq_msg_t message,message_buffer,message_buffer_aux,message_buffer_aux2,reply,reply_buffer;
							zmq_msg_init(&message);
                                                zmq_msg_init(&message_buffer);
							zmq_msg_init(&message_buffer_aux);
                                                zmq_msg_init(&message_buffer_aux2);
							zmq_msg_recv(&message, responder, 0);
							arg_pkt = * (enqueue_ndrange_kernel_*) zmq_msg_data(&message);
							zmq_msg_recv(&message_buffer, responder, 0);
							zmq_msg_recv(&message_buffer_aux, responder, 0);
							zmq_msg_recv(&message_buffer_aux2, responder, 0);
							arg_pkt.global_offset.buff_ptr = (char *) zmq_msg_data(&message_buffer);
							arg_pkt.global_size.buff_ptr = (char *) zmq_msg_data(&message_buffer_aux);
							arg_pkt.local_size.buff_ptr = (char *) zmq_msg_data(&message_buffer_aux2);
						 	clEnqueueNDRangeKernel_server(&arg_pkt, &ret_pkt);
							zmq_msg_init_size(&reply, sizeof(ret_pkt));
							zmq_msg_init_size(&reply_buffer,ret_pkt.global_offset.buff_len);
							memcpy(zmq_msg_data(&reply), &ret_pkt, sizeof(ret_pkt));
							memcpy(zmq_msg_data(&reply_buffer), ret_pkt.global_offset.buff_ptr,ret_pkt.global_offset.buff_len);
							zmq_msg_send(&reply, responder, ZMQ_SNDMORE);
							zmq_msg_send(&reply_buffer, responder, 0);
							zmq_msg_close(&message);
							zmq_msg_close(&message_buffer);
							zmq_msg_close(&reply);
							break;
						}
		}
	}
}



