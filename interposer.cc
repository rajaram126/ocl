#include <stdio.h>
#include <CL/cl.h>
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


// move to config file
char protocol[] = "tcp";
char  port[] = "5555";
char *nodes[] = {"127.0.0.1"};


int connect_zmq(char * node, void * requester) {
	char conn_str[50];
	sprintf(conn_str,"%s://%s:%s",protocol,node,port);
	return zmq_connect (requester, conn_str);
}

void invoke_zmq(void * requester, zmq_msg_t * header, zmq_msg_t* message, zmq_msg_t* send_buffer, zmq_msg_t * reply, zmq_msg_t * reply_buffer){
	zmq_msg_send(header, requester, ZMQ_SNDMORE);
	zmq_msg_send(message, requester, ZMQ_SNDMORE);
	zmq_msg_send(send_buffer, requester, 0);
	zmq_msg_recv(reply, requester, 0);
	zmq_msg_recv(reply_buffer, requester, 0);
}

void invoke_zmq(void * requester, zmq_msg_t * header, zmq_msg_t* message, zmq_msg_t* send_buffer,zmq_msg_t* send_next_buffer, zmq_msg_t * reply, zmq_msg_t * reply_buffer){
	zmq_msg_send(header, requester, ZMQ_SNDMORE);
	zmq_msg_send(message, requester, ZMQ_SNDMORE);
	zmq_msg_send(send_buffer, requester, ZMQ_SNDMORE);
	zmq_msg_send(send_next_buffer, requester, 0);
	zmq_msg_recv(reply, requester, 0);
	zmq_msg_recv(reply_buffer, requester, 0);
}


void invoke_zmq(void * requester, zmq_msg_t * header, zmq_msg_t* message, zmq_msg_t* send_buffer,zmq_msg_t* send_aux_buffer_1,zmq_msg_t* send_aux_buffer_2, zmq_msg_t * reply, zmq_msg_t * reply_buffer){
	zmq_msg_send(header, requester, ZMQ_SNDMORE);
	zmq_msg_send(message, requester, ZMQ_SNDMORE);
	zmq_msg_send(send_buffer, requester, ZMQ_SNDMORE);
	zmq_msg_send(send_aux_buffer_1, requester, ZMQ_SNDMORE);
	zmq_msg_send(send_aux_buffer_2, requester, 0);
	zmq_msg_recv(reply, requester, 0);
	zmq_msg_recv(reply_buffer, requester, 0);
}

void cleanup_messages(zmq_msg_t * header, zmq_msg_t* message, zmq_msg_t* send_buffer, zmq_msg_t * reply, zmq_msg_t * reply_buffer) {
	zmq_msg_close(message);
	zmq_msg_close(send_buffer);
	zmq_msg_close(reply);
	zmq_msg_close(reply_buffer);
	zmq_msg_close(header);
}


cl_int clGetPlatformIDs (cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms){

//Don't send the RPC if the args are invalid
	if(!num_entries && platforms){
		return CL_INVALID_VALUE;
	}

	if(!num_platforms && !platforms){
		return CL_INVALID_VALUE;
	}

	int num_nodes = sizeof(nodes)/sizeof(char *);
	

	int num_platforms_found = 0;
 	std::map<int, int> num_platforms_per_node;
 	std::map<int, char*> platforms_per_node;

	cl_int err = CL_SUCCESS;	

	for(int i=0; i<num_nodes; i++){

		get_platform_ids_ arg_pkt, ret_pkt;
		memset((char *)&arg_pkt, 0, sizeof(get_platform_ids_));
		memset((char *)&ret_pkt, 0, sizeof(get_platform_ids_));

		arg_pkt.platforms.buff_ptr = "\0";
		arg_pkt.platforms.buff_len = sizeof(char);

		ret_pkt.platforms.buff_ptr = NULL;


		void *context = zmq_ctx_new ();
    		void *requester = zmq_socket (context, ZMQ_REQ);
		connect_zmq(nodes[i], requester);
		zmq_msg_t header,message,message_buffer,reply,reply_buffer;

		invocation_header hd;
		hd.api_id = GET_PLATFORM_IDS;
		zmq_msg_init_size(&header, sizeof(hd));
		memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
		zmq_msg_init_size(&message, sizeof(arg_pkt));
		memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
		zmq_msg_init_size(&message_buffer, sizeof(char));
		memcpy(zmq_msg_data(&message_buffer), arg_pkt.platforms.buff_ptr, sizeof(char));
		zmq_msg_init(&reply);
		zmq_msg_init(&reply_buffer);
		invoke_zmq(requester,&header, &message, &message_buffer, &reply,&reply_buffer);
		
		ret_pkt = * (get_platform_ids_*) zmq_msg_data(&reply);
		//Todo free
		ret_pkt.platforms.buff_ptr = (char *) malloc(ret_pkt.platforms.buff_len);
		memcpy(ret_pkt.platforms.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.platforms.buff_len);
		cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer); 				
		int num_platforms_found_curr_node = ret_pkt.num_platforms_found;
		//todo : check for error
		if(num_platforms_found_curr_node){
			char *platforms_found_curr_node = ret_pkt.platforms.buff_ptr;
			num_platforms_found += num_platforms_found_curr_node;
			num_platforms_per_node.insert(std::pair<int, int>(i, num_platforms_found_curr_node));
			platforms_per_node.insert(std::pair<int, char*>(i, platforms_found_curr_node));

			for(int j=0; j<num_platforms_found_curr_node; j++){
				printf("[clGetPlatformIDs interposed] platforms[%d]=%p\n", j, *((cl_platform_id*)platforms_found_curr_node+j));
			}
		}
	zmq_close (requester);
    	zmq_ctx_destroy (context);

	}

	if(num_platforms){
		*num_platforms = num_platforms_found;
	}

	if(!platforms || !num_platforms_found){
		return err;
	}

	int num_entries_found = 0;
	for(int i=0; i<num_nodes; i++){

		std::map<int, int>::iterator it1 = num_platforms_per_node.find(i);
		if(it1 == num_platforms_per_node.end()){
			continue;
		}

		int num_platforms_curr_node = it1->second;

		int num_entries_needed = 0;
		if(num_entries_found + num_platforms_curr_node > num_entries){
			num_entries_needed = num_entries - num_entries_found;
		}else{
                        num_entries_needed = num_platforms_curr_node;
		}

		std::map<int, char*>::iterator it2 = platforms_per_node.find(i);
		assert(it2 != platforms_per_node.end());
		cl_platform_id *platforms_curr_node = (cl_platform_id *)(it2->second);

		for(int j=0; j<num_entries_needed; j++){
			cl_platform_id_ *platform_distr = (cl_platform_id_ *)malloc(sizeof(cl_platform_id_));
			platform_distr->node = nodes[i];
			platform_distr->clhandle = platforms_curr_node[j];
			printf("[clGetPlatformIDs interposed] platforms[%d] = %p\n", num_entries_found + j, platform_distr->clhandle);
		 	platforms[num_entries_found + j] = (cl_platform_id)platform_distr;	
		}
		num_entries_found += num_entries_needed;

	}	

	return CL_SUCCESS;

}


