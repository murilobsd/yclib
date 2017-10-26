#include <stdio.h>
#include "rns.h"
#include <string.h>
#include <stdlib.h>


int main(int argv, char** argc)
{
	rns_file_t* file = rns_file_open(argc[1], RNS_FILE_OPEN | RNS_FILE_READ);
	rns_file_t* file2 = rns_file_open(argc[3], RNS_FILE_CREAT | RNS_FILE_WRITE);
	int offset = atol(argc[2]);
	lseek(file->sfd, offset, SEEK_SET);
	char* buf = malloc(10000000);
	rns_file_read(file, buf, 10000000);
	rns_file_write(file2, buf, 10000000);

}
