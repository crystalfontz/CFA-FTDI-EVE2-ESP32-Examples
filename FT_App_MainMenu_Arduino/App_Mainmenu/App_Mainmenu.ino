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
* @file  App_MainMenu
* @brief MainMenu application with background animation.

Versoin 5.0 - April/17/2017 - Restructure source code
Version 0.1 - 2014/05/30 initial draft of the release notes
*/


#include "Platform.h"
#include "App_Common.h"
#include "sdcard.h"
#define NOTOUCH		-32768
#define MAX_MENUS 12
#define THUMBNAIL_ADDRESS (50*1024L)
#define MENU_POINTSIZE  5 // 16bit prec 


#define LOOPBACK_METHOD	 
//#define ANDROID_METHOD  
//#define WIN8_METHOD


//#define BACKGROUND_ANIMATION_1
//#define BACKGROUND_ANIMATION_2
#define BACKGROUND_ANIMATION_3 /* FIXED FOR 800x480 DISPLAY SIZE */
//#define BACKGROUND_ANIMATION_4
//#define BACKGROUND_ANIMATION_5
//#define BACKGROUND_ANIMATION_6

/* Global used for context hal */
Gpu_Hal_Context_t host,*phost;
extern Reader imageFile;

uint8_t keyin_cts = 0;
#define KEYIN_COUNTS	10

static struct {
  signed short dragprev;
  int vel;      // velocity
  long base;    // screen x coordinate, in 1/16ths pixel
  long limit;
} scroller;

/* Api to Intialise the scroller*/
static void scroller_init(uint32_t limit)
{
  scroller.dragprev = -32768;
  scroller.vel = 0;      // velocity
  scroller.base = 0;     // screen x coordinate, in 1/16ths pixel
  scroller.limit = limit;
}
/* Api used to scroll the screen horizontally*/
static void scroller_run()
{
  signed short sx;
  static uint16_t _delay = 0;
#ifdef FT801_ENABLE  
  static uint32_t prev_time = 0;
  uint32_t time  = millis();
  if(prev_time!=0)
  _delay += (time-prev_time); 
  prev_time = time;
  if(_delay<30)
  {
	scroller.base += scroller.vel;
	scroller.base = max(0, min(scroller.base, scroller.limit));
	return; 
  }		
#endif 
  sx = Gpu_Hal_Rd16(phost,REG_TOUCH_SCREEN_XY + 2);
  if ((sx != -32768) & (scroller.dragprev != -32768)) {
    scroller.vel = (scroller.dragprev - sx) << 4;
  } else {
    int change = max(1, abs(scroller.vel) >> 5);
    if (scroller.vel < 0)
      scroller.vel += change;
    if (scroller.vel > 0)
      scroller.vel -= change;
  }
  scroller.dragprev = sx;
  scroller.base += scroller.vel;
  scroller.base = max(0, min((int)scroller.base, (int)scroller.limit));
  _delay = 0;
}

/* Load the icons of the menu to GRAM*/ 
void Load_Thumbnails()
{
  #ifdef MSVC_PLATFORM	 	
  char *apps[] = {"1.jpg","2.jpg","3.jpg","4.jpg","5.jpg","6.jpg","7.jpg","8.jpg","9.jpg","10.jpg","11.jpg","12.jpg"};
  #endif
  #ifdef ARDUINO_PLATFORM 
  String stringOne; 
  #endif
  
  /* Display busy spinner */
  	App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
	Gpu_CoCmd_Dlstart(phost);        // start
	App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
#if defined(DISPLAY_RESOLUTION_WVGA)
	Gpu_CoCmd_Text(phost,DispWidth>>1,(DispHeight>>1)-50,29,OPT_CENTER,"Loading bitmap...");
#else
	Gpu_CoCmd_Text(phost,DispWidth>>1,DispHeight>>1,29,OPT_CENTER,"Loading bitmap...");
#endif
	Gpu_CoCmd_Spinner(phost, (DispWidth>>1),(DispHeight>>1)+50,0,0);
	App_WrCoCmd_Buffer(phost,DISPLAY());
	Gpu_CoCmd_Swap(phost);
	App_Flush_Co_Buffer(phost);
	Gpu_Hal_WaitCmdfifo_empty(phost);
    delay(100);
  
  int image_no = 1;
  do
  {	
	stringOne =String(image_no);
	stringOne+=".jpg";	
	char apps[stringOne.length()+1];
	stringOne.toCharArray(apps,stringOne.length()+1);
    
    Gpu_Hal_LoadImageToMemory(phost,apps,THUMBNAIL_ADDRESS+((image_no-1)*10000),LOADIMAGE);
    
    image_no++;
  }while(image_no<=MAX_MENUS);
  App_Set_DlBuffer_Index (0);
  App_WrDl_Buffer(phost,BITMAP_HANDLE(0));	
  App_WrDl_Buffer(phost,BITMAP_SOURCE(THUMBNAIL_ADDRESS));	
  App_WrDl_Buffer(phost,BITMAP_LAYOUT(RGB565,100*2,50));	
  App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 100, 50));	
  App_WrDl_Buffer(phost,DISPLAY());
  App_Flush_DL_Buffer(phost);
  Gpu_Hal_DLSwap(phost,DLSWAP_FRAME);
}

static uint8_t istouch()
{
  return !(Gpu_Hal_Rd16(phost,REG_TOUCH_RAW_XY) & 0x8000);
}

void Sine_wave(uint8_t amp,uint16_t address,uint16_t yoffset)
{
  uint16_t x = 0,y=0; 

  for(x=0;x<DispWidth+100;x+=10)
  {		
	y = (yoffset) + ((int32_t)amp*qsin(-65536*x/(25*10))/65536);
	Gpu_Hal_Wr32(phost,address+(x/10)*4,VERTEX2F(x*16,y*16));	
  }	
}

float linear(float p1,float p2,float t,uint16_t rate)
{
	float st  = (float)t/rate;
	return p1+(st*(p2-p1));
}
float acceleration(float p1,float p2,uint16_t t,uint16_t rate)
{
	float dst  = (float)t/rate;
	float st = SQ(dst);
	return p1+(st*(p2-p1));
}

float deceleration(float p1,float p2,uint16_t t,uint16_t rate)
{
	float st,dst  = (float)t/rate;
	dst = 1-dst;
	st = 1-SQ(dst);
	return p1+(st*(p2-p1));
}
static void polar_draw(int32_t r, float_t th,uint16_t ox,uint16_t oy)
{
  int32_t x, y;
  th = (th * 32768L / 180);
  polarxy(r, th, &x, &y, ox, oy);

#if defined(FT81X_ENABLE)
  App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0));
  App_WrCoCmd_Buffer(phost,VERTEX2F(x>>4,y>>4));
   App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(4));
#else
  App_WrCoCmd_Buffer(phost,VERTEX2F(x,y));
#endif
}

int16_t ftsize;
#define rotation_rate 2


#ifdef BACKGROUND_ANIMATION_1

uint16_t xoffset_array[30],dx_pts[20];
uint8_t yoffset_array[30],bitmap_handle[30],dy_pts[20];
uint8_t rate_cts[30],iteration_cts[30];