cl_int clGetDeviceIDs (cl_platform_id platform,cl_device_type device_type, cl_uint num_entries,cl_device_id *devices, cl_uint *num_devices){


//Don't send the RPC if the args are invalid
	if(!num_entries && devices){
		return CL_INVALID_VALUE;
	}

	if(!num_devices && !devices){
		return CL_INVALID_VALUE;
	}

	int num_devices_found = 0;

	cl_int err = CL_SUCCESS;	

	cl_platform_id_ *platform_distr = (cl_platform_id_ *)platform;

	char *node = platform_distr->node;
	cl_platform_id clhandle = platform_distr->clhandle;

	get_device_ids_ arg_pkt, ret_pkt;

	//Client receives this from the server - so this is a hack to keep the rpc libray from crashing
	//send one char , that is, pass an initialized pointer, which gets ignored
	arg_pkt.devices.buff_ptr = "\0";
	arg_pkt.devices.buff_len = sizeof(char);

	ret_pkt.devices.buff_ptr = NULL;

	arg_pkt.platform = (unsigned long)clhandle;
	arg_pkt.device_type = (int)device_type;
	
	void *context = zmq_ctx_new ();
        void *requester = zmq_socket (context, ZMQ_REQ);
        connect_zmq(node , requester);
        zmq_msg_t header,message,message_buffer,reply,reply_buffer;
	invocation_header hd;
        hd.api_id = GET_DEVICE_IDS;
        zmq_msg_init_size(&header, sizeof(hd));
        memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
        zmq_msg_init_size(&message, sizeof(arg_pkt));
        memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
        zmq_msg_init_size(&message_buffer, sizeof(char));
        memcpy(zmq_msg_data(&message_buffer), arg_pkt.devices.buff_ptr, sizeof(char));
        zmq_msg_init(&reply);
        zmq_msg_init(&reply_buffer);
        invoke_zmq(requester,&header, &message, &message_buffer, &reply,&reply_buffer);

        ret_pkt = * (get_device_ids_*) zmq_msg_data(&reply);
        //Todo free
        ret_pkt.devices.buff_ptr = (char *) malloc(ret_pkt.devices.buff_len);
        memcpy(ret_pkt.devices.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.devices.buff_len);
	cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);
zmq_close (requester);
    zmq_ctx_destroy (context);
	printf("[clGetPlatformIDs interposed] num_devices_found %d\n", ret_pkt.num_devices_found);

	cl_device_id *device_list = (cl_device_id *)malloc(ret_pkt.num_devices_found * sizeof(cl_device_id));
	char *device_list_returned = ret_pkt.devices.buff_ptr;

	if(ret_pkt.num_devices_found && (ret_pkt.err==CL_SUCCESS)){

		num_devices_found += ret_pkt.num_devices_found;

		for(int i=0; i<num_devices_found; i++){
			device_list[i] = *((cl_device_id*)device_list_returned+i);
			printf("[clGetDeviceIDs interposed] devices[%d]=%p\n", i, device_list[i]);
		}
	}


	if(num_devices){
		*num_devices = num_devices_found;
	}

	if(!devices || !num_devices_found){
		return err;
	}

	for(int i=0; i<num_entries; i++){

		/*If the application asks for more devices than there is on the given platform, then just return the ones that are available and set the rest of the devices array to zero*/
		if(i >= num_devices_found){
			devices[i] = 0;
			continue;
		}

		cl_device_id_ *device_distr = (cl_device_id_ *)malloc(sizeof(cl_device_id_));
		device_distr->node = node;
		device_distr->clhandle = device_list[i];
		printf("[clGetDeviceIDs interposed] devices[%d] = %p\n", i, device_distr->clhandle);
		devices[i] = (cl_device_id)device_distr;	

	}	

	//strictly speaking, this should not return a CL_SUCCESS if devices asked for > devices available
	return CL_SUCCESS;

}


cl_context clCreateContext (const cl_context_properties *properties,cl_uint num_devices, const cl_device_id *devices,void (CL_CALLBACK*pfn_notify)(const char *errinfo, const void *private_info,size_t cb, void *user_data),void *user_data, cl_int *errcode_ret){

	//Don't send the RPC if the args are invalid
	if(!devices || !num_devices){
		*errcode_ret = CL_INVALID_VALUE;
		return 0;
	}

	printf("[clCreateContext interposed] num_devices %d\n", num_devices);

	cl_int err = CL_SUCCESS;	

	std::map <char *, std::vector<cl_device_id> > devicevector_per_node;

	for(int i=0; i<num_devices; i++){

		cl_device_id_ *device_distr = (cl_device_id_ *)devices[i];

		char *node = device_distr->node;

		cl_device_id clhandle = device_distr->clhandle;

		std::map<char *, std::vector<cl_device_id> >::iterator it = devicevector_per_node.find(node);

		if(it == devicevector_per_node.end()){
			std::vector<cl_device_id> devicevector;
			devicevector.push_back(clhandle);
			devicevector_per_node.insert(make_pair(node, devicevector));
		} else {
			std::vector<cl_device_id> devicevector = it->second;
			devicevector.push_back(clhandle);
			devicevector_per_node[node] = devicevector;
		}

	}

	int num_context_tuples = devicevector_per_node.size();
	printf("[clCreateContext interposed] num_context_tuples %d\n", num_context_tuples);

	cl_context_ *context_distr = (cl_context_ *)malloc(sizeof(cl_context_));
	context_distr->context_tuples = (cl_context_elem_ *)malloc(sizeof(cl_context_elem_)*num_context_tuples);
	context_distr->num_context_tuples = num_context_tuples;

	int tuple_counter = 0;
	for(std::map <char *, std::vector<cl_device_id> >::iterator it=devicevector_per_node.begin();
		it != devicevector_per_node.end();
		it++ ){

		create_context_ arg_pkt, ret_pkt;
		memset((char *)&arg_pkt, 0, sizeof(create_context_));
		memset((char *)&ret_pkt, 0, sizeof(create_context_));

		std::vector<cl_device_id> devicevector = it->second;

		arg_pkt.num_devices = devicevector.size();

		cl_device_id *device_list = (cl_device_id *)malloc(arg_pkt.num_devices * sizeof(cl_device_id));

		int device_count = 0;
		for(std::vector<cl_device_id>::iterator itv=devicevector.begin();
			itv != devicevector.end();
			itv++){

			device_list[device_count++] = (*itv);
			printf("[clCreateContext interposed] clhandle %p\n", (*itv));

		}
		assert(devicevector.size() == device_count);

		arg_pkt.devices.buff_ptr = (char *)device_list;
		arg_pkt.devices.buff_len = sizeof(cl_device_id)*device_count;

		ret_pkt.devices.buff_ptr = NULL;
		char *curr_node = it->first;

		void *context = zmq_ctx_new ();
        	void *requester = zmq_socket (context, ZMQ_REQ);
        	connect_zmq(curr_node , requester);
        	zmq_msg_t header,message,message_buffer,reply,reply_buffer;
        	invocation_header hd;
        	hd.api_id = CREATE_CONTEXT;
        	zmq_msg_init_size(&header, sizeof(hd));
        	memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
        	zmq_msg_init_size(&message, sizeof(arg_pkt));
        	memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
        	zmq_msg_init_size(&message_buffer,arg_pkt.devices.buff_len);
        	memcpy(zmq_msg_data(&message_buffer), arg_pkt.devices.buff_ptr, arg_pkt.devices.buff_len);
        	zmq_msg_init(&reply);
        	zmq_msg_init(&reply_buffer);
        	invoke_zmq(requester,&header, &message, &message_buffer, &reply,&reply_buffer);

       	 	ret_pkt = * (create_context_*) zmq_msg_data(&reply);
        	//Todo free
        	ret_pkt.devices.buff_ptr = (char *) malloc(ret_pkt.devices.buff_len);
        	memcpy(ret_pkt.devices.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.devices.buff_len);
        	cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);
		//context_distr->context_tuples[tuple_counter] = (cl_context_elem_ *)malloc(sizeof(cl_context_elem_));
		context_distr->context_tuples[tuple_counter].clhandle = (cl_context)(ret_pkt.context);
		context_distr->context_tuples[tuple_counter].node = curr_node;

		zmq_close (requester);
    		zmq_ctx_destroy (context);

		tuple_counter++;

		printf("[clCreateContext interposed] context returned %p\n", ret_pkt.context);
		err |= ret_pkt.err;

	}

	*errcode_ret = err;

	return (cl_context)context_distr;
}



