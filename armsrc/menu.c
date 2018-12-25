//-----------------------------------------------------------------------------
// Cex, December 2018
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// The menus and button handler for PM3LCD
//-----------------------------------------------------------------------------

#include "proxmark3.h"
#include "LCD.h"
#include "fonts.h"
#include "apps.h"
#include "BigBuf.h"
#include "printf.h"
#include "util.h"
#include "iso14443a.h"
#include "string.h"
#include "lfsampling.h"

#define MAIN_MENU_ENTRIES 3
#define LF_MENU_ENTRIES 7
#define HF_MENU_ENTRIES 2
#define HW_MENU_ENTRIES 1

static int CButton=0;
static int curY=1;
static int submenu=0;
static int menuEntries=1;


//HID tag ID
static int HIDhigh2=0,HIDhigh=0, HIDlow=0;

//14a UID
static uint32_t uid_1st = 0;
static uint32_t uid_2nd = 0;


//Used for raw reading
void PrepBuffer()
{
	int i, max, min;
	uint8_t *dest = BigBuf_get_addr();;
	int n = BigBuf_max_traceLen();

	max=127;
	min=127;
	// prepare the buffer, find min and max, then use average as threshold
	for (i=0; i<n; i++) {
		if (dest[i]>max) max=dest[i];
		if (dest[i]<min) min=dest[i];
	}
	min = (max + min) / 2;
	for (i=0; i<n; i++) {
		if (dest[i] > min ) dest[i]=1;
		else dest[i]=0;
	}
}

//Used for LF search
bool graphJustNoise(uint8_t *BitStream, int size)
{
	static const uint8_t THRESHOLD = 15; //might not be high enough for noisy environments
	//test samples are not just noise
	bool justNoise1 = 1;
	for(int idx=0; idx < size && justNoise1 ;idx++){
		justNoise1 = BitStream[idx] < THRESHOLD;
	}
	return justNoise1;
}

//Show the main menu
void Show_Menu(void) {
	//Set level and number on entries
	submenu=0;
	menuEntries=MAIN_MENU_ENTRIES;
	
	// Show menu
	LCDString("  LF options         ",(char *)&FONT6x8,1,1+8*0,GREEN,WHITE);
	LCDString("  HF options         ",(char *)&FONT6x8,1,1+8*1,RED,WHITE);
	LCDString("  HW options         ",(char *)&FONT6x8,1,1+8*2,MAGENTA,WHITE);
	LCDFill(0, 1+8*3, 132, 8, WHITE);
	LCDFill(0, 1+8*4, 132, 8, WHITE);
	LCDFill(0, 1+8*5, 132, 8, WHITE);
	LCDFill(0, 1+8*6, 132, 8, WHITE);
	LCDFill(0, 1+8*7, 132, 8, WHITE);
	LCDFill(0, 1+8*8, 132, 8, WHITE);
	LCDFill(0, 1+8*9, 132, 8, WHITE);
	LCDFill(0, 1+8*10, 132, 8, WHITE);
	LCDFill(0, 1+8*11, 132, 8, WHITE);
	LCDFill(0, 1+8*12, 132, 8, WHITE);
	// Selected option
	LCDString("* ",(char *)&FONT6x8,1,1+8*0,BLACK,WHITE);

	//********************
	//TEST show LCD id
	char texto[24];
	sprintf(texto, "LCDid:%06lX",LCDGetId());
	LCDString(texto,     (char *)&FONT6x8,1,1+8*15,BLACK,WHITE);
	//TEST show battery voltage
	sprintf(texto,"Vb:%d",((MAX_ADC_BATTERY_VOLTAGE * AvgAdc(ADC_CHAN_BATT)) >> 10));
	LCDString(texto,     (char *)&FONT6x8,1+6*14,1+8*15,BLACK,WHITE);
	//********************
}	

