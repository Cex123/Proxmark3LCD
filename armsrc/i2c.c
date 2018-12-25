//-----------------------------------------------------------------------------
// Willok, June 2018
// Edits by Iceman, July 2018
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// The main i2c code, for communications with smart card module
//-----------------------------------------------------------------------------

#include "i2c.h"

#include <stdint.h>
#include <stdbool.h>
#include "string.h"  //for memset memcmp
#include "proxmark3.h"
#include "mifareutil.h" // for MF_DBGLEVEL
#include "BigBuf.h"
#include "apps.h"

#ifdef WITH_SMARTCARD
#include "smartcard.h"
#endif


//	������������
#define GPIO_RST  AT91C_PIO_PA1
#define GPIO_SCL  AT91C_PIO_PA5
#define GPIO_SDA  AT91C_PIO_PA7

#define SCL_H   HIGH(GPIO_SCL)
#define SCL_L   LOW(GPIO_SCL)
#define SDA_H   HIGH(GPIO_SDA)
#define SDA_L   LOW(GPIO_SDA)

#define SCL_read  (AT91C_BASE_PIOA->PIO_PDSR & GPIO_SCL)
#define SDA_read  (AT91C_BASE_PIOA->PIO_PDSR & GPIO_SDA)

#define I2C_ERROR  "I2C_WaitAck Error" 

static volatile unsigned long c;

//	ֱ��ʹ��ѭ������ʱ��һ��ѭ�� 6 ��ָ�48M�� Delay=1 ���Ϊ 200kbps
// timer.
// I2CSpinDelayClk(4) = 12.31us
// I2CSpinDelayClk(1) = 3.07us
static void __attribute__((optimize("O0"))) I2CSpinDelayClk(uint16_t delay) {
	for (c = delay * 2; c; c--) {};
}

//	ͨѶ�ӳٺ���     communication delay function	
#define I2C_DELAY_1CLK    I2CSpinDelayClk(1)
#define I2C_DELAY_2CLK    I2CSpinDelayClk(2)
#define I2C_DELAY_XCLK(x) I2CSpinDelayClk((x))


#define  ISO7618_MAX_FRAME 255

static void I2C_init(void) {
	// Configure reset pin
	AT91C_BASE_PIOA->PIO_PPUDR = GPIO_RST;  // disable pull up resistor
	AT91C_BASE_PIOA->PIO_MDDR = GPIO_RST;   // push-pull output (multidriver disabled)

	// Configure SCL and SDA pins
	AT91C_BASE_PIOA->PIO_PPUER |= (GPIO_SCL | GPIO_SDA);  // enable pull up resistor
	AT91C_BASE_PIOA->PIO_MDER |= (GPIO_SCL | GPIO_SDA);   // open drain output (multidriver enabled) - requires external pull up resistor

	// set all three outputs to high
	AT91C_BASE_PIOA->PIO_SODR |= (GPIO_SCL | GPIO_SDA | GPIO_RST);

	// configure all three pins as output, controlled by PIOA
	AT91C_BASE_PIOA->PIO_OER |= (GPIO_SCL | GPIO_SDA | GPIO_RST);
	AT91C_BASE_PIOA->PIO_PER |= (GPIO_SCL | GPIO_SDA | GPIO_RST);
}


// ���ø�λ״̬
// set the reset state
static void I2C_SetResetStatus(uint8_t LineRST, uint8_t LineSCK, uint8_t LineSDA) {
	if (LineRST)
		HIGH(GPIO_RST);
	else
		LOW(GPIO_RST);

	if (LineSCK)
		HIGH(GPIO_SCL);
	else
		LOW(GPIO_SCL);

	if (LineSDA)
		HIGH(GPIO_SDA);
	else
		LOW(GPIO_SDA);
}

// ��λ����������
// Reset the SIM_Adapter, then  enter the main program
// Note: the SIM_Adapter will not enter the main program after power up. Please run this function before use SIM_Adapter.
static void I2C_Reset_EnterMainProgram(void) {
	I2C_SetResetStatus(0, 0, 0);    // ���͸�λ��
	SpinDelay(30);
	I2C_SetResetStatus(1, 0, 0);    // �����λ
	SpinDelay(30);
	I2C_SetResetStatus(1, 1, 1);    // ����������
	SpinDelay(10);
}