void Backgroundanimation_1()
{
	static uint16_t bg_cts  = 0,fg = 0,wave_cts = 490;
	static uint8_t init = 0,_cell = 0,VEL;
	static uint8_t cts = 0;
	int16_t i = 0,j=0,xoff,yoff;
	float _xoff = 0;
	wave_cts = 490;
	if(cts>=5){cts = 0;if(_cell>0)_cell--; else _cell = 7;}cts++;
	_cell = 0;
	if(wave_cts<580)wave_cts+=10; else wave_cts = 0;
	if(istouch()) VEL = 2; else VEL = 1;

	if(!init)
	{
      	init = 1;
		Sine_wave(15,RAM_G,DispHeight/2);		
		Sine_wave(12,RAM_G+232,16+(DispHeight/2));
		Sine_wave(9,RAM_G+2*232,32+(DispHeight/2));
		Sine_wave(6,RAM_G+3*232,48+(DispHeight/2));
		for(i=0;i<30;i++)
		{		
			yoffset_array[i] = random(255);
			bitmap_handle[i] = 4+random(4);
			rate_cts[i] = 100+random(155);
			iteration_cts[i] = random(200);
		}
		Gpu_Hal_LoadImageToMemory(phost,"nts1.raw",220*1024L,LOAD);
		Gpu_Hal_LoadImageToMemory(phost,"nts2.raw",222*1024L,LOAD);
		Gpu_Hal_LoadImageToMemory(phost,"nts3.raw",232*1024,LOAD);
		Gpu_Hal_LoadImageToMemory(phost,"nts4.raw",242*1024,LOAD);
		Gpu_Hal_LoadImageToMemory(phost,"hline.raw",255*1024L,LOAD);
                
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(4));	
		App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(220*1024L));	
		App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4,10,50));	
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 40, 100));		
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(5));	
		App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(222*1024L));	
		App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4,25,60));	
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 100, 120));		
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(6));	
		App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(256506L));	
		App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4,10,40));	
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 40, 80));			
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(7));	
		App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(257306L));	
		App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4,10,24));	
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 40, 48));		
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(8));	
		App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(255*1024L));	
		App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,512,1));	
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, REPEAT, REPEAT, 512, 272));		
	}
//	
    App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
	App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(30,30,30));
	App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	App_WrCoCmd_Buffer(phost,COLOR_A(170));
	
	
	App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(8));	
	App_WrCoCmd_Buffer(phost,VERTEX2F((int16_t)0,0));	
	
	App_WrCoCmd_Buffer(phost,COLOR_RGB(30,30,30));
	App_WrCoCmd_Buffer(phost,COLOR_A(255));
	


	for(j=0;j<3;j++)
	{
	  switch(j)
	  {
		case 0:
		  App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(384));
		  App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(384));
		break;

		case 1:
		  App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
		  App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(256));
		break;

		case 2:			
		  App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(200));
		  App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(200));
		break;
	  }
	  for(i = 0;i<10;i++)
	  {
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(bitmap_handle[j*10+i]));
		if(bitmap_handle[j*10+i]==5)
		App_WrCoCmd_Buffer(phost,CELL(_cell));
		else 
		{
		  switch(j)
		  {
			case 0:
			App_WrCoCmd_Buffer(phost,CELL(0));
			break;

			case 1:
			App_WrCoCmd_Buffer(phost,CELL(0));
			break;

			case 2:
			App_WrCoCmd_Buffer(phost,CELL(1));				
			break;
		  }
		}
		xoff = linear(DispWidth,-50,iteration_cts[j*10+i],rate_cts[j*10+i]);
		yoff = linear(DispHeight/2,yoffset_array[j*10+i],iteration_cts[j*10+i],rate_cts[j*10+i]);
		App_WrCoCmd_Buffer(phost,VERTEX2F(xoff*16,yoff*16));
	  }
	}
	for(i = 0; i<30 ; i++)
	{
	    if(iteration_cts[i]==0)
	    {
			yoffset_array[i] = random(255);
			bitmap_handle[i] = 4+random(4);
	    }
	    if(iteration_cts[i]<rate_cts[i])iteration_cts[i]+=VEL; else{ iteration_cts[i] = 0;}
	}
	
	wave_cts = 490;	
	App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));
	App_WrCoCmd_Buffer(phost,COLOR_A(255));
	
	App_WrCoCmd_Buffer(phost,BEGIN(LINE_STRIP));
	Gpu_CoCmd_Append(phost,RAM_G,(wave_cts/10)*4);    
	App_WrCoCmd_Buffer(phost,END());
	
	App_WrCoCmd_Buffer(phost,BEGIN(LINE_STRIP));
	Gpu_CoCmd_Append(phost,RAM_G+232,(wave_cts/10)*4);
	App_WrCoCmd_Buffer(phost,END());
	
	App_WrCoCmd_Buffer(phost,BEGIN(LINE_STRIP));
	Gpu_CoCmd_Append(phost,RAM_G+2*232,(wave_cts/10)*4);
	App_WrCoCmd_Buffer(phost,END());
	
	App_WrCoCmd_Buffer(phost,BEGIN(LINE_STRIP));
	Gpu_CoCmd_Append(phost,RAM_G+3*232,(wave_cts/10)*4);
	App_WrCoCmd_Buffer(phost,END());
	
	App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
}
#endif

#ifdef BACKGROUND_ANIMATION_2

uint16_t point_size[20];
uint16_t xoffset_array[20],yoffset_array[20],dx_pts[20],dy_pts[20];
uint8_t color[20];