//Show the LF menu
void Show_LF_Menu(void) {
	//Set level and number on entries
	submenu=1;
	menuEntries=LF_MENU_ENTRIES;
	curY=1;
	// Show menu
	LCDString("  Read HID tag       ",(char *)&FONT6x8,1,1+8*0,GREEN,WHITE);
	LCDString("  Replay HID tag     ",(char *)&FONT6x8,1,1+8*1,RED,WHITE);
	LCDString("  Copy HID to T5557  ",(char *)&FONT6x8,1,1+8*2,MAGENTA,WHITE);
	LCDString("  ProxBrute HID tag  ",(char *)&FONT6x8,1,1+8*3,CYAN,WHITE);
	LCDString("  Read raw LF tag    ",(char *)&FONT6x8,1,1+8*4,YELLOW,WHITE);
	LCDString("  Replay raw LF tag  ",(char *)&FONT6x8,1,1+8*5,BLUE,WHITE);
	LCDString("  LF search          ",(char *)&FONT6x8,1,1+8*6,MAGENTA,WHITE);
	LCDFill(0, 1+8*8, 132, 8, WHITE);
	LCDFill(0, 1+8*9, 132, 8, WHITE);
	LCDFill(0, 1+8*10, 132, 8, WHITE);
	LCDFill(0, 1+8*11, 132, 8, WHITE);
	LCDFill(0, 1+8*12, 132, 8, WHITE);
	// Selected option
	LCDString("* ",(char *)&FONT6x8,1,1+8*0,BLACK,WHITE);

	//TEST show battery voltage
	char texto[24];
	sprintf(texto,"Vb:%d",((MAX_ADC_BATTERY_VOLTAGE * AvgAdc(ADC_CHAN_BATT)) >> 10));
	LCDString(texto,     (char *)&FONT6x8,1+6*14,1+8*15,BLACK,WHITE);
	//********************
}	

//Show the HF menu
void Show_HF_Menu(void) {
	//Set level and number on entries
	submenu=2;
	menuEntries=HF_MENU_ENTRIES;
	curY=1;
	// Show menu
	LCDString("  ISO14A reader      ",(char *)&FONT6x8,1,1+8*0,GREEN,WHITE);
	LCDString("  Copy UID to magicMF",(char *)&FONT6x8,1,1+8*1,RED,WHITE);
	LCDFill(0, 1+8*2, 132, 8, WHITE);
	LCDFill(0, 1+8*3, 132, 8, WHITE);
	LCDFill(0, 1+8*4, 132, 8, WHITE);
	LCDFill(0, 1+8*5, 132, 8, WHITE);
	LCDFill(0, 1+8*6, 132, 8, WHITE);
	LCDFill(0, 1+8*7, 132, 8, WHITE);
	LCDFill(0, 1+8*8, 132, 8, WHITE);
	LCDFill(0, 1+8*9, 132, 8, WHITE);
	LCDFill(0, 1+8*10, 132, 8, WHITE);
	LCDFill(0, 1+8*11, 132, 8, WHITE);
	LCDFill(0, 1+8*12, 132, 8, WHITE);
	// Selected option
	LCDString("* ",(char *)&FONT6x8,1,1+8*0,BLACK,WHITE);

	//********************
	//TEST show battery voltage
	char texto[24];
	sprintf(texto,"Vb:%d",((MAX_ADC_BATTERY_VOLTAGE * AvgAdc(ADC_CHAN_BATT)) >> 10));
	LCDString(texto,     (char *)&FONT6x8,1+6*14,1+8*15,BLACK,WHITE);
	//********************
}	

