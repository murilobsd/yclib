#include <stdio.h>
#include "rns.h"

int main(int argc, char** argv)
{
	int ret_code = 0;
	rns_file_t* file = rns_file_open(argv[1], RNS_FILE_OPEN | RNS_FILE_READ);
	if(file == NULL)
	{
		printf("open failed\n");
		--ret_code;
		goto ERROR_EXT;	
	}
	char buf[188];

	int ret = 0;
	int offset = 0;

	while(1)
	{

		ret = rns_file_read(file, buf, 188);
		if(ret < 0)
		{
			--ret_code;
			goto ERROR_EXT;
		}
		else if(ret == 0)
		{
			break;
		}
		if(buf[0] != 0x47)
		{
			printf("%d\n", offset);
			break;
		}
		offset += 188;
	}
	return ret_code;
ERROR_EXT:
	return ret_code;	
}
