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
* @file  App_Metaballs
* @brief Sample application to demonstrate EVE primitives and widgets.
Versoin 5.0 - April/17/2017 - Restructure source code
*/

#include "Platform.h"
#include "App_Common.h"

/* Global used for Hal Context */
Gpu_Hal_Context_t host,*phost;

/*****************************************************************************/
/* Example code to display few points at various offsets with various colors */



static int16_t v() 
{
#if defined(DISPLAY_RESOLUTION_WVGA) || defined( DISPLAY_RESOLUTION_HVGA_PORTRAIT)
  return random(50)-32;

#else
  return random(80)-32;
#endif
}

static uint8_t istouch()
{
  return !(Gpu_Hal_Rd16(phost,REG_TOUCH_RAW_XY) & 0x8000);
}

void Metaball()
{
  uint8_t w = 31,h = 18,numBlobs = 80,
            *recip,fadein,f,temp[31];
				
  int32_t 
	    centerx = (16 * 16 * (w / 2)),
	    centery = (16 * 16 * (h / 2)),
	    touching,
	    recipsz = (w*w + h*h) / 4 + 1,
	    tval,tval1,tval2,
	    sx,sy,
	    VEL,
            m,d,bx,by,dx,dy;
  int xx;
  struct 
  {
    int16_t x, y;
    schar8_t dx, dy;
  } blobs[80];
  
  
  for (tval=0; tval<numBlobs; ++tval) 
  {
    blobs[tval].x = random(16 * DispWidth);
    blobs[tval].y = random(16 * DispHeight);
    blobs[tval].dx = v();
    blobs[tval].dy = v();
  }

  recip = (uint8_t*)malloc(recipsz);
  for (tval = 0; tval < recipsz; tval++)
  {
    if (tval == 0 )
    {
      recip[tval] = 200;
    }else
    {
      recip[tval] = MIN(200, (DispWidth * 10) / (4 * tval));
    }
  }
  fadein = 255;
  f = 0;

  while(1)
  {
 //   Gpu_Hal_WaitCmdfifo_empty(phost);
    {
      touching = istouch();
      if (touching) 
      {
        sx = Gpu_Hal_Rd16(phost,REG_TOUCH_SCREEN_XY + 2);
	sy = Gpu_Hal_Rd16(phost,REG_TOUCH_SCREEN_XY);
	centerx = 16 * sx;
	centery = 16 * sy;
      } 
      else
      {
  	centerx = DispWidth * 16 / 2;
	centery = DispHeight * 16 / 2;
      }
    }
    VEL = touching ? 8 : 2;
    for (tval=0; tval<numBlobs; ++tval) 
    {
      if (blobs[tval].x < centerx)   blobs[tval].dx += VEL;    else
      blobs[tval].dx -= VEL;
      if (blobs[tval].y < centery)   blobs[tval].dy += VEL;   else
      blobs[tval].dy -= VEL;
      blobs[tval].x += blobs[tval].dx << 3;
      blobs[tval].y += blobs[tval].dy << 3;
    }
    blobs[random(numBlobs)].dx = v();
    blobs[random(numBlobs)].dy = v();
    for (tval1 = 0; tval1 < h; tval1++) 
    {
      for (tval2 = 0; tval2 < w; tval2++) 
      { 
	m = fadein;
	for (tval = 0; tval < 3; tval++)
	{
	  bx = blobs[tval].x >> 8;
	  by = blobs[tval].y >> 8;
	  dx = bx - tval2;
	  dy = by - tval1;
          d = SQ(dx) + SQ(dy);
	  m += recip[MIN(d >> 2, recipsz - 1)];
	}
	temp[tval2] = MIN(m,255);
      }
      Gpu_Hal_WrMem(phost,(f << 12) + (tval1 << 6),temp,w);
    }
    
    Gpu_CoCmd_Dlstart(phost);        // start
    App_WrCoCmd_Buffer(phost, CLEAR(1,1,1));	
    App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(f << 12));
    App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(L8, 64, 64));
    App_WrCoCmd_Buffer(phost, BITMAP_SIZE(BILINEAR, BORDER, BORDER, DispWidth, DispHeight));	
#ifdef DISPLAY_RESOLUTION_WVGA
    App_WrCoCmd_Buffer(phost, BITMAP_SIZE_H(DispWidth>>9, DispHeight>>9));
#endif
    App_WrCoCmd_Buffer(phost, SAVE_CONTEXT());
    App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_A(0x100 / 64));
    App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_E(0x100 / 64));
    App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));

    App_WrCoCmd_Buffer(phost, BLEND_FUNC(SRC_ALPHA, ZERO));
    App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 0, 0));
    App_WrCoCmd_Buffer(phost, VERTEX2II(0,0,0,0));   
    App_WrCoCmd_Buffer(phost, BLEND_FUNC(SRC_ALPHA, ONE));

    App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 0));
    App_WrCoCmd_Buffer(phost, VERTEX2II(0,0,0,0));
    App_WrCoCmd_Buffer(phost, RESTORE_CONTEXT());
    App_WrCoCmd_Buffer(phost, COLOR_RGB(0, 0, 0));	
    App_WrCoCmd_Buffer(phost, BEGIN(FTPOINTS));
    for (tval = 3; tval < numBlobs; tval++)
    {
	App_WrCoCmd_Buffer(phost,POINT_SIZE(3 * tval));
	App_WrCoCmd_Buffer(phost,VERTEX2F(blobs[tval].x, blobs[tval].y));
    }
    App_WrCoCmd_Buffer(phost,DISPLAY());
    Gpu_CoCmd_Swap(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);	
    fadein = MAX(fadein - 3, 1);
    f = (f + 1) & 3;
  }
}



#if defined(ARDUINO_PLATFORM)
const PROGMEM char * const info[] =
#else
char *info[] =
#endif
{  "EVE MetaBalls Application",
   "APP to demonstrate interactive random MetaBalls,",
   "using Points", 
   "& Bitmaps"
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

    Metaball();

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













