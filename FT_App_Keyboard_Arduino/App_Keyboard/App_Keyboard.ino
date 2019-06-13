/*============================================================================
// 
// Example Ardunio/Seeduino firmware for use with the Crystalfontz
// CFAF800480E0-050SC-A1-2 display module kit.
//
// This firmware was originally sourced from the FTDI/Bridgetek website and
// has been slightly modified to work correctly with the Crystalfontz
// kit hardware.
//
// http://www.crystalfontz.com
//
//--------------------------------------------------------------------------*/
/*****************************************************************************
* Copyright (c) Bridgetek Pte Ltd.
* Software License Agreement
*
* This code is provided as an example only and is not guaranteed by Bridgetek. 
* BRIDGETEK accept no responsibility for any issues resulting from its use. 
* The developer of the final application incorporating any parts of this 
* sample project is responsible for ensuring its safe and correct operation 
* and for any consequences resulting from its use.
*****************************************************************************/
/**
* @file  App_Keyboard
* @brief Sample application to demonstrate EVE primitives and widgets.
Versoin 5.0 - April/17/2017 - Restructure source code
*/


#include "Platform.h"
#include "App_Common.h"

/* Global used for context hal */
Gpu_Hal_Context_t host,*phost;

#define ON          1
#define OFF         0						 
#define Font        27					// Font Size
#ifdef DISPLAY_RESOLUTION_WVGA
#define MAX_LINES  6					// Max Lines allows to Display
#else
#define MAX_LINES   4
#endif
#define SPECIAL_FUN     251
#define BACK_SPACE	251				// Back space
#define CAPS_LOCK	252				// Caps Lock
#define NUMBER_LOCK	253				// Number Lock
#define BACK		254				// Exit 

#define LINE_STARTPOS	DispWidth/50			// Start of Line
#define LINE_ENDPOS	DispWidth			// max length of the line

struct 
{
	uint8_t Key_Detect :1;
	uint8_t Caps :1;
	uint8_t Numeric : 1;
	uint8_t Exit : 1;	
}Flag;


struct Notepad_buffer
{
	char8_t *temp;
	char8_t  notepad[MAX_LINES][80];
}Buffer;




static uchar8_t istouch()
{
  return !(Gpu_Hal_Rd16(phost,REG_TOUCH_RAW_XY) & 0x8000);
}

uint8_t Read_Keypad()
{
  static uint8_t Read_tag=0,temp_tag=0,ret_tag=0,touch_detect=1;	
  Read_tag = Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
  ret_tag = NULL;
  if(istouch()==0)  touch_detect = 0;
  if(Read_tag!=NULL)								// Allow if the Key is released
  {
    if(temp_tag!=Read_tag && touch_detect==0)
    {
      temp_tag = Read_tag;											// Load the Read tag to temp variable	
      App_Play_Sound(phost,0x51,100,100);
      touch_detect = 1;	
    }
  }
  else
  {
    if(temp_tag!=0)
    {	
      Flag.Key_Detect = 1;
      Read_tag = temp_tag;
    }
	 temp_tag = 0;
  }
  return Read_tag;
}

uint8_t Gpu_Rom_Font_WH(uint8_t Char,uint8_t font)
{
	uint32_t ptr,Wptr;
	uint8_t Width=0;
	
	ptr = Gpu_Hal_Rd32(phost,ROMFONT_TABLEADDRESS);
// read Width of the character
	Wptr = (ptr + (148 * (font- 16)))+Char;	// (table starts at font 16)
	Width = Gpu_Hal_Rd8(phost,Wptr);
	return Width;
}