void Backgroundanimation_2()
{
	static uint8_t init = 0;	
	uint8_t ptclrarray[6][3] = {
		{0xb9,0xba,0xde},
		{0x0c,0x61,0xb7},
		{0x01,0x18,0x4e},
		{0xbf,0x25,0xbd},
		{0x29,0x07,0x3a},
		{0xc9,0x61,0x22}};
	int32_t i,ptradius,colorindex;
	static uint8_t fg = 0;
	float vel = 2;
	static float t=0,t1=0;	
	int32_t xoffset,yoffset,x1offset,y1offset,rate = 2000;
	uint8_t pts;
	if(!init)
	{
		init = 1;
		for(pts=0;pts<20;pts++)
		{
			point_size[pts] = 136 *16 +random(375*16); 
			xoffset_array[pts] = random(512)*16;
			yoffset_array[pts] = random(512)*16;
			color[pts] = random(5);
			dx_pts[pts] = 240*16+random(240*16);
			dy_pts[pts] = 130*16+random(142*16);	
		}
	}
	if(istouch())	
	{ t1 = 0; vel  = 5; } else {
		vel = linear(5,2,t1,100); 
		if(t1<100) t1++; }
	App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
	App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));
	App_WrCoCmd_Buffer(phost,COLOR_A(255));


	//draw 20 points with various radius and additive blending
	App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
	App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE));
	App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,0));
	App_WrCoCmd_Buffer(phost,COLOR_A(50));

	/* compute points on top */
	for(i=0;i<5;i++)
	{
		ptradius = point_size[i]; 
		colorindex = color[i+0];
		App_WrCoCmd_Buffer(phost,COLOR_RGB(ptclrarray[colorindex][0],ptclrarray[colorindex][1],ptclrarray[colorindex][2]));
		yoffset = linear(dy_pts[i+0],yoffset_array[i+0],t,1000);
		
		xoffset = xoffset_array[i+0];
		App_WrCoCmd_Buffer(phost,POINT_SIZE(ptradius));
		App_WrCoCmd_Buffer(phost,VERTEX2F(xoffset,yoffset));
	}
	/* compute points on right */
	for(i=0;i<5;i++)
	{
		ptradius = point_size[5+i];
		colorindex = color[i+5];	
		App_WrCoCmd_Buffer(phost,COLOR_RGB(ptclrarray[colorindex][0],ptclrarray[colorindex][1],ptclrarray[colorindex][2]));				
		yoffset = linear(dy_pts[i+5],yoffset_array[i+5],t,1000);			
	
		yoffset = yoffset_array[i+5];		
		App_WrCoCmd_Buffer(phost,POINT_SIZE(ptradius));
		App_WrCoCmd_Buffer(phost,VERTEX2F(xoffset,yoffset));
	}
	///* compute points on left */
	for(i=0;i<5;i++)
	{
		ptradius = point_size[10+i];
		colorindex = color[i+10];		
		
		App_WrCoCmd_Buffer(phost,COLOR_RGB(ptclrarray[colorindex][0],ptclrarray[colorindex][1],ptclrarray[colorindex][2]));		
		xoffset = linear(dx_pts[i+0],xoffset_array[i+0],t,1000);
		
		yoffset = yoffset_array[i+5];
		App_WrCoCmd_Buffer(phost,POINT_SIZE(ptradius));
		App_WrCoCmd_Buffer(phost,VERTEX2F(xoffset,yoffset));
	}
	/* compute points on bottom */
	for(i=0;i<5;i++)
	{
		ptradius = point_size[15+i];
		colorindex = color[i+15];		
		App_WrCoCmd_Buffer(phost,COLOR_RGB(ptclrarray[colorindex][0],ptclrarray[colorindex][1],ptclrarray[colorindex][2]));
		yoffset = linear(dy_pts[i+15],yoffset_array[i+15],t,1000);
		xoffset = xoffset_array[i+15];		
		App_WrCoCmd_Buffer(phost,POINT_SIZE(ptradius));
		App_WrCoCmd_Buffer(phost,VERTEX2F(xoffset,yoffset));
	}
	
	//draw additive blend lines diagonally
	App_WrCoCmd_Buffer(phost,COLOR_A(100));
	App_WrCoCmd_Buffer(phost,COLOR_RGB(32,32,32));
	App_WrCoCmd_Buffer(phost,BEGIN(LINES));
	for(i=-1;i<4;i++)
	{
		xoffset = i*136;
		yoffset = 0;
		x1offset = 136*(2 + i);
		y1offset = 272;
		if(x1offset > 480)
		{
			y1offset = 272 - (x1offset - 480);
			x1offset = 480;
		}
		if(xoffset < 0)
		{
			yoffset = -xoffset;
			xoffset = 0;
		}
		App_WrCoCmd_Buffer(phost,VERTEX2II(xoffset,yoffset,0,0));
		App_WrCoCmd_Buffer(phost,VERTEX2II(x1offset,y1offset,0,0));
	}
	for(i=1;i<6;i++)
	{
		xoffset = i*136;
		yoffset = 0;
		x1offset = 136*(i - 2);
		y1offset = 272;
		if(x1offset < 0)
		{
			y1offset = 272 + x1offset;
			x1offset = 0;
		}
		if(xoffset > 480)
		{
			yoffset = (xoffset - 480);
			xoffset = 480;
		}
		App_WrCoCmd_Buffer(phost,VERTEX2II(xoffset,yoffset,0,0));
		App_WrCoCmd_Buffer(phost,VERTEX2II(x1offset,y1offset,0,0));
	}
	if(!fg)
	{
		if(t<rate) t+=vel; else
		fg = 1;   		
	}else
	{
		if(t>0) t-=vel;
		else
		fg=0;
	}
	App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
	return;
}
#endif

#ifdef BACKGROUND_ANIMATION_3
uint16_t xoffset_array[30], dx_pts[20];
uint16_t yoffset_array[30], dy_pts[20];
uint8_t bitmap_handle[30];
uint8_t rate_cts[30],iteration_cts[30];

void Backgroundanimation_3()
{
	int32_t i=0,rate = 1000;
	uint8_t alpha  = 0,VEL = 0;
	static uint16_t t1 = 0;
	int16_t xoff = 0 ,yoff = 0;

if(istouch())  
  { 
    t1 = 0; VEL  = 2; 
  } 
  else {
    VEL = linear(2,1,t1,100); 
    if(t1<100) t1++; 
  }
  //clear the background color
  App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
  App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(0,0,0));
  App_WrCoCmd_Buffer(phost,CLEAR_COLOR_A(0));
  App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,0));
  //draw the cmd gradient with scissors
  App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(DispWidth,DispHeight/2));
  App_WrCoCmd_Buffer(phost,SCISSOR_XY(0,0));
  Gpu_CoCmd_Gradient(phost,0,0,0x708fa1,0,DispHeight/2,0xc4cdd2);
  App_WrCoCmd_Buffer(phost,SCISSOR_XY(0,DispHeight/2));
  Gpu_CoCmd_Gradient(phost,0,DispHeight/2,0xc4cdd2,0,DispHeight,0x4f7588);
  
  App_WrCoCmd_Buffer(phost,SCISSOR_XY(0,0));
  App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(DispWidth,DispHeight));//reprogram with  default values

  //draw 20 points with various radious with additive blending
  App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0));
  App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));  
  App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,0));
  App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  for(i=0;i<20;i++)
  {
    //if(0 == i%4)    
    alpha  = linear(80,0,iteration_cts[i],rate_cts[i]);
    if(alpha<75)
    {
      App_WrCoCmd_Buffer(phost,POINT_SIZE(16*(30 + (3*i/2))));      
      App_WrCoCmd_Buffer(phost,COLOR_A(alpha));
      xoff = linear(xoffset_array[i],dx_pts[i],iteration_cts[i],rate_cts[i]);
      yoff = linear(yoffset_array[i],dy_pts[i],iteration_cts[i],rate_cts[i]);
      App_WrCoCmd_Buffer(phost,VERTEX2F(xoff,yoff));
    }
  }
//  
  App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
  for(i = 0; i<20 ; i++)
  {
    if(iteration_cts[i]==0)
    {
      xoffset_array[i] = random(DispWidth);
      //yoffset_array[i] = 100+random(DispHeight/4);
      yoffset_array[i] = random(DispHeight);
      dx_pts[i] = random(DispWidth);
      dy_pts[i] = random(DispHeight);        
      rate_cts[i] = 100+random(155);
    }
    if(iteration_cts[i]<rate_cts[i])iteration_cts[i]+=VEL; else{ iteration_cts[i] = 0;}
  }
	return;
}
#endif

#ifdef BACKGROUND_ANIMATION_4

uint16_t xoffset_array[30],dx_pts[20];
uint8_t yoffset_array[30],bitmap_handle[30],dy_pts[20];
uint8_t rate_cts[30],iteration_cts[30];