//	�ȴ�ʱ�ӱ��	
// Wait for the clock to go High.	
static bool WaitSCL_H_delay(uint32_t delay) {
	while (delay--) {
		if (SCL_read) {
			return true;
		}
		I2C_DELAY_1CLK;
	}
	return false;
}

// 15000 * 3.07us = 46050us. 46.05ms
static bool WaitSCL_H(void) {
	return WaitSCL_H_delay(15000);
}

bool WaitSCL_L_delay(uint32_t delay) {
	while (delay--)	{
		if (!SCL_read) {
			return true;
		}
		I2C_DELAY_1CLK;		
	}
	return false;	
}

bool WaitSCL_L(void) {
	return WaitSCL_L_delay(15000);
}

static bool I2C_Start(void) {

	I2C_DELAY_XCLK(4);
	SDA_H; I2C_DELAY_1CLK;
	SCL_H;
	if (!WaitSCL_H()) return false;

	I2C_DELAY_2CLK;

	if (!SCL_read) return false;
	if (!SDA_read) return false;

	SDA_L; I2C_DELAY_2CLK;
	return true;
}

// send i2c STOP
static void I2C_Stop(void) {
	SCL_L; I2C_DELAY_2CLK;
	SDA_L; I2C_DELAY_2CLK;
	SCL_H; I2C_DELAY_2CLK;
	if (!WaitSCL_H()) return;
	SDA_H;
	I2C_DELAY_XCLK(8);
}

static bool I2C_WaitAck(void) {
	SCL_L; I2C_DELAY_1CLK;
	SDA_H; I2C_DELAY_1CLK;
	SCL_H;
	if (!WaitSCL_H())
		return false;

	I2C_DELAY_2CLK;
	I2C_DELAY_2CLK;
	if (SDA_read) {
		SCL_L;
		return false;
	}
	SCL_L;
	return true;
}

static void I2C_SendByte(uint8_t data) {
	uint8_t bits = 8;

	while (bits--) {
		SCL_L; I2C_DELAY_1CLK;

		if (data & 0x80)
			SDA_H;
		else
			SDA_L;

		data <<= 1;

		I2C_DELAY_1CLK;

		SCL_H;
		if (!WaitSCL_H())
			return;

		I2C_DELAY_2CLK;
	}
	SCL_L;
}

bool I2C_is_available(void) {
	I2C_init();
	I2C_Reset_EnterMainProgram();
	if (!I2C_Start())  // some other device is active on the bus
		return true;
	I2C_SendByte(I2C_DEVICE_ADDRESS_MAIN & 0xFE);
	if (!I2C_WaitAck()) {  // no response from smartcard reader
		I2C_Stop();
		return false;
	}
	I2C_Stop();
	return true;
}

#ifdef WITH_SMARTCARD
// ��λ��������ģʽ
// Reset the SIM_Adapter, then enter the bootloader program
// Reserve��For firmware update.
static void I2C_Reset_EnterBootloader(void) {
	I2C_SetResetStatus(0, 1, 1);    // ���͸�λ��
	SpinDelay(100);
	I2C_SetResetStatus(1, 1, 1);    // �����λ
	SpinDelay(10);
}

// Wait max 300ms or until SCL goes LOW.
// Which ever comes first
static bool WaitSCL_L_300ms(void) {
	volatile uint16_t delay = 310;
	while ( delay-- ) {
		// exit on SCL LOW
		if (!SCL_read)
			return true;

		SpinDelay(1);
	}
	return (delay == 0);
}

static bool I2C_WaitForSim() {
	// variable delay here.
	if (!WaitSCL_L_300ms())
		return false;

	// 8051 speaks with smart card.
	// 1000*50*3.07 = 153.5ms
	// 1byte transfer == 1ms with max frame being 256bytes
	if (!WaitSCL_H_delay(10 * 1000 * 50))
		return false;

	return true;
}

