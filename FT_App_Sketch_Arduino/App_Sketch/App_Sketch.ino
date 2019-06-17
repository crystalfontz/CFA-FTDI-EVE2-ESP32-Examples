/*============================================================================
// 
// Example Espressif ESP32 SoC firmware (using Arduino platform) for use with
// the Crystalfontz CFAF800480E0-050SC-A1 display module.
//
// This firmware was originally sourced from the FTDI/Bridgetek website and
// has been slightly modified to work correctly with the Crystalfontz and
// ESP32 hardware.
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
* @file  App_Sketch.c/.ino
* @brief Sample application to demonstrate FT800 primitives and built-in widgets 

version 5.0 - April/17/2017 - Restructure source
version 4.0 - July/01/2014 - Support for FT81X chips.
version 3.1 - Jun/13/2014 - Application now uses CMD_CSKETCH for FT801 platform.
Version 3.0 - Support for FT801 platform.
Version 2.0 - Support for FT800 emulator platform.
Version 1.0 - Final version based on the requirements.
Version 0.1 - intial draft of the release notes
*
*/


#include "Platform.h"
#include "App_Common.h"


/* Global used for buffer optimization */
Gpu_Hal_Context_t host,*phost;

/* Application*/
void Sketch()
{
    uint32_t  tracker,color=0;
    uint16_t  val=32768;
    uint8_t tag =0;
    //  Set the bitmap properties , sketch properties and Tracker for the sliders
    Gpu_CoCmd_Dlstart(phost);
    Gpu_CoCmd_FgColor(phost,0xffffff);        // Set the bg color
    Gpu_CoCmd_Track(phost,(DispWidth-30),40,8,DispHeight-100,1);

#if defined FT801_ENABLE
    Gpu_CoCmd_CSketch(phost,0,10,DispWidth-40,DispHeight-30,0,L8,1500L);
#elif defined FT81X_ENABLE
    Gpu_CoCmd_Sketch(phost,0,10,DispWidth-40,DispHeight-30,0,L8);
#else
    Gpu_CoCmd_Sketch(phost,0,10,DispWidth-40,DispHeight-30,0,L8);
#endif
    /*
    #if defined FT801_ENABLE
    Gpu_CoCmd_CSketch(phost,0,10,DispWidth-40,DispHeight-20,0,L8,1500L);
    #elif defined FT81X_ENABLE
    Gpu_CoCmd_Sketch(phost,0,10,DispWidth-40,DispHeight-20,0,L8);
    #else
    Gpu_CoCmd_Sketch(phost,0,10,DispWidth-40,DispHeight-20,0,L8);
    #endif
    */
    Gpu_CoCmd_MemZero(phost,0L,(DispWidth-40)*(DispHeight-20L));  
    App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1));
    App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(0));
    App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,DispWidth-40,DispHeight-20));
#ifdef FT81X_ENABLE
    App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT_H((DispWidth-40)>>10,(DispHeight-20)>>9));
#endif
    App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,(DispWidth-40),(DispHeight-20)));
#ifdef FT81X_ENABLE
    App_WrCoCmd_Buffer(phost,BITMAP_SIZE_H((DispWidth-40)>>9,(DispHeight-20)>>9));
#endif
    Gpu_CoCmd_Swap(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);				
    while(1)
    {
        // Check the tracker
        tracker = Gpu_Hal_Rd32(phost,REG_TRACKER);	
        // Check the Tag 
        tag = Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
        //  clear the GRAM when user enter the Clear button
        if(tag==2)
        {
            Gpu_CoCmd_Dlstart(phost);  
            Gpu_CoCmd_MemZero(phost,0,(DispWidth-40)*(DispHeight-20L)); // Clear the gram frm 1024 		
            App_Flush_Co_Buffer(phost);
            Gpu_Hal_WaitCmdfifo_empty(phost);	
        }
        // compute the color from the tracker
        if((tracker&0xff)==1)      // check the tag val
        {
            val = (tracker>>16);		
        }
        color = val*255L;
        // Start the new display list
        Gpu_CoCmd_Dlstart(phost);                  // Start the display list
        App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));	  // clear the display     
        App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));  // color	
        Gpu_CoCmd_BgColor(phost,color);   
        App_WrCoCmd_Buffer(phost,TAG_MASK(1));
        App_WrCoCmd_Buffer(phost,TAG(1));          // assign the tag value 
        Gpu_CoCmd_FgColor(phost,color);
        // draw the sliders 
        Gpu_CoCmd_Slider(phost,(DispWidth-30),40,8,(DispHeight-100),0,val,65535);	 // slide j1 cmd  
        Gpu_CoCmd_FgColor(phost,(tag==2)?0x0000ff:color);
        App_WrCoCmd_Buffer(phost,TAG(2));          // assign the tag value 
        Gpu_CoCmd_Button(phost,(DispWidth-35),(DispHeight-45),35,25,26,0,"CLR");
        App_WrCoCmd_Buffer(phost,TAG_MASK(0));

        Gpu_CoCmd_Text(phost,DispWidth-35,10,26,0,"Color");

        App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));
        App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
        App_WrCoCmd_Buffer(phost,VERTEX2F(0,10*16));
        App_WrCoCmd_Buffer(phost,VERTEX2F((int16_t)(DispWidth-40)*16,(int16_t)(DispHeight-20)*16));			


        App_WrCoCmd_Buffer(phost,COLOR_RGB((color>>16)&0xff,(color>>8)&0xff,(color)&0xff));
        App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost,VERTEX2II(0,10,1,0));
        App_WrCoCmd_Buffer(phost,END());
        App_WrCoCmd_Buffer(phost,DISPLAY());
        Gpu_CoCmd_Swap(phost);
        App_Flush_Co_Buffer(phost);
        Gpu_Hal_WaitCmdfifo_empty(phost);	
    }
}

#if defined(ARDUINO_PLATFORM)
const PROGMEM char * const info[] =
#else
char *info[] =
#endif
{  "EVE Sketch Application",

    "APP to demonstrate interactive Sketch,", 
    "using Sketch, Slider,",
    "& Buttons."

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
    App_Common_Start(phost,(char**)info);
    Sketch();   
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