cl_command_queue clCreateCommandQueue (cl_context context, cl_device_id device,cl_command_queue_properties properties,cl_int *errcode_ret){

	char *context_node;

	cl_device_id_ *device_distr = (cl_device_id_ *)device;
	char *device_node = device_distr->node;
	cl_device_id device_clhandle = device_distr->clhandle;

	cl_context_ *context_distr = (cl_context_ *)context;

	int node_match_index = 0;
	for(int i=0; i<context_distr->num_context_tuples; i++){

		context_node = context_distr->context_tuples[i].node;

		if(context_node != device_node){
			continue;
		}

		node_match_index = i;
		break;

	}

	assert(device_node == context_node);

	cl_context context_clhandle = context_distr->context_tuples[node_match_index].clhandle;


	create_command_queue_ arg_pkt, ret_pkt;

	//device_node
	arg_pkt.context = (unsigned long)context_clhandle;
	arg_pkt.device = (unsigned long)device_clhandle;

	void *context_t = zmq_ctx_new ();
        void *requester = zmq_socket (context_t, ZMQ_REQ);
        connect_zmq(device_node , requester);
        zmq_msg_t header,message,message_buffer,reply,reply_buffer;
        invocation_header hd;
        hd.api_id = CREATE_COMMAND_QUEUE;
        zmq_msg_init_size(&header, sizeof(hd));
        memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
        zmq_msg_init_size(&message, sizeof(arg_pkt));
        memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
        zmq_msg_init_size(&message_buffer,sizeof(char));
        memcpy(zmq_msg_data(&message_buffer), "\0", sizeof(char));
        zmq_msg_init(&reply);
        zmq_msg_init(&reply_buffer);
        invoke_zmq(requester,&header, &message, &message_buffer, &reply,&reply_buffer);

        ret_pkt = * (create_command_queue_*) zmq_msg_data(&reply);

        cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);

zmq_close (requester);
    zmq_ctx_destroy (context);
	printf("[clCreateCommandQueue interposed] command queue returned %p\n", ret_pkt.command_queue);

	cl_command_queue_ *command_queue_distr = (cl_command_queue_ *)malloc(sizeof(cl_command_queue_));
	command_queue_distr->clhandle = (cl_command_queue)(ret_pkt.command_queue);
	command_queue_distr->node = device_node;
	*errcode_ret = ret_pkt.err;
	return (cl_command_queue)command_queue_distr;

}

std::vector<cl_mem_*> mems_created;

cl_mem clCreateBuffer (cl_context context, cl_mem_flags flags, size_t size, void *host_ptr, cl_int *errcode_ret){

	cl_int err = CL_SUCCESS;

	cl_context_ *context_distr = (cl_context_ *)context;
	int num_tuples = context_distr->num_context_tuples;

	cl_mem_ *mem_distr = (cl_mem_ *)malloc(sizeof(cl_mem_));
	mem_distr->mem_tuples = (cl_mem_elem_ *)malloc(num_tuples * sizeof(cl_mem_elem_));
	mem_distr->num_mem_tuples = num_tuples;

	//WORKAROUND
	printf("[clCreateBuffer interposed] mem_distr %p\n", mem_distr);
	printf("[clCreateBuffer interposed] num_tuples %d\n", num_tuples);
	mems_created.push_back(mem_distr);

	bool send_data = flags & CL_MEM_COPY_HOST_PTR;

	for(int i=0; i<context_distr->num_context_tuples; i++){

		char *node = context_distr->context_tuples[i].node;
		printf("[clCreateBuffer interposed] node %s\n", node);

		cl_context context_clhandle = context_distr->context_tuples[i].clhandle;
		printf("[clCreateBuffer interposed] context %p\n", context_clhandle);

		
		create_buffer_ arg_pkt, ret_pkt;
		memset((char *)&arg_pkt, 0, sizeof(create_buffer_));
		memset((char *)&ret_pkt, 0, sizeof(create_buffer_));

		arg_pkt.context = (unsigned long)context_clhandle;
		arg_pkt.flags = flags;
		arg_pkt.size = size;

		if(send_data){
			assert(host_ptr != NULL);
			arg_pkt.data.buff_ptr = (char *)host_ptr;
			arg_pkt.data.buff_len = size;
		} else {
			arg_pkt.data.buff_ptr = "\0";
			arg_pkt.data.buff_len = sizeof(char);
		}

		ret_pkt.data.buff_ptr = NULL;
		void *context = zmq_ctx_new ();
        	void *requester = zmq_socket (context, ZMQ_REQ);
        	connect_zmq(node , requester);
        	zmq_msg_t header,message,message_buffer,reply,reply_buffer;
        	invocation_header hd;
        	hd.api_id = CREATE_BUFFER;
        	zmq_msg_init_size(&header, sizeof(hd));
        	memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
        	zmq_msg_init_size(&message, sizeof(arg_pkt));
        	memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
        	zmq_msg_init_size(&message_buffer,arg_pkt.data.buff_len);
        	memcpy(zmq_msg_data(&message_buffer), arg_pkt.data.buff_ptr,arg_pkt.data.buff_len);
        	zmq_msg_init(&reply);
        	zmq_msg_init(&reply_buffer);
        	invoke_zmq(requester,&header, &message, &message_buffer, &reply,&reply_buffer);

        	ret_pkt = * (create_buffer_*) zmq_msg_data(&reply);
        	//Todo free
        	ret_pkt.data.buff_ptr = (char *) malloc(ret_pkt.data.buff_len);
        	memcpy(ret_pkt.data.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.data.buff_len);
        	cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);
zmq_close (requester);
    zmq_ctx_destroy (context);
		printf("[clCreateBuffer interposed] mem returned %p\n", ret_pkt.mem);

		mem_distr->mem_tuples[i].clhandle = (cl_mem)(ret_pkt.mem);
		mem_distr->mem_tuples[i].node = node;
		printf("[clCreateBuffer interposed] mem_distr->mem_tuples[%d].clhandle %p\n",i, mem_distr->mem_tuples[i].clhandle);
		printf("[clCreateBuffer interposed] mem_distr->mem_tuples[%d].node %s\n",i, mem_distr->mem_tuples[i].node);
		err |= ret_pkt.err;

	}

	*errcode_ret = err;
	printf("[clCreateBuffer interposed] cl_mem_ * returned %p\n", mem_distr);
	printf("[clCreateBuffer interposed] mem_distr->num_mem_tuples %d\n", mem_distr->num_mem_tuples);
	return (cl_mem)mem_distr;
}


