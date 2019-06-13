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
* @file  App_Lift.c/.ino
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
#include "sdcard.h"
#include "App_Lift.h"
#include "App_Common.h"

#define STARTUP_ADDRESS  100*1024L

#define AUDIO_SECTORS  4
#define AUDIO_RAM_SIZE  4096L
#define AUDIO_RAM_SPACE (AUDIO_RAM_SIZE-1)
extern Reader imageFile;


/* Global used for context hal */
Gpu_Hal_Context_t host,*phost;



PROGMEM const Gpu_Fonts_t g_Gpu_Fonts[] = {
    /* VC1 Hardware Fonts index 28*/
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,6,8,13,12,15,13,5,7,7,9,12,5,9,6,9,12,12,12,12,12,12,12,12,12,12,5,5,11,12,11,10,19,13,13,13,14,12,12,14,15,6,12,14,12,18,15,14,13,15,13,13,13,14,14,18,13,13,13,6,9,6,9,10,7,12,12,11,12,11,8,12,12,5,5,11,5,18,12,12,12,12,7,11,7,12,11,16,11,11,11,7,5,7,14,5,
        2,
        9,
        18,
        25,
        950172},
};




//================================Dec to binary and binary to dec =========================================
byte decToBcd(byte val)
{
    return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)
{
    return ( (val/16*10) + (val%16) );
}



/* APIs useful for bitmap postprocessing */
/* Identity transform matrix */
int16_t App_TrsMtrxLoadIdentity(App_GraphicsMatrix_t *pMatrix)
{
    /* Load the identity matrix into the input pointer */
    pMatrix->a = 1*APP_MATIRX_PRECITION;
    pMatrix->b = 0*APP_MATIRX_PRECITION;
    pMatrix->c = 0*APP_MATIRX_PRECITION;
    pMatrix->d = 0*APP_MATIRX_PRECITION;
    pMatrix->e = 1*APP_MATIRX_PRECITION;
    pMatrix->f = 0*APP_MATIRX_PRECITION;
    return 0;
}
int16_t App_UpdateTrsMtrx(App_GraphicsMatrix_t *pMatrix,App_PostProcess_Transform_t *ptrnsmtrx)
{
    /* perform the translation from float to FT800 specific transform units */
    ptrnsmtrx->Transforma = pMatrix->a*APP_MATRIX_GPU_PRECITION;
    ptrnsmtrx->Transformd = pMatrix->d*APP_MATRIX_GPU_PRECITION;
    ptrnsmtrx->Transformb = pMatrix->b*APP_MATRIX_GPU_PRECITION;
    ptrnsmtrx->Transforme = pMatrix->e*APP_MATRIX_GPU_PRECITION;
    ptrnsmtrx->Transformc = pMatrix->c*APP_MATRIX_GPU_PRECITION;
    ptrnsmtrx->Transformf = pMatrix->f*APP_MATRIX_GPU_PRECITION;

    return 0;
}

/* give out the translated values. */
int16_t App_TrsMtrxTranslate(App_GraphicsMatrix_t *pMatrix,float x,float y,float *pxres,float *pyres)
{
    float a,b,c,d,e,f;
    
    /* matrix multiplication */
    a = pMatrix->a; b = pMatrix->b; c = pMatrix->c; d = pMatrix->d; e = pMatrix->e; f = pMatrix->f;

    *pxres = x*a + y*b + c;
    *pyres = x*d + y*e + f;
    pMatrix->c = *pxres;
    pMatrix->f = *pyres;

    return 0;
}
/* API for rotation of angle degree - using vc++ math library - if not then use array of sine tables to compule both sine and cosine */
/* +ve degree for anti clock wise and -ve for clock wise */
/* note not checking the overflow cases */
int16_t App_TrsMtrxRotate(App_GraphicsMatrix_t *pMatrix,int32_t th)
{
    float pi = 3.1415926535;
    float angle = -th*pi/180;
    App_GraphicsMatrix_t tempmtx,tempinput;


    tempinput.a = pMatrix->a;
    tempinput.b = pMatrix->b;
    tempinput.c = pMatrix->c;
    tempinput.d = pMatrix->d;
    tempinput.e = pMatrix->e;
    tempinput.f = pMatrix->f;

    App_TrsMtrxLoadIdentity(&tempmtx);
    tempmtx.a = cos(angle)*APP_MATIRX_PRECITION;
    tempmtx.b = sin(angle)*APP_MATIRX_PRECITION;
    tempmtx.d = -sin(angle)*APP_MATIRX_PRECITION;
    tempmtx.e = cos(angle)*APP_MATIRX_PRECITION;

    /* perform matrix multiplecation and store in the input */
    pMatrix->a = tempinput.a*tempmtx.a + tempinput.b*tempmtx.d;
    pMatrix->b = tempinput.a*tempmtx.b + tempinput.b*tempmtx.e;
    pMatrix->c = tempinput.a*tempmtx.c + tempinput.b*tempmtx.f + tempinput.c*1;
    pMatrix->d = tempinput.d*tempmtx.a + tempinput.e*tempmtx.d;
    pMatrix->e = tempinput.d*tempmtx.b + tempinput.e*tempmtx.e;
    pMatrix->f = tempinput.d*tempmtx.c + tempinput.e*tempmtx.f + tempinput.f*1;

    return 0;
}
/* Scaling done for x and y factors - from 1 to 255 */
/* Input units are in terms of 65536 */
int16_t App_TrsMtrxScale(App_GraphicsMatrix_t *pMatrix,float xfactor,float yfactor)
{

    pMatrix->a /= xfactor;
    pMatrix->d /= xfactor;

    pMatrix->b /= yfactor;
    pMatrix->e /= yfactor;

    return 0;
}
/* flip the image - 1 for right flip, 2 for bottom flip */
int16_t App_TrsMtrxFlip(App_GraphicsMatrix_t *pMatrix,int32_t Option)
{
    /* need to verify both */
    if(APP_GRAPHICS_FLIP_RIGHT == (Option & APP_GRAPHICS_FLIP_RIGHT))
    {
        pMatrix->a = -pMatrix->a;
        pMatrix->d = -pMatrix->d;
    }
    if(APP_GRAPHICS_FLIP_BOTTOM == (Option & APP_GRAPHICS_FLIP_BOTTOM))
    {
        pMatrix->b = -pMatrix->b;
        pMatrix->e = -pMatrix->e;
    }

    return 0;
}
/* Arrawy used for custom fonts */
S_LiftAppFont_t G_LiftAppFontArrayNumbers[1] = 
{
    //font structure
    {
        /* Max Width */
        80,
        /* Max Height */
        156,
        /* Max Stride */
        80,
        /* format */
        L8,
        /* Each character width */
        80,80,80,80,80,80,80,80,80,80,80,80,
    }
};
S_LiftAppFont_t G_LiftAppFontArrayArrow[1] = 
{ //arrow structure
    {
        /* Max Width */
        80,
        /* Max Height */
        85,
        /* Max Stride */
        80,
        /* format */
        L8,
        /* Each character width */
        80,
    }
};
/* API to check the status of previous DLSWAP and perform DLSWAP of new DL */
/* Check for the status of previous DLSWAP and if still not done wait for few ms and check again */
void App_GPU_DLSwap(uint8_t DL_Swap_Type)
{
    uint8_t Swap_Type = DLSWAP_FRAME,Swap_Done = DLSWAP_FRAME;

    if(DL_Swap_Type == DLSWAP_LINE)
    {
        Swap_Type = DLSWAP_LINE;
    }

    /* Perform a new DL swap */
    Gpu_Hal_Wr8(phost,REG_DLSWAP,Swap_Type);

    /* Wait till the swap is done */
    while(Swap_Done)
    {
        Swap_Done = Gpu_Hal_Rd8(phost,REG_DLSWAP);

        if(DLSWAP_DONE != Swap_Done)
        {
            Gpu_Hal_Sleep(10);//wait for 10ms
        }
    } 
}