void Backgroundanimation_4()
{
	static char init = 0;
	int32_t i,linesize;	
	int16_t xoff = 0 ,yoff = 0;
	uint8_t tval = 0,numBlobs=20,VEL = 0,alpha = 0;
	if(!init)
	{	
		/*Load background raw data*/
#if defined(FT81X_ENABLE) && defined(DISPLAY_RESOLUTION_WVGA)
		Gpu_Hal_LoadImageToMemory(phost,"hline_H.raw",855*1024L,LOAD);
#else
		Gpu_Hal_LoadImageToMemory(phost,"hline.raw",255*1024L,LOAD);
#endif

		init = 1;
	}

	if(istouch())	
		VEL = 2; else VEL = 1;

	for(i = 0; i<numBlobs ; i++)
	{
		if(iteration_cts[i]==0)
		{
			xoffset_array[i] = random(DispWidth);
			yoffset_array[i] = random(DispHeight);
			dx_pts[i] = random(DispWidth);
			dy_pts[i] = random(DispHeight);	
			rate_cts[i] = 500+random(500);
		}
		if(iteration_cts[i]<rate_cts[i])iteration_cts[i]+=VEL; else{ iteration_cts[i] = 0;}
	}

	//draw the bitmap at location 0 with RGB565
	App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
	App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1));//bitmap handle 2 is used for background balls
	App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(255*1024L));
	App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8, 512, 1));
	App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, REPEAT, 512, 512));

	//clear the background color
	App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(255,255,255));
	App_WrCoCmd_Buffer(phost,CLEAR_COLOR_A(0));
	App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
	App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));


	Gpu_CoCmd_LoadIdentity(phost);
	Gpu_CoCmd_Rotate(phost, 60*65536/360);//rotate by 30 degrees clock wise

	Gpu_CoCmd_SetMatrix(phost );

	App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,1));
	App_WrCoCmd_Buffer(phost,BLEND_FUNC(ONE,ZERO));
	App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xb4,0x00));
	App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	App_WrCoCmd_Buffer(phost,VERTEX2II(0,0,1,0));	

	App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));
	App_WrCoCmd_Buffer(phost,BLEND_FUNC(DST_ALPHA,ONE_MINUS_DST_ALPHA));
	App_WrCoCmd_Buffer(phost,COLOR_RGB(0xff,0xb4,0x00));
	App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
	App_WrCoCmd_Buffer(phost,VERTEX2II(0,0,0,0));
	App_WrCoCmd_Buffer(phost,VERTEX2II(DispWidth,DispHeight,0,0));

	App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
	App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_DST_ALPHA));
	App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,0));
	App_WrCoCmd_Buffer(phost,COLOR_RGB(0x96,0x6e,0x0d));
	App_WrCoCmd_Buffer(phost,LINE_WIDTH(16*1));
	for(i=0;i<numBlobs;i++)
	{
		int32_t xoffset,yoffset;
		if(0 == i%4)
		{
			linesize = 16*(25 + (3*i/4));
		}
		alpha  = linear(80,0,iteration_cts[i],rate_cts[i]);
		if(alpha<75)
		{
			xoff = linear(xoffset_array[i],dx_pts[i],iteration_cts[i],rate_cts[i]);
			yoff = linear(yoffset_array[i],dy_pts[i],iteration_cts[i],rate_cts[i]);
			App_WrCoCmd_Buffer(phost,VERTEX2F(xoff*16,yoff*16));
			App_WrCoCmd_Buffer(phost,VERTEX2F(xoff*16+linesize,yoff*16+linesize));
		}
	}
	App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
}
#endif

#ifdef BACKGROUND_ANIMATION_5
static struct 
{
	uint8_t init;
	uint16_t xoffset_array[12];
	uint16_t yoffset_array[12];
	int16_t yoffset_array_source[12];
	uint16_t iteration_cts[12];
	uint8_t disable_cts[20][12];
	uint8_t radius_a[12];
	uint8_t radius_b[12];
	uint16_t angle[12];
	uint8_t number_of_firebubbles;
}firebubbles;


void draw_bubbles(uint8_t inc)
{
	int16_t i,j,yoff,xoff;
	for(j=0;j<3;j++)
	{
		App_WrCoCmd_Buffer(phost,CELL(j));
		if(j==2) 
			App_WrCoCmd_Buffer(phost,COLOR_A(150));
		else
			App_WrCoCmd_Buffer(phost,COLOR_A(255));
		for(i=0;i<firebubbles.number_of_firebubbles;i++)
		{
			if(firebubbles.iteration_cts[j*5+i] < firebubbles.yoffset_array[j*5+i]+0)
			{
				yoff = acceleration(firebubbles.yoffset_array_source[j*5+i],firebubbles.yoffset_array[j*5+i],firebubbles.iteration_cts[j*5+i],firebubbles.yoffset_array[j*5+i]*1);
				App_WrCoCmd_Buffer(phost,VERTEX2F(firebubbles.xoffset_array[j*5+i]*16,yoff*16));

				if(inc){
				if(firebubbles.iteration_cts[j*5+i]<firebubbles.yoffset_array[j*5+i]*1)  firebubbles.iteration_cts[j*5+i]+=1;  					
				}
			}
		}
	}
}

void collaid_bubbles(uint8_t inc)
{
	int16_t i,j,k,yoff,xoff,temp;
	static uint8_t rate = 50;
	App_WrCoCmd_Buffer(phost,CELL(3));
	for(j=0;j<3;j++)
	{
		for(i=0;i<firebubbles.number_of_firebubbles;i++)
		{		
			if(firebubbles.iteration_cts[j*5+i]>=firebubbles.yoffset_array[j*5+i])
			{
				for(k=0;k<12;k++)	
				{
					App_WrCoCmd_Buffer(phost,COLOR_A(200-firebubbles.disable_cts[j*5+i][k]*10));		
					temp = (uint8_t)deceleration(0,firebubbles.radius_a[k],firebubbles.disable_cts[j*5+i][k],20);
					xoff = firebubbles.xoffset_array[j*5+i]+10 + (temp)*cos(firebubbles.angle[k]*0.01744);  //3.14/180=0.01744		
					temp = (uint8_t)deceleration(0,firebubbles.radius_b[k],firebubbles.disable_cts[j*5+i][k],20);
					yoff = firebubbles.yoffset_array[j*5+i]+10 + (temp)*sin(firebubbles.angle[k]*0.01744);  //3.14/180=0.01744
					yoff = firebubbles.yoffset_array[j*5+i]+10 + (temp)*sin(firebubbles.angle[k]*0.01744);  //3.14/180=0.01744
					App_WrCoCmd_Buffer(phost,VERTEX2F(xoff*16,yoff*16));
					if(inc)
					{
						temp =  j*5+i;
						if(firebubbles.disable_cts[temp][k]<20)
						firebubbles.disable_cts[temp][k]++;
						else
						{
							firebubbles.disable_cts[temp][k] = 0;
							firebubbles.iteration_cts[temp] = 0;							
							firebubbles.xoffset_array[temp] = random(DispWidth);
							firebubbles.yoffset_array_source[temp] = -50-random(100);
							if(j==0)
							firebubbles.yoffset_array[temp] = random(20)+(DispHeight-50); 		
							else if(j==1)
							firebubbles.yoffset_array[temp] = random(20)+(DispHeight-75); 		
							else if(j==2)
							firebubbles.yoffset_array[temp] = random(20)+(DispHeight-95); 		
							
						}
					}
				}
			}
		}
	}
}

void Backgroundanimation_5()
{
	int16_t i,j,yoff,xoff;
	firebubbles.number_of_firebubbles = 3;
	
	if(!firebubbles.init)
	{
		firebubbles.init = 1;		
		for(i=0;i<12;i++)
		{
			firebubbles.xoffset_array[i] = random(DispWidth);
			firebubbles.yoffset_array[i] = random(50) + (DispHeight-80);
			firebubbles.yoffset_array_source[i] =  -50 - random(100);		
		}		
		for(i = 0;i<12;i++)
		{
				firebubbles.radius_a[i] = random(70);
				firebubbles.radius_b[i] = random(10);
				firebubbles.angle[i] =  random(360);
		}
		Gpu_Hal_LoadImageToMemory(phost,"fire.raw",0*1024L,LOAD);		
		Gpu_Hal_LoadImageToMemory(phost,"floor.raw",7*1024L,LOAD);		
		Gpu_Hal_LoadImageToMemory(phost,"grad.raw",17*1024L,LOAD);
		
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(5));	
		App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(0));	
		App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,40,40));
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 40, 40));
		
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(6));	
		App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(7*1024));	
		App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,240,40));
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 480, 80));
		
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(8));	
		App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(17*1024));	
		App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,60,30));
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 120, 60));
		for(i=0;i<80;i++)
		{
			Gpu_Hal_Wr8(phost,19*1024L+i,255-(i*2));	
		}
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(7));	
		App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(19*1024));	
		App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L8,1,80));
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, REPEAT, BORDER, 480, 80));
	}	
	App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
	App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
	App_WrCoCmd_Buffer(phost,COLOR_RGB(139,92,50));	
	
	App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));		
	App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));			
	App_WrCoCmd_Buffer(phost,VERTEX2II(0,DispHeight-80,6,0));	
	App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(170));		
	App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(200));
	App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE));
	App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));		
	App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(256));
	App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE_MINUS_SRC_ALPHA));
	App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));	
	App_WrCoCmd_Buffer(phost,VERTEX2II(0,DispHeight-80,7,0));
	App_WrCoCmd_Buffer(phost,BLEND_FUNC(SRC_ALPHA,ONE));
	App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(5));	
	App_WrCoCmd_Buffer(phost,COLOR_RGB(200,10,10));	
	App_WrCoCmd_Buffer(phost,COLOR_A(200));	
	draw_bubbles(0);	
	App_WrCoCmd_Buffer(phost,COLOR_RGB(255,220,3));	
	App_WrCoCmd_Buffer(phost,COLOR_A(255));	
	draw_bubbles(1);
	App_WrCoCmd_Buffer(phost,COLOR_RGB(200,10,10));	
	collaid_bubbles(0);	
	App_WrCoCmd_Buffer(phost,COLOR_RGB(255,220,3));	
	collaid_bubbles(1);		
	App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
}
#endif