cl_program clCreateProgramWithSource (cl_context context, cl_uint count, const char **strings,const size_t *lengths, cl_int *errcode_ret){

	cl_program program = 0;
	cl_int err = CL_SUCCESS;

	if(!count){
	   *errcode_ret = CL_INVALID_VALUE;
	   return 0;
	}

	if(!strings){
	   *errcode_ret = CL_INVALID_VALUE;
	   return 0;
	}

	int program_str_size = 0;
	for(int i=0; i<count; i++){
            const char *string = strings[i];
            if(!string){
		*errcode_ret = CL_INVALID_VALUE;
                return 0;
            }
	    if((!lengths) || (!lengths[i])){
		program_str_size += strlen(string);
	    } else {
		program_str_size += lengths[i];	
	    }	    
	}
	printf("[clCreateProgramWithSource interposed] program_str_size %d\n", program_str_size);

	char *program_str = (char *)malloc(program_str_size);
	int program_str_offset = 0;
	for(int i=0; i<count; i++){
	    const char *string = strings[i];
	    assert(string != NULL);
	    if((!lengths) || (!lengths[i])){
		strcpy(program_str + program_str_offset, string);
		program_str_offset += strlen(string);
	    } else {
		char *program_str_curr = program_str + program_str_offset;
		for(int j=0; j<lengths[i]; j++){
			program_str_curr[j] = string[j];
		}
		program_str_offset += lengths[i];
	    }
	}
	program_str[program_str_offset] = '\0';//Null terminate the string

	cl_context_ *context_distr = (cl_context_ *)context;
	int num_tuples = context_distr->num_context_tuples;

	cl_program_ *program_distr = (cl_program_ *)malloc(sizeof(cl_program_));
	program_distr->program_tuples = (cl_program_elem_ *)malloc(num_tuples * sizeof(cl_program_elem_));
	program_distr->num_program_tuples = num_tuples;

	for(int i=0; i<context_distr->num_context_tuples; i++){

		char *node = context_distr->context_tuples[i].node;
		printf("[clCreateProgramWithSource interposed] node %s\n", node);

		cl_context context_clhandle = context_distr->context_tuples[i].clhandle;
		printf("[clCreateProgramWithSource interposed] context %p\n", context_clhandle);

		create_program_with_source_ arg_pkt, ret_pkt;

		arg_pkt.context = (unsigned long)context_clhandle;
		arg_pkt.program_str.buff_ptr = program_str;
		arg_pkt.program_str.buff_len = program_str_offset+1;//The extra 1 accounts for the null char

                void *context = zmq_ctx_new ();
                void *requester = zmq_socket (context, ZMQ_REQ);
                connect_zmq(node , requester);
                zmq_msg_t header,message,message_buffer,reply,reply_buffer;
                invocation_header hd;
                hd.api_id = CREATE_PROGRAM_WITH_SOURCE;
                zmq_msg_init_size(&header, sizeof(hd));
                memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
                zmq_msg_init_size(&message, sizeof(arg_pkt));
                memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
                zmq_msg_init_size(&message_buffer,arg_pkt.program_str.buff_len);
                memcpy(zmq_msg_data(&message_buffer), arg_pkt.program_str.buff_ptr,arg_pkt.program_str.buff_len);
                zmq_msg_init(&reply);
                zmq_msg_init(&reply_buffer);
                invoke_zmq(requester,&header, &message, &message_buffer, &reply,&reply_buffer);

                ret_pkt = * (create_program_with_source_*) zmq_msg_data(&reply);
                //Todo free
                ret_pkt.program_str.buff_ptr = (char *) malloc(ret_pkt.program_str.buff_len);
                memcpy(ret_pkt.program_str.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.program_str.buff_len);
                cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);
zmq_close (requester);
    zmq_ctx_destroy (context);

		printf("[clCreateProgramWithSource interposed] program returned %p\n", ret_pkt.program);

		program_distr->program_tuples[i].clhandle = (cl_program)(ret_pkt.program);
		program_distr->program_tuples[i].node = node;

		printf("[clCreateProgramWithSource interposed] program_tuple[%d].clhandle %p\n", i, program_distr->program_tuples[i].clhandle);
		printf("[clCreateProgramWithSource interposed] program_tuple[%d].node %s\n", i, program_distr->program_tuples[i].node);

		err |= ret_pkt.err;

	}


	*errcode_ret = err;
	return (cl_program)program_distr;

	//*errcode_ret = CL_SUCCESS;
	//return program;
}

