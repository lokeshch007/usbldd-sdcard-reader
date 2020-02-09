#include "ssp.h"

void ssp_init(void){
				
				/**SSP Pinselection**/
				SSP_PINS_SELECT_CLEAR();
				SSP_PINS_SELECT_SET();

				/**SSEL pin made as output and disabled**/
				SSP_MAKE_SSEL_OUTPUT();
				SSP_DISABLE_SLAVE();

				//set up prescalar 18 for SSP
				SSP_SET_PRESCALAR(CPSR_Val);
				
				//SSP setting as 8-Bit transfer
				SSP_SET_8BIT();

				//setting CPOL CPHA
				SSP_SET_CPOL_CPHA();

				//setting SCR
				SSP_SET_SCR();

				//enabling SSP
				SSP_ENABLE();

}

uint8_t ssp_write(uint8_t val){
				uint16_t temp;
				//enable slave 
				SSP_ENABLE_SLAVE();

				//write value to DR register
				DR_WRITE(val);
				
				//wait for acknowledgement so RNE becomes 1
				while(RNE_VAL == 0);

				//read DR to clear it
				temp=DR_VALUE;

				//disabling slave by setting P0.16
				SSP_DISABLE_SLAVE();
				
				//return received data from Slave
				return temp;
}

uint8_t ssp_read(void){
				uint8_t spiData_u8,dummy_u8;

				//write 0xff to DR register
				DR_WRITE(0xff);

				//wait until data is received
				while(RNE_VAL == 0);

				//read Status register to clear flags
				dummy_u8=STATUS_REGISTER;

				//read data from DR register
				spiData_u8=DR_VALUE;

				return spiData_u8;

}