#ifdef BACKGROUND_ANIMATION_6

#define ANGLE 18 
int16_t xoffset_array[20],dx_pts[20],dy_pts[20];
uint8_t yoffset_array[20],point_size[20];
uint8_t rate_cts[20],iteration_cts[20];

void Backgroundanimation_6()
{
	static float move = 0;
	static uint8_t init = 0;
	int z= 0,x=0,y=0;
	int16_t Ox=0, Oy=0;
	uint8_t VEL = 0;
	if(istouch()) VEL = 2; else VEL = 1;
	if(!init)
	{
		for(z = 0; z<20 ; z++)
		{
			point_size[z] = 20 +random(20); 		
			dx_pts[z] = random(DispWidth+120);
			if(dx_pts[z]<DispWidth)
			dy_pts[z] = -random(DispHeight);
			else
			dy_pts[z] = random(DispHeight);				
			rate_cts[z] = 100+random(155);
		}
		init = 1;
	}
	
	move+=0.1; 
	if(move>=90)
	move = 0;
	App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
	Gpu_CoCmd_Gradient(phost,0, 0, 0x183c78, 340, 0,0x4560dd);
	App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
	App_WrCoCmd_Buffer(phost,COLOR_A(120));
	App_WrCoCmd_Buffer(phost,LINE_WIDTH(15*16));
	App_WrCoCmd_Buffer(phost,BEGIN(LINES));
	for(z=0;z<20;z++)
	{
		polar_draw((int32_t)(0),move+z*ANGLE,0,272);
		polar_draw((int32_t)(600),move+z*ANGLE,0,272);
	}	
	App_WrCoCmd_Buffer(phost,COLOR_A(60));
	App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
	App_WrCoCmd_Buffer(phost,POINT_SIZE(95*16));
	App_WrCoCmd_Buffer(phost,VERTEX2F(0,272*16));	
	App_WrCoCmd_Buffer(phost,COLOR_A(200));
	for(z=0;z<20;z++)
	{
		App_WrCoCmd_Buffer(phost,POINT_SIZE(point_size[z]*16));
		Ox = linear(0,dx_pts[z],iteration_cts[z],rate_cts[z]);
		Oy = linear(272,dy_pts[z],iteration_cts[z],rate_cts[z]);
		App_WrCoCmd_Buffer(phost,VERTEX2F(Ox*16,Oy*16));	
	}	
	App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
	for(z = 0; z<20 ; z++)
	{		
		if(iteration_cts[z]<rate_cts[z])iteration_cts[z]+=VEL; else{ iteration_cts[z] = 0;}
	}	
}
#endif
/* API for android method*/

void show_icon(uint8_t iconno)
{
	App_Play_Sound(phost,0x51,100,108);
	do
	{
		Gpu_CoCmd_Dlstart(phost);   
		App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		#ifdef BACKGROUND_ANIMATION_1		
			Backgroundanimation_1();
		#endif
		#ifdef BACKGROUND_ANIMATION_2		
			Backgroundanimation_2();
		#endif
		#ifdef BACKGROUND_ANIMATION_3		
			Backgroundanimation_3();
		#endif
		#ifdef BACKGROUND_ANIMATION_4		
			Backgroundanimation_4();
		#endif
		#ifdef BACKGROUND_ANIMATION_5		
			Backgroundanimation_5();
		#endif		
		#ifdef BACKGROUND_ANIMATION_6		
			Backgroundanimation_6();
		#endif	
		App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
		App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0));		  
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,200,100)); 
		App_WrCoCmd_Buffer(phost,CELL(iconno-1));
		App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(128));
		App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(128));
		App_WrCoCmd_Buffer(phost,VERTEX2F(((DispWidth-200)/2)*16,((DispHeight-100)/2)*16));	
		App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
		#ifdef BACKGROUND_ANIMATION_4
		App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
		#endif
		App_WrCoCmd_Buffer(phost,TAG('H'));
		App_WrCoCmd_Buffer(phost,VERTEX2II(5,5,14,0));	
		App_WrCoCmd_Buffer(phost,DISPLAY());
		Gpu_CoCmd_Swap(phost);
		App_Flush_Co_Buffer(phost);
		Gpu_Hal_WaitCmdfifo_empty(phost);		
	}while(App_Read_Tag(phost)!='H');
	App_Play_Sound(phost,0x51,100,108);
	scroller.vel = 0;
}

