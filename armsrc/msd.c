#include "proxmark3.h"
#include "apps.h"
#include "msd.h"
#include "util.h"
#include "string.h"

unsigned char ver2_card = FALSE;  //!< Flag to indicate version 2.0 SD card
unsigned char sdhc_card = FALSE;  //!< Flag to indicate version SDHC card
unsigned int sd_numsectors; //!< Total number of sectors on card
unsigned char sd_sectorbuffer[512];   //!< buffer to hold one sector of card

// TODO fix timeouts
unsigned int timeval = 0;
/**
 * Return SD card present status.
 *
 * Indicate the presence of a memory card in the SD slot.
 * \return  Card Status
 *
 */
unsigned char sd_card_detect(void)
{
/*
	PIO_PER = PIN_CARD_DETECT;
    PIO_ODR = PIN_CARD_DETECT;
    PIO_PPUER = PIN_CARD_DETECT;

    if (ISSET(pPIO->PIO_PDSR,PIN_CARD_DETECT))
        return SD_OK;
    else
        return SD_NOCARD;
*/
    return SD_OK;
}

/**
 * SD command
 *
 * Send SD command to SD card
 * \param   cmd     Card command
 * \param   arg     Argument
 *
 */
void sd_command(unsigned char cmd, unsigned int arg)
{
	spi_com(SPI_MSD_MODE,0xff,0);        // dummy byte
	spi_com(SPI_MSD_MODE,cmd | 0x40,0);  // send command
	spi_com(SPI_MSD_MODE,arg>>24,0);     // send argument
	spi_com(SPI_MSD_MODE,arg>>16,0);
	spi_com(SPI_MSD_MODE,arg>>8,0);
	spi_com(SPI_MSD_MODE,arg,0);

    switch(cmd)
    {
    	case SD_GO_IDLE_STATE:
    		spi_com(SPI_MSD_MODE,0x95,1); // CRC for CMD0
    	    break;
    	/*
    	 * CRC for CMD8 always enabled see:
    	 * Physical Layer Simplified Specification Version 2.00
    	 * chapter 7.2.2 Bus Transfer Protection
    	 */
    	case SD_SEND_IF_COND:
    		spi_com(SPI_MSD_MODE,0x87,1); // CRC for CMD8, argument 0x000001AA, see sd_init
    		break;

    	default:
    		spi_com(SPI_MSD_MODE,0xFF,1); // send dummy CRC for all other commands
    }
}

/**
 * Send dummys
 *
 * Send 10 dummy bytes to card
 *
 */
void sd_send_dummys(void)
{
    for(unsigned char i=0; i < 9; i++)
    	spi_com(SPI_MSD_MODE,0xff,0);

    spi_com(SPI_MSD_MODE,0xff,1);
}

/**
 * Get response
 *
 * Get card response tokens
 * \return Response token
 *
 */
unsigned char sd_get_response(void)
{
	unsigned int tmout = timeval + 1000;    // 1 second timeout
    unsigned char b = 0xff;

    while ((b == 0xff) && (timeval < tmout))
    {
        b = spi_com(SPI_MSD_MODE,0xff,0);
    }
    return b;
}

/**
 * Get data token
 *
 * Get card data response tokens
 * \return Data token
 *
 */
unsigned char sd_get_datatoken(void)
{
	unsigned int tmout = timeval + 1000;    // 1 second timeout
    unsigned char b = 0xff;

    while ((b != SD_STARTBLOCK_READ) && (timeval < tmout))
    {
        b = spi_com(SPI_MSD_MODE,0xff,0);
    }
    return b;
}

/**
 * Init SD card
 *
 * Init SD card
 * \return  Error code
 *
 */
