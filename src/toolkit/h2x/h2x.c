#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char transform(int c)
{
	if(c >= 0 && c <=9)
	{
		return '0' + c;
	}

	if(c >= 10)
	{
		return 'a' + c - 10;
	}

}
int main(int argc, char** argv)
{
	char buf[9];
	memset(buf, '0', 8);
	buf[8] = 0;
	int a = atol(argv[1]);
	int i = 7;
	while(a)
	{		
            int c = a % 16;
            buf[i] = transform(c);
            --i;
            a = a >> 4;
            if(i == 0)
            {
                break;
            }
	}
	printf("%s\n", buf);
}