/* API to compute the bitmap offset wrt input characters and scale 
ResizeVal is in terms of 8 bits precision - ex 1 will be equal to 256
xOffset and  yOffset are in terms of 4 bits precision - 1/16th pixel format and are the center of the display character
Even the FT800 display list is generated in this API
*/
int32_t LiftAppComputeBitmap(S_LiftAppFont_t *pLAFont, int32_t FloorNum,uint8_t TotalNumChar,int32_t ResizeVal,int16_t xOffset,int16_t yOffset)
{
    /* compute the total number of digits in the input */
    uint8_t TotalChar = 1;
    int32_t TotHzSz,TotVtSz,FlNum,i,xoff,yoff;
    char8_t FontValArray[8];

    TotalChar = 1;
    FlNum = FloorNum;
    TotHzSz = pLAFont->MaxWidth;
    FontValArray[0] = '\0';//requirement from dec2ascii api

    Gpu_Hal_Dec2Ascii(FontValArray,FloorNum);
    TotalChar = strlen(FontValArray);
    if(0 != TotalNumChar)
    {
        TotalChar = TotalNumChar;
    }
    TotHzSz = (pLAFont->MaxWidth * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
    TotVtSz = (pLAFont->MaxHeight * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;

    //change the x and y offset to center of the characters
    xoff = xOffset*16 - TotHzSz*TotalChar/2;
    yoff = yOffset*16 - TotVtSz/2;
    /* since resize value is same for both the sides the resize is same for all characters */
    for(i=0;i<TotalChar;i++)
    {
        /* Calculation of cell number from  */
        if('-' == FontValArray[i])
        {
            App_WrCoCmd_Buffer(phost,CELL(0));
        }
        else
        {

            App_WrCoCmd_Buffer(phost,CELL(FontValArray[i]-'0' + 1));
        }
        App_WrCoCmd_Buffer(phost,VERTEX2F(xoff,yoff));
        //increment the character after every ittiration
        xoff += TotHzSz;
    }
    return 0;
}

int32_t LiftAppComputeBitmapRowRotate(S_LiftAppFont_t *pLAFont, int32_t FloorNum,uint8_t TotalNumChar,int32_t ResizeVal,int16_t xOffset,int16_t yOffset)
{
    /* compute the total number of digits in the input */
    uint8_t TotalChar = 1;
    int32_t TotHzSz,TotVtSz,FlNum,i,xoff,yoff;
    char8_t FontValArray[8];

    TotalChar = 1;
    FlNum = FloorNum;
    TotHzSz = pLAFont->MaxWidth;
    FontValArray[0] = '\0';//requirement from dec2ascii api

    Gpu_Hal_Dec2Ascii(FontValArray,FloorNum);
    TotalChar = strlen(FontValArray);
    if(0 != TotalNumChar)
    {
        TotalChar = TotalNumChar;
    }
    TotHzSz = (pLAFont->MaxWidth * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
    TotVtSz = (pLAFont->MaxHeight * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;

    //change the x and y offset to center of the characters
    xoff = xOffset*16 - TotVtSz/2;
    yoff = yOffset*16 - (TotHzSz*TotalChar/2) + (TotHzSz*(TotalChar-1));
    /* since resize value is same for both the sides the resize is same for all characters */
    for(i=0;i<TotalChar;i++)
    {
        /* Calculation of cell number from  */
        if('-' == FontValArray[i])
        {
            App_WrCoCmd_Buffer(phost,CELL(0));
        }
        else
        {

            App_WrCoCmd_Buffer(phost,CELL(FontValArray[i]-'0' + 1));
        }
        App_WrCoCmd_Buffer(phost,VERTEX2F(xoff,yoff));
        //increment the character after every ittiration
        yoff -= TotHzSz;
    }
    return 0;
}


/* API for floor number logic */
int32_t LiftApp_FTransition(S_LiftAppCtxt_t *pLACtxt, int32_t dstaddr/*,uint8_t opt_landscape*/)
{

    char8_t StringArray[8];
    int32_t dst_addr_st_file;
    StringArray[0] = '\0';

    /* check for the current floor number logic */
    if(pLACtxt->CurrFloorNum == pLACtxt->NextFloorNum)
    {

        /* first time only entry */
        if(LIFTAPP_DIR_NONE != pLACtxt->ArrowDir)
        {
            pLACtxt->CurrFloorNumStagnantRate = pLACtxt->SLATransPrms.FloorNumStagnantRate;
            pLACtxt->CurrFloorNumResizeRate = pLACtxt->SLATransPrms.ResizeRate;
        }
        /* Make the direction to 0 */
        pLACtxt->ArrowDir = LIFTAPP_DIR_NONE;//arrow is disabled in none direction
        {
            //for landscape orienation
            if(pLACtxt->opt_orientation == 1)
            {
                if((LIFTAPPAUDIOSTATE_NONE == pLACtxt->AudioState) && (0 == pLACtxt->AudioPlayFlag))
                {
                    char8_t *pstring;
                    
                    pLACtxt->AudioState = LIFTAPPAUDIOSTATE_INIT;
                    pLACtxt->AudioFileIdx = 0;
                    pLACtxt->AudioCurrAddr = dstaddr;
                    pLACtxt->AudioCurrFileSz = 0;
                    pstring = pLACtxt->AudioFileNames;
                    *pstring = '\0';
                    strcpy(pstring,"bl.wav");
                    pstring += strlen(pstring);
                    *pstring++ = '\0';
                    StringArray[0] = '\0';
                    Gpu_Hal_Dec2Ascii(StringArray,(int32_t)pLACtxt->CurrFloorNum);
                    strcat(StringArray, ".wav");
                    strcpy(pstring,StringArray);
                    pstring += strlen(pstring);
                    *pstring++ = '\0';
                    *pstring = '\0';
                    pLACtxt->AudioPlayFlag = 1;
                    //App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */
                }
                /* load the ring tone */
                App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */
                
            }
            
            //for portrait orientation
            else if(pLACtxt->opt_orientation == 0)  
            {
                
                if((LIFTAPPAUDIOSTATE_NONE == pLACtxt->AudioState) && (0 == pLACtxt->AudioPlayFlag))
                {
                    char8_t *pstring;
                    
                    pLACtxt->AudioState = LIFTAPPAUDIOSTATE_INIT;
                    pLACtxt->AudioFileIdx = 0;
                    pLACtxt->AudioCurrAddr = dstaddr;
                    pLACtxt->AudioCurrFileSz = 0;
                    pstring = pLACtxt->AudioFileNames;
                    *pstring = '\0';
                    strcpy(pstring,"bf.wav");
                    pstring += strlen(pstring);
                    *pstring++ = '\0';
                    StringArray[0] = '\0';
                    Gpu_Hal_Dec2Ascii(StringArray,(int32_t)pLACtxt->CurrFloorNum);
                    strcat(StringArray, ".wav");
                    strcpy(pstring,StringArray);
                    pstring += strlen(pstring);
                    *pstring++ = '\0';
                    *pstring = '\0';
                    //App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */
                    pLACtxt->AudioPlayFlag = 1;
                }
                /* load the ring tone */
                App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */
                
            }

        }


        /* resizing of floor number */
        if(pLACtxt->CurrFloorNumResizeRate > 0)
        {
            pLACtxt->CurrFloorNumResizeRate--;
            return 0;
        }
        
        pLACtxt->CurrFloorNumResizeRate = 0;

        /* rate implmenetation */
        if(pLACtxt->CurrFloorNumStagnantRate > 0)
        {
            
            pLACtxt->CurrFloorNumStagnantRate--;//rate based logic
            return 0;//do not perform any thing

        }

        pLACtxt->CurrFloorNumStagnantRate = 0;
        
        /* delay the below code till rate is completed - modify the rate if application over rights */
        //limitation - make sure the max is +ve
        while((pLACtxt->CurrFloorNum == pLACtxt->NextFloorNum) || (0 == pLACtxt->NextFloorNum))//to make sure the same floor number is not assigned
        {
            pLACtxt->NextFloorNum = pLACtxt->SLATransPrms.MinFloorNum + random(pLACtxt->SLATransPrms.MaxFloorNum - pLACtxt->SLATransPrms.MinFloorNum);
        }

        pLACtxt->ArrowDir = LIFTAPP_DIR_DOWN;
        /* generate a new random number and change the direction as well */
        //if(LIFTAPPAUDIOSTATE_NONE == pLACtxt->AudioState) 
        {
            char8_t *pstring;
            
            pLACtxt->AudioState = LIFTAPPAUDIOSTATE_INIT;
            pLACtxt->AudioPlayFlag = 0;
            pstring = pLACtxt->AudioFileNames;
            *pstring = '\0';                
            /* load the ring tone */
            if(pLACtxt->NextFloorNum > pLACtxt->CurrFloorNum)
            {
                pLACtxt->ArrowDir = LIFTAPP_DIR_UP;
                //App_LoadRawAndPlay("gu.wav",(int32_t)dstaddr,0,0);
                strcpy(pstring,"gu.wav");
            }
            else
            {
                //App_LoadRawAndPlay("gd.wav",(int32_t)dstaddr,0,0);
                strcpy(pstring,"gd.wav");
            }
            pLACtxt->AudioFileIdx = 0;
            pLACtxt->AudioCurrAddr = dstaddr;
            pLACtxt->AudioCurrFileSz = 0;

            pstring += strlen(pstring);
            *pstring++ = '\0';
            *pstring++ = '\0';
            *pstring = '\0';
        }
        App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */

        /* set the starting of the arrow resize */
        pLACtxt->CurrArrowResizeRate = pLACtxt->SLATransPrms.ResizeRate;
        //set the curr floor number change 
        pLACtxt->CurrFloorNumChangeRate = pLACtxt->SLATransPrms.FloorNumChangeRate;
        return 0;
    }

    /* moving up or moving down logic */
    /* rate based on count */
    App_AudioPlay(pLACtxt,dstaddr,2*1024);/* load the floor number audio */
    
    pLACtxt->CurrArrowResizeRate--;
    if(pLACtxt->CurrArrowResizeRate <= 0)
    {
        pLACtxt->CurrArrowResizeRate = pLACtxt->SLATransPrms.ResizeRate;
    }


    //changing floor numbers
    pLACtxt->CurrFloorNumChangeRate--;
    if(pLACtxt->CurrFloorNumChangeRate <= 0)
    {
        pLACtxt->CurrFloorNumChangeRate = pLACtxt->SLATransPrms.FloorNumChangeRate;
        //change the floor number wrt direction
        if(LIFTAPP_DIR_DOWN == pLACtxt->ArrowDir)
        {
            pLACtxt->CurrFloorNum -=1;
            if(0 == pLACtxt->CurrFloorNum)
            {
                pLACtxt->CurrFloorNum -=1;
            }
        }
        else
        {
            pLACtxt->CurrFloorNum +=1;
            if(0 == pLACtxt->CurrFloorNum)
            {
                pLACtxt->CurrFloorNum +=1;
            }
        }
    }
    return 0;
}

//API for single bitmap computation - follow the above api for more details
int32_t LiftAppComputeBitmap_Single(S_LiftAppFont_t *pLAFont, int32_t BitmapIdx,int32_t ResizeVal,int16_t xOffset,int16_t yOffset)
{
    /* compute the total number of digits in the input */
    int32_t TotHzSz,TotVtSz,FlNum,i,xoff,yoff;

    TotHzSz = (pLAFont->MaxWidth * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;
    TotVtSz = (pLAFont->MaxHeight * 16 * ResizeVal)/LIFTAPPCHARRESIZEPRECISION;

    //change the x and y offset to center of the characters
    xoff = xOffset*16 - TotHzSz/2;
    yoff = yOffset*16 - TotVtSz/2;

    App_WrCoCmd_Buffer(phost,CELL(BitmapIdx));
    App_WrCoCmd_Buffer(phost,VERTEX2F(xoff,yoff));

    return 0;
}

int16_t linear(int16_t p1,int16_t p2,uint16_t t,uint16_t rate)
{
    float st  = (float)t/rate;
    return p1+(st*(p2-p1));
}


/***********************API used to SET the ICON******************************************/
/*Refer the code flow in the flowchart availble in the Application Note */

int32_t App_AudioPlay(S_LiftAppCtxt_t *pCtxt,int32_t DstAddr,int32_t BuffChunkSz)
{
    /* awitch according to the current audio state */         
    //Serial.println(pCtxt->AudioState,DEC);
    switch(pCtxt->AudioState)
    {
    case LIFTAPPAUDIOSTATE_INIT:
        {
            /* Initialize all the parameters and start downloading */
            pCtxt->AudioFileIdx = 0;
            pCtxt->AudioCurrAddr = DstAddr;
            pCtxt->AudioCurrFileSz = 0;
            pCtxt->AudioState = LIFTAPPAUDIOSTATE_DOWNLOAD; 
            Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,0);
            Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);
            Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);    
            
            break;
        }
    case LIFTAPPAUDIOSTATE_DOWNLOAD:
        {
            uint8_t pbuff[512];//temp buffer for sd read data
            uint16_t blocklen;
            int16_t i,NumSectors;
            
            /* Initialize all the parameters and start downloading */
            NumSectors = BuffChunkSz/LIFTAPPCHUNKSIZE*1L;//hardcoding wrt sdcard sector size
            /* After downloading all the files into graphics RAM change the state to playback */  
            
            if((pCtxt->AudioFileIdx >= LIFTAPPMAXFILENAMEARRAY) || (0 == strlen(&pCtxt->AudioFileNames[pCtxt->AudioFileIdx])))
            {
                pCtxt->AudioState = LIFTAPPAUDIOSTATE_PLAYBACK;
            }            
            else if(pCtxt->AudioCurrFileSz <= 0)
            {
                /* open a new file and start downloading data into the current audio buffer */
                if(0 == strlen(&pCtxt->AudioFileNames[pCtxt->AudioFileIdx]))
                {
                    pCtxt->AudioState = LIFTAPPAUDIOSTATE_PLAYBACK;
                }
                else
                {
                    
                    if(0 == imageFile.openfile(&pCtxt->AudioFileNames[pCtxt->AudioFileIdx]))
                    {
                        /* file open failure */
                        pCtxt->AudioFileIdx += strlen(&pCtxt->AudioFileNames[pCtxt->AudioFileIdx]);
                        pCtxt->AudioFileIdx++;
                        break;
                    }          
                    /* file open success */
                    
                    imageFile.readsector(pbuff);
                    pCtxt->AudioCurrFileSz = (int32_t)((pbuff[45]<<24) | (pbuff[44]<<16) | (pbuff[43]<<8) | (pbuff[42]));
                    blocklen = pCtxt->AudioCurrFileSz>512?512:pCtxt->AudioCurrFileSz;
                    Gpu_Hal_WrMem(phost,pCtxt->AudioCurrAddr,&pbuff[46],blocklen - 46);                
                    pCtxt->AudioCurrAddr += (blocklen - 46);
                    pCtxt->AudioCurrFileSz -= (blocklen - 46);
                }
            }
            else
            {
                /* Download data into audio buffer */
                for(i=0;i<NumSectors;i++)
                {
                    imageFile.readsector(pbuff);
                    //Serial.println(FileSaveLen,DEC);
                    blocklen = pCtxt->AudioCurrFileSz>512?512:pCtxt->AudioCurrFileSz;
                    Gpu_Hal_WrMem(phost,pCtxt->AudioCurrAddr,pbuff,blocklen);
                    pCtxt->AudioCurrAddr += (blocklen);
                    pCtxt->AudioCurrFileSz -= (blocklen);
                    
                    if(pCtxt->AudioCurrFileSz <= 0)
                    {
                        pCtxt->AudioFileIdx += strlen(&pCtxt->AudioFileNames[pCtxt->AudioFileIdx]);
                        pCtxt->AudioFileIdx++;//increment the index to open a new file
                        break;
                    }
                }
            }
            break;
        }
    case LIFTAPPAUDIOSTATE_PLAYBACK:
        {
            /* Downloading is finished and start the playback */
            Gpu_Hal_Wr32(phost, REG_PLAYBACK_START,DstAddr);//Audio playback start address 
            //Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,totalbufflen);//Length of raw data buffer in bytes    
            Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,(pCtxt->AudioCurrAddr - DstAddr));
            Gpu_Hal_Wr16(phost, REG_PLAYBACK_FREQ,11025);//Frequency
            Gpu_Hal_Wr8(phost, REG_PLAYBACK_FORMAT,ULAW_SAMPLES);//Current sampling frequency
            Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);
            Gpu_Hal_Wr8(phost, REG_VOL_PB,255);       
            Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);    

            pCtxt->AudioState = LIFTAPPAUDIOSTATE_STOP;

            break;
        }
    case LIFTAPPAUDIOSTATE_STOP:
        {
            int32_t rdptr;
            /* Stop the audio playback in case of one shot */
            
            /* Check wether the playback is finished - audio engine */
            rdptr = Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) - DstAddr;
            if((pCtxt->AudioCurrAddr <= rdptr) || (0 == rdptr))
            {
                
                /* Reset the respective parameters before entering none state */
                pCtxt->AudioFileIdx = 0;
                pCtxt->AudioState = LIFTAPPAUDIOSTATE_NONE;
                pCtxt->AudioFileNames[0] = '\0';
                pCtxt->AudioFileNames[1] = '\0';
            }      
            break;
        }
    case LIFTAPPAUDIOSTATE_NONE:
    default:
        {
            /* Nothing done in this state */
            break;
        }
        
    }      
    return 0;
}