// Send i2c ACK
static void I2C_Ack(void) {
	SCL_L; I2C_DELAY_2CLK;
	SDA_L; I2C_DELAY_2CLK;
	SCL_H; I2C_DELAY_2CLK;
	if (!WaitSCL_H()) return;
	SCL_L; I2C_DELAY_2CLK;
}

// Send i2c NACK
static void I2C_NoAck(void) {
	SCL_L; I2C_DELAY_2CLK;
	SDA_H; I2C_DELAY_2CLK;
	SCL_H; I2C_DELAY_2CLK;
	if (!WaitSCL_H()) return;
	SCL_L; I2C_DELAY_2CLK;
}

static int16_t I2C_ReadByte(void) {
	uint8_t bits = 8, b = 0;

	SDA_H;
	while (bits--) {
		b <<= 1;
		SCL_L; 
		if (!WaitSCL_L()) return -2;
		
		I2C_DELAY_1CLK;

		SCL_H;
		if (!WaitSCL_H()) return -1;

		I2C_DELAY_1CLK;
		if (SDA_read)
			b |= 0x01;
	}
	SCL_L;
	return b;
}

// Sends one byte  ( command to be written, SlaveDevice address)
static bool I2C_WriteCmd(uint8_t device_cmd, uint8_t device_address) {
	bool bBreak = true;
	do {
		if (!I2C_Start())
			return false;
		//[C0]
		I2C_SendByte(device_address & 0xFE);
		if (!I2C_WaitAck())
			break;

		I2C_SendByte(device_cmd);
		if (!I2C_WaitAck())
			break;

		bBreak = false;
	} while (false);

	I2C_Stop();
	if (bBreak) {
		if ( MF_DBGLEVEL > 3 ) DbpString(I2C_ERROR);
		return false;
	}
	return true;
}

// д��1�ֽ����� ����д�����ݣ���д���ַ���������ͣ�
// Sends 1 byte data (Data to be written, command to be written , SlaveDevice address  ).
static bool I2C_WriteByte(uint8_t data, uint8_t device_cmd, uint8_t device_address) {
	bool bBreak = true;
	do {
		if (!I2C_Start())
			return false;

		I2C_SendByte(device_address & 0xFE);
		if (!I2C_WaitAck())
			break;

		I2C_SendByte(device_cmd);
		if (!I2C_WaitAck())
			break;

		I2C_SendByte(data);
		if (!I2C_WaitAck())
			break;

		bBreak = false;
	} while (false);

	I2C_Stop();
	if (bBreak) {
		if ( MF_DBGLEVEL > 3 ) DbpString(I2C_ERROR);
		return false;
	}
	return true;
}

//	д��1�����ݣ���д�������ַ����д�볤�ȣ���д���ַ���������ͣ�	
//Sends a string of data (Array, length, command to be written , SlaveDevice address  ).
// len = uint8 (max buffer to write 256bytes)
static bool I2C_BufferWrite(uint8_t *data, uint8_t len, uint8_t device_cmd, uint8_t device_address) {
	bool bBreak = true;
	do {
		if (!I2C_Start())
			return false;

		I2C_SendByte(device_address & 0xFE);
		if (!I2C_WaitAck())
			break;

		I2C_SendByte(device_cmd);
		if (!I2C_WaitAck())
			break;

		while (len) {
			
			I2C_SendByte(*data);
			if (!I2C_WaitAck())
				break;

			len--;
			data++;
		}

		if (len == 0)
			bBreak = false;
	} while (false);

	I2C_Stop();
	if (bBreak) {
		if ( MF_DBGLEVEL > 3 ) DbpString(I2C_ERROR);
		return false;
	}
	return true;
}

