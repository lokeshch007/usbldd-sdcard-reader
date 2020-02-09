/***************************************************************************************************
                                   ExploreEmbedded
 ****************************************************************************************************
 * File:   main.c
 * Version: 16.0
 * Author: ExploreEmbedded
 * Website: http://www.exploreembedded.com/wiki
 * Description: File contains the example program to demonstrate the sdcard interface using fat32 file system.

The libraries have been tested on ExploreEmbedded development boards. We strongly believe that the
library works on any of development boards for respective controllers. However, ExploreEmbedded
disclaims any kind of hardware failure resulting out of usage of libraries, directly or indirectly.
Files may be subject to change without prior notice. The revision history contains the information
related to updates.


GNU GENERAL PUBLIC LICENSE:
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

Errors and omissions should be reported to codelibraries@exploreembedded.com
 **************************************************************************************************/


/************************************************************************************************
 * 		:%s/temp/old/g  --> replaces all 'temp' words with 'old'
 */

#include <lpc17xx.h>
#include<stdlib.h>
#include "sdcard.h"
#include "uart.h"
#include "fat32.h"
#include "stdutils.h"
#include "delay.h"
//#include "ssp.h"     


#define FILE_READ         '1'
#define FILE_WRITE        '2'
#define FILE_COPY         '3'
#define FILE_DELETE       '4'
#define FILE_LIST         '5'
#define MEMORY_STATICS    '6'



