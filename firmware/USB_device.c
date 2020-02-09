
#include <stdio.h>
#include <string.h>
#include<stdlib.h>
#include "sdcard.h"
#include "fat32.h"
#include "stdutils.h"
#include "delay.h" 
#include "lcd.h"
#include "usbapi.h"
#include "usbdebug.h"
#include "usbstruct.h"
#include "rtc.h"
#include "led.h"

#include "LPC17xx.h"


/*************SD CARD identifiers and MACROS**************************/
uint8_t returnStatus,sdcardType;
char ch,sourceFileName[12],destFileName[12];
fileConfig_st *srcFilePtr,*destFilePtr;
fileInfo fileList;
uint32_t totalMemory,freeMemory;
uint8_t returnStatus,sdcardType;
static volatile uint8_t pending=0;
char option[6],read_filename[12],write_filename[12];
char write_file_buf[64],read_file_buf[64];
static volatile uint8_t write_file_open=0;

#define FILE_READ         (strcmp(option,"READ")==0)
#define FILE_WRITE        (strcmp(option,"WRITE")==0)
#define FILE_COPY         '3'
#define FILE_DELETE       '4'
#define FILE_LIST         (strcmp(option,"LIST")==0)
#define MEMORY_STATICS    '6'

/*************SD CARD identifiers and MACROS ENDS**************************/

#define usbMAX_SEND_BLOCK		( 20 / portTICK_PERIOD_MS )
#define usbBUFFER_LEN			( 20 )

#define INCREMENT_ECHO_BY 1
#define BAUD_RATE	115200

#define INT_IN_EP		0x81
#define BULK_OUT_EP		0x05
#define BULK_IN_EP		0x82

#define MAX_PACKET_SIZE	64

#define LE_WORD(x)		((x)&0xFF),((x)>>8)
#define EMPTY_USB_BUFFER	strcpy(abBulkBuf," ")

static unsigned char abBulkBuf[64];
static unsigned char write_buffer[64];
static unsigned char list_buffer[64];
//static unsigned char abClassReqData[8];


// forward declaration of interrupt handler
void USBIntHandler(void);

#define BULK_IN_1_EP		0x82	//refer pg.no.110 of Axelson, Field: bEndpointAddress
#define BULK_IN_2_EP		0x85	
#define BULK_OUT_1_EP		0x08	


static const unsigned char abDescriptors[] = {

	/* Device descriptor */
	0x12,					// Length of descriptor
	DESC_DEVICE,       		// Type of descriptor
	LE_WORD(0x0200),		// bcdUSB
	0x00,              		// bDeviceClass
	0x00,              		// bDeviceSubClass
	0x00,              		// bDeviceProtocol
	MAX_PACKET_SIZE0,  		// bMaxPacketSize
	LE_WORD(0xABCD),		// idVendor
	LE_WORD(0x2017),		// idProduct
	LE_WORD(0x0100),		// bcdDevice
	0x01,              		// iManufacturer
	0x02,              		// iProduct
	0x03,              		// iSerialNumber
	0x01,              		// bNumConfigurations

	// Configuration Descriptor
	0x09,
	DESC_CONFIGURATION,
	LE_WORD(0x27),  		// wTotalLength
	0x01,  					// bNumInterfaces
	0x01,  					// bConfigurationValue
	0x00,  					// iConfiguration
	0x80,  					// bmAttributes		//bus powered
	0x32,  					// bMaxPower	(100mA Current)

	// Interface Descriptor
	0x09,
	DESC_INTERFACE,
	0x00,  		 			// bInterfaceNumber
	0x00,   				// bAlternateSetting
	0x03,   				// bNumEndPoints
	0xFF,   				// bInterfaceClass
	0x00,   				// bInterfaceSubClass
	0x00,   				// bInterfaceProtocol
	0x00,   				// iInterface

	// Bulk IN 1 Endpoint
	0x07,
	DESC_ENDPOINT,
	BULK_IN_1_EP,					// bEndpointAddress
	0x02,   					// bmAttributes = Bulk
	LE_WORD(MAX_PACKET_SIZE),	// wMaxPacketSize
	0x00,						// bInterval

	// Bulk IN 2 Endpoint
	0x07,
	DESC_ENDPOINT,
	BULK_IN_2_EP,				// bEndpointAddress
	0x02,   					// bmAttributes = Bulk
	LE_WORD(MAX_PACKET_SIZE),	// wMaxPacketSize
	0x00,						// bInterval

	// Bulk OUT 1 Endpoint
	0x07,
	DESC_ENDPOINT,
	BULK_OUT_1_EP,				// bEndpointAddress
	0x02,   					// bmAttributes = Bulk
	LE_WORD(MAX_PACKET_SIZE),	// wMaxPacketSize
	0x00,						// bInterval

	// string descriptors
	0x04,
	DESC_STRING,
	LE_WORD(0x0409),

	// manufacturer string
	0x18,
	DESC_STRING,
	'D', 0, 'E', 0, 'S', 0, 'D' , 0, ' ', 0, 'A', 0, 'U', 0, 'G', 0, '1', 0, '7', 0 , '.', 0,

	// product string
	0x10,
	DESC_STRING,
	'U', 0, 'S', 0, 'B', 0, ' ', 0, 'A', 0, 'R', 0, 'M', 0,


	// serial number string
	0x12,
	DESC_STRING,
	'0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '7', 0,

	// terminator
	0
};

