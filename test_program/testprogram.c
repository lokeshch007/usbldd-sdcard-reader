/** Header file section **/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>


/*
 * @fd:	file descriptor for /dev/pendrive_driver
 * @ret: return status 
 * @opt: option for operation selection
 * @i,j: looping varibles
 * @temp_out_string: character buffer used while writing when packet size is less than 64
 * @in_string: string buffer used in reading data from device
 * @ch: character variable used for looping of strings
 * @write_file_name: buffer for name of new file to be written to SD card
 * @read_file_name: buffer for name of file to be read
 * @new_filebuf: buffer for data to be written to new file
 *
 * steps:
 * 1. open driver file in O_RDWR mode
 * 2. select operation
 * 3. + 0:EXIT
 * 	  + 1:WRITE
 * 	  	- Read new File name of maximum 11 character
 * 	  	- write 'WRITE' string to device
 * 	  	- write name of file to device
 * 	  	+ READ,WRITE FILE DATA 
 * 	  	  - Read character by character from user untill EOF('>')
 * 	  	  - write the character into 'new_filebuf',if new_filebuf fills then write the new_filebuf to device
 * 		  - if EOF reached by USER in less than 64 bytes, then
 * 		  	allocate user entered number of bytes+1 to temp_out_string
 * 		  	write all that bytes to temp_out_string then write that temp_out_string to device, free buffer
 * 		  - sleep for 1 second
 * 		  - write '__END__' string to device
 * 		  - sleep for 1 second
 * 	  + 2:READ
 * 	  	- Read file name to be read
 * 	  	- write 'READ' string to device
 * 	  	- sleep for 1 second
 * 	  	- write file name to device
 * 	  	- read 64 bytes repeatedly untill recived data is '__END__' and print it
 * 			
 */