// ����1�����ݣ���Ŷ������ݣ����������ȣ���������ַ���������ͣ�
// read 1 strings of data (Data array, Readout length, command to be written , SlaveDevice address  ).
// len = uint8 (max buffer to read 256bytes)
static int16_t I2C_BufferRead(uint8_t *data, uint8_t len, uint8_t device_cmd, uint8_t device_address) {

	if ( !data || len == 0 )
		return 0;

	// extra wait  500us (514us measured)
	// 200us  (xx measured)
	SpinDelayUs(600);
	bool bBreak = true;
	uint16_t readcount = 0;

	do {
		if (!I2C_Start())
			return 0;

		// 0xB0 / 0xC0  == i2c write
		I2C_SendByte(device_address & 0xFE);
		if (!I2C_WaitAck())
			break;

		I2C_SendByte(device_cmd);
		if (!I2C_WaitAck())
			break;

		// 0xB1 / 0xC1 == i2c read
		I2C_Start();
		I2C_SendByte(device_address | 1);
		if (!I2C_WaitAck())
			break;

		bBreak = false;
	} while (false);

	if (bBreak) {
		I2C_Stop();
		if ( MF_DBGLEVEL > 3 ) DbpString(I2C_ERROR);
		return 0;
	}

	while (len) {

		int16_t tmp = I2C_ReadByte();
		if ( tmp < 0 )
			return tmp;
		
		*data = (uint8_t)tmp & 0xFF;
		
		len--;

		// ��ȡ�ĵ�һ���ֽ�Ϊ��������	
		// The first byte in response is the message length
		if (!readcount && (len > *data)) {
			len = *data;
		} else {
			data++;
		}
		readcount++;

		// acknowledgements. After last byte send NACK.
		if (len == 0)
			I2C_NoAck();
		else
			I2C_Ack();
	}

	I2C_Stop();
	// return bytecount - first byte (which is length byte)
	return --readcount;
}

static int16_t I2C_ReadFW(uint8_t *data, uint8_t len, uint8_t msb, uint8_t lsb, uint8_t device_address) {
	//START, 0xB0, 0x00, 0x00, START, 0xB1, xx, yy, zz, ......, STOP	
	bool bBreak = true;
	uint8_t	readcount = 0;

	// sending
	do {
		if (!I2C_Start())
			return 0;

		// 0xB0 / 0xC0  i2c write
		I2C_SendByte(device_address & 0xFE);
		if (!I2C_WaitAck())
			break;

		// msb
		I2C_SendByte(msb);
		if (!I2C_WaitAck())
			break;

		// lsb
		I2C_SendByte(lsb);
		if (!I2C_WaitAck())
			break;
		
		// 0xB1 / 0xC1  i2c read
		I2C_Start();
		I2C_SendByte(device_address | 1);
		if (!I2C_WaitAck())
			break;

		bBreak = false;
	} while (false);

	if (bBreak) {
		I2C_Stop();
		if ( MF_DBGLEVEL > 3 ) DbpString(I2C_ERROR);
		return 0;
	}

	// reading
	while (len) {
		
		int16_t tmp = I2C_ReadByte();
		if ( tmp < 0 )
			return tmp;
		
		*data = (uint8_t)tmp & 0xFF;
		
		data++;
		readcount++;
		len--;

		// acknowledgements. After last byte send NACK.		
		if (len == 0)
			I2C_NoAck();
		else
			I2C_Ack();
	}

	I2C_Stop();
	return readcount;
}

static bool I2C_WriteFW(uint8_t *data, uint8_t len, uint8_t msb, uint8_t lsb, uint8_t device_address) {
	//START, 0xB0, 0x00, 0x00, xx, yy, zz, ......, STOP	
	bool bBreak = true;

	do {
		if (!I2C_Start())
			return false;

		// 0xB0  == i2c write
		I2C_SendByte(device_address & 0xFE);
		if (!I2C_WaitAck())
			break;

		// msb
		I2C_SendByte(msb);
		if (!I2C_WaitAck())
			break;

		// lsb
		I2C_SendByte(lsb);
		if (!I2C_WaitAck())
			break;

		while (len) {
			I2C_SendByte(*data);
			if (!I2C_WaitAck())
				break;

			len--;
			data++;
		}

		if (len == 0)
			bBreak = false;
	} while (false);

	I2C_Stop();
	if (bBreak) {
		if ( MF_DBGLEVEL > 3 ) DbpString(I2C_ERROR);
		return false;
	}
	return true;
}

