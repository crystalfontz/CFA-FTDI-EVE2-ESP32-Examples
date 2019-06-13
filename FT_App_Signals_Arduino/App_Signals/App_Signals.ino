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
* @file  App_Signals.c/.ino
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

/* Global used for Hal Context */
Gpu_Hal_Context_t host,*phost;



static uint8_t rate = 2;
static int16_t x,y,tx;
static uint8_t beats[10];
static char8_t beats_Incr[10] = {-10,10,5,-5,-20,20,12,-12,-5,5};
static uint16_t add2write = 0;

static bool_t istouch()
{
    return !(Gpu_Hal_Rd16(phost,REG_TOUCH_RAW_XY) & 0x8000);
}

void Sine_wave(uint8_t amp)
{
    static uint8_t played = 0,change=0;
    x+=rate;
    if(x>DispWidth) x  = 0;
    y = (DispHeight/2) + ((int32_t)amp*qsin(-65536*x/(25*rate))/65536);
    if(played==0 &&  change < y){ 
        played = 1;
        App_Play_Sound(phost,(108<<8 | 0x10),50,100); 
    }
    if(change > y)
        played = 0;
    change = y;
    Gpu_Hal_Wr16(phost,RAM_G+(x/rate)*4,VERTEX2F(x*16,y*16));	
}



void Sawtooth_wave(uint8_t amp)
{
    static uint16_t temp=0;
    static uint8_t pk = 0;
    x+=rate;
    if(x>DispWidth){ x  = 0;}
    temp+=2; if(temp>65535L) temp = 0;
    y = (temp % amp);
    pk = y/(amp-2);
    if(pk) App_Play_Sound(phost,(108<<8 | 0x10),50,100);				 
    y = (DispHeight/2)-y;
    Gpu_Hal_Wr16(phost,RAM_G+(x/rate)*4,VERTEX2F(x*16,y*16));	
}

void Triangle_wave(uint8_t amp)
{
    static uint16_t temp=0;
    static uint8_t pk = 0,dc=0,p=0;
    x+=rate;
    if(x>DispWidth){ x  = 0;}
    temp+=2; if(temp>65535L) temp = 0;
    y = (temp % amp);
    pk = (y/(amp-2))%2;
    dc = (temp / amp)%2;  
    if(pk) { if(p==0){ p=1; App_Play_Sound(phost,(108<<8 | 0x10),50,100); } else  p=0;}
    if(dc) y = (DispHeight/2) -(amp-y);  else
        y = (DispHeight/2) - y;
    Gpu_Hal_Wr16(phost,RAM_G+(x/rate)*4,VERTEX2F(x*16,y*16));	
}
static uint16_t temp_x,temp_p,temp_y,en;

void Heartbeat()
{	 uint16_t tval;
y = DispHeight/2;
for(tval=0;tval<10;tval++)
{
    y = y+(beats_Incr[tval]*5);
    beats[tval] = y;
    //printf(" \n tval=%d   beats=   %d",tval,beats[tval]);
}
x+=rate; if(x>DispWidth){ x  = 0;temp_p = 0;temp_y=0;
y=DispHeight/2;
en=0;temp_x=0;}
tx = 5*rate;
tx = ((temp_p+1)*tx) + temp_p*temp_x;
if(tx<=x){ if(0==en)	  en = 1;}
if(en==1){
    if(y!=beats[temp_y])
    {
        //printf("\n y=%d beats[temp_y]=%d",y,beats[temp_y]);
        y += beats_Incr[temp_y] * 5;
        temp_y++;
        if(y==(DispHeight/2)+beats_Incr[4] * 5)
            App_Play_Sound(phost,(108<<8 | 0x10),50,100);
    }
    else
    {
        temp_y++;
        if(temp_y>9) {
            temp_y = 0;   temp_p++;
            en = 0;  temp_x = x - tx;

        }
    }}
Gpu_Hal_Wr32(phost,RAM_G+(x/rate)*4,VERTEX2F(x*16,y*16));
//printf("\n x=%d, y=%d",x,y);

}