int main(void)
{
	int fd,copy_fd,ret,opt,i=0,j=0,count=0;
	char *temp_out_string;
	char in_string[64];
	char ch;
	char write_file_name[12],read_file_name[12],copy_file_name[12],send_file_name[12];
	char *new_filebuf;


	fd=open("/dev/pendrive_driver",O_RDWR);
	if(fd < 0)
	{
		perror("open() error");
		return -1;
	}

	while(1)
	{
		printf("\nSelect operation:\n");
		printf("0. EXIT\n");
		printf("1. Write FILE\n");
		printf("2. Read FILE\n");
		printf("3. List Files\n");
		printf("4. Copy a file from SD Card\n");
		printf(">>");
		scanf("%d",&opt);

		switch(opt)
		{
			case 	0	:	/** EXIT **/
				break;

			case	1	:	/** Write File operation **/
				printf("New FILE name(11charcters MAX): ");
				getchar();
				gets(write_file_name);
				printf("'%s' is given File Name\n",write_file_name);
				write(fd,"WRITE",6);
				printf("'WRITE' command sent to Device\n");
				sleep(1);

				write(fd,write_file_name,strlen(write_file_name)+1);
				printf("'%s' filename sent to Device\n",write_file_name);
				sleep(1);

				printf("Enter data ['>' to EOF]: ");
				i=0;
				new_filebuf=(char*)malloc(64);
				if(new_filebuf == NULL)
				{
					perror("malloc() error");
					exit(-1);
				}
				while((ch=getchar()) != '>')
				{
					if(i<64)
					{
						new_filebuf[i++]=ch;
					}
					if(i>=64)
					{
						printf("Writing given 64 Bytes to card wait..\n");
						write(fd,new_filebuf,sizeof(new_filebuf));
						sleep(1);
						printf("64Bytes written to Card\n");
						i=0;
					}
				}
				temp_out_string=(char*)malloc(i);
				if(temp_out_string == NULL)
				{
					perror("malloc() error");
					close(fd);
					return;
				}
				j=0;
				while(j<i)
				{
					temp_out_string[j]=new_filebuf[j];
					j++;
				}
				temp_out_string[j]='\0';
				write(fd,temp_out_string,j);
				sleep(2);
				printf("Total data written to SD Card\n");
				write(fd,"__END__",8);
				sleep(1);
				printf("__END__ sent to device\n");
				free(temp_out_string);
				free(new_filebuf);

				break;

			case	2	:	/** READ File operation **/
				printf("File Name to read: ");
				scanf("%*c%[^\n]s",read_file_name);

				printf("'%s' is given File Name\n",read_file_name);

				write(fd,"READ",5);
				printf("'READ' command sent to Device\n");
				sleep(1);
				getchar();
				printf("press any key to continue...");
				getchar();

				//File name sending to Device
				printf("\n");
				write(fd,read_file_name,sizeof(read_file_name));
				printf("'%s' file name sent to Device\n");

				printf("'READ_OPERATION' started...\n");
				//reading untill Device send '__END__'
				while(1)
				{
					ret=read(fd,in_string,64);
					if(ret < 0)
					{
						perror("read() error");
						break;
					}

					if(strcmp(in_string,"__END__") == 0)
					{
						printf("\n__EOF__\n");
						break;
					}
					printf("%s",in_string);
				}
				break;

			case	3	:	/** LIST Files operation **/
				write(fd,"LIST",5);
				printf("'LIST' command sent to Device\n");
				sleep(1);
				printf("LIST Read operation Initiated..\n\n");
				while(1)
				{
					ret=read(fd,in_string,64);
					if(ret < 0)
					{
						perror("read() error");
						break;
					}
					//sleep(1);


					if(strcmp(in_string,"__LIST_END__") == 0)
					{
						printf("\n__LIST_ENDED__\n");
						break;
					}
					printf("%s\n",in_string);
					count++;
					//sleep(1);
				}
				printf("Total %d files\n",count);
				count=0;
				//strcpy(in_string," ");
				break;

			case	4	:	/** Copy File from SD Card **/
				printf("File Name to COPY: ");
				scanf("%*c%[^\n]s",copy_file_name);

				printf("'%s' is given File Name\n",copy_file_name);

				write(fd,"READ",5);
				printf("'READ' command sent to Device\n");
				sleep(1);
				getchar();
				printf("press any key to continue...");
				getchar();

				//sending File name to Device
				printf("\n");
				write(fd,copy_file_name,sizeof(copy_file_name));
				printf("'%s' file name sent to Device\n");
				
				//File creation in Host
				copy_fd=open(copy_file_name,O_WRONLY|O_CREAT,0666);
				if(copy_fd<0)
				{
					perror("open() error: Unable to create file in host");
					exit(-1);
				}

				printf("copying.");
				while(1)
				{
					ret=read(fd,in_string,64);
					if(ret < 0)
					{
						perror("read() error");
						break;
					}

					if(strcmp(in_string,"__END__") == 0)
					{
						printf("\n__EOF__\n");
						break;
					}
					printf(".",in_string);
					write(copy_fd,in_string,strlen(in_string));
				}
				printf("\n");
				close(copy_fd);
				break;


			//case	5	:	/** Send File to SD Card **/
			/*	printf("File name:");
				getchar();
				gets(send_file_name);
				printf("'%s' is given File Name\n",send_file_name);

				//Opening Source file in HOST in READ MODE

				write(fd,"WRITE",6);
				printf("'WRITE' command sent to Device\n");
				sleep(1);

				write(fd,send_file_name,strlen(send_file_name)+1);
				printf("'%s' filename sent to Device\n",send_file_name);
				sleep(1);

				printf("Enter data ['>' to EOF]: ");
				i=0;
				new_filebuf=(char*)malloc(64);
				if(new_filebuf == NULL)
				{
					perror("malloc() error");
					exit(-1);
				}
				while((ch=getchar()) != '>')
				{
					if(i<64)
					{
						new_filebuf[i++]=ch;
					}
					if(i>=64)
					{
						printf("Writing given 64 Bytes to card wait..\n");
						write(fd,new_filebuf,sizeof(new_filebuf));
						sleep(1);
						printf("64Bytes written to Card\n");
						i=0;
					}
				}
				temp_out_string=(char*)malloc(i);
				if(temp_out_string == NULL)
				{
					perror("malloc() error");
					close(fd);
					return;
				}
				j=0;
				while(j<i)
				{
					temp_out_string[j]=new_filebuf[j];
					j++;
				}
				temp_out_string[j]='\0';
				write(fd,temp_out_string,j);
				sleep(2);
				printf("Total data written to SD Card\n");
				write(fd,"__END__",8);
				sleep(1);
				printf("__END__ sent to device\n");
				free(temp_out_string);
				free(new_filebuf);

				break;*/

			default	:	printf("Wrong option\n");
						break;
		}
		if(opt == 0)
			break;
	}
	return 0;
}