void I2C_print_status(void) {
	DbpString("Smart card module (ISO 7816)");
	uint8_t resp[] = {0,0,0,0};
	I2C_init();
	I2C_Reset_EnterMainProgram();
	uint8_t len = I2C_BufferRead(resp, sizeof(resp), I2C_DEVICE_CMD_GETVERSION, I2C_DEVICE_ADDRESS_MAIN);
	if ( len > 0 )
		Dbprintf("  version.................v%x.%02x", resp[0], resp[1]);
	else
		DbpString("  version.................FAILED");
}

// Will read response from smart card module,  retries 3 times to get the data.
static bool sc_rx_bytes(uint8_t* dest, uint8_t *destlen) {
 	uint8_t i = 3;
	int16_t len = 0;
	while (i--) {
		
		I2C_WaitForSim();
	
		len = I2C_BufferRead(dest, *destlen, I2C_DEVICE_CMD_READ, I2C_DEVICE_ADDRESS_MAIN);
		
		if ( len > 1 ){
			break;
		} else if ( len == 1 ) {
			continue;
		} else if ( len <= 0 ) {
			return false;	
		} 
	}
 	// after three
	if ( len <= 1 )
		return false;
	
	*destlen = (uint8_t)len & 0xFF;
	return true;
}

static bool GetATR(smart_card_atr_t *card_ptr) {

	if ( !card_ptr ) {
		return false;
	}

	card_ptr->atr_len = 0;
	memset(card_ptr->atr, 0, sizeof(card_ptr->atr));

	// Send ATR
	// start [C0 01] stop start C1 len aa bb cc stop]
	I2C_WriteCmd(I2C_DEVICE_CMD_GENERATE_ATR, I2C_DEVICE_ADDRESS_MAIN);
	uint8_t	cmd[1] = {1};
	LogTrace(cmd, 1, 0, 0, NULL, true);

	// wait for sim card to answer.
	// 1byte = 1ms, max frame 256bytes. Should wait 256ms at least just in case.
	if (!I2C_WaitForSim()) 
		return false;

	// read bytes from module
	uint8_t len = sizeof(card_ptr->atr);
	if ( !sc_rx_bytes(card_ptr->atr, &len) )		
		return false;

	uint8_t pos_td = 1;
	if ( (card_ptr->atr[1] & 0x10) == 0x10) pos_td++;
	if ( (card_ptr->atr[1] & 0x20) == 0x20) pos_td++;
	if ( (card_ptr->atr[1] & 0x40) == 0x40) pos_td++;
	
	// T0 indicate presence T=0 vs T=1.  T=1 has checksum TCK
	if ( (card_ptr->atr[1] & 0x80) == 0x80) {
		
		pos_td++;
	
		// 1 == T1 ,  presence of checksum TCK
		if ( (card_ptr->atr[pos_td] & 0x01) == 0x01) {
 			uint8_t chksum = 0;
			// xor property.  will be zero when xored with chksum.
			for (uint8_t i = 1; i < len; ++i)
				chksum ^= card_ptr->atr[i];
 			if ( chksum ) {
				if ( MF_DBGLEVEL > 2) DbpString("Wrong ATR checksum");
			}
		}
	}

	// for some reason we only get first byte of atr, if that is so, send dummy command to retrieve the rest of the atr 
	if (len == 1) {

		uint8_t data[1] = {0};
		I2C_BufferWrite(data, len, I2C_DEVICE_CMD_SEND, I2C_DEVICE_ADDRESS_MAIN);

		if ( !I2C_WaitForSim() )
			return false;

		uint8_t len2 = I2C_BufferRead(card_ptr->atr + len, sizeof(card_ptr->atr) - len, I2C_DEVICE_CMD_READ, I2C_DEVICE_ADDRESS_MAIN);
		len = len + len2;
	}

	card_ptr->atr_len = len;
	LogTrace(card_ptr->atr, card_ptr->atr_len, 0, 0, NULL, false);

	return true;
}

void SmartCardAtr(void) {
	smart_card_atr_t card;
	LED_D_ON();
	clear_trace();
	set_tracing(true);
	I2C_init();
	I2C_Reset_EnterMainProgram();
	bool isOK = GetATR( &card );
	cmd_send(CMD_ACK, isOK, sizeof(smart_card_atr_t), 0, &card, sizeof(smart_card_atr_t));
	set_tracing(false);
	LEDsoff();
}

