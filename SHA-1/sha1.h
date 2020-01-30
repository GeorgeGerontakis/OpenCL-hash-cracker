#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#ifndef uint32_t
#define uint32_t unsigned int
#endif

#define MAX_SOURCE_SIZE 0x10000000

// Function prototypes.
void loadSource();
void createDevice();
void createkernel();
void createCLobjs();
void sha1Init(cl_uint*, size_t, int, const char*, const int*, const int*, int*);
void runKernel();