// notepad
void Notepad(void)
{
  /*local variables*/
  uint8_t Line=0;
  uint16_t Disp_pos = 0,But_opt;
  uint16_t Read_sfk=0,	tval;
  uint16_t noofchars=0,line2disp =0,nextline = 0;	
  uint8_t   font = 27;

// Clear Linebuffer
  for(tval=0;tval<MAX_LINES;tval++)
  memset(&Buffer.notepad[tval],'\0',sizeof(Buffer.notepad[tval]));

/*intial setup*/
  Line = 0;					// Starting line
  Disp_pos=LINE_STARTPOS;	                        // starting pos                                                      
  Flag.Numeric = OFF;                             // Disable the numbers and spcial charaters
  memset((Buffer.notepad[Line]+0),'_',1);	  	// For Cursor	
  Disp_pos += Gpu_Rom_Font_WH(Buffer.notepad[Line][0],Font);	// Update the Disp_Pos
  noofchars+=1;                                                   // for cursor
/*enter*/
  Flag.Exit = 0;
  do
  {
    Read_sfk = Read_Keypad();                // read the keys   
     
    if(Flag.Key_Detect)
    {                    // check if key is pressed
    Flag.Key_Detect = 0;                     // clear it  
    if(Read_sfk >= SPECIAL_FUN)
    {              // check any special function keys are pressed 
      switch(Read_sfk)
      {
        case BACK_SPACE:
          if(noofchars>1)  // check in the line there is any characters are present,cursor not include
          {
	    noofchars-=1;                      // clear the character inthe buffer
	    Disp_pos -= Gpu_Rom_Font_WH(*(Buffer.notepad[Line]+noofchars-1),Font); // Update the Disp_Pos										
          }else 
          {
            if(Line >= (MAX_LINES-1)) Line--; else Line=0;              // check the lines
            noofchars = strlen(Buffer.notepad[Line]);		    // Read the len of the line 
            for(tval=0;tval<noofchars;tval++)			     // Compute the length of the Line
            Disp_pos += Gpu_Rom_Font_WH(Buffer.notepad[Line][tval],Font);  // Update the Disp_Pos
          }
          Buffer.temp = (Buffer.notepad[Line]+noofchars);     // load into temporary buffer         									
          Buffer.temp[-1] = '_';				  // update the string  							
          Buffer.temp[0] = '\0';	
        break;

        case CAPS_LOCK:
          Flag.Caps ^= 1;        // toggle the caps lock on when the key detect
        break;
      
        case NUMBER_LOCK:
          Flag.Numeric ^= 1;    // toggle the number lock on when the key detect
        break;

        case BACK:
          for(tval=0;tval<MAX_LINES;tval++)
          memset(&Buffer.notepad[tval],'\0',sizeof(Buffer.notepad[tval]));    
          Line = 0;					// Starting line
          Disp_pos=LINE_STARTPOS;	                        // starting pos                                                      
   //       Flag.Numeric = OFF;                             // Disable the numbers and spcial charaters
          memset((Buffer.notepad[Line]+0),'_',1);	  	// For Cursor	
          Disp_pos += Gpu_Rom_Font_WH(Buffer.notepad[Line][0],Font);	// Update the Disp_Pos
          noofchars+=1;                                                
        break;
      }
    }
    else 
    {
      Disp_pos += Gpu_Rom_Font_WH(Read_sfk,Font);              // update dispos
      Buffer.temp = Buffer.notepad[Line]+strlen(Buffer.notepad[Line]);  // load into temporary buffer 
      Buffer.temp[-1] = Read_sfk;		 // update the string  									
      Buffer.temp[0] = '_';													
      Buffer.temp[1] = '\0';	
      noofchars = strlen(Buffer.notepad[Line]);    // get the string len
      if(Disp_pos > LINE_ENDPOS)                  // check if there is place to put a character in a specific line
      {
	Buffer.temp = Buffer.notepad[Line]+(strlen(Buffer.notepad[Line])-1);					
	Buffer.temp[0] = '\0';
	noofchars-=1;
	Disp_pos = LINE_STARTPOS;
	Line++;	if(Line >= MAX_LINES)	Line = 0;													
	memset((Buffer.notepad[Line]),'\0',sizeof(Buffer.notepad[Line]));	// Clear the line buffer
	for(;noofchars>=1;noofchars--,tval++)
	{
	  if(Buffer.notepad[Line-1][noofchars] == ' ' ||Buffer.notepad[Line-1][noofchars] =='.')// In case of space(New String) or end of statement(.)
	  {
            memset(Buffer.notepad[Line-1]+noofchars,'\0',1);
	    noofchars+=1;							// Include the space			
	    memcpy(&Buffer.notepad[Line],(Buffer.notepad[Line-1]+noofchars),tval);
	    break;
	  }
	}
	noofchars = strlen(Buffer.notepad[Line]);
	Buffer.temp = Buffer.notepad[Line]+noofchars;	
	Buffer.temp[0] = '_';	
	Buffer.temp[1] = '\0';						
	for(tval=0;tval<noofchars;tval++)									
	Disp_pos += Gpu_Rom_Font_WH(Buffer.notepad[Line][tval],Font);	// Update the Disp_Pos
      }
    }
  }
		
// Display List start 
  Gpu_CoCmd_Dlstart(phost); 
  App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(100,100,100));        
  App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));	
  App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  App_WrCoCmd_Buffer(phost,TAG_MASK(1));            // enable tagbuffer updation
  Gpu_CoCmd_FgColor(phost,0x703800);  
  Gpu_CoCmd_BgColor(phost,0x703800);
  But_opt = (Read_sfk== BACK)?  OPT_FLAT:0;          // button color change if the button during press
  App_WrCoCmd_Buffer(phost,TAG(BACK));														// Back		 Return to Home
  Gpu_CoCmd_Button(phost,(DispWidth*0.855),(DispHeight*0.83),(DispWidth*0.146),(DispHeight*0.112),font,But_opt,"Clear");	
  But_opt = (Read_sfk==BACK_SPACE)? OPT_FLAT:0;
  App_WrCoCmd_Buffer(phost,TAG(BACK_SPACE));													// BackSpace
  Gpu_CoCmd_Button(phost,(phost,DispWidth*0.875),(DispHeight*0.70),(DispWidth*0.125),(DispHeight*0.112),font,But_opt,"<-");	
  But_opt = (Read_sfk==' ')? OPT_FLAT:0;
  App_WrCoCmd_Buffer(phost,TAG(' '));															// Space
  Gpu_CoCmd_Button(phost,(DispWidth*0.115),(DispHeight*0.83),(DispWidth*0.73),(DispHeight*0.112),font,But_opt,"Space");

  if(Flag.Numeric==OFF)
  {
    Gpu_CoCmd_Keys(phost,0,(DispHeight*0.442),DispWidth,(DispHeight*0.112),font,Read_sfk,(Flag.Caps == ON ?"QWERTYUIOP":"qwertyuiop"));
    Gpu_CoCmd_Keys(phost,(DispWidth*0.042),(DispHeight*0.57),(DispWidth*0.96),(DispHeight*0.112),font,Read_sfk,(Flag.Caps == ON ?"ASDFGHJKL":"asdfghjkl"));
    Gpu_CoCmd_Keys(phost,(DispWidth*0.125),(DispHeight*0.70),(DispWidth*0.73),(DispHeight*0.112),font,Read_sfk,(Flag.Caps == ON ?"ZXCVBNM":"zxcvbnm"));	

    But_opt = (Read_sfk== CAPS_LOCK)? OPT_FLAT:0;
    App_WrCoCmd_Buffer(phost,TAG(CAPS_LOCK));																	// Capslock
    Gpu_CoCmd_Button(phost,0,(DispHeight*0.70),(DispWidth*0.10),(DispHeight*0.112),font,But_opt,"a^");
    But_opt = (Read_sfk== NUMBER_LOCK)? OPT_FLAT:0;
    App_WrCoCmd_Buffer(phost,TAG(NUMBER_LOCK));																// Numberlock		
    Gpu_CoCmd_Button(phost,0,(DispHeight*0.83),(DispWidth*0.10),(DispHeight*0.112),font,But_opt,"12*");
  }
  if(Flag.Numeric==ON)
  {		
    Gpu_CoCmd_Keys(phost,(DispWidth*0),(DispHeight*0.442),DispWidth,(DispHeight*0.112),font,Read_sfk,"1234567890");
    Gpu_CoCmd_Keys(phost,(DispWidth*0.042),(DispHeight*0.57),(DispWidth*0.96),(DispHeight*0.112),font,Read_sfk,"-@#$%^&*(");
    Gpu_CoCmd_Keys(phost,(DispWidth*0.125),(DispHeight*0.70),(DispWidth*0.73),(DispHeight*0.112),font,Read_sfk,")_+[]{}");				
    But_opt = (Read_sfk== NUMBER_LOCK)? OPT_FLAT:0;
    App_WrCoCmd_Buffer(phost,TAG(253));													// Numberlock
    Gpu_CoCmd_Button(phost,0,(DispHeight*0.83),(DispWidth*0.10),(DispHeight*0.112),font,But_opt,"AB*");
  }
  App_WrCoCmd_Buffer(phost,TAG_MASK(0));													// Disable the tag buffer updates
  App_WrCoCmd_Buffer(phost,SCISSOR_XY(0,0));
  App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(DispWidth,(uint16_t)(DispHeight*0.405)));	
  App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
  App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));												// Text Color
  line2disp = 0;
  while(line2disp <= Line)
  {
    nextline = 3 + (line2disp * (DispHeight*.073));
    Gpu_CoCmd_Text(phost,line2disp,nextline,font,0,(const char*)&Buffer.notepad[line2disp]);
    line2disp++;
  }
  App_WrCoCmd_Buffer(phost,DISPLAY());
  Gpu_CoCmd_Swap(phost);
  App_Flush_Co_Buffer(phost);
  Gpu_Hal_WaitCmdfifo_empty(phost);
 }while(1);		

}


#if defined(ARDUINO_PLATFORM)
prog_char8_t * const info[] =
#else
char *info[] =
#endif 
{ "EVE KeyBoard Application",

"APP to demonstrate interactive Key Board,",
"using String, Keys",
"& Buttons."
 }; 
#if defined(MSVC_PLATFORM) || defined(FT9XX_PLATFORM)
/* Main entry point */
int32_t main(int32_t argc,char8_t *argv[])
#endif
#if defined(ARDUINO_PLATFORM) || defined(MSVC_FT800EMU)
void setup()
#endif
{
    phost = &host;
    /* Init HW Hal */
    App_Common_Init(phost);
    /* Show Logo, do calibration and display welcome screeen */
    App_Common_Start(phost,info);
    /* Main application */
	Notepad();   
    /* Close all the opened handles */
    App_Common_Close(phost);      

#ifdef MSVC_PLATFORM
	return 0;
#endif
}

void loop()
{
}


/* Nothing beyond this */













