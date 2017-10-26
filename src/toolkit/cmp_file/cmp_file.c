#include <stdio.h>
#include "rns.h"
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 10000000

int cmp(int size, char* buf, char* buf2)
{
    int left = 0;
    int right = size;
    int mid = 0;
    while(1)
    {
        mid = (left + right) / 2;
        int s = mid - left;

        if(s == 0)
        {
            return mid;
        }
    
        if(memcmp(buf + left, buf2 + left, s) == 0)
        {
            left = mid;
        }
        else
        {
            right = mid;
        }

        
    }
}


int main(int argv, char** argc)
{
    rns_file_t* file = rns_file_open(argc[1], RNS_FILE_OPEN | RNS_FILE_READ);
    rns_file_t* file2 = rns_file_open(argc[2], RNS_FILE_OPEN | RNS_FILE_READ);

    int size = rns_file_size(file);
    int size2 = rns_file_size(file2);

    int readBtypes = 0;
    
    int s = size < size2 ? size : size2;
    
    char* buf = malloc(BUF_SIZE);
    char* buf2 = malloc(BUF_SIZE);


    while(readBtypes < s)
    {
        int nSize = BUF_SIZE < s - readBtypes ? BUF_SIZE : s - readBtypes;
        
        rns_file_read(file, buf, nSize);
        rns_file_read(file2, buf2, nSize);

        if(memcmp(buf, buf2, nSize) != 0)
        {
            printf("%d\n",  readBtypes + cmp(nSize, buf, buf2));
            return 0;
        }
        readBtypes += nSize;
    }

    if(size != size2)
    {
        printf("%d\n", s);
    }
    else
    {
        printf("0\n");
    }
    return 0;
}
