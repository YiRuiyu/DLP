#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	char  var[] = "Hello World";
	int i = 0;
	while( var[i] != '\0' )
	{
		printf("%c", var[i]);
		i++;
	}
	return 0;
}