void Gpu_Radiobutton(int16_t x,int16_t y,uint32_t bgcolor,uint32_t fgcolor,uint8_t psize,uint8_t tag,uint8_t option)
{
    uint8_t check_size = psize/2;
    App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
    App_WrCoCmd_Buffer(phost,COLOR_RGB((bgcolor >> 16 & 0xff),(bgcolor >> 8 & 0xff),(bgcolor & 0xff)));
    App_WrCoCmd_Buffer(phost,TAG(tag));
    App_WrCoCmd_Buffer(phost,POINT_SIZE(psize*16));
    App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
    App_WrCoCmd_Buffer(phost,VERTEX2F(x*16,y*16));
    if(tag == option)
    {
        App_WrCoCmd_Buffer(phost,COLOR_RGB((fgcolor >> 16 & 0xff),(fgcolor >> 8 & 0xff),(fgcolor & 0xff)));
        App_WrCoCmd_Buffer(phost,POINT_SIZE(check_size*16));
        App_WrCoCmd_Buffer(phost,VERTEX2F(x*16,y*16));
    }
    App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());  
}


void Waves()
{
    int16_t i,xx=0;
    uint16_t tval,th_to,amp;
    uint8_t  temp[480],hide_x=0,fg=1,tag,opt=3;
    // ========  Bg =========================================================
    for(tval=0;tval<=DispHeight/2;tval++)	
    {
        temp[DispHeight-tval] = temp[tval] = (tval*0.90);		
    }
    Gpu_Hal_WrMem(phost,DispWidth*4L,temp,sizeof(temp));


    y = DispHeight/2;
    for(tval=0;tval<10;tval++)
    {
        y = y+(beats_Incr[tval]*5);
        beats[tval] = y;
    }

    for(tval=0;tval<DispWidth;tval+=rate)
    {
        Gpu_Hal_Wr32(phost,RAM_G+((tval/rate)*4),VERTEX2F(tval*16,y*16));			
    }
    add2write = 0;

    while(1)
    {


        // ========  Menu  =========================================================
        if(istouch())   fg = 1;
        if(fg){ th_to=0; if(hide_x>0)hide_x=0; else
            fg = 0;  }
        else 
        { th_to++;
        if(th_to > 250) {
            if( hide_x < 85) hide_x++;   else
                th_to = 0;     }
        }
        //==========Option =========================================================    
        tag = App_Read_Tag(phost);
        if(tag!=0)
        {
            x = 0;   temp_p = 0;en = 0; temp_x = 0; temp_y = 0; //reset
            if(tag>2)  opt = tag;
            if(tag==1)if(rate>1)rate--;	
            if(tag==2)if(rate<6)rate++;			
            y = (DispHeight/2);
            for(tval=0;tval<DispWidth;tval+=rate)
            {
                Gpu_Hal_Wr32(phost,RAM_G+((tval/rate)*4),VERTEX2F(tval*16,y*16));			
            }
            add2write = 0;
        }    
        //========= Signals ========================================================
        amp = 100;
        switch(opt)
        {
        case 5:

            Triangle_wave(amp);
            break;

        case 4:
            Sawtooth_wave(amp);
            break;

        case 3:
            Sine_wave(amp);
            break;

        case 6:
            amp = 50;
            Heartbeat();					
            break;
        }
        //=========Display list start===================================================    
        Gpu_CoCmd_Dlstart(phost); 
        App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
        App_WrCoCmd_Buffer(phost,COLOR_RGB(0x12,0x4A,0x26)); 
        App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(DispWidth*4L));	
        App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,1,DispHeight));	
#ifdef DISPLAY_RESOLUTION_WVGA
        App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT_H(0,DispHeight>>9));
#endif
        App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, REPEAT, BORDER, DispWidth, DispHeight));
#ifdef DISPLAY_RESOLUTION_WVGA
        App_WrCoCmd_Buffer(phost,BITMAP_SIZE_H( DispWidth>>9, DispHeight>>9));