//Show the HW menu
void Show_HW_Menu(void) {
	//Set level and number on entries
	submenu=3;
	menuEntries=HW_MENU_ENTRIES;
	curY=1;
	// Show menu
	LCDString("  Antenna tune       ",(char *)&FONT6x8,1,1+8*0,GREEN,WHITE);
	LCDFill(0, 1+8*1, 132, 8, WHITE);
	LCDFill(0, 1+8*2, 132, 8, WHITE);
	LCDFill(0, 1+8*3, 132, 8, WHITE);
	LCDFill(0, 1+8*4, 132, 8, WHITE);
	LCDFill(0, 1+8*5, 132, 8, WHITE);
	LCDFill(0, 1+8*6, 132, 8, WHITE);
	LCDFill(0, 1+8*7, 132, 8, WHITE);
	LCDFill(0, 1+8*8, 132, 8, WHITE);
	LCDFill(0, 1+8*9, 132, 8, WHITE);
	LCDFill(0, 1+8*10, 132, 8, WHITE);
	LCDFill(0, 1+8*11, 132, 8, WHITE);
	LCDFill(0, 1+8*12, 132, 8, WHITE);
	// Selected option
	LCDString("* ",(char *)&FONT6x8,1,1+8*0,BLACK,WHITE);

	//********************
	//TEST show battery voltage
	char texto[24];
	sprintf(texto,"Vb:%d",((MAX_ADC_BATTERY_VOLTAGE * AvgAdc(ADC_CHAN_BATT)) >> 10));
	LCDString(texto,     (char *)&FONT6x8,1+6*14,1+8*15,BLACK,WHITE);
	//********************
}	

