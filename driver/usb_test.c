#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
	unsigned char wrbuf[64] = "03/02/1992", rdbuf[256];
	int fd, ret;
	
	if(argc!=2)
	{
		printf("%s <device path>\n", argv[0]);
		_exit(1);
	}
	
	fd = open(argv[1], O_RDWR);
	if(fd < 0)
	{
		perror("open() failed");
		_exit(1);
	}

	
	ret = write(fd, wrbuf, strlen(wrbuf)+1);	
	printf("Buf Written(%d) : %s\n", ret, wrbuf);
	
	ret = read(fd, rdbuf, sizeof(rdbuf));
	printf("Buf Read(%d) : %s\n", ret, rdbuf);
	
	ret = read(fd, rdbuf, sizeof(rdbuf));
	printf("Buf Read(%d) : %s\n", ret, rdbuf);

	
	//lseek(fd, 6, SEEK_SET);


	close(fd);
	return 0;
}