char sd_init(void)
{
    // Card not initalized

	unsigned char retries;
	unsigned char resp;

    DbpString("Init SD card");

    sd_send_dummys();

    for(retries = 0, resp = 0; (retries < 5) && (resp != SD_R1_IDLE_STATE) ; retries++)
    {
        // send CMD0 to reset card
        sd_command(SD_GO_IDLE_STATE,0);
        resp = sd_get_response();

        Dbprintf("go idle resp %d", resp);
        SpinDelay(100);
    }

    if(resp != SD_R1_IDLE_STATE) return SD_E_IDLE;

    // send CMD8 to check voltage range
    // this also determines if the card is a 2.0 (or later) card
    sd_command(SD_SEND_IF_COND,0x000001AA);
    resp = sd_get_response();

    Dbprintf("CMD8resp %d", resp);

    if ((resp & SD_R1_ILLEGAL_COM) != SD_R1_ILLEGAL_COM)
    {
    	DbpString("2.0 card");
    	unsigned int r7reply;
        ver2_card = TRUE;  // mark this as a version2 card
        r7reply = sd_get_response();
        r7reply <<= 8;
        r7reply |= sd_get_response();
        r7reply <<= 8;
        r7reply |= sd_get_response();
        r7reply <<= 8;
        r7reply |= sd_get_response();

        Dbprintf("CMD8REPLY: %d", r7reply);

        // verify that we're compatible
        if ( (r7reply & 0x00000fff) != 0x01AA )
        {
        	DbpString("Voltage range mismatch");
            return SD_E_VOLT;  // voltage range mismatch, unsuable card
        }
    }
    else
    {
    	DbpString("Not a 2.0 card");
    }

    sd_send_dummys();

    /*
     * send ACMD41 until we get a 0 back, indicating card is done initializing
     * wait for max 5 seconds
     *
     */
    for (retries=0,resp=0; !resp && retries<50; retries++)
    {
    	unsigned char i;
        // send CMD55
        sd_command(SD_APP_CMD, 0);    // CMD55, prepare for APP cmd

        DbpString("Sending CMD55");

        if ((sd_get_response() & 0xFE) != 0)
        {
        	DbpString("CMD55 failed");
        }
        // send ACMD41
        DbpString("Sending ACMD41");

        if(ver2_card)
        	sd_command(SD_ACMD_SEND_OP_COND, 1UL << 30); // ACMD41, HCS bit 1
        else
        	sd_command(SD_ACMD_SEND_OP_COND, 0); // ACMD41, HCS bit 0

        i = sd_get_response();

        Dbprintf("response %d", i);

        if (i != 0)
        {
            sd_send_dummys();
            SpinDelay(100);
        }
        else
            resp = 1;

        SpinDelay(250);
    }

    if (!resp)
    {
    	DbpString("not valid");
        return SD_E_INIT;          // init failure
    }
    sd_send_dummys();     // clean up

    if (ver2_card)
    {
    	unsigned int ocr;
        // check for High Cap etc

        // send CMD58
        DbpString("sending CMD58");

        sd_command(SD_READ_OCR,0);    // CMD58, get OCR
        if (sd_get_response() != 0)
        {
        	DbpString("CMD58 failed");
        }
        else
        {
            // 0x80, 0xff, 0x80, 0x00 would be expected normally
            // 0xC0 if high cap

            ocr = sd_get_response();
            ocr <<= 8;
            ocr |= 0xff;
            ocr <<= 8;
            ocr |= sd_get_response();
            ocr <<= 8;
            ocr |= sd_get_response();

            Dbprintf("OCR %d", ocr);

            if((ocr & 0xC0000000) == 0xC0000000)
            {
            	DbpString("SDHC card");
            	sdhc_card = TRUE; // Set HC flag.
            }
        }
    }
    sd_send_dummys();     // clean up

    sd_info();

    DbpString("Init SD card OK");
    return SD_OK;
}