int main()
{
    uint8_t returnStatus,sdcardType,option;
    char ch,sourceFileName[12],destFileName[12];
    fileConfig_st *srcFilePtr,*destFilePtr;
    fileInfo fileList;
    uint32_t totalMemory,freeMemory;
		char filestring_uart[30];

    SystemInit();
    uart_init(9600);
    returnStatus = SD_Init(&sdcardType);

    if(returnStatus)
    {
        if(returnStatus == SDCARD_NOT_DETECTED)
        {
            uart_puts("SD card not detected..\r\n");
        }
        else if(returnStatus == SDCARD_INIT_FAILED)
        {
            uart_puts("Card Initialization failed..\r\n");
        }
        else if(returnStatus == SDCARD_FAT_INVALID)
        {
            uart_puts("Invalid Fat filesystem\r\n");
        }
        while(1);
    }
    else
    {
        uart_puts("SD Card Detected!\r\n");
    }


    while(1)
    {
        uart_puts("Press any key to continue\r\n");
        uart_getch();
        uart_puts("------File options---------\r\n");
        uart_puts("1: Read File \r\n");
        uart_puts("2: Write File \r\n");
        uart_puts("3: File Copy \r\n");
        uart_puts("4: Delete File \r\n");
        uart_puts("5: Get File List \r\n");
        uart_puts("6: Memory Statics \r\n");
        uart_puts("--------------------------- \r\n");
        uart_puts("Choose one of the options: \r\n");


        do
        {
            option = uart_getch();
        }while((option<'1') || (option>'6'));

       	//option=uart_getch();
        uart_puts("option taken\n\r\n\r");

        switch(option)
        {

        case FILE_READ: /* Read a File */
            uart_puts("\n\rEnter File name max 11 chars including file type: ");
            uart_gets(sourceFileName);

            srcFilePtr = FILE_Open(sourceFileName,READ,&returnStatus);
            if(srcFilePtr == 0)
            {
                uart_puts("\n\rFile Opening Failed");
            }
            else
            {
                 uart_puts("File Content: ");
                while(1)
                {
                    ch = FILE_GetCh(srcFilePtr);
                    if(ch == EOF)
                        break;
                    uart_putch(ch);
                }

                FILE_Close(srcFilePtr);
            }
            break;



        case FILE_WRITE: /* Write to a File */
            uart_puts("\n\rEnter File name max 11 chars including file type: ");
            uart_gets(sourceFileName);

            srcFilePtr = FILE_Open(sourceFileName,WRITE,&returnStatus);
            if(srcFilePtr == 0)
            {
                uart_puts("\n\rFile Opening Failed");
            }
            else
            {
                uart_puts("\n\rEnter text ending with '>' :");
                while(1)
                {
                    ch = uart_getch();
                    if(ch == '>')
                    {
                        FILE_PutCh(srcFilePtr,EOF);
                        break;
                    }
                    else
                    {   uart_putch(ch);
                    FILE_PutCh(srcFilePtr,ch);
                    }
                }
                uart_puts("\n\rData saved to file, closing the file.");
                FILE_Close(srcFilePtr);
            }
            break;



				case FILE_COPY: /* File Copy */
            uart_puts("\n\rEnter source File name max 11 chars including file type: ");
            uart_gets(sourceFileName);

            srcFilePtr = FILE_Open(sourceFileName,READ,&returnStatus);
            if(srcFilePtr == 0)
            {
                uart_puts("\n\rSource File Opening Failed");
            }
            else
            {
                uart_puts("\n\rEnter destination File name max 11 chars including file type: ");
                uart_gets(destFileName);

                destFilePtr = FILE_Open(destFileName,WRITE,&returnStatus);

                if(destFilePtr == 0)
                {
                    uart_puts("\n\rDestination File Opening Failed");

                }
                else
                {
                    uart_puts("\n\rCopying the file.");
                    while(1)
                    {
                        ch = FILE_GetCh(srcFilePtr);
                        FILE_PutCh(destFilePtr,ch);
                        if(ch == EOF)
                        {
                            break;
                        }
                    }
                    uart_puts("\n\rDone Copying..");
                    FILE_Close(destFilePtr);
                }

                FILE_Close(srcFilePtr);
            }
            break;



        case FILE_DELETE: // FIle Delete
            uart_puts("\n\rEnter File name to be deleted max 11 chars including file type: ");
            uart_gets(sourceFileName);

            uart_puts("\n\rDeleting File: ");
            returnStatus = FILE_Delete(sourceFileName);
            if(returnStatus == FAT32_FILE_OPENED_CANNOT_BE_DELETED)
            {
                uart_puts("\n\rFile is open cannot be deleted");
            }
            else
            {
                uart_puts("\n\rDone! File Deleted");
            }  			
            break;



        case FILE_LIST: //Print the files with size
            while(1)
            {
                returnStatus = FILE_GetList(&fileList);
                if(returnStatus != FAT32_END_OF_FILE_LIST)
                {
					uart_puts(fileList.fileName);
					uart_puts("  ");

					itoa(fileList.fileSize,filestring_uart,10);
                    uart_puts(filestring_uart);
					uart_puts(" bytes\r\n");
                }
                else
                {
                    break;
                }
            }
            break;


        case MEMORY_STATICS: //Print the SD CARD memory Total/Free in bytes
            uart_puts("\n\rMemory Statics is being calculated..");
            FILE_GetMemoryStatics(&totalMemory,&freeMemory);
						
						itoa(totalMemory,filestring_uart,10);
						uart_puts("\nTotal Memory : "); uart_puts(filestring_uart); uart_puts("bytes\r\n");
						
						int temp_mem;
						temp_mem=freeMemory;						
						itoa(temp_mem,filestring_uart,10);
						uart_puts("Free Memory : "); uart_puts(filestring_uart); uart_puts("bytes");
            
						temp_mem=freeMemory/1024;						
						itoa(temp_mem,filestring_uart,10);
						uart_puts("   "); uart_puts(filestring_uart); uart_puts(" Kb");
						
						temp_mem=freeMemory/1024/1024;						
						itoa(temp_mem,filestring_uart,10);
						uart_puts("   "); uart_puts(filestring_uart); uart_puts(" Gb\r\n");
						//UART0_Printf("\n\rTotal memory:%Ubytes  Free memory:%Ubytes",totalMemory,freeMemory);
            break;

        }// End of switch
    }// End of while
}// End of main