void SmartCardRaw( uint64_t arg0, uint64_t arg1, uint8_t *data ) {

	LED_D_ON();

	uint8_t len = 0;
	uint8_t *resp = BigBuf_malloc(ISO7618_MAX_FRAME);
	smartcard_command_t flags = arg0;

	if ((flags & SC_CONNECT))
		clear_trace();

	set_tracing(true);

	if ((flags & SC_CONNECT)) {	

		I2C_init();
		I2C_Reset_EnterMainProgram();

		if ( !(flags & SC_NO_SELECT) ) {
			smart_card_atr_t card;
			bool gotATR = GetATR( &card );
			//cmd_send(CMD_ACK, gotATR, sizeof(smart_card_atr_t), 0, &card, sizeof(smart_card_atr_t));
			if ( !gotATR )
				goto OUT;
		}
	}

	if ((flags & SC_RAW)) {

		LogTrace(data, arg1, 0, 0, NULL, true);

		// Send raw bytes
		// asBytes = A0 A4 00 00 02
		// arg1 = len 5
		I2C_BufferWrite(data, arg1, I2C_DEVICE_CMD_SEND, I2C_DEVICE_ADDRESS_MAIN);

		if ( !I2C_WaitForSim() )
			goto OUT;

		// read bytes from module
		len = ISO7618_MAX_FRAME;
		sc_rx_bytes(resp, &len);
		LogTrace(resp, len, 0, 0, NULL, false);
	}
OUT:
	cmd_send(CMD_ACK, len, 0, 0, resp, len);
	set_tracing(false);
	LEDsoff();
}

void SmartCardUpgrade(uint64_t arg0) {

	LED_C_ON();

	#define I2C_BLOCK_SIZE 128
	// write.   Sector0,  with 11,22,33,44
	// erase is 128bytes, and takes 50ms to execute

	I2C_init();
	I2C_Reset_EnterBootloader();

	bool isOK = true;
	int16_t res = 0;
	uint16_t length = arg0;
	uint16_t pos = 0;
	uint8_t *fwdata = BigBuf_get_addr();
	uint8_t *verfiydata = BigBuf_malloc(I2C_BLOCK_SIZE);

	while (length) {

		uint8_t msb = (pos >> 8) & 0xFF;
		uint8_t lsb = pos & 0xFF;

		Dbprintf("FW %02X%02X", msb, lsb);

		size_t size = MIN(I2C_BLOCK_SIZE, length);

		// write
		res = I2C_WriteFW(fwdata+pos, size, msb, lsb, I2C_DEVICE_ADDRESS_BOOT);
		if ( !res ) {
			DbpString("Writing failed");
			isOK = false;
			break;
		}

		// writing takes time.
		SpinDelay(50);

		// read
		res = I2C_ReadFW(verfiydata, size, msb, lsb, I2C_DEVICE_ADDRESS_BOOT);
		if ( res <= 0) {
			DbpString("Reading back failed");
			isOK = false;
			break;
		}

		// cmp
		if ( 0 != memcmp(fwdata+pos, verfiydata, size)) {
			DbpString("not equal data");
			isOK = false;
			break;
		}

		length -= size;
		pos += size;
	}
	cmd_send(CMD_ACK, isOK, pos, 0, 0, 0);
	LED_C_OFF();
}

// unfinished (or not needed?)
//void SmartCardSetBaud(uint64_t arg0) {
//}

void SmartCardSetClock(uint64_t arg0) {
	LED_D_ON();
	set_tracing(true);
	I2C_init();
	I2C_Reset_EnterMainProgram();

	// Send SIM CLC
	// start [C0 05 xx] stop
	I2C_WriteByte(arg0, I2C_DEVICE_CMD_SIM_CLC, I2C_DEVICE_ADDRESS_MAIN);

	cmd_send(CMD_ACK, 1, 0, 0, 0, 0);
	set_tracing(false);
	LEDsoff();
}

#endif