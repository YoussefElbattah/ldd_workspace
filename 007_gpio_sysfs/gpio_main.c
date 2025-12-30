#include <stdio.h>


int main(int argc, char **argv){
	FILE *file_attr = fopen(argv[1], O_RDWR);

	if(!file_attr)
		printf("Open failed\n");
		return errno;

}
