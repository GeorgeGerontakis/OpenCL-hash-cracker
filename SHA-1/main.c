#include "sha1.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#define TRUE 1
#define FALSE 0

struct timeval start, end;

int isHashValid(char*);
void hexStr2uint(char*, cl_uint*, int);
double getExecutionTime();

int main(int argc, char** argv) {

	// Check the arguments.
	if (argc != 3) {
		printf("Usage: %s 'path to list file' 'SHA1 hash to break'\n", argv[0]);
		exit(1);
	}

	// Check the hash.
	if (isHashValid(argv[2]) == FALSE) {
		fprintf(stderr, "Not a valid SHA1 hash.\n");
		exit(2);
	}

	// Open the password list file.
	int fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(3);
	}

	// Calculate the size of the file.
	size_t fileSize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	// Read the contents of the file and then close it.
	char* fileContents = (char*)malloc(fileSize * sizeof(char));
	if (fileContents == NULL) {
		perror("malloc");
		exit(4);
	}
	read(fd, fileContents, fileSize);
	close(fd);
	
	// Find how many passwords are in the file.
	int newLineCounter = 0;
	for (size_t i = 0; i < fileSize; i++) {
		if (fileContents[i] == '\n')
			newLineCounter++;
	}

	// Allocate memory for the hash that is being searched.
	cl_uint* hashUint = (cl_uint*)malloc(5 * sizeof(cl_uint));
	hexStr2uint(argv[2], hashUint, 5);

	// Allocate memory for the sizes and indeces of the passwords.
	int* passwordSizes = (int*)malloc( (newLineCounter+1) * sizeof(int) );
	if (passwordSizes == NULL) {
		perror("malloc");
		exit(5);
	}
	int* passwordIndeces = (int*)malloc( (newLineCounter+1) * sizeof(int) );
	if (passwordIndeces == NULL) {
		perror("malloc");
		exit(6);
	}
	passwordIndeces[0] = 0;

	// Populate the size and index arrays of the passwords.
	int currentPasswordLength = 0;
	int currentPasswordIndex = 0;
	for (size_t i = 0; i < fileSize; i++) {
		char curChar = fileContents[i];
		if (curChar == '\n') {
			passwordSizes[currentPasswordIndex++] = currentPasswordLength;
			currentPasswordLength = 0;
			passwordIndeces[currentPasswordIndex] = i+1;

		}
		else if (i == fileSize-1)
			passwordSizes[currentPasswordIndex++] = currentPasswordLength + 1;
		else
			currentPasswordLength++;
	}

	// Initialize and execute the kernel.
	int ret = -1;
	sha1Init(hashUint, fileSize, newLineCounter+1, fileContents, passwordSizes, passwordIndeces, &ret);
	gettimeofday(&start, NULL);
	runKernel();
	gettimeofday(&end, NULL);

	// Print the result.
	if (ret == -1)
		printf("Password not found in the file.\n");
	else {
		printf("Password found: ");
		int passSize = passwordSizes[ret];
		int passIdx = passwordIndeces[ret];
		for (int i = 0; i < passSize; i++) {
			printf("%c", fileContents[passIdx + i]);
		}
		printf("\n");
	}
	
	printf("Kernel execution time: %.3f seconds.\n", getExecutionTime());

	free(hashUint);
	free(fileContents);
	free(passwordSizes);
	free(passwordIndeces);

	return 0;
}

int isHashValid(char* hash) {
	if (strlen(hash) != 40)
		return FALSE;

	for (int i = 0; i < 40; i++) {
		int ascii = (int)hash[i];

		// Check if the characher is a valid hexadecimal digit.
		if ( !(ascii >= 0x30 && ascii <= 0x39
			|| ascii >= 0x41 && ascii <= 0x46
			|| ascii >= 0x61 && ascii <= 0x66))
			return FALSE;
	}

	return TRUE;
}

void hexStr2uint(char* str, cl_uint* res, int size) {
	for (int i = 0; i < size; i++) {
		char* tmpStr = str + i*8;
		cl_uint tmpRes = 0;

		for (int j = 0; j < 8; j++) {
			char c = tmpStr[j];

			unsigned int value = (unsigned int)c;

			// A-F --> 41-46
			// a-f --> 61-66
			// 0-9 --> 31-39

			// Convert ascii to value.
			if (value >= 0x30 && value <= 0x39)
				value -= 0x30;
			else if (value >= 0x41 && value <= 0x46)
				value -= 0x37;
			else if (value >= 0x61 && value <= 0x66)
				value -= 0x57;

			value &= 0xF;
			value <<= (7-j)*4;
			tmpRes |= value;
		}
		res[i] = tmpRes;
	}
}

double getExecutionTime() {
	return ( (end.tv_sec*1000000 + end.tv_usec) - (start.tv_sec*1000000 + start.tv_usec) ) / 1000000.0;
}