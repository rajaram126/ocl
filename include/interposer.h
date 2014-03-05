#include "CL/opencl.h"
#define TRUE 1
#define FALSE 0
#define GET_PLATFORM_IDS 0
#define GET_DEVICE_IDS 1
#define CREATE_CONTEXT 2
#define CREATE_COMMAND_QUEUE 3
#define CREATE_BUFFER 4
#define CREATE_PROGRAM_WITH_SOURCE 5
#define BUILD_PROGRAM 6
#define BUILD_PROGRAM_AUX 7
#define CREATE_KERNEL 8
#define SET_KERNEL_ARG 9
#define ENQUEUE_WRITE_BUFFER 10
#define ENQUEUE_READ_BUFFER 11
#define ENQUEUE_NDRANGE_KERNEL 12
#define GET_PLATFORM_INFO 13

typedef int bool_t;
typedef struct buff_t {
	unsigned int buff_len;
	char *buff_ptr;
} buff_;

typedef struct invocation_header_t {
int api_id;
}invocation_header;

typedef struct get_platform_ids_t {
	int err;
	unsigned int num_platforms_found;
	buff_ platforms;
}get_platform_ids_;

typedef struct get_platform_info_t {
	int err;
	cl_platform_id platform;
	cl_platform_info param_name;
	size_t param_value_size;
	buff_ param_value;
}get_platform_info_;



typedef struct get_device_ids_t {
	unsigned long platform;
	int device_type;
	int err;
	unsigned int num_devices_found;
	buff_ devices;
} get_device_ids_;


typedef struct get_device_info_t {
	int err;
	cl_device_id device;
	cl_device_info param_name;
	size_t param_value_size;
	buff_ param_value;
}get_device_info_;


typedef struct create_context_t {
	unsigned long context;
	int err;
	unsigned int num_devices;
	buff_ devices;
} create_context_;

typedef struct create_command_queue {
	unsigned long context; //Client to Server
	unsigned long device; //C2S
	int err; //Server to Client
	unsigned long command_queue; //S2C
} create_command_queue_;

typedef struct create_buffer {
	unsigned long context; //Client to Server
	int flags; //C2S
	buff_ data; //C2S
	unsigned long size; //C2S
	int err; //Server to Client
	unsigned long mem; //S2C
} create_buffer_;

typedef struct create_program_with_source {
	unsigned long context; //Client to Server
	buff_ program_str; //C2S
	int err; //Server to Client
	unsigned long program; //S2C
} create_program_with_source_;

typedef struct build_program_t {
	unsigned long program; //Client to Server
	bool_t all_devices; //C2S
	unsigned int num_devices; //C2S
	buff_ devices; //C2S
	buff_ options; //C2S
	int err; //Server to Client
} build_program_;

typedef struct create_kernel {
	unsigned long program; //Client to Server
	buff_ kernel_name; //C2S
	int err; //Server to Client
	unsigned long kernel; //S2C
} create_kernel_;

typedef struct set_kernel_arg {
	unsigned long kernel; //Client to Server
	unsigned long mem; //C2S
	unsigned long image; //C2S
	unsigned long sampler; //C2S
	bool_t is_clobj; //C2S
	bool_t is_mem; //C2S
	bool_t is_image; //C2S
	bool_t is_sampler; //C2S
	bool_t is_null_arg; //C2S
	int arg_index; //C2S
	int arg_size; //C2S
	buff_ plain_old_data; //C2S
	int err; //Server to Client
} set_kernel_arg_;

typedef struct enqueue_write_buffer {
	unsigned long mem; //Client to Server
	unsigned long command_queue; //C2S
	int blocking; //C2S
	unsigned long size; //C2S
	unsigned long offset; //C2S
	buff_ data; //C2S
	int err; //Server to Client
} enqueue_write_buffer_;

typedef struct enqueue_read_buffer {
	unsigned long mem; //Client to Server
	unsigned long command_queue; //C2S
	int blocking; //C2S
	unsigned long size; //C2S
	unsigned long offset; //C2S
	buff_ data; //Server to Client
	int err; //S2C
} enqueue_read_buffer_;

typedef struct enqueue_ndrange_kernel {
	unsigned long kernel; //Client to Server
	unsigned long command_queue; //C2S
	int work_dim; //C2S
	buff_ global_offset; //C2S
	buff_ global_size; //C2S
	buff_ local_size; //C2S
	int err; //Server to Client
} enqueue_ndrange_kernel_;



typedef struct cl_platform_id_distr {

	char *node;
	cl_platform_id clhandle;

} cl_platform_id_;

typedef struct cl_device_id_distr {

	char *node;
	cl_device_id clhandle;

} cl_device_id_;

typedef struct cl_context_distr_elem {
	char *node;
	cl_context clhandle;
} cl_context_elem_;

typedef struct cl_context_distr {

	cl_context_elem_ *context_tuples;
	unsigned int num_context_tuples;

} cl_context_;

typedef struct cl_command_queue_distr {

	char *node;
	cl_command_queue clhandle;

} cl_command_queue_;

typedef struct cl_mem_distr_elem {
	char *node;
	cl_mem clhandle;
} cl_mem_elem_;

typedef struct cl_mem_distr {

	cl_mem_elem_ *mem_tuples;
	unsigned int num_mem_tuples;

} cl_mem_;

typedef struct cl_program_distr_elem {
	char *node;
	cl_program clhandle;
} cl_program_elem_;

typedef struct cl_program_distr {

	cl_program_elem_ *program_tuples;
	unsigned int num_program_tuples;

} cl_program_;

typedef struct cl_kernel_distr_elem {
	char *node;
	cl_kernel clhandle;
} cl_kernel_elem_;

typedef struct cl_kernel_distr {

	cl_kernel_elem_ *kernel_tuples;
	unsigned int num_kernel_tuples;

} cl_kernel_;