void android_menu()
{
	uint8_t image_height = 50,image_width = 100;
	uint8_t dt = 30,dx,dy;
	uint8_t col,row,per_frame,noof_frame,current_frame=0;
	uint8_t i,key_in=0,key_in_counts=0,temp=0;

	int16_t Ox,Oy,sx,drag=0,prev=0,drag_dt=30,dragth=0;

// for points

	uint8_t  point_offset,point_dt =15;

 
	dx = (dt*2)+image_width;	
	dy = (10*2)+image_height;
	col = DispWidth/dx;
	row = 2;
	per_frame = col*row;
	noof_frame = (MAX_MENUS-1)/per_frame;

	point_offset = (DispWidth-(noof_frame+1)*(MENU_POINTSIZE+point_dt))/2;

	scroller_init((DispWidth*noof_frame)*16);

	while(1)
	{
		/*Read touch screen x varaiation and tag in*/
		sx =  Gpu_Hal_Rd16(phost,REG_TOUCH_SCREEN_XY + 2);
		key_in = App_Read_Tag(phost);
		
		/*Check if any tag in*/    
		if(sx!=NOTOUCH)			
		keyin_cts++;     	
		
		/*Move into the particular frame based on dragdt now 30pixels*/
		if(sx==NOTOUCH)
		{
			keyin_cts = 0; 		
			if(drag>((current_frame*DispWidth)+drag_dt)) drag = min((current_frame+1)*DispWidth,drag+15); 
			if(drag<((current_frame*DispWidth)-drag_dt)) drag = max((current_frame-1)*DispWidth,drag-15); 
			if(dragth==drag) current_frame = drag/DispWidth;
			dragth = drag;
			scroller.vel = 0; 
			scroller.base = dragth*16;				// 16bit pre
		}
		/*if tag in but still pendown take a scroller basevalue*/
		else if(keyin_cts>KEYIN_COUNTS)
		{
			key_in = 0;
			drag = scroller.base>>4;
		}
		if(key_in==0)  scroller_run();

		/*Display list start*/   
		Gpu_CoCmd_Dlstart(phost);   
		App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		#ifdef BACKGROUND_ANIMATION_1		
		Backgroundanimation_1();
		#endif
		#ifdef BACKGROUND_ANIMATION_2		
				Backgroundanimation_2();
		#endif
		#ifdef BACKGROUND_ANIMATION_3		
				Backgroundanimation_3();
		#endif
		#ifdef BACKGROUND_ANIMATION_4		
				Backgroundanimation_4();
		#endif
		#ifdef BACKGROUND_ANIMATION_5		
				Backgroundanimation_5();
		#endif		
		#ifdef BACKGROUND_ANIMATION_6		
				Backgroundanimation_6();
		#endif	
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0));
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,100,50));
		App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		App_WrCoCmd_Buffer(phost,LINE_WIDTH(25));				// for rect	
		App_WrCoCmd_Buffer(phost,BEGIN(RECTS));	
		App_WrCoCmd_Buffer(phost,COLOR_A(150));
		App_WrCoCmd_Buffer(phost,COLOR_RGB(100,106,156));
		Oy = 10;
		for(i=0;i<=noof_frame;i++)
		{
			Ox = 10;  
			Ox+=(i*DispWidth);
			Ox-=drag;
			if(i==0) App_WrCoCmd_Buffer(phost,COLOR_RGB(156,100,128));
			if(i==1) App_WrCoCmd_Buffer(phost,COLOR_RGB(100,106,156));
			if(i==2) App_WrCoCmd_Buffer(phost,COLOR_RGB(156,152,100));   
			App_WrCoCmd_Buffer(phost,VERTEX2F((Ox)*16,(Oy)*16));	
			App_WrCoCmd_Buffer(phost,VERTEX2F((Ox+DispWidth-20)*16,(int16_t)(DispHeight*0.75)*16));					// i pixels wide than image width +1 
		}
		App_WrCoCmd_Buffer(phost,COLOR_A(255));
		App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255)); 
		for(i=0;i<MAX_MENUS;i++)
		{
			Ox = dt+dx*(i%col);                                          // Calculate the xoffsets
			Ox +=((i/per_frame)*DispWidth);       
			Ox -= drag;        
			Oy = dt+(dy*((i/col)%row));
			if(Ox > (DispWidth+dt)) 0;	
			else
			{
				App_WrCoCmd_Buffer(phost,VERTEX2F((Ox-1)*16,(Oy-1)*16));	
				App_WrCoCmd_Buffer(phost,VERTEX2F((image_width+Ox+1)*16,(image_height+Oy+1)*16));
			}					// i pixels wide than image width +1
		} 
		App_WrCoCmd_Buffer(phost,TAG_MASK(1)); 
		App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));		
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0));
		for(i=0;i<MAX_MENUS;i++)
		{
			Ox = dt+dx*(i%col);                                          // Calculate the xoffsets
			Ox +=((i/per_frame)*DispWidth);       
			Ox -= drag;        
			Oy = dt+(dy*((i/col)%row));	
			if(Ox > (DispWidth+dt) || Ox < -dx) 0;	
			else
			{
				App_WrCoCmd_Buffer(phost,CELL(i));
				App_WrCoCmd_Buffer(phost,TAG(i+1));	
				App_WrCoCmd_Buffer(phost,VERTEX2F(Ox*16,Oy*16));	
			}
		}
		App_WrCoCmd_Buffer(phost,TAG_MASK(0));

		// frame_no_points

		App_WrCoCmd_Buffer(phost,POINT_SIZE(MENU_POINTSIZE*16));
		App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));				
		App_WrCoCmd_Buffer(phost,COLOR_A(50));	
		Oy = DispHeight - 20;
		for(i=0;i<=noof_frame;i++)
		{
		  Ox = point_offset+(i*(MENU_POINTSIZE+point_dt));
		  App_WrCoCmd_Buffer(phost,VERTEX2F(Ox*16,Oy*16));		
		}

		Ox = point_offset+(current_frame*(MENU_POINTSIZE+point_dt));
		App_WrCoCmd_Buffer(phost,COLOR_A(255));
		App_WrCoCmd_Buffer(phost,VERTEX2F(Ox*16,Oy*16));	

		App_WrCoCmd_Buffer(phost,DISPLAY());
		Gpu_CoCmd_Swap(phost);
		App_Flush_Co_Buffer(phost);
		Gpu_Hal_WaitCmdfifo_empty(phost);	
		if(key_in>0 && key_in<=12 && !istouch())
		show_icon(key_in);
	}
}

// Api to Single Row//
void menu_loopback()
{
	uint8_t keyin_cts = 0,temp_keyin;
	uint8_t image_height = 50,image_width = 100; 
	uint8_t dt = 30,dx,dy;
	uint8_t per_frame,no_frames,key_in,current_frame; 
	int16_t sx,drag,Oy,Ox,dragth,i;
	dx = (dt*2)+image_width;	
	dy = (10*2)+image_height;
	per_frame = DispWidth/dx;
	no_frames = (MAX_MENUS-1)/per_frame;

	scroller_init((DispWidth*no_frames)*16);
	while(1)
	{
/*Read touch screen x varaiation and tag in*/
              sx =  Gpu_Hal_Rd16(phost,REG_TOUCH_SCREEN_XY + 2);
              key_in =  App_Read_Tag(phost);

              /*Check if any tag in*/    
              if(sx!=NOTOUCH)      
              keyin_cts++;               
              /*Move into the particular frame based on dragdt now 30pixels*/
              if(sx==NOTOUCH)
              keyin_cts = 0;                           
              /*if tag in but still pendown take a scroller basevalue*/
              else if(keyin_cts>KEYIN_COUNTS)
              key_in = 0;
              
              if(key_in==0)scroller_run();
              drag = scroller.base>>4;   


		App_Set_CmdBuffer_Index(0);
		Gpu_CoCmd_Dlstart(phost);   
		App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		#ifdef BACKGROUND_ANIMATION_1		
				Backgroundanimation_1();
		#endif
		#ifdef BACKGROUND_ANIMATION_2		
				Backgroundanimation_2();
		#endif
		#ifdef BACKGROUND_ANIMATION_3		
				Backgroundanimation_3();
		#endif
		#ifdef BACKGROUND_ANIMATION_4		
				Backgroundanimation_4();
		#endif
		#ifdef BACKGROUND_ANIMATION_5		
			Backgroundanimation_5();
		#endif		
		#ifdef BACKGROUND_ANIMATION_6		
			Backgroundanimation_6();
		#endif			
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0));	 
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 100, 50));	
		App_WrCoCmd_Buffer(phost,TAG_MASK(1));
		App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
		App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16)); // for rect	

		Oy = (DispHeight-image_width)/2;					//dt+(dy*((i/col)%row));
		current_frame = drag/dx;                       // noof items moved in +/- directions
		dragth = drag%dx;

		App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
		for(i=-1;i<(per_frame+1);i++)
		{
			Ox = dt+dx*i;
			Ox-=dragth;
			if(Ox > (DispWidth+dt) || Ox < -dx) 0;						
			else
			{      
				App_WrCoCmd_Buffer(phost,VERTEX2F((Ox-1)*16,(Oy-1)*16));	
				App_WrCoCmd_Buffer(phost,VERTEX2F((image_width+Ox+1)*16,(image_height+Oy+1)*16));					// i pixels wide than image width +1  
			}
		}

		App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));											// draw the bitmap
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0));
		for(i=-1;i<(per_frame+1);i++)
		{
			Ox = dt+dx*i;
			Ox-=dragth;
			if(Ox > (DispWidth+dt) || Ox < -dx) 0;						
			else
			{ 
				App_WrCoCmd_Buffer(phost,CELL((MAX_MENUS+i+current_frame)%12));
				App_WrCoCmd_Buffer(phost,TAG((1+i+current_frame)%(MAX_MENUS+1)));	
				App_WrCoCmd_Buffer(phost,VERTEX2F(Ox*16,Oy*16));	
			}
		}
		
		App_WrCoCmd_Buffer(phost,DISPLAY());
		Gpu_CoCmd_Swap(phost);
		App_Flush_Co_Buffer(phost);
		Gpu_Hal_WaitCmdfifo_empty(phost);	
		if(key_in>0 && key_in<=12 && !istouch())
		show_icon(key_in);
	}  
}  