#endif
        App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost,TAG(0));
        App_WrCoCmd_Buffer(phost,VERTEX2F(0,0));
        App_WrCoCmd_Buffer(phost,COLOR_RGB(0x1B,0xE0,0x67));	

        App_WrCoCmd_Buffer(phost,COLOR_RGB(0x1B,0xE0,0x67));
        App_WrCoCmd_Buffer(phost,LINE_WIDTH(2*16));
        App_WrCoCmd_Buffer(phost,BEGIN(LINE_STRIP));
        Gpu_CoCmd_Append(phost,RAM_G,(x/rate)*4);
        App_WrCoCmd_Buffer(phost,END());

        App_WrCoCmd_Buffer(phost,BEGIN(LINE_STRIP));  
        if((x/rate)<(DispWidth/rate)-(50/rate))            // else it screw up
            Gpu_CoCmd_Append(phost,RAM_G+(x/rate)*4+((50/rate)*4),((DispWidth/rate)*4)-((x/rate)*4)-((50/rate)*4));

        App_WrCoCmd_Buffer(phost,END());	

        App_WrCoCmd_Buffer(phost,POINT_SIZE(6*16));
        App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
        App_WrCoCmd_Buffer(phost,VERTEX2F(x*16,y*16));	
        App_WrCoCmd_Buffer(phost,END());
        App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xff,0xff));
        App_WrCoCmd_Buffer(phost,COLOR_A(100));	
        App_WrCoCmd_Buffer(phost,BEGIN(EDGE_STRIP_R));
        App_WrCoCmd_Buffer(phost,VERTEX2F((hide_x+DispWidth-80)*16,0));
        App_WrCoCmd_Buffer(phost,VERTEX2F((hide_x+DispWidth-80)*16,DispHeight*16));
        App_WrCoCmd_Buffer(phost,COLOR_A(255));
        Gpu_Radiobutton(hide_x+DispWidth-70,DispHeight-48,0xffffff,0,8,3,opt);		
        Gpu_Radiobutton(hide_x+DispWidth-70,DispHeight-28,0xffffff,0,8,4,opt);
        Gpu_Radiobutton(hide_x+DispWidth-70,DispHeight-8 ,0xffffff,0,8,5,opt);
        Gpu_Radiobutton(hide_x+DispWidth-70,DispHeight-68,0xffffff,0,8,6,opt);
        Gpu_CoCmd_Text(phost,hide_x+DispWidth-60,DispHeight-48,26,OPT_CENTERY,"Sine");
        Gpu_CoCmd_Text(phost,hide_x+DispWidth-60,DispHeight-28,26,OPT_CENTERY,"Sawtooth");
        Gpu_CoCmd_Text(phost,hide_x+DispWidth-60,DispHeight-8 ,26,OPT_CENTERY,"Triangle");
        Gpu_CoCmd_Text(phost,hide_x+DispWidth-60,DispHeight-68,26,OPT_CENTERY,"ECG");
        Gpu_CoCmd_Text(phost,(hide_x+DispWidth-60),20,30,OPT_CENTERY|OPT_CENTERX,"-");
        Gpu_CoCmd_Text(phost,(hide_x+DispWidth-20),20,30,OPT_CENTERY|OPT_CENTERX,"+");
        Gpu_CoCmd_Text(phost,(hide_x+DispWidth-80),50,28,0,"Rate:");
        Gpu_CoCmd_Number(phost,(hide_x+DispWidth-30),50,28,0,rate);		
        Gpu_CoCmd_Text(phost,(hide_x+DispWidth-80),80,28,0,"Pk:");
        Gpu_CoCmd_Number(phost,(hide_x+DispWidth-40),80,28,0,amp);		
        App_WrCoCmd_Buffer(phost,COLOR_A(50));
        App_WrCoCmd_Buffer(phost,POINT_SIZE(15*16));
        App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
        App_WrCoCmd_Buffer(phost,TAG(1));
        App_WrCoCmd_Buffer(phost,VERTEX2F((hide_x+DispWidth-60)*16,20*16));		
        App_WrCoCmd_Buffer(phost,TAG(2));
        App_WrCoCmd_Buffer(phost,VERTEX2F((hide_x+DispWidth-20)*16,20*16));	

        App_WrCoCmd_Buffer(phost,DISPLAY());
        Gpu_CoCmd_Swap(phost);
        App_Flush_Co_Buffer(phost);
        Gpu_Hal_WaitCmdfifo_empty(phost); 
        //==========================End==================================================    
    }
}  

#if defined(ARDUINO_PLATFORM)
prog_char8_t * const info[] =
#else
char *info[] =
#endif 
{ "EVE Signals Application",

    "APP to demonstrate drawing Signals,",
    "using Strips, Points",
    "& Blend function."
};

#if defined MSVC_PLATFORM || defined FT9XX_PLATFORM
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
	Waves();    
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