#if 0
/* API to play the music files */
uint32_t play_music(Reader &r /*char8_t *pFileName*/,uint32_t dstaddr,uint8_t Option,uint32_t Buffersz)
{

    uint8_t pbuff[512];
    // open the audio file from the SD card
    //Serial.println(pFileName);    
#ifdef MSVC_PLATFORM        
    FILE *pFile = NULL;
#endif 

    //uint8_t music_playing = 0;
    int32_t filesz = 0,chunksize = 512,totalbufflen = 64*1024,currreadlen = 0;
    //uint8_t *pBuff = NULL;
    uint32_t wrptr = dstaddr,return_val = 0;
    uint32_t rdptr,freebuffspace;


    if(2 == Option)
    {
        Gpu_Hal_Wr32(phost, REG_PLAYBACK_START,dstaddr);//Audio playback start address 
        //Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,totalbufflen);//Length of raw data buffer in bytes    
        Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,Buffersz);
        Gpu_Hal_Wr16(phost, REG_PLAYBACK_FREQ,11025);//Frequency
        Gpu_Hal_Wr8(phost, REG_PLAYBACK_FORMAT,ULAW_SAMPLES);//Current sampling frequency
        Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);
        Gpu_Hal_Wr8(phost, REG_VOL_PB,255);       
        Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);    
        return 0;
    }
    if(0 == music_playing)
    {
#ifdef MSVC_PLATFORM
        pFile = fopen(pFileName,"rb+");
        fseek(pFile,0,SEEK_END);
        filesz = ftell(pFile);

        fseek(pFile,42,SEEK_SET);
        filesz = 0;
        fread(&filesz,1,4,pFile);
        fileszsave = filesz;
#endif

#ifdef ARDUINO_PLATFORM

        filesz = r.size;
        fileszsave = filesz;
        
#endif        
        while(filesz > 0)
        {
            currreadlen = filesz;

            if(currreadlen > 512)
            {
                currreadlen = 512;
            }
#ifdef MSVC_PLATFORM
            fread(pBuff,1,currreadlen,pFile);
#endif

            r.readsector(pbuff);
            
            //Gpu_Hal_WrMemFromFlash(phost, wrptr, (uint8_t *)pBuff,currreadlen);
            Gpu_Hal_WrMem(phost, wrptr, pbuff,currreadlen);
            wrptr +=  currreadlen;
            //Serial.println(wrptr,DEC);
            if(wrptr > (dstaddr + totalbufflen))
            {
                wrptr = dstaddr;
                
            }
            
            filesz -= currreadlen;   

        }

        if((0 == music_playing) && (0 == Option))
        {
            
            Gpu_Hal_Wr32(phost, REG_PLAYBACK_START,dstaddr);//Audio playback start address 
            Gpu_Hal_Wr32(phost, REG_PLAYBACK_LENGTH,fileszsave);
            Gpu_Hal_Wr16(phost, REG_PLAYBACK_FREQ,11025);//Frequency
            Gpu_Hal_Wr8(phost, REG_PLAYBACK_FORMAT,ULAW_SAMPLES);//Current sampling frequency
            Gpu_Hal_Wr8(phost, REG_PLAYBACK_LOOP,0);
            Gpu_Hal_Wr8(phost, REG_VOL_PB,255);       
            Gpu_Hal_Wr8(phost, REG_PLAYBACK_PLAY,1);
            music_playing = 1;
            
        }

        if(filesz == 0)
        {
            dstaddr = dstaddr + fileszsave;
            music_playing = 0;
            return dstaddr;
        }
    }
    else if(0 == Option)
    {
        rdptr = Gpu_Hal_Rd32(phost,REG_PLAYBACK_READPTR) - dstaddr;
        if((fileszsave <= rdptr) || (0 == rdptr))
        {
            music_playing = 0;
        }
        
    }

    /* return the current write pointer in any case */
    return wrptr;
}
#endif