/**
 * Get Card Information.
 *
 * Read and print some card information and return card size in bytes
 *
 * \return     Cardsize in bytes
 * \todo This return value should be changed to allow cards > 4GB
 *
*/
unsigned int sd_info(void)
{
    int i;
    unsigned int l;
    unsigned int w;
    unsigned char b;

    unsigned int csize;
    unsigned char csize_mult;
    unsigned int blockno;
    unsigned int mult;
#define TR_SD 1
#ifdef TR_SD
    sd_send_dummys();     // cleanup

    sd_command(SD_SEND_CID,0);
    if (sd_get_datatoken() != SD_STARTBLOCK_READ)
    	DbpString("Error during CID read");
    else
    {
    	DbpString("CID read");

    	Dbprintf("Manufacturer ID: %x", spi_com(SPI_MSD_MODE,0xff,0));

        w = spi_com(SPI_MSD_MODE,0xff,0);
        w <<= 8;
        w |= spi_com(SPI_MSD_MODE,0xff,0);
        Dbprintf("OEM/Application ID %d", w);

        DbpString("Product Name: ");
        for (i=0;i<6;i++)
        	Dbprintf("%c", spi_com(SPI_MSD_MODE,0xff,0));

        Dbprintf("Product Revision: %d", spi_com(SPI_MSD_MODE,0xff,0));

        l = spi_com(SPI_MSD_MODE,0xff,0);
        l <<= 8;
        l |= spi_com(SPI_MSD_MODE,0xff,0);
        l <<= 8;
        l |= spi_com(SPI_MSD_MODE,0xff,0);
        l <<= 8;
        l |= spi_com(SPI_MSD_MODE,0xff,0);
        Dbprintf("Serial Number: %d", l);
        Dbprintf("Manuf. Date Code: %d", spi_com(SPI_MSD_MODE,0xff,0));
    }

    spi_com(SPI_MSD_MODE,0xff,0);    // skip checksum

#endif

    sd_send_dummys();     // cleanup

    sd_command(SD_SEND_CSD,0);
    if ((b = sd_get_datatoken()) != SD_STARTBLOCK_READ)
    {
    	Dbprintf("Error during CSD read, token was %d", b);
        sd_send_dummys();
        return 0;
    }
    else
    {
    	DbpString("CSD read");
    }

    // we need C_SIZE (bits 62-73 (bytes ) and C_SIZE_MULT (bits 47-49)
    //  0,  8 , 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96,104,112,120
    // 16  15   14  13  12  11  10   9   8   7   6   5   4   3   2   1

    // read 1 byte [bits 127:120]
    l = spi_com(SPI_MSD_MODE,0xff,0);

    if(l == 0) // CSD 1.0 structure
    {
    	DbpString("CSD 1.0");

    	// skip next 5 bytes [bits 119:80]
        for (i=0;i<5;i++)
        	spi_com(SPI_MSD_MODE,0xff,0);

        // get dword from [bits 79:56]
        l = spi_com(SPI_MSD_MODE,0xff,0);
        l <<= 8;
        l |= spi_com(SPI_MSD_MODE,0xff,0);
        l <<= 8;
        l |= spi_com(SPI_MSD_MODE,0xff,0);

        // shift down to to access [bits 73:62]
        l >>= 6;
        csize = (unsigned int) (l & 0x0fff);
        Dbprintf("C_SIZE %d", csize);

        // get word from [bits 55:40]
        w = spi_com(SPI_MSD_MODE,0xff,0);
        w <<= 8;
        w |= spi_com(SPI_MSD_MODE,0xff,0);

        // shift down to to access [bits 49:47]
        w >>= 7;
        csize_mult = (unsigned int) (w & 0x07);
        Dbprintf("C_SIZE_MULT %d", csize_mult);

        mult = 1 << (csize_mult+2);
        blockno = (unsigned int) ((unsigned int)csize+1) * mult;
        Dbprintf("mult %d", mult);
        Dbprintf("blockno %d", blockno);
        Dbprintf("card size %d, %d", blockno * 512L, blockno / 2048L);

        sd_send_dummys();
        sd_numsectors = blockno;

        return blockno * 512L;
    }
    else
    if( l == 0x40) // CSD 2.0 structure
    {
    	DbpString("CSD 2.0");

        // skip next 6 bytes [bits 119:72]
        for (i=0;i<6;i++)
        	spi_com(SPI_MSD_MODE,0xff,0);

        // get dword from [bits 71:48]
        l = spi_com(SPI_MSD_MODE,0xff,0);
        l <<= 8;
        l |= spi_com(SPI_MSD_MODE,0xff,0);
        l <<= 8;
        l |= spi_com(SPI_MSD_MODE,0xff,0);

        l &= 0x0000ffff; // mask c_size field

        unsigned int byte_size = ((l+1) * 524288L);

        Dbprintf("C_SIZE %d", l);
        Dbprintf("card size %d, %d", byte_size , ((l+1)>>1));

        sd_send_dummys();
        sd_numsectors = (byte_size / 512)-1;

        return byte_size;
    }
    else
    {
    	DbpString("Invalid CSD structure!");
    	sd_numsectors = 0;
    	return 0;
    }
}