cl_int clBuildProgram (cl_program program, cl_uint num_devices, const cl_device_id *device_list, const char *options, void (CL_CALLBACK*pfn_notify)(cl_program program, void *user_data), void *user_data){

        if((!device_list && num_devices)&&(device_list && !num_devices)){
                return CL_INVALID_VALUE;
        }

        cl_int err = CL_SUCCESS;

        cl_program_ *program_distr = (cl_program_ *)program;

	for(int i=0; i<program_distr->num_program_tuples; i++ ){

		printf("[clBuildProgram interposed] program_tuple[%d].clhandle %p\n", i, program_distr->program_tuples[i].clhandle);
		printf("[clBuildProgram interposed] program_tuple[%d].node %s\n", i, program_distr->program_tuples[i].node);
	}

	//-cl-kernel-arg-info not supported in the compiler version installed
	//WORKAROUND added in clCreateBuffer
	//char *kernel_arg_option = "-cl-kernel-arg-info";
	char *kernel_arg_option = "";

	int tot_options_size;
	if(options){
	  tot_options_size = strlen(kernel_arg_option) + strlen(options);
	} else {
          tot_options_size = strlen(kernel_arg_option);
	}	

	char tot_options[tot_options_size+1];

	if(options){
		strcpy(tot_options, options);
		tot_options[strlen(options)] = ' ';
		for(int i=0; i<strlen(kernel_arg_option); i++){
			tot_options[strlen(options)+1+i] = kernel_arg_option[i];	
		}
	} else {
		for(int i=0; i<strlen(kernel_arg_option); i++){
			tot_options[i] = kernel_arg_option[i];
		}
	}

	tot_options[tot_options_size] = '\0';


	if(!device_list){ //Build the program for all devices in the context

		for(int j=0; j<program_distr->num_program_tuples; j++){


			build_program_ arg_pkt, ret_pkt;

			arg_pkt.all_devices = TRUE;
			arg_pkt.devices.buff_ptr = "\0";
			arg_pkt.devices.buff_len = sizeof(char);

			arg_pkt.options.buff_ptr = tot_options;
			arg_pkt.options.buff_len = tot_options_size;

			ret_pkt.devices.buff_ptr = NULL;
			ret_pkt.options.buff_ptr = NULL;

			arg_pkt.program = (unsigned long)(program_distr->program_tuples[j].clhandle);
			printf("[clBuildProgram interposed] program %p", arg_pkt.program);

			char *curr_node = program_distr->program_tuples[j].node;

			void *context = zmq_ctx_new ();
                	void *requester = zmq_socket (context, ZMQ_REQ);
                	connect_zmq(curr_node , requester);
                	zmq_msg_t header,message,message_buffer,message_aux_buffer,reply,reply_buffer;
                	invocation_header hd;
                	hd.api_id = BUILD_PROGRAM;
                	zmq_msg_init_size(&header, sizeof(hd));
                	memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
                	zmq_msg_init_size(&message, sizeof(arg_pkt));
                	memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
                	zmq_msg_init_size(&message_buffer,arg_pkt.devices.buff_len);
                	memcpy(zmq_msg_data(&message_buffer), arg_pkt.devices.buff_ptr,arg_pkt.devices.buff_len);
			zmq_msg_init_size(&message_aux_buffer,arg_pkt.options.buff_len);
                	memcpy(zmq_msg_data(&message_aux_buffer), arg_pkt.options.buff_ptr,arg_pkt.options.buff_len);
                	zmq_msg_init(&reply);
                	zmq_msg_init(&reply_buffer);
                	invoke_zmq(requester,&header, &message, &message_buffer,&message_aux_buffer, &reply,&reply_buffer);

                	ret_pkt = * (build_program_*) zmq_msg_data(&reply);
                	//Todo free
                	ret_pkt.options.buff_ptr = (char *) malloc(ret_pkt.options.buff_len);
                	memcpy(ret_pkt.options.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.options.buff_len);
                	cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);

zmq_close (requester);
    zmq_ctx_destroy (context);
			printf("[clBuildProgram interposed]clnt_call OK\n");
			err |= ret_pkt.err;

		}

		return err;
	}

	//Arrange the device list passed across per node aggregates
        std::map <char *, std::vector<cl_device_id> > devicevector_per_node;

        for(int i=0; i<num_devices; i++){

                cl_device_id_ *device_distr = (cl_device_id_ *)device_list[i];

                char *node = device_distr->node;

                cl_device_id clhandle = device_distr->clhandle;

                std::map<char *, std::vector<cl_device_id> >::iterator it = devicevector_per_node.find(node);

                if(it == devicevector_per_node.end()){
                        std::vector<cl_device_id> devicevector;
                        devicevector.push_back(clhandle);
                        devicevector_per_node.insert(make_pair(node, devicevector));
                } else {
                        std::vector<cl_device_id> devicevector = it->second;
                        devicevector.push_back(clhandle);
                        devicevector_per_node[node] = devicevector;
                }

        }


        int tuple_counter = 0;
        for(std::map <char *, std::vector<cl_device_id> >::iterator it=devicevector_per_node.begin();
                it != devicevector_per_node.end();
                it++ ){


                build_program_ arg_pkt, ret_pkt;


                std::vector<cl_device_id> devicevector = it->second;

                arg_pkt.num_devices = devicevector.size();

                cl_device_id *device_list = (cl_device_id *)malloc(arg_pkt.num_devices * sizeof(cl_device_id));

                int device_count = 0;
                for(std::vector<cl_device_id>::iterator itv=devicevector.begin();
                        itv != devicevector.end();
                        itv++){

                        device_list[device_count++] = (*itv);
                        printf("[clBuildProgram interposed] device %p\n", (*itv));

                }
                assert(devicevector.size() == device_count);

		arg_pkt.all_devices = FALSE;
		arg_pkt.devices.buff_ptr = (char *)device_list;
                arg_pkt.devices.buff_len = sizeof(cl_device_id)*device_count;

		arg_pkt.options.buff_ptr = tot_options;
		arg_pkt.options.buff_len = tot_options_size;

		printf("[clBuildProgram interposed] options %s len %d len(from strlen) %d\n", arg_pkt.options.buff_ptr, arg_pkt.options.buff_len, strlen(arg_pkt.options.buff_ptr));

                ret_pkt.devices.buff_ptr = NULL;
                ret_pkt.options.buff_ptr = NULL;

                char *curr_node = it->first;
		printf("[clBuildProgram interposed] devices on node %s\n", curr_node);

		//Can replace this search repititively by first arranging program_distr into a map
		//and then doing a find each time
		for(int j=0; j<program_distr->num_program_tuples; j++){
			if(curr_node != program_distr->program_tuples[j].node){
				continue;
			}
			//printf("[clBuildProgram interposed] program_tuple[%d].clhandle %p\n", j, program_distr->program_tuples[j].clhandle);
			//printf("[clBuildProgram interposed] program_tuple[%d].node %s\n", j, program_distr->program_tuples[j].node);
			arg_pkt.program = (unsigned long)(program_distr->program_tuples[j].clhandle);
			//printf("[clBuildProgram interposed] program %p\n", arg_pkt.program);
			break;
		}			
		printf("[clBuildProgram interposed] program %p\n", arg_pkt.program);

		void *context = zmq_ctx_new ();
                void *requester = zmq_socket (context, ZMQ_REQ);
                connect_zmq(curr_node , requester);
                zmq_msg_t header,message,message_buffer,message_aux_buffer,reply,reply_buffer;
                invocation_header hd;
                hd.api_id = BUILD_PROGRAM_AUX;
                zmq_msg_init_size(&header, sizeof(hd));
                memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
                zmq_msg_init_size(&message, sizeof(arg_pkt));
                memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
                zmq_msg_init_size(&message_buffer,arg_pkt.devices.buff_len);
                memcpy(zmq_msg_data(&message_buffer), arg_pkt.devices.buff_ptr,arg_pkt.devices.buff_len);
		zmq_msg_init_size(&message_aux_buffer,arg_pkt.options.buff_len);
                memcpy(zmq_msg_data(&message_aux_buffer), arg_pkt.options.buff_ptr,arg_pkt.options.buff_len);
                zmq_msg_init(&reply);
                zmq_msg_init(&reply_buffer);
                invoke_zmq(requester,&header, &message, &message_buffer,&message_aux_buffer, &reply,&reply_buffer);

                ret_pkt = * (build_program_*) zmq_msg_data(&reply);
                	//Todo free
                ret_pkt.options.buff_ptr = (char *) malloc(ret_pkt.options.buff_len);
                memcpy(ret_pkt.options.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.options.buff_len);
                cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);
	
