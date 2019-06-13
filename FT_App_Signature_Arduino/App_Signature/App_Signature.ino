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
* @file  App_Signature.c/.ino
* @brief Sample application to demonstrate EVE primitives and widgets.
Versoin 5.0 - April/17/2017 - Restructure source code
Version 4.0 - July/01/2015 - Added FT81x support
version 3.1 - Jun/13/2014 - Added CMD_CSKETCH for FT801 platform. 
Version 3.0 - Support for FT801 platform.
Version 2.0 - Support for FT800 emulator platform.
Version 1.0 - Final version based on the requirements.
Version 0.1 - initial draft of the release notes
*/



#include "Platform.h"
#include "App_Common.h"


/* Global used for context hal */
Gpu_Hal_Context_t host,*phost;


static void rotate_around(int16_t x, int16_t y, int16_t a)
{
  Gpu_CoCmd_LoadIdentity(phost);
  Gpu_CoCmd_Translate(phost,F16(x),F16(y));
  Gpu_CoCmd_Rotate(phost,a);
  Gpu_CoCmd_Translate(phost,F16(-x),F16(-y));
  Gpu_CoCmd_SetMatrix(phost);
}

void Signature()
{
  uint16_t w,h,x,y,tag;
	
  int16_t sw = 2 * DispWidth / 3;
  int16_t sh = sw / 3;
  int16_t ox = ( DispWidth - sw) / 2;
  int16_t oy = (2 * DispHeight / 3) - sh;
  uint16_t a = 0; 
  
  x = DispWidth*0.168;
  y = DispHeight*0.317;
  w =  DispWidth-(2*x);
  h =  DispHeight-(2.5*y);
  
 
  Gpu_CoCmd_Dlstart(phost);        // start
  App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Gpu_CoCmd_MemZero(phost,10*1024L,480L*272L); // Clear the gram frm 1024 
#if defined FT801_ENABLE
  Gpu_CoCmd_CSketch(phost,x,y,w,h,10*1024L,L8,1500L);
#else
  Gpu_CoCmd_Sketch(phost,x,y,w,h,10*1024L,L8);
#endif
  App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1));    // handle for background stars
  App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(10*1024L));
  App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,w,h));
#if defined(FT81X_ENABLE)
  App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT_H(w>>10,h>>9));
#endif
  App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,w,h));
#if defined(FT81X_ENABLE)
  App_WrCoCmd_Buffer(phost,BITMAP_SIZE_H(w>>9,h>>9));
#endif
  Gpu_CoCmd_Swap(phost);
  App_Flush_Co_Buffer(phost);
  Gpu_Hal_WaitCmdfifo_empty(phost);		
 
  while(1)
  {
   //  read the Tag 
     tag = App_Read_Tag(phost);
   // Clear the GRAM when the Clear button enter by user 
    if(tag=='O')
    {
		App_Play_Sound(phost,0x50,255,0xc0);
  		Gpu_CoCmd_Dlstart(phost);  
  		Gpu_CoCmd_MemZero(phost,10*1024L,480L*272L); // Clear the gram frm 1024 		
		App_Flush_Co_Buffer(phost);
        Gpu_Hal_WaitCmdfifo_empty(phost);	
    }
    Gpu_CoCmd_Dlstart(phost);        // start
    App_WrCoCmd_Buffer(phost, CLEAR(1,1,1));	
    App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
    App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA, ONE));
    App_WrCoCmd_Buffer(phost,COLOR_RGB(78, 0, 0));
  // Background animation  
    App_WrCoCmd_Buffer(phost,CMD_LOADIDENTITY);
    rotate_around(ox, oy, 47*a);
    App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 13, 1));

    App_WrCoCmd_Buffer(phost,COLOR_RGB(0, 40, 0));
    rotate_around(ox + sw, oy, 53*a);
    App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 13, 1));
    
    App_WrCoCmd_Buffer(phost,COLOR_RGB(0, 0, 78));
    rotate_around(ox, oy + sh, 57*a);
    App_WrCoCmd_Buffer(phost,VERTEX2II(0, 0, 13, 1));
    App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
   // Signature area 
    Gpu_CoCmd_FgColor(phost,0xffffff);        // Set the fg color
	App_WrCoCmd_Buffer(phost,TAG_MASK(1));
	App_WrCoCmd_Buffer(phost,TAG('S'));
    Gpu_CoCmd_Button(phost,x,y,w,h,31,OPT_FLAT,"");

    
    // Sketch on the screen
      
    App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
    App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    App_WrCoCmd_Buffer(phost,VERTEX2II(x,y,1,0));
    
    if(Gpu_Hal_Rd8(phost,REG_TOUCH_TAG)=='O')Gpu_CoCmd_FgColor(phost,0x003300); else 
    Gpu_CoCmd_FgColor(phost,0x005500);
    App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
    App_WrCoCmd_Buffer(phost,TAG('O'));
#if defined(DISPLAY_RESOLUTION_HVGA_PORTRAIT)
    Gpu_CoCmd_Button(phost,DispWidth / 2 - sw / 4, DispHeight - sh / 2 - 3-30, sw / 2, sh / 2,  28, 0, "CLEAR");
#else
	Gpu_CoCmd_Button(phost,DispWidth / 2 - sw / 4, DispHeight - sh / 2 - 3, sw / 2, sh / 2,  28, 0, "CLEAR");
#endif
	App_WrCoCmd_Buffer(phost,TAG_MASK(0));
    App_WrCoCmd_Buffer(phost,DISPLAY());
    Gpu_CoCmd_Swap(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);	
    a+=1;
  }
}


#if defined(ARDUINO_PLATFORM)
prog_char8_t * const info[] =
#else
char *info[] =
#endif 
{  
    "EVE Signature Application",
    "APP to demonstrate interactive Signature,", 
    "using Sketch,Bitmap rotation,",
    "& Buttons.",
}; 

#if defined MSVC_PLATFORM | defined FT9XX_PLATFORM
/* Main entry point */
int32_t main(int32_t argc,char8_t *argv[])
#endif
#if  defined(ARDUINO_PLATFORM) || defined(MSVC_FT800EMU)
void setup()
#endif
{
    phost = &host;
    /* Init HW Hal */
    App_Common_Init(phost);
    /* Show Logo, do calibration and display welcome screeen */
    App_Common_Start(phost,info);
    /* Main application */
	Signature();    
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













