#include <unistd.h>
#include <fcntl.h>
#include "sha1.h"

// OpenCL setup variables.
cl_platform_id platform_id = NULL;
cl_device_id device_id = NULL;  
cl_uint ret_num_devices;
cl_uint ret_num_platforms;
cl_context context;
cl_int ret;

// Kernel variables.
char* sourceStr;
size_t sourceSize;
cl_program program;
cl_kernel kernel;
cl_command_queue command_queue;

// Device memory.
cl_mem d_hash;
cl_mem d_passwords;
cl_mem d_passwordFileSize;
cl_mem d_passwordNumber;
cl_mem d_passwordSizes;
cl_mem d_passwordIndeces;
cl_mem d_retIndex;

// Host memory.
cl_uint* h_hash;
size_t h_passwordFileSize;
int h_passwordNumber;
char* h_passwords;
int* h_passwordSizes;
int* h_passwordIndeces;
int* h_retIndex;

// Thread number.
size_t global_work_size=1;
size_t local_work_size=1;


void sha1Init(cl_uint* ahash, size_t aPasswordFileSize, int aPasswordNumber, const char* aPasswords, const int* aPasswordSizes, const int* aPasswordIndeces, int* aRetIndex) {
	h_hash = ahash;
	h_passwordFileSize = aPasswordFileSize;
	h_passwordNumber = aPasswordNumber;
	h_passwords = aPasswords;
	h_passwordSizes = aPasswordSizes;
	h_passwordIndeces = aPasswordIndeces;
	h_retIndex = aRetIndex;

	global_work_size = h_passwordNumber;

	loadSource();
	createDevice();
	createkernel();

	printf("Allocating device memory...\n");
	createCLobjs();
}

void loadSource() {

	int fd = open("sha1.cl", O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(7);
	}

	size_t sourceSize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	sourceStr = (char*)malloc(sourceSize * sizeof(char));

	read(fd, sourceStr, sourceSize);
	close(fd);
}

void createCLobjs() {

	// Allocate memory in GPU.
	d_hash = clCreateBuffer(context, CL_MEM_READ_ONLY, 5 * sizeof(cl_uint), NULL, &ret);
	d_passwords = clCreateBuffer(context, CL_MEM_READ_ONLY, h_passwordFileSize * sizeof(char), NULL, &ret);
	d_passwordFileSize = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(size_t), NULL, &ret);
	d_passwordNumber = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int), NULL, &ret);
	d_passwordSizes = clCreateBuffer(context, CL_MEM_READ_ONLY, h_passwordNumber * sizeof(int), NULL, &ret);
	d_passwordIndeces = clCreateBuffer(context, CL_MEM_READ_ONLY, h_passwordNumber * sizeof(int), NULL, &ret);
	d_retIndex = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int), NULL, &ret);

	// Copy data to GPU.
	clEnqueueWriteBuffer(command_queue, d_hash, CL_TRUE, 0, 5 * sizeof(cl_uint), h_hash, 0, NULL, NULL);
	clEnqueueWriteBuffer(command_queue, d_passwords, CL_TRUE, 0, h_passwordFileSize * sizeof(char), h_passwords, 0, NULL, NULL);
	clEnqueueWriteBuffer(command_queue, d_passwordFileSize, CL_TRUE, 0, sizeof(size_t), &h_passwordFileSize, 0, NULL, NULL);
	clEnqueueWriteBuffer(command_queue, d_passwordNumber, CL_TRUE, 0, sizeof(int), &h_passwordNumber, 0, NULL, NULL);
	clEnqueueWriteBuffer(command_queue, d_passwordSizes, CL_TRUE, 0, h_passwordNumber * sizeof(int), h_passwordSizes, 0, NULL, NULL);
	clEnqueueWriteBuffer(command_queue, d_passwordIndeces, CL_TRUE, 0, h_passwordNumber * sizeof(int), h_passwordIndeces, 0, NULL, NULL);
	clEnqueueWriteBuffer(command_queue, d_retIndex, CL_TRUE, 0, sizeof(int), h_retIndex, 0, NULL, NULL);

	clSetKernelArg(kernel, 0, sizeof(d_hash), (void*) &d_hash);
	clSetKernelArg(kernel, 1, sizeof(d_passwords), (void*) &d_passwords);
	clSetKernelArg(kernel, 2, sizeof(d_passwordFileSize), (void*) &d_passwordFileSize);
	clSetKernelArg(kernel, 3, sizeof(d_passwordNumber), (void*) &d_passwordNumber);
	clSetKernelArg(kernel, 4, sizeof(d_passwordSizes), (void*) &d_passwordSizes);
	clSetKernelArg(kernel, 5, sizeof(d_passwordIndeces), (void*) &d_passwordIndeces);
	clSetKernelArg(kernel, 6, sizeof(d_retIndex), (void*) &d_retIndex);
}

void runKernel() {
	printf("Executing kernel...\n");
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
	ret = clFinish(command_queue);
	clEnqueueReadBuffer(command_queue, d_retIndex, CL_TRUE, 0, sizeof(int), h_retIndex, 0, NULL, NULL);
}

void createDevice() {
	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_ALL, 1, &device_id, &ret_num_devices);

	context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
}

void createkernel() {
	program = clCreateProgramWithSource(context, 1, (const char **)&sourceStr, (const size_t *)&sourceSize, &ret);
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
	kernel = clCreateKernel(program, "breakHash", &ret);
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

	free(sourceStr);
}