void font_display(int16_t BMoffsetx,int16_t BMoffsety,uint32_t stringlenghth,char8_t *string_display,uint8_t opt_landscape)
{
    uint16_t k;
    
    if(0 == opt_landscape)
    {
        for(k=0;k<stringlenghth;k++)
        {
            App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety,8,string_display[k]));
            BMoffsety -= pgm_read_byte(&g_Gpu_Fonts[0].FontWidth[string_display[k]]);
            
        }
    }
    else if(1 == opt_landscape)
    {
        for(k=0;k<stringlenghth;k++)
        {
            App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety,28,string_display[k]));
            BMoffsetx += pgm_read_byte(&g_Gpu_Fonts[0].FontWidth[string_display[k]]);
        }

    }
}



/* Lift demo application demonstrates background animation and foreground digits and arrow direction */
void LiftApp_Landscape()
{
    /* Download the raw data of custom fonts into memory */
    int16_t BMoffsetx,BMoffsety,BHt = 156,BWd = 80,BSt = 80,Bfmt = L8,BArrowHt = 85,BArrowWd = 80,BArrowSt = 80,BArrowfmt = L8;
    int16_t NumBallsRange = 6, NumBallsEach = 10,RandomVal = 16;
    int32_t Baddr2,i,SzInc = 16,SzFlag = 0;
    uint8_t fontr = 255,fontg = 255,fontb = 255;
    S_LiftAppRate_t *pLARate;
    S_LiftAppTrasParams_t *pLATParams;
    S_LiftAppBallsLinear_t S_LiftBallsArray[6*10],*pLiftBalls = NULL;//as of now 80 balls are been plotted
    float temptransx,temptransy,running_rate;
    S_LiftAppCtxt_t S_LACtxt;

    /* Initial setup code to setup all the required bitmap handles globally */
    App_Set_DlBuffer_Index(0);

    
    /* load the bitmap raw data */


    pLARate = &S_LACtxt.SLARate;
    pLATParams = &S_LACtxt.SLATransPrms;
    uint32_t time =0;
    time = millis();

    //initialize all the rate parameters
    pLARate->CurrTime = 0;
    pLARate->IttrCount = 0;

    /* Initialize all the transition parameters - all the below are in terms of basic units of rate
either they can be based on itterations or based on time giffies */
    pLATParams->FloorNumChangeRate = 64;
    pLATParams->MaxFloorNum = 7;
    pLATParams->MinFloorNum = -2;
    pLATParams->ResizeRate = 32;
    pLATParams->ResizeDimMax = 2*256;
    pLATParams->ResizeDimMin = 1*256;
    pLATParams->FloorNumStagnantRate = 128;

    /* Initialization of lift context parameters */
    S_LACtxt.ArrowDir = LIFTAPP_DIR_DOWN;//going down direction
    S_LACtxt.CurrFloorNum = 5;//current floor number to start with
    S_LACtxt.NextFloorNum = -2;//destination floor number as of now
    S_LACtxt.CurrArrowResizeRate = pLATParams->ResizeRate;
    S_LACtxt.CurrFloorNumResizeRate = pLATParams->ResizeRate;
    S_LACtxt.CurrFloorNumStagnantRate = 0;
    S_LACtxt.CurrFloorNumChangeRate = S_LACtxt.SLATransPrms.FloorNumChangeRate;
    S_LACtxt.AudioState = 0;
    S_LACtxt.AudioPlayFlag = 0;
    S_LACtxt.opt_orientation = 1;

    /* Display a spinner while loading bitmaps */
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
	//App_WrCoCmd_Buffer(phost,DISPLAY());
	//Gpu_CoCmd_Swap(phost);
	App_Flush_Co_Buffer(phost);
	Gpu_Hal_WaitCmdfifo_empty(phost);

	Gpu_CoCmd_Dlstart(phost);        // start
    delay(1000);
    App_WrDl_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
    App_WrDl_Buffer(phost, CLEAR(1, 1, 1)); // clear screen

    /* Load bitmaps into RAM */
    App_WrDl_Buffer(phost,BEGIN(BITMAPS));
    Baddr2 = RAM_G + (20 *1024);
    if(1 == imageFile.openfile("font.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"font.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(0));//handle 0 is used for all the characters
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(Bfmt, BSt, BHt));
    App_WrDl_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BWd*2, BHt*2));

    Baddr2 = ((Baddr2 + (int32_t)BSt*BHt*11 + 15)&~15);
    if(1 == imageFile.openfile("arr.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"arr.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(1));//bitmap handle 1 is used for arrow
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(BArrowfmt, BArrowSt, BArrowHt));
    App_WrDl_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BArrowWd*2, BArrowHt*2));//make sure the bitmap is displayed when rotation happens

    Baddr2 = Baddr2 + 80*85;
    if(1 == imageFile.openfile("bs6.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs6.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(2));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 60, 60));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 60, 55));

    Baddr2 += 60*55;
    if(1 == imageFile.openfile("bs5.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs5.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(3));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 50, 46));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 50, 46));

    Baddr2 += 50*46;
    if(1 == imageFile.openfile("bs4.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs4.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(4));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 40, 37));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 40, 37));

    Baddr2 += 40*37;
    if(1 == imageFile.openfile("bs3.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs3.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(5));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 30, 27));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 30, 27));

    Baddr2 += 30*27;
    if(1 == imageFile.openfile("bs2.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs2.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(6));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 20, 18));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 20, 18));

    Baddr2 += 20*18;
    if(1 == imageFile.openfile("bs1.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs1.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(7));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 10, 10));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 10, 10));
    App_WrDl_Buffer(phost,VERTEX2II(0,0,7,0));

    Baddr2 += 10*10;
    if(1 == imageFile.openfile("logo.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"logo.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(11));//handle 11 is used for logo
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(ARGB4, 99*2, 30));
    App_WrDl_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 99, 30));//make sure whole bitmap is displayed after rotation - as height is greater than width
    Baddr2 += 99*2*30;


    App_Set_CmdBuffer_Index(0);

    /* Download the commands into fifo */ 
    App_Flush_Co_Buffer(phost);

    /* Wait till coprocessor completes the operation */
    Gpu_Hal_WaitCmdfifo_empty(phost);

    Baddr2 += 10*10;
    Baddr2 =  ((Baddr2 + 7)&~7);//audio engine's requirement


    
    App_WrDl_Buffer(phost, DISPLAY());
    /* Download the DL into DL RAM */
    App_Flush_DL_Buffer(phost);

    /* Do a swap */
    App_GPU_DLSwap(DLSWAP_FRAME);
    delay(30);

    /* compute the random values at the starting */
    pLiftBalls = S_LiftBallsArray;
    for(i=0;i<(NumBallsRange*NumBallsEach);i++)
    {
        pLiftBalls->xOffset = random(DispWidth*16);
        pLiftBalls->yOffset = random(DispHeight*16);
        pLiftBalls->dx = random(RandomVal*8) - RandomVal*4;
        pLiftBalls->dy = -1*random(RandomVal*8);
        pLiftBalls++;
    }

    while(1)
    {   
        //loop_start = millis();
        /* Logic of user touch - change background or skin wrt use touch */
        /* Logic of transition - change of floor numbers and direction */
        LiftApp_FTransition(&S_LACtxt,Baddr2);    

        Gpu_CoCmd_Dlstart(phost);
        App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
        App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
        App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
        /* Background gradient */
        Gpu_CoCmd_Gradient(phost, 0,0,0x66B4E8,0,DispHeight,0x132B3B);

        /* Draw background bitmaps */
        App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
        pLiftBalls = S_LiftBallsArray;
        App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
        
        App_WrCoCmd_Buffer(phost,COLOR_A(64));
        App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
        for(i=0;i<(NumBallsRange*NumBallsEach);i++)
        {
            /* handle inst insertion - check for the index */
            if(0 == (i%NumBallsEach))
            {
                App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(2 + (i/NumBallsEach)));
            }
            /* recalculate the background balls offset and respective rate when ball moves out of the diaply area */
            if( (pLiftBalls->xOffset > ((DispWidth + 60)*16)) || 
                    (pLiftBalls->yOffset > ((DispHeight + 60) *16)) ||
                    (pLiftBalls->xOffset < (-60*16)) || 
                    (pLiftBalls->yOffset < (-60*16)) )
            {
                /* always offset starts from below the screen and moves upwards */
                pLiftBalls->xOffset = random(DispWidth*16);
                pLiftBalls->yOffset = DispHeight*16 + random(60*16);

                pLiftBalls->dx = random(RandomVal*8) - RandomVal*4;
                pLiftBalls->dy = -1*random(RandomVal*8);
            }
            App_WrCoCmd_Buffer(phost,VERTEX2F(pLiftBalls->xOffset, pLiftBalls->yOffset));
            pLiftBalls->xOffset += pLiftBalls->dx;
            pLiftBalls->yOffset += pLiftBalls->dy;
            pLiftBalls++;
        }
        App_WrCoCmd_Buffer(phost,END());
        App_WrCoCmd_Buffer(phost,ALPHA_FUNC(ALWAYS,0));
        App_WrCoCmd_Buffer(phost,COLOR_A(255));

        if(LIFTAPP_DIR_NONE != S_LACtxt.ArrowDir)//do not display the arrow in case of no direction, meaning stagnant
        {

            //calculation of size value based on the rate
            //the bitmaps are scaled from original resolution till 2 times the resolution on both x and y axis
            SzInc = 16 + (S_LACtxt.SLATransPrms.ResizeRate/2 - abs(S_LACtxt.CurrArrowResizeRate%S_LACtxt.SLATransPrms.ResizeRate - S_LACtxt.SLATransPrms.ResizeRate/2));
            App_WrCoCmd_Buffer(phost, COLOR_RGB(fontr, fontg, fontb));
            /* Draw the arrow first */
            App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1)); 
            App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
            //BMoffsetx = ((DispWidth/4) - (BArrowWd*SzInc/32));
            BMoffsetx = (BArrowWd - (BArrowWd*SzInc/32));
            BMoffsety = ((DispHeight/2) - (BArrowHt*SzInc/32));

            /* perform inplace flip and scale of bitmap in case of direction is up */
            if(LIFTAPP_DIR_UP == S_LACtxt.ArrowDir)
            {
                App_GraphicsMatrix_t S_GPUMatrix;
                App_TrsMtrxLoadIdentity(&S_GPUMatrix);
                App_TrsMtrxTranslate(&S_GPUMatrix,80/2.0,85/2.0,&temptransx,&temptransy);
                App_TrsMtrxScale(&S_GPUMatrix,(SzInc/16.0),(SzInc/16.0));
                App_TrsMtrxFlip(&S_GPUMatrix,APP_GRAPHICS_FLIP_BOTTOM);   
                App_TrsMtrxTranslate(&S_GPUMatrix,(-80*SzInc)/32.0,(-85*SzInc)/32.0,&temptransx,&temptransy);
                
                //App_TrsMtrxTranslate(&S_GPUMatrix,-80/2.0,-85/2.0,&temptransx,&temptransy);
                {
                    App_PostProcess_Transform_t S_GPUTrasMatrix;
                    App_UpdateTrsMtrx(&S_GPUMatrix,&S_GPUTrasMatrix);
                    App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(S_GPUTrasMatrix.Transforma));
                    App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(S_GPUTrasMatrix.Transformb));
                    App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(S_GPUTrasMatrix.Transformc));
                    App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(S_GPUTrasMatrix.Transformd));
                    App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(S_GPUTrasMatrix.Transforme));
                    App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(S_GPUTrasMatrix.Transformf));
                }
            }
            else if(LIFTAPP_DIR_DOWN == S_LACtxt.ArrowDir)
            {
                //perform only scaling as rotation is not required
                App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_A(256*16/SzInc));
                App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_E(256*16/SzInc));      
            }
            LiftAppComputeBitmap(G_LiftAppFontArrayArrow,-1,1,SzInc*16,BArrowWd,(DispHeight/2));    
            Gpu_CoCmd_LoadIdentity(phost);
            Gpu_CoCmd_SetMatrix(phost);
        }
        /* Draw the font bitmaps */
        /* algorithm for animation - floor numbers will change in dimensions when stagnant at a perticular floor */
        /* arrow will change in case of movement of lift */
        /* display the bitmap with increased size */
        SzInc = 16 + (S_LACtxt.SLATransPrms.ResizeRate/2 - abs(S_LACtxt.CurrFloorNumResizeRate%S_LACtxt.SLATransPrms.ResizeRate - S_LACtxt.SLATransPrms.ResizeRate/2));
        //BMoffsetx = ((DispWidth*3/4) - (BWd/2));
        //BMoffsetx = ((DispWidth*3/4) - (BWd*SzInc/32));
        BMoffsetx = DispWidth - BWd*2 - (BWd*SzInc/32);
        
        //BMoffsety = ((DispHeight/2) - (BHt/2));
        BMoffsety = ((DispHeight/2) - (BHt*SzInc/32));
        /* calculate the resolution change based on the number of characters used as well as */
        App_WrCoCmd_Buffer(phost, COLOR_RGB(fontr, fontg, fontb));
        App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_A(256*16/SzInc));
        App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_E(256*16/SzInc));
        App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0)); 
        App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
        LiftAppComputeBitmap(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,DispWidth - BWd*2,(DispHeight/2));

        App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_A(256));
        App_WrCoCmd_Buffer(phost, BITMAP_TRANSFORM_E(256));


        
        BMoffsety = 240;
        BMoffsetx = 0;
        
        App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS)); 
        int16_t t;
        {
            char8_t text_run_disp[] = "The running text is displayed here.";
            int16_t stringlen_text_run_disp;
            stringlen_text_run_disp = strlen(text_run_disp);
            if(t < 300)
            t++;
            BMoffsetx = linear(480,0,t,300);
            //Gpu_CoCmd_Text(phost,BMoffsetx,BMoffsety,28,OPT_CENTERX|OPT_CENTERY,"The running test is displayed here.");
            font_display(BMoffsetx,BMoffsety,stringlen_text_run_disp,text_run_disp, 1);
            //t++;
        }
        
        BMoffsety = 220;
        BMoffsetx = 0;  
        
        uint32_t disp =0,hr,minutes,sec ;
        uint32_t temp = millis()-time;
        hr = (temp/3600000L)%12;
        minutes = (temp/60000L)%60;
        sec = (temp/1000L)%60;
        
        
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety,28,(hr/10)+'0'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 10,BMoffsety,28,(hr%10)+'0'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 22,BMoffsety,28,':'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 30,BMoffsety,28,(minutes/10)+'0'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 40,BMoffsety,28,(minutes%10)+'0'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 52,BMoffsety,28,':'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx+ 60,BMoffsety,28,(sec/10)+'0'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx + 70,BMoffsety,28,(sec%10)+'0'));

        App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(11));
        App_WrCoCmd_Buffer(phost,VERTEX2II(10,10,11,0));


        App_WrCoCmd_Buffer(phost, DISPLAY() );
        Gpu_CoCmd_Swap(phost);
        /* Download the commands into fifo */
        App_Flush_Co_Buffer(phost);
        t++;
        /* Wait till coprocessor completes the operation
    TBD - for maximum throughput we can check for only the leftout space in fifo and cotinue constructing the display lists */
        Gpu_Hal_WaitCmdfifo_empty(phost);
        
        //rate count increment logic
        pLARate->IttrCount++;
        
    }

}