//Perform the selected LF action
void perform_LF_action(void)
{
	char TagID[25];
	int i;

	switch (curY) {
		case 1:
			LCDString("Reading HID tag...",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			LCDFill(0, 1+8* 10, 132, 8, WHITE);
			LCDFill(0, 1+8* 11, 132, 8, WHITE);
			LCDFill(0, 1+8* 12, 132, 8, WHITE);
			CmdHIDdemodFSK(1, &HIDhigh2, &HIDhigh, &HIDlow, 0);
			PWMC_Beep(1,10000,50);
			if (HIDhigh2 !=0) sprintf(TagID,"TAG: %X%08X%08X",HIDhigh2,HIDhigh,HIDlow);
			else sprintf(TagID,"TAG: %X%08X",HIDhigh,HIDlow);
			LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*10,BLACK,WHITE);
			sprintf(TagID,"ID: %05d",(HIDlow>>1)&0xFFFF);
			LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE);
			break;

		case 2:
			LCDFill(0, 1+8* 12, 132, 8, WHITE);
			LCDString("Replaying HID tag...",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			CmdHIDsimTAG(HIDhigh2, HIDhigh, HIDlow, 0);
			break;

		case 3:
			LCDFill(0, 1+8* 12, 132, 8, WHITE);
			if (HIDhigh | HIDlow){
				 LCDString("T5557 write...     ",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
				 if (HIDhigh2 !=0) CopyHIDtoT55x7(HIDhigh2, HIDhigh, HIDlow, 1);
				 else CopyHIDtoT55x7(0, HIDhigh, HIDlow, 0);
				 LCDString("DONE!",(char *)&FONT6x8,2+6*14,1+8*9,BLACK,WHITE);
				 PWMC_Beep(1,10000,50); // this make beep-beep
				 PWMC_Beep(1,20000,100);
				 PWMC_Beep(1,25000,100);
			}
			else LCDString("Invalid HID data",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);   
			break;

		case 4:  // Find a valid tag ID by brute forcing, starting at a readed tag
			LCDString("Reading HID tag...",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			LCDFill(0, 1+8* 10, 132, 8, WHITE);
			LCDFill(0, 1+8* 11, 132, 8, WHITE);
			LCDFill(0, 1+8* 12, 132, 8, WHITE);
			CmdHIDdemodFSK(1, &HIDhigh2, &HIDhigh, &HIDlow, 0);
			PWMC_Beep(1,10000,50);
			if (HIDhigh2 !=0) sprintf(TagID,"TAG: %X%08X%08X",HIDhigh2,HIDhigh,HIDlow);
			else sprintf(TagID,"TAG: %X%08X",HIDhigh,HIDlow);
			LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*10,BLACK,WHITE);
			sprintf(TagID,"ID: %05d",(HIDlow>>1)&0xFFFF);
			LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE);
			LCDString("Press button to start",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			while (!BUTTON_PRESS()); // wait for button to be pressed to start trying IDs
			while (BUTTON_PRESS()); // wait for button to be released to start trying IDs
			SpinDelay(200); // avoid switch bounces
			LCDString("Trying...            ",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			for (i=HIDlow-1;i>0;i--) {
				if(BUTTON_PRESS()) {
					LCDString("STOP      ",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
					break;
				}
				if (HIDhigh2 !=0) sprintf(TagID,"TAG: %X%08X%08X",HIDhigh2,HIDhigh,HIDlow);
				else sprintf(TagID,"TAG: %X%08X",HIDhigh,HIDlow);
				LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*10,BLACK,WHITE);
				sprintf(TagID,"ID: %05d",(i>>1)&0xFFFF);
				LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE);
				CmdHIDsimTAG(HIDhigh2, HIDhigh, i, 0); // Try ID
				SpinDelay(500);
			}
			break;

		case 5:
			LCDString("Reading raw tag...",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			LCDFill(0, 1+8* 10, 132, 8, WHITE);
			LCDFill(0, 1+8* 11, 132, 8, WHITE);
			LCDFill(0, 1+8* 12, 132, 8, WHITE);
			AcquireRawAdcSamples125k(0);
			PrepBuffer();
			PWMC_Beep(1,10000,50);
			break;

		case 6:
			LCDString("Replaying raw tag...",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			LCDFill(0, 1+8* 10, 132, 8, WHITE);
			LCDFill(0, 1+8* 11, 132, 8, WHITE);
			LCDFill(0, 1+8* 12, 132, 8, WHITE);
			SimulateTagLowFrequency(BigBuf_max_traceLen(),0, 1);
			break;

		case 7:
			LCDString("Searching for LF tags.",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			LCDFill(0, 1+8* 10, 132, 8, WHITE);
			LCDFill(0, 1+8* 11, 132, 8, WHITE);
			LCDFill(0, 1+8* 12, 132, 8, WHITE);

//			uint32_t wordData = 0;
//			int ans=0;
			int ID2, ID1, ID0;

			SampleLF(true, 30000);

			// only run if buffer is just noise as it should be for hitag/cotag
			if (graphJustNoise(BigBuf_get_addr(), 1000)) {
				LCDString("Just noise!",(char *)&FONT6x8,2+6*1,1+8*10,BLACK,WHITE);
//				// test for em4x05 in reader talk first mode.
//				if (EM4x05Block0Test(&wordData)) {
//					PrintAndLog("\nValid EM4x05/EM4x69 Chip Found\nUse lf em 4x05readword/dump commands to read\n");
//					break;
//				}
//				ans=CmdLFHitagReader("26"); // 26 = RHT2F_UID_ONLY
//				if (ans==0) {
//					break;
//				}
//				ans=CmdCOTAGRead("");
//				if (ans>0) {
//					PrintAndLog("\nValid COTAG ID Found!");
//					break;
//				}
//				PrintAndLog("\nNo Data Found! - maybe not an LF tag?\n");
//				break;
			}
			
			// Check for other LF tags
			CmdIOdemodFSK(1, &ID1, &ID0, 0);
			if ((ID1 != 0) || (ID0 != 0)) {
				LCDString("IO Prox tag found!",(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE);
				break;
			}

//			ans=CmdFSKdemodPyramid("");
//			if (ans>0) {
//				PrintAndLog("\nValid Pyramid ID Found!");
//				return CheckChipType(cmdp);
//			}
//			ans=CmdFSKdemodParadox("");
//			if (ans>0) {
//				PrintAndLog("\nValid Paradox ID Found!");
//				return CheckChipType(cmdp);
//			}

			CmdAWIDdemodFSK(1, &ID1, &ID0, 0);
			if ((ID1 != 0) || (ID0 != 0)) {
				LCDString("AWID tag found!",(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE);
				break;
			}
			CmdHIDdemodFSK(1, &ID2, &ID1, &ID0, 0);
			if ((ID2 != 0) || (ID1 != 0) || (ID0 != 0)) {
				LCDString("HID tag found!",(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE);
				break;
			}
			CmdEM410xdemod(1, &ID1, &ID0, 0);
			if ((ID1 != 0) || (ID0 != 0)) {
				LCDString("EM410x tag found!",(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE);
				break;
			}

//			ans=CmdVisa2kDemod("");
//			if (ans>0) {
//				PrintAndLog("\nValid Visa2000 ID Found!");
//				return CheckChipType(cmdp);
//			}
//			ans=CmdG_Prox_II_Demod("");
//			if (ans>0) {
//				PrintAndLog("\nValid G Prox II ID Found!");
//				return CheckChipType(cmdp);
//			}
//			ans=CmdFdxDemod(""); //biphase
//			if (ans>0) {
//				PrintAndLog("\nValid FDX-B ID Found!");
//				return CheckChipType(cmdp);
//			}
//			ans=EM4x50Read("", false); //ask
//			if (ans>0) {
//				PrintAndLog("\nValid EM4x50 ID Found!");
//				return 1;
//			}
//			ans=CmdJablotronDemod("");
//			if (ans>0) {
//				PrintAndLog("\nValid Jablotron ID Found!");
//				return CheckChipType(cmdp);
//			}
//			ans=CmdNoralsyDemod("");
//			if (ans>0) {
//				PrintAndLog("\nValid Noralsy ID Found!");
//				return CheckChipType(cmdp);
//			}
//			ans=CmdSecurakeyDemod("");
//			if (ans>0) {
//				PrintAndLog("\nValid Securakey ID Found!");
//				return CheckChipType(cmdp);
//			}
//			ans=CmdVikingDemod("");
//			if (ans>0) {
//				PrintAndLog("\nValid Viking ID Found!");
//				return CheckChipType(cmdp);
//			}
//			ans=CmdIndalaDecode(""); //psk
//			if (ans>0) {
//				PrintAndLog("\nValid Indala ID Found!");
//				return CheckChipType(cmdp);
//			}
//			ans=CmdPSKNexWatch("");
//			if (ans>0) {
//				PrintAndLog("\nValid NexWatch ID Found!");
//				return CheckChipType(cmdp);
//			}
//			ans=CmdPacDemod("");
//			if (ans>0) {
//				PrintAndLog("\nValid PAC/Stanley ID Found!");
//				return CheckChipType(cmdp);
//			}
//			PrintAndLog("\nNo Known Tags Found!\n");
			LCDString("No Known Tag Found!",(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE);
			break;
		default:
			break;
	}

}

//Perform the selected HF action
void perform_HF_action(void)
{
	char TagID[25];
	uint32_t uid_tmp1 = 0;
	uint32_t uid_tmp2 = 0;
	iso14a_card_select_t hi14a_card;

	switch (curY) {
		case 1:
			LCDString("Reading ISO14a tag...",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			LCDFill(0, 1+8* 10, 132, 8, WHITE);
			LCDFill(0, 1+8* 11, 132, 8, WHITE);
			LCDFill(0, 1+8* 12, 132, 8, WHITE);
			/* Code for reading from 14a tag */
			uint8_t uid[10] ={0};
			uint32_t cuid;
			iso14443a_setup(FPGA_HF_ISO14443A_READER_MOD);
			for ( ; ; )
			{
				WDT_HIT();
				if (BUTTON_PRESS()) {
					break;
				}
				if (!iso14443a_select_card(uid, &hi14a_card, &cuid, true, 0, true))
					continue;
				else
				{
					PWMC_Beep(1,10000,50);
					uint8_t *dst = (uint8_t *)&uid_tmp1;
					// Set UID byte order
					for (int i=0; i<4; i++)
						dst[i] = uid[3-i];
					dst = (uint8_t *)&uid_tmp2;
					for (int i=0; i<4; i++)
						dst[i] = uid[7-i];
					if (uid_tmp2) {
						uid_1st = (uid_tmp1)>>8;
						uid_2nd = (uid_tmp1<<24) + (uid_tmp2>>8);
						sprintf(TagID,"UID7: %06lX%08lX", uid_1st, uid_2nd);
						LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*10,BLACK,WHITE);
					}
					else {
						uid_1st = uid_tmp1;
						uid_2nd = uid_tmp2;
						sprintf(TagID,"UID4: %08lX", uid_1st);
						LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*10,BLACK,WHITE);
					}
					sprintf(TagID,"ATQA = %02X%02X",hi14a_card.atqa[0],hi14a_card.atqa[1]);
					LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE);
					switch (hi14a_card.sak) {
						case 0x00: LCDString("MF ultralight",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						case 0x08: LCDString("MF classic 1K",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						case 0x09: LCDString("MF mini",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						case 0x10: LCDString("MF+ 1k",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						case 0x11: LCDString("MF+ 4k",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						case 0x18: LCDString("MF classic 4K",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						case 0x20: LCDString("MF DES|+|JCOP",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						case 0x28: LCDString("JCOP31/41 v2.3.1",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						case 0x38: LCDString("Nokia MF classic 4K",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						case 0x88: LCDString("Infineon MF classic 1K",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						case 0x98: LCDString("Gemplus MPCOS",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); break;
						default: sprintf(TagID,"SAK = %02X",hi14a_card.sak); 
							       LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE);
							       break;
					}
					
					break;
				}
			}
			break;
			
		case 2:
			LCDString("Writting MFC UID...",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			LCDFill(0, 1+8* 11, 132, 8, WHITE);
			LCDFill(0, 1+8* 12, 132, 8, WHITE);
			
			uint8_t oldBlock0[16] = {0}, newBlock0[16] = {0}, testBlock0[16] = {0};
			// arg0 = Flags == CSETBLOCK_SINGLE_OPER=0x1F, arg1=returnSlot, arg2=blockNo
			MifareCGetBlock(0x3F, 1, 0, oldBlock0);
			if (oldBlock0[0] == 0 && oldBlock0[0] == oldBlock0[1]  && oldBlock0[1] == oldBlock0[2] && oldBlock0[2] == oldBlock0[3]) {
				LCDString("No changeable tag!",(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE); 
			}
			else {
				sprintf(TagID,"target UID: %02X%02X%02X%02X", oldBlock0[0],oldBlock0[1],oldBlock0[2],oldBlock0[3]);
				LCDString((char*)&TagID,(char *)&FONT6x8,2+6*1,1+8*11,BLACK,WHITE);
				memcpy(newBlock0,oldBlock0,16);
				// Copy uid_1st (2nd is for longer UIDs not supported if classic)
				newBlock0[0] = uid_1st>>24;
				newBlock0[1] = 0xFF & (uid_1st>>16);
				newBlock0[2] = 0xFF & (uid_1st>>8);
				newBlock0[3] = 0xFF & (uid_1st);
				newBlock0[4] = newBlock0[0]^newBlock0[1]^newBlock0[2]^newBlock0[3];
				// arg0 = needWipe, arg1 = workFlags, arg2 = blockNo, datain
				MifareCSetBlock(0, 0xFF,0, newBlock0);
				MifareCGetBlock(0x3F, 1, 0, testBlock0);
				if (memcmp(testBlock0,newBlock0,16)==0)
				{
					LCDString("Clone successfull!",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); 
				}
				else {
					LCDString("Clone failed!",(char *)&FONT6x8,2+6*1,1+8*12,BLACK,WHITE); 
				}
			}
			break;
		default:
			break;
	}
}

//Perform the selected HW action
void perform_HW_action(void)
{
	char texto[24];
	
	switch (curY) {
		case 1:
			LCDString("Measuring antennas...",(char *)&FONT6x8,2,1+8*9,BLACK,WHITE);
			LCDFill(0, 1+8* 10, 132, 8, WHITE);
			LCDFill(0, 1+8* 11, 132, 8, WHITE);
			LCDFill(0, 1+8* 12, 132, 8, WHITE);
			FpgaWriteConfWord(FPGA_MAJOR_MODE_LF_ADC | FPGA_LF_ADC_READER_FIELD);
			SpinDelay(50);
			WDT_HIT();
			FpgaSendCommand(FPGA_CMD_SET_DIVISOR, 95); // voltage at 125Khz
			SpinDelay(20);
			sprintf(texto,"125KHz: %dmV",((MAX_ADC_LF_VOLTAGE * AvgAdc(ADC_CHAN_LF)) >> 10));
			LCDString(texto,     (char *)&FONT6x8,1+6*1,1+8*10,BLACK,WHITE);
			FpgaSendCommand(FPGA_CMD_SET_DIVISOR, 89); // voltage at 134Khz
			SpinDelay(20);
			sprintf(texto,"134KHz: %dmV",((MAX_ADC_LF_VOLTAGE * AvgAdc(ADC_CHAN_LF)) >> 10));
			LCDString(texto,     (char *)&FONT6x8,1+6*1,1+8*11,BLACK,WHITE);
			FpgaWriteConfWord(FPGA_MAJOR_MODE_HF_READER_RX_XCORR);
			SpinDelay(20);
			sprintf(texto,"13.56MHz: %dmV",((MAX_ADC_HF_VOLTAGE_LOW * AvgAdc(ADC_CHAN_HF_LOW)) >> 10));
			LCDString(texto,     (char *)&FONT6x8,1+6*1,1+8*12,BLACK,WHITE);
			FpgaWriteConfWord(FPGA_MAJOR_MODE_OFF);
			break;
		default:
			break;
	}
}


//Joystick buttons reader
void Check_Button(void)
{
	CButton=0;
	while(BUTTON_PRESS()) {
		CButton=1;
		WDT_HIT();
	}
	while(BUTTON_UP()) {
		CButton=2;
		WDT_HIT();
	}
	while(BUTTON_DOWN()) {
		CButton=3;
		WDT_HIT();
	}
	while(BUTTON_LEFT()) {
		CButton=4;
		WDT_HIT();
	}
	while(BUTTON_RIGHT()) {
		CButton=5;
		WDT_HIT();
	}
	//LED_A_ON();
	if (CButton != 0) SpinDelay(200); // filter switch bounces.
}
//Perform actions based on menu item selected
void Action_Button(void)
{
	switch (CButton) {
		// Center
		case 1:
			PWMC_Beep(1,5000,100);
			break;

		// Up
		case 2:
			PWMC_Beep(1,10000,50);
			LCDFill(0, 1+8*(curY-1), 6, 8, WHITE);
//			LCDFill(0, 1+8* 8, 132, 8, WHITE);
//			LCDFill(0, 1+8* 10, 132, 8, WHITE);
//			LCDFill(0, 1+8* 11, 132, 8, WHITE);
			if(curY<2) curY=menuEntries;
			else curY--;
			if (curY>3) LCDFill(0, 1+8* 9, 132, 8, WHITE);
			LCDString("* ",(char *)&FONT6x8,1,1+8*(curY-1),BLACK,WHITE);
			break;

		// Down
		case 3:
			PWMC_Beep(1,10000,50);
			LCDFill(0, 1+8*(curY-1), 6, 8, WHITE);
//			LCDFill(0, 1+8* 8, 132, 8, WHITE);
//			LCDFill(0, 1+8* 10, 132, 8, WHITE);
//			LCDFill(0, 1+8* 11, 132, 8, WHITE);
			if(curY>(menuEntries-1)) curY=1;
			else curY++;
			if (curY>3) LCDFill(0, 1+8* 9, 132, 8, WHITE);
			LCDString("* ",(char *)&FONT6x8,1,1+8*(curY-1),BLACK,WHITE);
			break;

		// Left
		case 4:
			PWMC_Beep(1,10000,50);
			if (submenu>0) Show_Menu();
			break;

		// Right
		case 5:
			PWMC_Beep(1,10000,50);
			switch(submenu) {
				//Main
				case 0:
					switch (curY) {
						case 1: 
							Show_LF_Menu();
							break;
						case 2:
							Show_HF_Menu();
							break;
						case 3:
							Show_HW_Menu();
							break;
						default:
							break;
					}
					break;
				// LF
				case 1:
					perform_LF_action();
					break;
				// HF
				case 2:
					perform_HF_action();
					break;
				// HW
				case 3:
					perform_HW_action();
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}	