zmq_close (requester);
    zmq_ctx_destroy (context);
                printf("[clBuildProgram interposed]clnt_call OK\n");
                err |= ret_pkt.err;

        }

        return err;

}


cl_kernel clCreateKernel (cl_program program,const char *kernel_name, cl_int *errcode_ret){

	//cl_kernel kernel = 0;

	cl_int err = CL_SUCCESS;
	cl_program_ *program_distr = (cl_program_ *)program;
	int num_tuples = program_distr->num_program_tuples;

	cl_kernel_ *kernel_distr = (cl_kernel_ *)malloc(sizeof(cl_kernel_));
	kernel_distr->kernel_tuples = (cl_kernel_elem_ *)malloc(num_tuples * sizeof(cl_kernel_elem_));
	kernel_distr->num_kernel_tuples = num_tuples;

	for(int i=0; i<program_distr->num_program_tuples; i++){

		char *node = program_distr->program_tuples[i].node;
		printf("[clCreateKernel interposed] node %s\n", node);

		cl_program program_clhandle = program_distr->program_tuples[i].clhandle;
		printf("[clCreateKernel interposed] program %p\n", program_clhandle);



		create_kernel_ arg_pkt, ret_pkt;
		memset((char *)&arg_pkt, 0, sizeof(create_kernel_));
		memset((char *)&ret_pkt, 0, sizeof(create_kernel_));

		arg_pkt.program = (unsigned long)program_clhandle;
		arg_pkt.kernel_name.buff_ptr = (char *)kernel_name;
		arg_pkt.kernel_name.buff_len = strlen(kernel_name);

		void *context = zmq_ctx_new ();
                void *requester = zmq_socket (context, ZMQ_REQ);
                connect_zmq(node , requester);
                zmq_msg_t header,message,message_buffer,reply,reply_buffer;
                invocation_header hd;
                hd.api_id = CREATE_KERNEL;
                zmq_msg_init_size(&header, sizeof(hd));
                memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
                zmq_msg_init_size(&message, sizeof(arg_pkt));
                memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
                zmq_msg_init_size(&message_buffer,arg_pkt.kernel_name.buff_len);
                memcpy(zmq_msg_data(&message_buffer), arg_pkt.kernel_name.buff_ptr,arg_pkt.kernel_name.buff_len);
                zmq_msg_init(&reply);
                zmq_msg_init(&reply_buffer);
                invoke_zmq(requester,&header, &message, &message_buffer, &reply,&reply_buffer);

                ret_pkt = * (create_kernel_*) zmq_msg_data(&reply);
                //Todo free
                ret_pkt.kernel_name.buff_ptr = (char *) malloc(ret_pkt.kernel_name.buff_len);
                memcpy(ret_pkt.kernel_name.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.kernel_name.buff_len);
                cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);

zmq_close (requester);
    zmq_ctx_destroy (context);
		printf("[clCreateKernel interposed] kernel returned %p\n", ret_pkt.kernel);

		kernel_distr->kernel_tuples[i].clhandle = (cl_kernel)(ret_pkt.kernel);
		kernel_distr->kernel_tuples[i].node = node;

		printf("[clCreateKernel interposed] kernel_tuple[%d].clhandle %p\n", i, kernel_distr->kernel_tuples[i].clhandle);
		printf("[clCreateKernel interposed] kernel_tuple[%d].node %s\n", i, kernel_distr->kernel_tuples[i].node);

		err |= ret_pkt.err;

	}


	*errcode_ret = err;
	return (cl_kernel)kernel_distr;

	//*errcode_ret = CL_SUCCESS;
	//return kernel;
}

cl_int clSetKernelArg (cl_kernel kernel, cl_uint arg_index,size_t arg_size, const void *arg_value){

	cl_kernel_ *kernel_distr = (cl_kernel_ *)kernel;
	int num_kernel_tuples = kernel_distr->num_kernel_tuples;
	cl_int err = CL_SUCCESS;

	bool is_null_arg = FALSE;
	if(!arg_value){
		is_null_arg = TRUE;
	}

	bool is_mem = FALSE, is_image = FALSE, is_sampler = FALSE;
	std::map<char*, cl_mem> mem_node_map;
	//std::map<char*, cl_image> image_node_map; //cl_image not supported for the version installed on shiva
	std::map<char*, cl_sampler> sampler_node_map;

	if(!is_null_arg){

		printf("[clSetKernelArg interposed] is NOT null arg\n");
		printf("[clSetKernelArg interposed] arg_value %p\n", *((cl_mem_ **)arg_value));
		std::vector<cl_mem_ *>::iterator it = find(mems_created.begin(), mems_created.end(), *((cl_mem_ **)arg_value)); //cl_mem_ was pushed into the mems_created vector...and now cl_mem_ * is received...so it has to be dereferenced first
		if(it != mems_created.end()){
			printf("[clSetKernelArg interposed] is mem\n");
			is_mem = TRUE;
			cl_mem_ *mem_distr = (*it);
			printf("[clSetKernelArg interposed] mem_distr %p\n", mem_distr);
			printf("[clSetKernelArg interposed] mem_distr->num_mem_tuples %d\n", mem_distr->num_mem_tuples);
			for(int i=0; i<mem_distr->num_mem_tuples; i++){
				mem_node_map.insert(std::pair<char*, cl_mem>(mem_distr->mem_tuples[i].node, mem_distr->mem_tuples[i].clhandle));
				printf("[clSetKernelArg interposed] node %s\n", mem_distr->mem_tuples[i].node);
				printf("[clSetKernelArg interposed] cl_mem %p\n", mem_distr->mem_tuples[i].clhandle);
			}
		}

		//add check for cl_image
		//add check for cl_sampler

	}

	bool is_clobj = FALSE;
	if(!is_null_arg && (is_mem || is_image || is_sampler)){
		is_clobj = TRUE;
	}

	for (int i=0; i<num_kernel_tuples; i++){

             

		set_kernel_arg_ arg_pkt, ret_pkt;


		arg_pkt.is_null_arg = is_null_arg;

		arg_pkt.is_clobj = is_clobj;
		arg_pkt.is_mem = is_mem;
		arg_pkt.is_image = is_image;
		arg_pkt.is_sampler = is_sampler;

		arg_pkt.arg_index = arg_index;

		arg_pkt.arg_size = arg_size;

		cl_kernel kernel_clhandle = kernel_distr->kernel_tuples[i].clhandle;
                printf("[clSetKernelArg interposed] kernel %p\n", kernel_clhandle);
		char *node = kernel_distr->kernel_tuples[i].node;
                printf("[clSetKernelArg interposed] node %s\n", node);

                arg_pkt.kernel = (unsigned long)kernel_clhandle;

		if(is_clobj || is_null_arg){
			if(is_clobj && is_mem){
				std::map<char*, cl_mem>::iterator it = mem_node_map.find(node);
				assert(it != mem_node_map.end());
				arg_pkt.mem = (unsigned long)(it->second);
				printf("[clSetKernelArg interposed] mem %p\n", (cl_mem)(arg_pkt.mem));
			}
			//for cl_image
			//for cl_sampler
			arg_pkt.plain_old_data.buff_ptr = "\0";
			arg_pkt.plain_old_data.buff_len = sizeof(char);
		} else {
			arg_pkt.plain_old_data.buff_ptr = (char *)arg_value;
			arg_pkt.plain_old_data.buff_len = arg_size;
		}

		ret_pkt.plain_old_data.buff_ptr = NULL;
		void *context = zmq_ctx_new ();
                void *requester = zmq_socket (context, ZMQ_REQ);
                connect_zmq(node , requester);
                zmq_msg_t header,message,message_buffer,reply,reply_buffer;
                invocation_header hd;
                hd.api_id = SET_KERNEL_ARG;
                zmq_msg_init_size(&header, sizeof(hd));
                memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
                zmq_msg_init_size(&message, sizeof(arg_pkt));
                memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
                zmq_msg_init_size(&message_buffer,arg_pkt.plain_old_data.buff_len);
                memcpy(zmq_msg_data(&message_buffer), arg_pkt.plain_old_data.buff_ptr,arg_pkt.plain_old_data.buff_len);
                zmq_msg_init(&reply);
                zmq_msg_init(&reply_buffer);
                invoke_zmq(requester,&header, &message, &message_buffer, &reply,&reply_buffer);

                ret_pkt = * (set_kernel_arg_*) zmq_msg_data(&reply);
                //Todo free
                ret_pkt.plain_old_data.buff_ptr = (char *) malloc(ret_pkt.plain_old_data.buff_len);
                memcpy(ret_pkt.plain_old_data.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.plain_old_data.buff_len);
                cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);

zmq_close (requester);
    zmq_ctx_destroy (context);

                printf("[clSetKernelArg interposed]clnt_call OK\n");
                err |= ret_pkt.err;

        }

	printf("[clSetKernelArg interposed] err %d\n", err);
        return err;

}