/**
  Local function to handle incoming bulk data

  @param [in] bEP
  @param [in] bEPStatus
pending:
0 - No operation pending
1 - one operation strated either READ or WRITE
2 - READ FILE operation pending
3 - WRITE FILE operation pending
4 - LIST Files
*/
static void BulkOut(unsigned char bEP, unsigned char bEPStatus)
{
	int i=0,iLen;

	( void ) bEPStatus;

	// get data from USB into intermediate buffer
	if(FILE_WRITE && pending==3)
	{
		iLen = USBHwEPRead(bEP,write_buffer, sizeof(abBulkBuf));
	}
	else
	{
		iLen = USBHwEPRead(bEP, abBulkBuf, sizeof(abBulkBuf));
	}
	lcd_clear();
	lcd_puts(LCD_LINE1, abBulkBuf);
	lcd_puts(LCD_LINE2,"operation...");
	if(pending == 0)
	{
		strcpy(option,abBulkBuf);
		lcd_clear();
		lcd_puts(LCD_LINE1,"option sel");
		lcd_puts(LCD_LINE2,option);
		if(FILE_LIST)
		{
			pending=4;
			//EMPTY_USB_BUFFER;
			return;
		}
		pending=1;
	}
	else if(pending == 1)
	{
		if(FILE_READ)
		{
			strcpy(read_filename,abBulkBuf);
			lcd_clear();
			lcd_puts(LCD_LINE1," READ FILE NAME ");
			lcd_puts(LCD_LINE2,read_filename);
			pending=2;
		}
		else if(FILE_WRITE)
		{
			strcpy(write_filename,abBulkBuf);
			i=0;
			lcd_clear();
			lcd_puts(LCD_LINE1,"WRITE FILE NAME ");
			lcd_puts(LCD_LINE2,write_filename);
			pending=3;
		}


		else
		{
			lcd_clear();
			if(FILE_READ)
			{
				lcd_puts(LCD_LINE1,"Read ERROR for  ");
				lcd_puts(LCD_LINE2,read_filename);
			}
			if(FILE_WRITE)
			{
				lcd_puts(LCD_LINE1,"Write ERROR for  ");
				lcd_puts(LCD_LINE2,write_filename);
			}
			pending=0;
			write_file_open=0;
		}
	}
	else if(pending == 3)
	{		
		if(write_file_open == 0)
		{
			srcFilePtr = FILE_Open(write_filename,WRITE,&returnStatus);
			if(srcFilePtr == 0)
			{
				lcd_clear();
				lcd_puts(LCD_LINE1,"write ERROR");
				lcd_puts(LCD_LINE2,"open() error");
				pending=0;
				return;
			}
			write_file_open=1;
		}

		
		if(strcmp(write_buffer,"__END__") == 0)
		{
			FILE_PutCh(srcFilePtr,EOF_FAT);
			FILE_Close(srcFilePtr);
			write_file_open=0;
			pending=0;
			return;
		}
		strcpy(write_file_buf,write_buffer);		
		while(i<strlen(write_file_buf))
		{
			FILE_PutCh(srcFilePtr,write_file_buf[i++]);
		}
	}

	//EMPTY_USB_BUFFER;

}


/**
  Local function to handle outgoing bulk data

  @param [in] bEP
  @param [in] bEPStatus
  */
