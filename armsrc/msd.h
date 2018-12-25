#ifndef SDCARD_H_
#define SDCARD_H_

#define SD_GO_IDLE_STATE            0       //!< software reset
#define SD_SEND_OP_COND             1       //!< brings card out of idle state
#define SD_SEND_IF_COND             8       //!< send interface condition
#define SD_SEND_CSD                 9       //!< ask card to send card specific data (CSD)
#define SD_SEND_CID                 10      //!< ask card to send card identification (CID)
#define SD_STOP_MULTI_TRANS         12      //!< stop transmission on multiple block read
#define SD_SEND_STATUS              13      //!< ask the card to send its status register
#define SD_SET_BLOCKLEN             16      //!< sets the block length used by the memory card
#define SD_READ_SINGLE_BLOCK        17      //!< read single block
#define SD_READ_MULTI_BLOCK         18      //!< read multiple block
#define SD_WRITE_BLOCK              24      //!< writes a single block
#define SD_WRITE_MULTI_BLOCK        25      //!< writes multiple blocks
#define SD_PROGRAM_CSD              27      //!< change the bits in CSD
#define SD_SET_WRITE_PROT           28      //!< sets the write protection bit
#define SD_CLR_WRITE_PROT           29      //!< clears the write protection bit
#define SD_SEND_WRITE_PROT          30      //!< checks the write protection bit
#define SD_TAG_SECTOR_START         32      //!< Sets the address of the first sector of the erase group#define SD_TAG_SECTOR_END                      33
#define SD_SET_LAST_GROUP           33      //!< Sets the address of the last sector of the erase group
#define SD_UNTAG_SECTOR             34      //!< removes a sector from the selected group
#define SD_TAG_ERASE_GROUP_START    35      //!< Sets the address of the first group
#define SD_TAG_ERARE_GROUP_END      36      //!< Sets the address of the last erase group
#define SD_UNTAG_ERASE_GROUP        37      //!< removes a group from the selected section
#define SD_ERASE                    38      //!< erase all selected groups
#define SD_LOCK_BLOCK               42      //!< locks a block
#define SD_APP_CMD                  55      //!< next command is an application specific command
#define SD_READ_OCR                 58      //!< reads the OCR register
#define SD_CRC_ON_OFF               59      //!< turns CRC off

#define SD_ACMD_SEND_OP_COND        41      //!<  reads the OCR register

// R1 Response bit-defines
#define SD_R1_BUSY                  0x80    //!< card busy, not used
#define SD_R1_PARAMETER             0x40    //!< command’s argument out of range
#define SD_R1_ADDRESS               0x20    //!< misaligned address error
#define SD_R1_ERASE_SEQ             0x10    //!< error in the sequence of erase commands occurred
#define SD_R1_COM_CRC               0x08    //!< CRC check of the last command failed
#define SD_R1_ILLEGAL_COM           0x04    //!< illegal command code was detected
#define SD_R1_ERASE_RESET           0x02    //!< erase sequence was cleared before executing
#define SD_R1_IDLE_STATE            0x01    //!< card in idle state

// Data Start tokens
#define SD_STARTBLOCK_READ          0xFE    //!< data token single block read
#define SD_STARTBLOCK_WRITE         0xFE    //!< data token single block write
#define SD_STARTBLOCK_MWRITE        0xFC    //!< data token multi block write

// SD function return codes
enum {
    SD_OK = 0,          //!< Status OK
    SD_ERROR,           //!< Card error
    SD_NOCARD,           //!< Card not present
    SD_STATUS_READY,    //!< Card ready for new transfer
    SD_STATUS_BUSY,     //!< Card busy
    SD_E_IDLE,          //!< Go to idle mode error
    SD_E_VOLT,          //!< Voltage range mismatch error
    SD_E_INIT           //!< Card init error
};

unsigned char sd_card_detect(void);
char sd_init(void);
unsigned int sd_info(void);
char sd_readsector(unsigned int lba, unsigned char *buffer, void *pArgument);
unsigned char sd_read_n(unsigned int block,unsigned int loffset, unsigned int nbytes, unsigned char * buffer);
char sd_writesector(unsigned int lba, unsigned char *buffer, void *pArgument);
#endif /*SDCARD_H_*/