cl_int clEnqueueWriteBuffer (cl_command_queue command_queue, cl_mem buffer,cl_bool blocking_write, size_t offset, size_t size,const void *ptr, cl_uint num_events_in_wait_list,const cl_event *event_wait_list, cl_event *event){

	cl_int err = CL_SUCCESS;
	char *mem_node;

	cl_command_queue_ *command_queue_distr = (cl_command_queue_ *)command_queue;
	char *command_queue_node = command_queue_distr->node;
	cl_command_queue command_queue_clhandle = command_queue_distr->clhandle;

	cl_mem_ *mem_distr = (cl_mem_ *)buffer;
	printf("[clEnqueueWriteBuffer interposed] mem_distr %p\n", mem_distr);

	int node_match_index = 0;
	for(int i=0; i<mem_distr->num_mem_tuples; i++){

		mem_node = mem_distr->mem_tuples[i].node;
		printf("[clEnqueueWriteBuffer interposed] mem_distr->mem_tuples[%d].node %s\n", i, mem_distr->mem_tuples[i].node);
		printf("[clEnqueueWriteBuffer interposed] mem_distr->mem_tuples[%d].clhandle %p\n", i, mem_distr->mem_tuples[i].clhandle);

		if(mem_node != command_queue_node){
			continue;
		}

		node_match_index = i;
		break;

	}

	assert(mem_node == command_queue_node);

	cl_mem mem_clhandle = mem_distr->mem_tuples[node_match_index].clhandle;
	printf("[clEnqueueWriteBuffer interposed] mem_clhandle %p\n", mem_clhandle);



	enqueue_write_buffer_ arg_pkt, ret_pkt;

	arg_pkt.mem = (unsigned long)mem_clhandle;
	arg_pkt.command_queue = (unsigned long)command_queue_clhandle;

	arg_pkt.blocking = blocking_write;
	arg_pkt.size = size;
	arg_pkt.offset = offset;

	arg_pkt.data.buff_ptr = (char *)ptr;
	arg_pkt.data.buff_len = size;

	ret_pkt.data.buff_ptr = NULL;
	//
	void *context = zmq_ctx_new ();
        void *requester = zmq_socket (context, ZMQ_REQ);
        connect_zmq(command_queue_node , requester);
        zmq_msg_t header,message,message_buffer,reply,reply_buffer;
        invocation_header hd;
        hd.api_id = ENQUEUE_WRITE_BUFFER;
        zmq_msg_init_size(&header, sizeof(hd));
        memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
        zmq_msg_init_size(&message, sizeof(arg_pkt));
        memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));        
	zmq_msg_init_size(&message_buffer,arg_pkt.data.buff_len);
        memcpy(zmq_msg_data(&message_buffer), arg_pkt.data.buff_ptr,arg_pkt.data.buff_len);
        zmq_msg_init(&reply);
        zmq_msg_init(&reply_buffer);
        invoke_zmq(requester,&header, &message, &message_buffer, &reply,&reply_buffer);

        ret_pkt = * (enqueue_write_buffer_*) zmq_msg_data(&reply);
                //Todo free
        ret_pkt.data.buff_ptr = (char *) malloc(ret_pkt.data.buff_len);
        memcpy(ret_pkt.data.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.data.buff_len);
        cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);

zmq_close (requester);
    zmq_ctx_destroy (context);
	printf("[clEnqueueWriteBuffer interposed] err returned %d\n", ret_pkt.err);

	err = ret_pkt.err;
	return err;

}


cl_int clEnqueueReadBuffer (cl_command_queue command_queue, cl_mem buffer,cl_bool blocking_read, size_t offset, size_t size,  void *ptr, cl_uint num_events_in_wait_list,const cl_event *event_wait_list, cl_event *event){

	printf("[clEnqueueReadBuffer interposed] started...\n");

	cl_int err = CL_SUCCESS;
	char *mem_node;

	cl_command_queue_ *command_queue_distr = (cl_command_queue_ *)command_queue;
	char *command_queue_node = command_queue_distr->node;
	cl_command_queue command_queue_clhandle = command_queue_distr->clhandle;

	cl_mem_ *mem_distr = (cl_mem_ *)buffer;
	printf("[clEnqueueReadBuffer interposed] mem_distr %p\n", mem_distr);

	int node_match_index = 0;
	for(int i=0; i<mem_distr->num_mem_tuples; i++){

		mem_node = mem_distr->mem_tuples[i].node;
		printf("[clEnqueueReadBuffer interposed] mem_distr->mem_tuples[%d].node %s\n", i, mem_distr->mem_tuples[i].node);
		printf("[clEnqueueReadBuffer interposed] mem_distr->mem_tuples[%d].clhandle %p\n", i, mem_distr->mem_tuples[i].clhandle);

		if(mem_node != command_queue_node){
			continue;
		}

		node_match_index = i;
		break;

	}

	assert(mem_node == command_queue_node);

	cl_mem mem_clhandle = mem_distr->mem_tuples[node_match_index].clhandle;
	printf("[clEnqueueReadBuffer interposed] mem_clhandle %p\n", mem_clhandle);

	enqueue_read_buffer_ arg_pkt, ret_pkt;

	arg_pkt.mem = (unsigned long)mem_clhandle;
	arg_pkt.command_queue = (unsigned long)command_queue_clhandle;

	arg_pkt.blocking = blocking_read;
	arg_pkt.size = size;
	arg_pkt.offset = offset;

	arg_pkt.data.buff_ptr = "\0";
	arg_pkt.data.buff_len = sizeof(char);

	ret_pkt.data.buff_ptr = NULL;

	void *context = zmq_ctx_new ();
        void *requester = zmq_socket (context, ZMQ_REQ);
        connect_zmq(command_queue_node , requester);
        zmq_msg_t header,message,message_buffer,reply,reply_buffer;
        invocation_header hd;
        hd.api_id = ENQUEUE_READ_BUFFER;
        zmq_msg_init_size(&header, sizeof(hd));
        memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
        zmq_msg_init_size(&message, sizeof(arg_pkt));
        memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
        zmq_msg_init_size(&message_buffer,arg_pkt.data.buff_len);
        memcpy(zmq_msg_data(&message_buffer), arg_pkt.data.buff_ptr,arg_pkt.data.buff_len);
        zmq_msg_init(&reply);
        zmq_msg_init(&reply_buffer);
        invoke_zmq(requester,&header, &message, &message_buffer, &reply,&reply_buffer);

        ret_pkt = * (enqueue_read_buffer_*) zmq_msg_data(&reply);
                //Todo free
        ret_pkt.data.buff_ptr = (char *) malloc(ret_pkt.data.buff_len);
        memcpy(ret_pkt.data.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.data.buff_len);
        cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);