// Api to tiles type menu*/
void menu_win8()
{
	uint8_t current_frame = 0,total_frames = 0,key_in=0;	
	int16_t frame_xoffset = 0,frame_xoffset_th= 0; 
	uint8_t menus_per_frame = 0;
	uint8_t col = 3,row = 2,option;
	uint16_t image_height = 50,image_width = 100,rectangle_width,rectangle_height; 
	int16_t Ox,Oy,i,sx;
#if defined(DISPLAY_RESOLUTION_WVGA)
	uint8_t frame_xoffset_dt = 50;
#else
	uint8_t frame_xoffset_dt = 30;
#endif


	uint8_t color[12][3] = {  0xE0,0x01B,0xA2,
								 0x1B,0xE0,0xA8,
								 0x9E,0x9E,0x73,
								 0xE0,0x8E,0x1B,
								 0xB8,0x91,0xB3,
								 0x6E,0x96,0x8e,
								 0x1B,0x60,0xE0,
								 0xC7,0xE3,0x7B,
								 0x8B,0x1B,0xE0,
								 0xE3,0x91,0xC1,
								 0xE0,0x8E,0x1B,
								 0xAC,0x12,0xE3,
							};

	char *menudetails[20]=	{  "Music",  "Gauges ",  "Gradient",  "Photo",  "Metaball",  "Notepad",  "Signature",  "Sketch","Swiss","Waves","Player","Clocks"};

	uint8_t  point_offset,frame_point_dt =15;

 
	uint16_t dx = (frame_xoffset_dt*2)+image_width;	
	uint16_t dy = (10*2)+image_height;
	col = DispWidth/dx;
	menus_per_frame = col*row;
	total_frames = (MAX_MENUS-1)/menus_per_frame;

	point_offset = (DispWidth-(total_frames+1)*(MENU_POINTSIZE+frame_point_dt))/2;	
	/*Load menu Thumbnails*/
	//Load_Thumbnails();
	/*Intilaize the scroller*/
	scroller_init((DispWidth*total_frames)*16);


	while(1)
	{ 
		/*Read touch screen x varaiation and tag in*/
		sx =  Gpu_Hal_Rd16(phost,REG_TOUCH_SCREEN_XY + 2);
		key_in =  App_Read_Tag(phost);

		/*Check if any tag in*/    
		if(sx!=NOTOUCH)	
		keyin_cts++;     	
			   
		
		/*Move into the particular frame based on dragdt now 30pixels*/
		if(sx==NOTOUCH)
		{
			keyin_cts = 0;
			frame_xoffset = scroller.base>>4; 	
		}
		/*if tag in but still pendown take a scroller basevalue*/
		else if(keyin_cts>KEYIN_COUNTS)
		{
			key_in = 0;
			frame_xoffset = scroller.base>>4;
		}
		if(key_in==0)scroller_run();
		
	    Gpu_CoCmd_Dlstart(phost);   
	    App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
		App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0));		  
		App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST,BORDER,BORDER,100,50)); 
	    App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
		#ifdef BACKGROUND_ANIMATION_1
				Backgroundanimation_1();
		#endif

		#ifdef BACKGROUND_ANIMATION_2
				Backgroundanimation_2();
		#endif
		#ifdef BACKGROUND_ANIMATION_3
				Backgroundanimation_3();
		#endif
		#ifdef BACKGROUND_ANIMATION_4
				Backgroundanimation_4();
		#endif

		#ifdef BACKGROUND_ANIMATION_5
				Backgroundanimation_5();
		#endif
		#ifdef BACKGROUND_ANIMATION_6		
				Backgroundanimation_6();
		#endif
	    App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());

#if defined(DISPLAY_RESOLUTION_WVGA) && defined(FT81X_ENABLE)
		App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(0));
#endif
	   	   
#if defined(DISPLAY_RESOLUTION_WQVGA) || defined(DISPLAY_RESOLUTION_WVGA)
		for(option=0;option<3;option++)
		{
		   switch(option)
		   {
			  case 0:
				 App_WrCoCmd_Buffer(phost,LINE_WIDTH(1*16));
				 App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
			  break;
			  case 1:
				  App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
				  App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
			  break;
			  case 2:	  
				  App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
			  break;
		   }
#if defined(DISPLAY_RESOLUTION_WVGA)
		   //rectangle_width = 366;
		   rectangle_width = 360;
		   rectangle_height = 200;
#else
		   rectangle_width = 220;
		   rectangle_height = 100;
#endif
		   for(i=0;i<4;i+=1)
		   {
				if(i<2)
				{
					Ox = 10+DispWidth*i;
					//Ox = 0 +DispWidth*i;
					Oy = 10;
				}else
				{	
#if defined(DISPLAY_RESOLUTION_WVGA)
					Ox = 400+DispWidth*(i%2);
					Oy = 220;
#else
					Ox = 250+DispWidth*(i%2);
					Oy = 120;
#endif
				}
				Ox -= frame_xoffset;		
				//if(Ox > (DispWidth+frame_xoffset_dt) || Ox < -512) 0;	
				if(Ox > (DispWidth+frame_xoffset_dt) || Ox < (-DispWidth)) 0;
				else
				{	
					App_WrCoCmd_Buffer(phost,TAG(i+1));	
					switch(option)
					{
						case 0:
							App_WrCoCmd_Buffer(phost,COLOR_RGB(color[i][0],color[i][1],color[i][2]));	
							#if defined(DISPLAY_RESOLUTION_WVGA)
								App_WrCoCmd_Buffer(phost,VERTEX2F((Ox),(Oy)));	
								App_WrCoCmd_Buffer(phost,VERTEX2F((rectangle_width+Ox),(rectangle_height+Oy)));					// i pixels wide than image width +1
							#else
								App_WrCoCmd_Buffer(phost,VERTEX2F((Ox)*16,(Oy)*16));	
								App_WrCoCmd_Buffer(phost,VERTEX2F((rectangle_width+Ox)*16,(rectangle_height+Oy)*16));	
							#endif
						break;
						case 1:
							App_WrCoCmd_Buffer(phost,CELL(i));
							#if defined(DISPLAY_RESOLUTION_WVGA)
								App_WrCoCmd_Buffer(phost,VERTEX2F((55+Ox),(25+Oy)));
							#else
								App_WrCoCmd_Buffer(phost,VERTEX2F((55+Ox)*16,(25+Oy)*16));
							#endif
						break;

						case 2:
							Gpu_CoCmd_Text(phost,Ox+10,Oy+80,26,0,menudetails[i]);
						break;
					}
				}
		   }
#if defined(DISPLAY_RESOLUTION_WVGA)
		   rectangle_width = 170;
		   rectangle_height = 200;	
#else
		   rectangle_width = 100;
		   rectangle_height = 100;	
#endif
		   if(option==1)  App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(512));
		  
		   for(i=0;i<8;i+=1)
		   {
				if(i<4)
				{
#if defined(DISPLAY_RESOLUTION_WVGA)  
					//Ox = 400+DispWidth*(i/2)+(image_width*(i%2))+(20*(i%2));	// 20 is space between two icon
					Ox = 400+DispWidth*(i/2)+(rectangle_width*(i%2))+(20*(i%2));
					Oy = 10;
#else
					Ox = 250+DispWidth*(i/2)+(image_width*(i%2))+(20*(i%2));	// 20 is space between two icon
					//Ox = 150+DispWidth*(i/2)+(image_width*(i%2))+(20*(i%2));
					Oy = 10;
#endif
				}
				else
				{	
#if defined(DISPLAY_RESOLUTION_WVGA)

					//Ox = 10 + (DispWidth*(i/6)) + (((i-4)%2)*image_width) + (((i-4)%2)*20);
					Ox = 10 + (DispWidth*(i/6)) + (((i-4)%2)*rectangle_width) + (((i-4)%2)*20);
					Oy = 220;
#else
					Ox = 10+DispWidth*(i/6)+ (((i-4)%2)*image_width)+(((i-4)%2)*20);
					Oy = 120;
#endif
				}
				Ox -= frame_xoffset;		
				//if(Ox > (DispWidth+frame_xoffset_dt) || Ox < -512) 0;	
				if(Ox > (DispWidth+frame_xoffset_dt) || Ox < -DispWidth) 0;	
				else
				{
					App_WrCoCmd_Buffer(phost,TAG(i+5));	
					switch(option)
					{	
						case 0:
								App_WrCoCmd_Buffer(phost,COLOR_RGB(color[i+5][0],color[i+5][1],color[i+5][2]));	
								#if defined(DISPLAY_RESOLUTION_WVGA)
									App_WrCoCmd_Buffer(phost,VERTEX2F((Ox),(Oy)));	
									App_WrCoCmd_Buffer(phost,VERTEX2F((rectangle_width+Ox),(rectangle_height+Oy)));					// i pixels wide than image width +1
								#else
									App_WrCoCmd_Buffer(phost,VERTEX2F((Ox)*16,(Oy)*16));	
									App_WrCoCmd_Buffer(phost,VERTEX2F((rectangle_width+Ox)*16,(rectangle_height+Oy)*16));	
								#endif
						break;

						case 1:							
								App_WrCoCmd_Buffer(phost,CELL(i+4));		
								#if defined(DISPLAY_RESOLUTION_WVGA)
									App_WrCoCmd_Buffer(phost,VERTEX2F((25+Ox),(25+Oy)));	
								#else
									App_WrCoCmd_Buffer(phost,VERTEX2F((25+Ox)*16,(25+Oy)*16));
								#endif
						break;

						case 2:
								Gpu_CoCmd_Text(phost,Ox+10,Oy+80,26,0,menudetails[i+4]);
						break;
					}								// i pixels wide than image width +1
				}
		   }
		   if(option==1)  App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
		}			
  