/*  API to construct the display list of the lift application*/
void LiftApp_Portrait()
{
    /* Download the raw data of custom fonts into memory */
    int16_t BMoffsetx,BMoffsety,BHt = 156,BWd = 80,BSt = 80,Bfmt = L8,BArrowHt = 85,BArrowWd = 80,BArrowSt = 80,BArrowfmt = L8;
    int16_t NumBallsRange = 6, NumBallsEach = 10,RandomVal = 16;
    int32_t Baddr2,i,SzInc = 16,SzFlag = 0;
    uint8_t fontr = 255,fontg = 255,fontb = 255;
    S_LiftAppRate_t *pLARate;
    S_LiftAppTrasParams_t *pLATParams;
    S_LiftAppBallsLinear_t S_LiftBallsArray[6*10],*pLiftBalls = NULL;//as of now 80 stars are been plotted
    App_GraphicsMatrix_t S_GPUMatrix;
    //App_PostProcess_Transform_t S_GPUTrasMatrix;
    float temptransx,temptransy,running_rate;
    S_LiftAppCtxt_t S_LACtxt;



    /* load the bitmap raw data */

    /* Initial setup code to setup all the required bitmap handles globally */
    App_Set_DlBuffer_Index(0);
    pLARate = &S_LACtxt.SLARate;
    pLATParams = &S_LACtxt.SLATransPrms;
    
    //initialize all the rate parameters
    pLARate->CurrTime = 0;
    pLARate->IttrCount = 0;

    /* Initialize all the transition parameters - all the below are in terms of basic units of rate
either they can be based on itterations or based on time giffies */
    pLATParams->FloorNumChangeRate = 64;
    pLATParams->MaxFloorNum = 90;
    pLATParams->MinFloorNum = 70;
    pLATParams->ResizeRate = 32;
    pLATParams->ResizeDimMax = 2*256;
    pLATParams->ResizeDimMin = 1*256;
    pLATParams->FloorNumStagnantRate = 128;

    /* Initialization of lift context parameters */
    S_LACtxt.ArrowDir = LIFTAPP_DIR_DOWN;//going down direction
    //S_LACtxt.CurrFloorNum = 87;//current floor number to start with
    S_LACtxt.CurrFloorNum = 80;//current floor number to start with
    //S_LACtxt.NextFloorNum = 80;//destination floor number as of now
    S_LACtxt.NextFloorNum = 77;//destination floor number as of now
    S_LACtxt.CurrArrowResizeRate = pLATParams->ResizeRate;
    S_LACtxt.CurrFloorNumResizeRate = pLATParams->ResizeRate;
    S_LACtxt.CurrFloorNumStagnantRate = 0;
    S_LACtxt.CurrFloorNumChangeRate = S_LACtxt.SLATransPrms.FloorNumChangeRate;
    S_LACtxt.AudioState = LIFTAPPAUDIOSTATE_NONE;
    S_LACtxt.AudioFileIdx = 0;
    S_LACtxt.AudioCurrAddr = 0;
    S_LACtxt.AudioCurrFileSz = 0;
    S_LACtxt.AudioFileNames[0] = '\0';
    S_LACtxt.AudioPlayFlag = 0;
    S_LACtxt.opt_orientation = 0;
    
    /* Display a spinner while loading bitmaps */
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
	//App_WrCoCmd_Buffer(phost,DISPLAY());
	//Gpu_CoCmd_Swap(phost);
	App_Flush_Co_Buffer(phost);
	Gpu_Hal_WaitCmdfifo_empty(phost);

	Gpu_CoCmd_Dlstart(phost);        // start
    delay(1000);
    App_WrDl_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
    App_WrDl_Buffer(phost, CLEAR(1, 1, 1)); // clear screen

    /* Load bitmaps into RAM */
    
    Baddr2 = RAM_G + (20 *1024);
    //        Gpu_Hal_ResetDLBuffer(phost);
    if(1 == imageFile.openfile("font.raw"))
    //Gpu_CoCmd_Text(phost,DispWidth/2,DispHeight/2,28,OPT_CENTERX|OPT_CENTERY,"Font is not displayed");
    Gpu_Hal_LoadImageToMemory(phost,"font.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(0));//handle 0 is used for all the characters
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(Bfmt, BSt, BHt));
    App_WrDl_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BHt*2, BHt*2));//make sure whole bitmap is displayed after rotation - as height is greater than width


    Baddr2 = ((Baddr2 + (int32_t)BSt*BHt*11 + 15)&~15);
    if(1 == imageFile.openfile("arr.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"arr.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(1));//bitmap handle 1 is used for arrow
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(BArrowfmt, BArrowSt, BArrowHt));
    App_WrDl_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, BArrowHt*2, BArrowHt*2));//make sure whole bitmap is displayed after rotation - as height is greater than width

    Baddr2 = Baddr2 + 80*85;
    if(1 == imageFile.openfile("bs6.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs6.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(2));//bitmap handle 2 is used for background stars
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 60, 55));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 60, 55));

    Baddr2 += 60*55;
    if(1 == imageFile.openfile("bs5.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs5.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(3));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 50, 46));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 50, 46));

    Baddr2 += 50*46;
    if(1 == imageFile.openfile("bs4.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs4.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(4));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8,40,37));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER,40,37));

    Baddr2 += 40*37;
    if(1 == imageFile.openfile("bs3.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs3.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(5));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 30, 27));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 30, 27));

    Baddr2 += 30*27;
    if(1 == imageFile.openfile("bs2.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs2.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(6));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 20, 18));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 20, 18));

    Baddr2 += 20*18;
    if(1 == imageFile.openfile("bs1.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"bs1.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(7));//bitmap handle 2 is used for background balls
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L8, 10, 10));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 10, 10));

    Baddr2 += 10*10;
    if(1 == imageFile.openfile("logo.raw"))
    Gpu_Hal_LoadImageToMemory(phost,"logo.raw",Baddr2,LOAD);
    App_WrDl_Buffer(phost,BITMAP_HANDLE(11));//handle 11 is used for logo
    App_WrDl_Buffer(phost,BITMAP_SOURCE(Baddr2));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(ARGB4, 99*2, 30));
    App_WrDl_Buffer(phost,BITMAP_SIZE(BILINEAR, BORDER, BORDER, 99*2, 99*2));//make sure whole bitmap is displayed after rotation - as height is greater than width
    Baddr2 += 99*2*30;

    // Read the font address from 0xFFFFC location 
    //FontTableAddress = Gpu_Hal_Rd32(phost,0xFFFFC);

    App_Set_CmdBuffer_Index(0);

    /* Download the commands into fifo */ 
    //App_Flush_Co_Buffer(phost);

    /* Wait till coprocessor completes the operation */
    //Gpu_Hal_WaitCmdfifo_empty(phost);

    App_WrDl_Buffer(phost, BITMAP_HANDLE(8));
    App_WrDl_Buffer(phost,BITMAP_LAYOUT(L4,9,25));
    App_WrDl_Buffer(phost,BITMAP_SOURCE(950172));
    App_WrDl_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 25, 25));
    Baddr2 += 10*10;
    Baddr2 =  (int32_t)((Baddr2 + 7)&~7);//audio engine's requirement

    uint32_t time =0;
    time = millis();

    App_WrDl_Buffer(phost, DISPLAY());
    /* Download the DL into DL RAM */
    App_Flush_DL_Buffer(phost);

    /* Do a swap */
    App_GPU_DLSwap(DLSWAP_FRAME);
    Gpu_Hal_Sleep(30);

    /* compute the random values at the starting */
    pLiftBalls = S_LiftBallsArray;
    for(i=0;i<(NumBallsRange*NumBallsEach);i++)
    {
        //always start from the right and move towards left
        pLiftBalls->xOffset = random(DispWidth*16);
        pLiftBalls->yOffset = random(DispHeight*16);
        pLiftBalls->dx = -1*random(RandomVal*8);//always -ve
        pLiftBalls->dy = random(RandomVal*8) - RandomVal*4;
        pLiftBalls++;
    }
    // Serial.begin(9600);
    // while(1);
    while(1)
    {   
        /* Logic of user touch - change background or skin wrt use touch */
        /* Logic of transition - change of floor numbers and direction */
        LiftApp_FTransition(&S_LACtxt,Baddr2);    
        Gpu_CoCmd_Dlstart(phost);
        App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
        App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1)); // clear screen
        App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
        App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());
        /* Background gradient */
        Gpu_CoCmd_Gradient(phost, 0,0,0x66B4E8,480,0,0x132B3B);


        /* Draw background bitmaps */
        App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
        pLiftBalls = S_LiftBallsArray;
        App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
        
        App_WrCoCmd_Buffer(phost,COLOR_A(64));
        App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
        for(i=0;i<(NumBallsRange*NumBallsEach);i++)
        {     
            if(0 == i%NumBallsEach)
            {       
                App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(2 + (i/NumBallsEach)));
            }
            if( ( (pLiftBalls->xOffset > ((DispWidth + 60)*16)) || (pLiftBalls->yOffset > ((DispHeight + 60) *16)) ) ||
                    ( (pLiftBalls->xOffset < (-60*16)) || (pLiftBalls->yOffset < (-60*16)) ))
            {
                pLiftBalls->xOffset = DispWidth*16 + random(80*16);
                pLiftBalls->yOffset = random(DispHeight*16);
                pLiftBalls->dx = -1*random(RandomVal*8);
                pLiftBalls->dy = random(RandomVal*8) - RandomVal*4;

            }
            App_WrCoCmd_Buffer(phost,VERTEX2F(pLiftBalls->xOffset, pLiftBalls->yOffset));
            pLiftBalls->xOffset += pLiftBalls->dx;
            pLiftBalls->yOffset += pLiftBalls->dy;
            pLiftBalls++;
        }
        App_WrCoCmd_Buffer(phost,END());
        App_WrCoCmd_Buffer(phost,ALPHA_FUNC(ALWAYS,0));
        App_WrCoCmd_Buffer(phost,COLOR_A(255));

        if(LIFTAPP_DIR_NONE != S_LACtxt.ArrowDir)//do not display the arrow in case of no direction, meaning stagnant
        {
            App_GraphicsMatrix_t S_GPUMatrix;
            //calculation of size value based on the rate
            //the bitmaps are scaled from original resolution till 2 times the resolution on both x and y axis
            SzInc = 16 + (S_LACtxt.SLATransPrms.ResizeRate/2 - abs(S_LACtxt.CurrArrowResizeRate%S_LACtxt.SLATransPrms.ResizeRate - S_LACtxt.SLATransPrms.ResizeRate/2));
            App_WrCoCmd_Buffer(phost, COLOR_RGB(fontr, fontg, fontb));
            /* Draw the arrow first */
            App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(1)); 
            App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
            //BMoffsetx = ((DispWidth/4) - (BArrowWd*SzInc/32));
            BMoffsetx = (BArrowWd - (BArrowWd*SzInc/32));
            BMoffsety = ((DispHeight/2) - (BArrowHt*SzInc/32));

            /* perform inplace flip and scale of bitmap in case of direction is up */
            App_TrsMtrxLoadIdentity(&S_GPUMatrix);
            App_TrsMtrxTranslate(&S_GPUMatrix,80/2.0,85/2.0,&temptransx,&temptransy);
            App_TrsMtrxScale(&S_GPUMatrix,(SzInc/16.0),(SzInc/16.0));
            App_TrsMtrxRotate(&S_GPUMatrix,90);
            if(LIFTAPP_DIR_UP == S_LACtxt.ArrowDir)
            {
                App_TrsMtrxFlip(&S_GPUMatrix,APP_GRAPHICS_FLIP_RIGHT);    
            }

            App_TrsMtrxTranslate(&S_GPUMatrix,(-80*SzInc)/32.0,(-85*SzInc)/32.0,&temptransx,&temptransy);
            {
                App_PostProcess_Transform_t S_GPUTrasMatrix;
                App_UpdateTrsMtrx(&S_GPUMatrix,&S_GPUTrasMatrix);
                App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(S_GPUTrasMatrix.Transforma));
                App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(S_GPUTrasMatrix.Transformb));
                App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(S_GPUTrasMatrix.Transformc));
                App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(S_GPUTrasMatrix.Transformd));
                App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(S_GPUTrasMatrix.Transforme));
                App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(S_GPUTrasMatrix.Transformf));
            }
            LiftAppComputeBitmap_Single(G_LiftAppFontArrayArrow,0,SzInc*16,BArrowWd,(DispHeight/2));
            Gpu_CoCmd_LoadIdentity(phost);
            Gpu_CoCmd_SetMatrix(phost);
        }

        
        /* algorithm for animation - floor numbers will change in dimensions when stagnant at a perticular floor */
        /* arrow will change in case of movement of lift */
        /* display the bitmap with increased size */
        SzInc = 16 + (S_LACtxt.SLATransPrms.ResizeRate/2 - abs(S_LACtxt.CurrFloorNumResizeRate%S_LACtxt.SLATransPrms.ResizeRate - S_LACtxt.SLATransPrms.ResizeRate/2));
        BMoffsetx = DispWidth - BWd*2 - (BWd*SzInc/32);
        BMoffsety = ((DispHeight/2) - (BHt*SzInc/32));

        /* calculate the resolution change based on the number of characters used as well as */
        App_WrCoCmd_Buffer(phost, COLOR_RGB(fontr, fontg, fontb));
        /* perform inplace flip and scale of bitmap in case of direction is up */
        App_TrsMtrxLoadIdentity(&S_GPUMatrix);
        App_TrsMtrxTranslate(&S_GPUMatrix,BWd/2.0,BHt/2.0,&temptransx,&temptransy);
        App_TrsMtrxScale(&S_GPUMatrix,(SzInc/16.0),(SzInc/16.0));
        App_TrsMtrxRotate(&S_GPUMatrix,90);
        App_TrsMtrxTranslate(&S_GPUMatrix,(-BHt*SzInc)/32.0,(-BWd*SzInc)/32.0,&temptransx,&temptransy);
        {
            App_PostProcess_Transform_t S_GPUTrasMatrix;
            App_UpdateTrsMtrx(&S_GPUMatrix,&S_GPUTrasMatrix);
            App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(S_GPUTrasMatrix.Transforma));
            App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(S_GPUTrasMatrix.Transformb));
            App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(S_GPUTrasMatrix.Transformc));
            App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(S_GPUTrasMatrix.Transformd));
            App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(S_GPUTrasMatrix.Transforme));
            App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(S_GPUTrasMatrix.Transformf));
        }
        App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(0)); 
        App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS)); // start drawing bitmaps
        LiftAppComputeBitmapRowRotate(G_LiftAppFontArrayNumbers,S_LACtxt.CurrFloorNum,0,SzInc*16,DispWidth - BHt,(DispHeight/2));
        
        

        BMoffsetx = ((DispWidth /2) + 210);
        BMoffsety = ((DispHeight /2) + 130);
        App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
        
        App_WrCoCmd_Buffer(phost, BITMAP_HANDLE(28));
        App_WrCoCmd_Buffer(phost, BITMAP_SIZE(NEAREST,BORDER,BORDER,25,25));

        Gpu_CoCmd_LoadIdentity(phost);
        Gpu_CoCmd_Translate(phost,18*65536/2,25*65536/2);
        Gpu_CoCmd_Rotate(phost,-90*65536/360);
        Gpu_CoCmd_Translate(phost,-25*65536/2,-18*65536/2);
        Gpu_CoCmd_SetMatrix(phost);

        int16_t t;
        {
            char8_t text_run_disp[] = "The running text is displayed here.";
            int16_t stringlen_text_run_disp;
            stringlen_text_run_disp = strlen(text_run_disp);
            if(t < 200)
            t++;
            BMoffsety = linear(0,272,t,200);
            
            font_display(BMoffsetx,BMoffsety,stringlen_text_run_disp,text_run_disp, 0);
        }
        BMoffsetx = ((DispWidth /2) + 190);
        BMoffsety = ((DispHeight /2) + 105);  
        uint32_t disp =0,hr,minutes,sec ;
        uint32_t temp = millis()-time;
        hr = (temp/3600000L)%12;
        minutes = (temp/60000L)%60;
        sec = (temp/1000L)%60;
        
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety,8,(hr/10)+'0'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-10,8,(hr%10)+'0'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-23,8,':'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-30,8,(minutes/10)+'0'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-40,8,(minutes%10)+'0'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-53,8,':'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-60,8,(sec/10)+'0'));
        App_WrCoCmd_Buffer(phost, VERTEX2II(BMoffsetx,BMoffsety-70,8,(sec%10)+'0'));
        /*running_rate = *///linear((BMoffsety + 130),(BMoffsety +250),0,200);
        App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());
        Gpu_CoCmd_LoadIdentity(phost);
        Gpu_CoCmd_Translate(phost,129*65536,50*65536);
        Gpu_CoCmd_Rotate(phost,270*65536/360);
        Gpu_CoCmd_Translate(phost,-80*65536,-50*65536);
        Gpu_CoCmd_SetMatrix(phost);

        App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
        App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(11));
        //App_WrCoCmd_Buffer(phost,VERTEX2F(-100*16,120*16));
        App_WrCoCmd_Buffer(phost,VERTEX2F(-70*16,130*16));
        App_WrCoCmd_Buffer(phost, DISPLAY() );
        Gpu_CoCmd_Swap(phost);
        /* Download the commands into fifo */
        App_Flush_Co_Buffer(phost);
        
        t++;
        /* Wait till coprocessor completes the operation */
        Gpu_Hal_WaitCmdfifo_empty(phost);
        //rate count increment - 
        pLARate->IttrCount++;
    }

}

/* Startup Screen Information*/ 

prog_char8_t *info[] = {  "EVE Lift Application",
    "API to demonstrate lift application,",
    "with BRIDGETEK logo",
    "& date and time display"
}; 


#ifdef MSVC_PLATFORM
/* Main entry point */
int32_t main(int32_t argc,char8_t *argv[])
#endif
#ifdef ARDUINO_PLATFORM
void setup()
#endif
{
    phost = &host;
    /* Init HW Hal */
    App_Common_Init(phost);
    /* Show Logo, do calibration and display welcome screeen */
    App_Common_Start(phost,info);


#ifdef ORIENTATION_PORTRAIT

    LiftApp_Portrait();

#endif

#ifdef ORIENTATION_LANDSCAPE

    LiftApp_Landscape();

#endif	
    //
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