zmq_close (requester);    
zmq_ctx_destroy (context);
	printf("[clEnqueueReadBuffer interposed] err returned %d\n", ret_pkt.err);

	err = ret_pkt.err;
	return err;

}


cl_int clEnqueueNDRangeKernel (cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t *global_work_offset, const size_t *global_work_size, const size_t *local_work_size, cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event){

	cl_int err = CL_SUCCESS;
	char *kernel_node;

	cl_command_queue_ *command_queue_distr = (cl_command_queue_ *)command_queue;
	char *command_queue_node = command_queue_distr->node;
	cl_command_queue command_queue_clhandle = command_queue_distr->clhandle;

	cl_kernel_ *kernel_distr = (cl_kernel_ *)kernel;
	printf("[clEnqueueNDRangeKernel interposed] kernel_distr %p\n", kernel_distr);

	int node_match_index = 0;
	for(int i=0; i<kernel_distr->num_kernel_tuples; i++){

		kernel_node = kernel_distr->kernel_tuples[i].node;
		printf("[clEnqueueNDRangeKernel interposed] kernel_distr->kernel_tuples[%d].node %s\n", i, kernel_distr->kernel_tuples[i].node);
		printf("[clEnqueueNDRangeKernel interposed] kernel_distr->kernel_tuples[%d].clhandle %p\n", i, kernel_distr->kernel_tuples[i].clhandle);

		if(kernel_node != command_queue_node){
			continue;
		}

		node_match_index = i;
		break;

	}

	assert(kernel_node == command_queue_node);

	cl_kernel kernel_clhandle = kernel_distr->kernel_tuples[node_match_index].clhandle;
	printf("[clEnqueueNDRangeKernel interposed] kernel_clhandle %p\n", kernel_clhandle);



	enqueue_ndrange_kernel_ arg_pkt, ret_pkt;

	arg_pkt.kernel = (unsigned long)kernel_clhandle;
	arg_pkt.command_queue = (unsigned long)command_queue_clhandle;

	arg_pkt.work_dim = work_dim;

	if(global_work_offset){
		arg_pkt.global_offset.buff_ptr = (char *)global_work_offset;
		arg_pkt.global_offset.buff_len = sizeof(size_t) * work_dim;
	}else{
		arg_pkt.global_offset.buff_ptr = "\0";
		arg_pkt.global_offset.buff_len = sizeof(char);
	}

	arg_pkt.global_size.buff_ptr = (char *)global_work_size;
	arg_pkt.global_size.buff_len = sizeof(size_t) * work_dim;

	if(local_work_size){
		arg_pkt.local_size.buff_ptr = (char *)local_work_size;
		arg_pkt.local_size.buff_len = sizeof(size_t) * work_dim;
	}else{
		arg_pkt.local_size.buff_ptr = "\0";
		arg_pkt.local_size.buff_len = sizeof(char);
	}

	ret_pkt.global_offset.buff_ptr = NULL;	
	ret_pkt.global_size.buff_ptr = NULL;	
	ret_pkt.local_size.buff_ptr = NULL;	

	 void *context = zmq_ctx_new ();
         void *requester = zmq_socket (context, ZMQ_REQ);
         connect_zmq(command_queue_node , requester);
         zmq_msg_t header,message,message_buffer,message_aux_buffer,message_aux_buffer2,reply,reply_buffer;
         invocation_header hd;
         hd.api_id = ENQUEUE_NDRANGE_KERNEL;
         zmq_msg_init_size(&header, sizeof(hd));
         memcpy(zmq_msg_data(&header), &hd, sizeof(hd));
         zmq_msg_init_size(&message, sizeof(arg_pkt));
         memcpy(zmq_msg_data(&message), &arg_pkt, sizeof(arg_pkt));
         zmq_msg_init_size(&message_buffer,arg_pkt.global_offset.buff_len);
         memcpy(zmq_msg_data(&message_buffer), arg_pkt.global_offset.buff_ptr,arg_pkt.global_offset.buff_len);
         zmq_msg_init_size(&message_aux_buffer,arg_pkt.global_size.buff_len);
         memcpy(zmq_msg_data(&message_aux_buffer), arg_pkt.global_size.buff_ptr,arg_pkt.global_size.buff_len);
	 zmq_msg_init_size(&message_aux_buffer2,arg_pkt.local_size.buff_len);
         memcpy(zmq_msg_data(&message_aux_buffer2), arg_pkt.local_size.buff_ptr,arg_pkt.local_size.buff_len);
         zmq_msg_init(&reply);
         zmq_msg_init(&reply_buffer);
         invoke_zmq(requester,&header, &message, &message_buffer,&message_aux_buffer,&message_aux_buffer2, &reply,&reply_buffer);

         ret_pkt = * (enqueue_ndrange_kernel_ *) zmq_msg_data(&reply);
                        //Todo free
         ret_pkt.global_offset.buff_ptr = (char *) malloc(ret_pkt.global_offset.buff_len);
         memcpy(ret_pkt.global_offset.buff_ptr, zmq_msg_data(&reply_buffer), ret_pkt.global_offset.buff_len);
         cleanup_messages(&header, &message, &message_buffer, &reply, &reply_buffer);

zmq_close (requester);
    zmq_ctx_destroy (context);
	printf("[clEnqueueNDRangeKernel interposed] err returned %d\n", ret_pkt.err);

	err = ret_pkt.err;
	return err;

}

cl_int clFinish(cl_command_queue command_queue){

	return CL_SUCCESS;

}