/**
 * Read SD sector.
 *
 * Read a single 512 byte sector from the SD card
 * \param   lba         Logical sectornumber to read from
 * \param   buffer      Pointer to buffer for received data
 * \param   fCallback   Callback function name
 * \param   pArgument   Callback argument
 * \return 0 on success, -1 on error
 *
*/
char sd_readsector(unsigned int lba,
		unsigned char *buffer,
                     void *pArgument)
{

	Dbprintf("SDrd %d", lba);

    if(sdhc_card)
    	 // on new High Capacity cards, the lba is sent
    	sd_command(SD_READ_SINGLE_BLOCK,lba);
    else
    	sd_command(SD_READ_SINGLE_BLOCK,lba<<9);
    	// send read command and logical sector address
    	// the address sent to the card is the BYTE address
    	// so the lba needs to be multiplied by 512

    if (sd_get_response() != 0) // if no valid token
    {
        sd_send_dummys(); // cleanup and
        return SD_ERROR;   // return error code
    }

    if (sd_get_datatoken() != SD_STARTBLOCK_READ) // if no valid token
    {
        sd_send_dummys(); // cleanup and
        return SD_ERROR;   // return error code
    }

    for (unsigned int i=0; i<512 ; i++)             // read sector data
        *buffer++ = spi_com(SPI_MSD_MODE,0xff,0);

    spi_com(SPI_MSD_MODE,0xff,0);    // ignore dummy checksum
    spi_com(SPI_MSD_MODE,0xff,0);    // ignore dummy checksum

    sd_send_dummys();     // cleanup

    return SD_OK;                       // return success
}


/**
 * Read part of a SD sector.
 *
 * Read part of a single 512 byte sector from the SD card
 * \param  block   Logical sectornumber to read from
 * \param  loffset Offset to first byte we should read
 * \param  nbytes  Number of bytes to read
 * \param  buffer  Pointer to buffer for received data
 * \return 1 on success, 0 on error
 *
*/
unsigned char sd_read_n(unsigned int block,unsigned int loffset, unsigned int nbytes, unsigned char * buffer)
{
    sd_readsector(block,sd_sectorbuffer, 0);
    memcpy(buffer,&sd_sectorbuffer[loffset],nbytes);
    return 0;
}


/**
 * Write SD sector.
 *
 * Write a single 512 byte sector to the SD card
 *
 * \param   lba         Logical sectornumber to write to
 * \param   buffer      Pointer to buffer with data to send
 * \param   fCallback   Callback function name
 * \param   pArgument   Callback argument
 * \return 0 on success, -1 on error
*/
char sd_writesector(unsigned int lba,
		unsigned char *buffer,
                      void *pArgument)
{
	unsigned int i;
    unsigned int tmout;

    Dbprintf("SDwr %d", lba);

    if(sdhc_card)
    	 // on new High Capacity cards, the lba is sent
    	sd_command(SD_WRITE_BLOCK,lba);
    else
    	sd_command(SD_WRITE_BLOCK,lba<<9);
    	// send read command and logical sector address
    	// the address sent to the card is the BYTE address
    	// so the lba needs to be multiplied by 512

    if (sd_get_response() != 0) // if no valid token
    {
        sd_send_dummys(); // cleanup and
        return SD_ERROR;   // return error code
    }

    spi_com(SPI_MSD_MODE,0xfe,0);    // send data token

    for (i=0;i<512;i++)             // write sector data
    {
    	spi_com(SPI_MSD_MODE,*buffer++,0);
    }

    spi_com(SPI_MSD_MODE,0xff,0);    // send dummy checksum
    spi_com(SPI_MSD_MODE,0xff,0);    // send dummy checksum

    if ( (sd_get_response()&0x0F) != 0x05) // if no valid token
    {
        sd_send_dummys(); // cleanup and
        return SD_ERROR;   // return error code
    }

    //
    // wait while the card is busy
    // writing the data
    //
    tmout = timeval + 1000;

    // wait for the SO pin to go high
    while (1)
    {
    	unsigned char b = spi_com(SPI_MSD_MODE,0xff,0);

        if (b == 0xff) break;   // check SO high

        if (timeval > tmout)    // if timeout
        {
            sd_send_dummys();   // cleanup and
            return SD_ERROR;    // return failure
        }

    }
    sd_send_dummys(); // cleanup

    return SD_OK;   // return success
}