static void BulkIn(unsigned char bEP, unsigned char bEPStatus)
{
	int i=0,bytes_read=0;
	switch(pending)
	{
		case 2 ://READ FILE Operation pending
			if(FILE_READ)//read file name storing
			{
				strcpy(read_filename,abBulkBuf);
				lcd_puts(LCD_LINE1,"READING... ");
				lcd_puts(LCD_LINE2,read_filename);
				srcFilePtr = FILE_Open(read_filename,READ,&returnStatus);
				if(srcFilePtr == 0)
				{
					lcd_clear();
					lcd_puts(LCD_LINE1,"fopen() failed");
					lcd_puts(LCD_LINE2,read_filename);
				}
				else
				{
					lcd_clear();
					lcd_puts(LCD_LINE1,"Reading.....");

					int i=0;
					char temmp[16];
					while(1)
					{
						sprintf(temmp,"%d",bytes_read);
						lcd_puts(LCD_LINE2,temmp);
						ch = FILE_GetCh(srcFilePtr);
						if(ch == EOF_FAT)
						{
							abBulkBuf[i]='\0';
							USBHwEPWrite(bEP,abBulkBuf,strlen(abBulkBuf)+1);
							strcpy(abBulkBuf,"__END__");
							USBHwEPWrite(bEP,abBulkBuf,8);
							break;
						}
						if(i == 63)
						{
							USBHwEPWrite(bEP, abBulkBuf, 64);
							i=0;
						}

						abBulkBuf[i++]=ch;
						++bytes_read;

					}
					FILE_Close(srcFilePtr);
					sprintf(temmp,"  %d bytes Read",bytes_read);
					lcd_puts(LCD_LINE1,temmp);
					sprintf(temmp,"%s closed",read_filename);
					lcd_puts(LCD_LINE2,temmp);
					pending=0;
					srcFilePtr=0;
				}

			}

			else
			{
				lcd_clear();
				lcd_puts(LCD_LINE1,"      ERROR2    ");
				lcd_puts(LCD_LINE2," WRONG FILE OPER");
			}

			//EMPTY_USB_BUFFER;
			break;


		case 4 ://LIST Files operation Pending
			i=0;
			int count=0,j=0,file_count=0;
			char temmp[20];
			//strcpy(abBulkBuf," ");
			while(1)
			{
				returnStatus = FILE_GetList(&fileList);
				if(returnStatus != FAT32_END_OF_FILE_LIST)
				{
					count += strlen(fileList.fileName);
					file_count=file_count+1;
					lcd_puts(LCD_LINE1,fileList.fileName);
					sprintf(temmp,"Files: %d",file_count);
					lcd_puts(LCD_LINE2,temmp);
					/*if(count < 63)
					{
						strcat(abBulkBuf,fileList.fileName);
						strcat(abBulkBuf,"\n");
						i=i+strlen(fileList.fileName)+1;

					}
					if(i>=64)
					{
					//strcpy(abBulkBuf,fileList.fileName);
						USBHwEPWrite(bEP,abBulkBuf,64);
						i=0;
						strcpy(abBulkBuf," ");
						count=0;
					}*/
					/*for(j=0;i<strlen(fileList.fileName);j++)
					{
						list_buffer[i]=fileList.fileName[j];
						i=i+1;
					}
					if(i>=64)
					{
					//strcpy(abBulkBuf,fileList.fileName);*/
						USBHwEPWrite(bEP,fileList.fileName,strlen(fileList.fileName)+1);
						/*i=0;
						strcpy(list_buffer," ");
						count=0;*/
					//}
					
				}
				if(returnStatus == FAT32_END_OF_FILE_LIST)
				{
					strcpy(abBulkBuf,"__LIST_END__");
					USBHwEPWrite(bEP,abBulkBuf,strlen(abBulkBuf)+1);
					pending=0;
					break;
				}
			}

			break;

	}
}



//void USBIntHandler(void)
void USB_IRQHandler(void)
{
	USBHwISR();
}


int main(void)
{
	SystemInit();//system clocks initialisation
	lcd_init();//lcd initialisation
	led_init();//stamp led initialisztion

	returnStatus = SD_Init(&sdcardType);//

	if(returnStatus)
	{
		if(returnStatus == SDCARD_NOT_DETECTED)
		{
			lcd_puts(LCD_LINE1,"   INSERT CARD  ");
		}
		else if(returnStatus == SDCARD_INIT_FAILED)
		{
			lcd_puts(LCD_LINE1,"Card INIT failed");
		}
		else if(returnStatus == SDCARD_FAT_INVALID)
		{
			lcd_puts(LCD_LINE1," Invalid FAT FS ");
		}
		while(1);
	}
	else
	{
		led_on();/*************led ON***************/
	}
	/***********************************************/

	// initialise stack
	USBInit();

	// register descriptors
	USBRegisterDescriptors(abDescriptors);


	// register endpoint handlers
	//	USBHwRegisterEPIntHandler(INT_IN_EP, NULL);
	USBHwRegisterEPIntHandler(BULK_IN_1_EP, BulkIn);
	USBHwRegisterEPIntHandler(BULK_OUT_1_EP, BulkOut);

	// enable bulk-in interrupts on NAKs
	USBHwNakIntEnable(INACK_BI);

	DBG("Starting USB communication\n");

	NVIC_EnableIRQ( USB_IRQn );

	// connect to bus

	DBG("Connecting to USB bus\n");
	USBHwConnect(TRUE);
	lcd_clear();
	lcd_puts(LCD_LINE1,"SD_CARD :   OK ");
	lcd_puts(LCD_LINE2,"USB     :   OK ");


	// echo any character received (do USB stuff in interrupt)
	for( ;; );
}

