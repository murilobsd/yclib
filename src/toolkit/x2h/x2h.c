#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int transform(char c)
{
	if(c >= '0' && c <= '9')
	{
		return c - '0';
	}
	else if(c >= 'a' && c <= 'f')
	{
		return c - 'a' + 10;
	}
	else if(c >= 'A' && c <= 'F')
	{
		return c - 'A' + 10;
	}
}

int main(int argc, char** argv)
{
    int i = 0;
    int value = 0;
    for(i = 0; i < strlen(argv[1]); ++i)
    {
        if(i != 0)
        {
            value = value << 4;
        }
        value += transform(argv[1][i]);
    }
    printf("%d\n", value);
    return 0;
}