#elif defined(DISPLAY_RESOLUTION_HVGA_PORTRAIT) || defined(DISPLAY_RESOLUTION_QVGA)
		
  rectangle_width = 170;
  rectangle_height = 100;	
  App_WrCoCmd_Buffer(phost,BEGIN(RECTS));
  for(i=0;i<12;i+=2)
  {
    Ox = 10+DispWidth*(i/4);
    Ox -= frame_xoffset;
    Oy = 10+(110*((i/col)%row));
    App_WrCoCmd_Buffer(phost,COLOR_RGB(color[i][0],color[i][1],color[i][2]));
	App_WrCoCmd_Buffer(phost,TAG(i+1));	
    App_WrCoCmd_Buffer(phost,VERTEX2F((Ox)*16,(Oy)*16)	);	
    App_WrCoCmd_Buffer(phost,VERTEX2F((rectangle_width+Ox)*16,(rectangle_height+Oy)*16));					// i pixels wide than image width +1
  }

  rectangle_width = 110;
  rectangle_height = 100;	

  for(i=1;i<12;i+=2)
  {
    Ox = 200+DispWidth*(i/4);
    Ox -= frame_xoffset;
    Oy = 10+(110*((i/col)%row));
    App_WrCoCmd_Buffer(phost,COLOR_RGB(color[i][0],color[i][1],color[i][2]));
	App_WrCoCmd_Buffer(phost,TAG(i+1));	
    App_WrCoCmd_Buffer(phost,VERTEX2F((Ox)*16,(Oy)*16));	
    App_WrCoCmd_Buffer(phost,VERTEX2F((rectangle_width+Ox)*16,(rectangle_height+Oy)*16));					// i pixels wide than image width +1
  }
   App_WrCoCmd_Buffer(phost,TAG_MASK(0));

  App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  for(i=0;i<12;i+=2)
  {
    Ox = 10+DispWidth*(i/4);
    Ox -= frame_xoffset;
    Oy = 10+(110*((i/col)%row));
    Gpu_CoCmd_Text(phost,Ox+10,Oy+80,26,0,menudetails[i]);
  }
  for(i=1;i<12;i+=2)
  {
    Ox = 200+DispWidth*(i/4);
    Ox -= frame_xoffset;
    Oy = 10+(110*((i/col)%row));
    Gpu_CoCmd_Text(phost,Ox+10,Oy+80,26,0,menudetails[i]);
  }
		
  rectangle_height = 100;
  rectangle_height = 50;
  App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));											// draw the bitmap
  App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0));
  for(i=0;i<12;i+=2)
  {
    Ox = 75+DispWidth*(i/4);
    Ox -= frame_xoffset;
    Oy = 20+(110*((i/col)%row));
    App_WrCoCmd_Buffer(phost,CELL(i));
    App_WrCoCmd_Buffer(phost,TAG(i+1));	
    App_WrCoCmd_Buffer(phost,VERTEX2F(Ox*16,Oy*16));	
  }
  App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(512));
  for(i=1;i<12;i+=2)
  {
    Ox = 230+DispWidth*(i/4);
    Ox -= frame_xoffset;
    Oy = 20+(110*((i/col)%row));
    App_WrCoCmd_Buffer(phost,CELL(i));    
    App_WrCoCmd_Buffer(phost,TAG(i+1));	
    App_WrCoCmd_Buffer(phost,VERTEX2F(Ox*16,Oy*16));	
  }
  App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
#endif
#if defined(DISPLAY_RESOLUTION_WVGA) && defined(FT81X_ENABLE)
  App_WrCoCmd_Buffer(phost, VERTEX_FORMAT(4));
#endif
  App_WrCoCmd_Buffer(phost,TAG_MASK(0));

  App_WrCoCmd_Buffer(phost,DISPLAY());
  Gpu_CoCmd_Swap(phost);
  App_Flush_Co_Buffer(phost);
  Gpu_Hal_WaitCmdfifo_empty(phost);	 

  if(key_in!=0 && key_in<=12 && !istouch())
  show_icon(key_in);
  }

}

#if defined(ARDUINO_PLATFORM)
prog_char8_t * const info[] =
#else
char *info[] =
#endif
{
	"EVE MainMenu Application",
	"APP to demonstrate interactive menus,",
	"using Jpeg decode,",
	"Rectangle & Points."
};

#if defined(MSVC_PLATFORM) || defined(FT9XX_PLATFORM)
/* Main entry point */
int32_t main(int32_t argc,char8_t *argv[])
#endif
#if defined(ARDUINO_PLATFORM)||defined(MSVC_FT800EMU)
void setup()
#endif
{	
  Serial.begin(9600);
  Serial.println(F("STARTING"));
    phost = &host;
    /* Init HW Hal */
    App_Common_Init(phost);
    /* Show Logo, do calibration and display welcome screeen */
    App_Common_Start(phost,(char**)info);
    /* Main application */

    Serial.print(F("WIDTH="));
    Serial.println(DispWidth);
    Serial.print(F("HEIGHT="));
    Serial.println(DispHeight);    
    
	Load_Thumbnails();
	#if defined(ANDROID_METHOD)
	android_menu();
	#elif defined(LOOPBACK_METHOD)
	menu_loopback();
	#elif defined(WIN8_METHOD)
	menu_win8();
	#endif

    /* Close all the opened handles */
    App_Common_Close(phost);
#if  defined(MSVC_PLATFORM) || defined(FT9XX_PLATFORM)
    return 0;
#endif
}


void loop()
{
}

/* Nothing beyond this */
