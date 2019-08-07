/*
Copyright (c) Bridgetek Pte Ltd

THIS SOFTWARE IS PROVIDED BY BRIDGETEK PTE LTD "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
BRIDGETEK PTE LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES LOSS OF USE, DATA, OR PROFITS OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

BRIDGETEK DRIVERS MAY BE USED ONLY IN CONJUNCTION WITH PRODUCTS BASED ON BRIDGETEK PARTS.

BRIDGETEK DRIVERS MAY BE DISTRIBUTED IN ANY FORM AS LONG AS LICENSE INFORMATION IS NOT MODIFIED.

IF A CUSTOM VENDOR ID AND/OR PRODUCT ID OR DESCRIPTION STRING ARE USED, IT IS THE
RESPONSIBILITY OF THE PRODUCT MANUFACTURER TO MAINTAIN ANY CHANGES AND SUBSEQUENT WHQL
RE-CERTIFICATION AS A RESULT OF MAKING THESE CHANGES.

Abstract:

This file contains is functions for all UI fields.

Author : BRIDGETEK 

Revision History: 
0.1 - date 2013.04.24 - initial version
0.2 - date 2014.04.28 - Split in individual files according to platform
1.0 - date 2014.11.24 - Addition of FT81x
1.1 - date 2015.04.15 - Introduction of module specific configurations
*/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

// yes, we are now compiling for ESP32, but we are still using the Arduino platform
#define ARDUINO_PLATFORM

/////////////////////////////////////////////////////////////

//module selection (CHOOSE ONE)

//#define CFAF800480E0_050SC_A1_2
//#define CFAF240400C0_030SC_A1_2

/////////////////////////////////////////////////////////////

//check
#if (!defined(CFAF800480E0_050SC_A1_2) && !defined(CFAF240400C0_030SC_A1_2))
//if you get a build error here, one of the module types above has not been un-commented
#error TARGET MODULE TYPE NOT SELECTED!
#endif

/////////////////////////////////////////////////////////////
//#define FT80X_ENABLE                           (1)

#ifdef CFAF800480E0_050SC_A1_2
//HAL Configs for Crystalfontz CFAF800480E0-050SC-A1
#define DISPLAY_RESOLUTION_WVGA                 (1)
#define FT81X_ENABLE                            (1)
#define ENABLE_SPI_SINGLE                       (1)
#define SDCARD_CS                               (15)
#define FT800_INT                               (4)
#define FT800_PD_N                              (0)
#define FT800_CS                                (5)
#define ARDUINO_PLATFORM_SPI                    (1)
#define FT81X_GT911 /*goodix gt911 enable*/
#define SPI_CLK_FREQ                            (16000000) /* speed up SPI to 16Mhz on the ESP32 */
//Timing for Crystalfontz CFAF800480E0-050SC-A1
//============================================================================
// Define RGB output pins order, determined by PCB layout
#define LCD_SWIZZLE      (0)
// Define active edge of PCLK. Observed by scope:
//  0: Data is put out coincident with falling edge of the clock.
//     Rising edge of the clock is in the middle of the data.
//  1: Data is put out coincident with rising edge of the clock.
//     Falling edge of the clock is in the middle of the data.
#define LCD_PCLKPOL      (1)
// LCD drive strength: 0=5mA, 1=10mA
#define LCD_DRIVE_10MA   (0)
// Spread Spectrum on RGB signals. Probably not a good idea at higher
// PCLK frequencies.
#define LCD_PCLK_CSPREAD (0)
//This is a 24-bit display, so no need to dither.
#define LCD_DITHER       (0)
//----------------------------------------------------------------------------
// Pixel clock divisor (based on 60MHz internal clock)
//   0 = disable
//   1 = 60MHz
//   2 = 30MHz
//   3 = 20MHz
//   etc
// Our target is 33MHz, 30MHz (div of 2) is as close as we can get.
#define LCD_PCLK         (2)
//----------------------------------------------------------------------------
// Frame_Rate = 30MHz / (LCD_VCYCLE*LCD_HCYCLE)
//            = 30MHz / (863*511) = 68Hz or 14.7mS
//----------------------------------------------------------------------------
// Horizontal timing (minimum values from ILI6122_SPEC_V008.pdf page 45)
// Target 60Hz frame rate, using the largest possible line time in order to
// maximize the time that the FT8xx has to process each line.
#define HPX   (800)    // Horizontal Pixel Width
#define HSW   (1)      // Horizontal Sync Width (1~40)
#define HBP   (46-HSW) // Horizontal Back Porch (must be 46, includes HSW)
#define HFP   (16)     // Horizontal Front Porch (16~210~354)
#define HPP   (116)    // Horizontal Pixel Padding (tot=863: 862~1056~1200)
                       // FTDI needs at least 1 here
// Define the constants needed by the FT8xx based on the timing
// Active width of LCD display
#define LCD_WIDTH   (HPX)
// Start of horizontal sync pulse
#define LCD_HSYNC0  (HFP)
// End of horizontal sync pulse
#define LCD_HSYNC1  (HFP+HSW)
// Start of active line
#define LCD_HOFFSET (HFP+HSW+HBP)
// Total number of clocks per line
#define LCD_HCYCLE  (HPX+HFP+HSW+HBP+HPP)
//----------------------------------------------------------------------------
// Vertical timing (minimum values from ILI6122_SPEC_V008.pdf page 46)
#define VLH   (480)   // Vertical Line Height
#define VS    (1)     // Vertical Sync (in lines)  (1~20)
#define VBP   (23-VS) // Vertical Back Porch (must be 23, includes VS)
#define VFP   (7)     // Vertical Front Porch (7~22~147)
#define VLP   (1)     // Vertical Line Padding (tot=511: 510~525~650)
                      // FTDI needs at least 1 here
// Define the constants needed by the FT8xx based on the timing
// Active height of LCD display
#define LCD_HEIGHT  (VLH)
// Start of vertical sync pulse
#define LCD_VSYNC0  (VFP)
// End of vertical sync pulse
#define LCD_VSYNC1  (VFP+VS)
// Start of active screen
#define LCD_VOFFSET (VFP+VS+VBP)
// Total number of lines per screen
#define LCD_VCYCLE  (VLH+VFP+VS+VBP+VLP)
//============================================================================
#endif

#ifdef CFAF240400C0_030SC_A1_2
//HAL Configs for Crystalfontz CFAF240400C0-030SC-A1
#define DISPLAY_RESOLUTION_WVGA                 (1)
#define FT81X_ENABLE                            (1)
#define ENABLE_SPI_SINGLE                       (1)
#define SDCARD_CS                               (15)
#define FT800_INT                               (4)
#define FT800_PD_N                              (0)
#define FT800_CS                                (5)
#define ARDUINO_PLATFORM_SPI                    (1)
#define FT81X_CTOUCH /*ctouch bug fix enable*/
#define SPI_CLK_FREQ                            (16000000) /* speed up SPI to 16Mhz on the ESP32 */
//Timing for Crystalfontz CFAF240400C0-030SC-A1
//============================================================================
// Define RGB output pins order, determined by PCB layout
#define LCD_SWIZZLE      (2)
// Define active edge of PCLK. Observed by scope:
//  0: Data is put out coincident with falling edge of the clock.
//     Rising edge of the clock is in the middle of the data.
//  1: Data is put out coincident with rising edge of the clock.
//     Falling edge of the clock is in the middle of the data.
#define LCD_PCLKPOL      (0)
// LCD drive strength: 0=5mA, 1=10mA
#define LCD_DRIVE_10MA   (0)
// Spread Spectrum on RGB signals. Probably not a good idea at higher
// PCLK frequencies.
#define LCD_PCLK_CSPREAD (0)
//This is not a 24-bit display, so dither.
#define LCD_DITHER       (1)
//----------------------------------------------------------------------------
// Pixel clock divisor (based on 60MHz internal clock)
//   0 = disable
//   1 = 60MHz
//   2 = 30MHz
//   3 = 20MHz
//   etc
#define LCD_PCLK         (5)
//----------------------------------------------------------------------------
// Horizontal timing (minimum values from ILI6122_SPEC_V008.pdf page 45)
// Target 60Hz frame rate, using the largest possible line time in order to
// maximize the time that the FT8xx has to process each line.
#define HPX   (240)    // Horizontal Pixel Width
#define HSW   (1)      // Horizontal Sync Width (1~40)
#define HBP   (46-HSW) // Horizontal Back Porch (must be 46, includes HSW)
#define HFP   (16)     // Horizontal Front Porch (16~210~354)
#define HPP   (1)    // Horizontal Pixel Padding (tot=863: 862~1056~1200)
                       // FTDI needs at least 1 here
// Define the constants needed by the FT8xx based on the timing
// Active width of LCD display
#define LCD_WIDTH   (HPX)
// Start of horizontal sync pulse
#define LCD_HSYNC0  (HFP)
// End of horizontal sync pulse
#define LCD_HSYNC1  (HFP+HSW)
// Start of active line
#define LCD_HOFFSET (HFP+HSW+HBP)
// Total number of clocks per line
#define LCD_HCYCLE  (HPX+HFP+HSW+HBP+HPP)
//----------------------------------------------------------------------------
// Vertical timing (minimum values from ILI6122_SPEC_V008.pdf page 46)
#define VLH   (400)   // Vertical Line Height
#define VS    (1)     // Vertical Sync (in lines)  (1~20)
#define VBP   (23-VS) // Vertical Back Porch (must be 23, includes VS)
#define VFP   (7)     // Vertical Front Porch (7~22~147)
#define VLP   (1)     // Vertical Line Padding (tot=511: 510~525~650)
                      // FTDI needs at least 1 here
// Define the constants needed by the FT8xx based on the timing
// Active height of LCD display
#define LCD_HEIGHT  (VLH)
// Start of vertical sync pulse
#define LCD_VSYNC0  (VFP)
// End of vertical sync pulse
#define LCD_VSYNC1  (VFP+VS)
// Start of active screen
#define LCD_VOFFSET (VFP+VS+VBP)
// Total number of lines per screen
#define LCD_VCYCLE  (VLH+VFP+VS+VBP+VLP)
//============================================================================
#endif

//match up with defs used by Gpu_Hal
#define DispWidth      LCD_WIDTH
#define DispHeight     LCD_HEIGHT
#define DispHCycle     LCD_HCYCLE
#define DispHOffset    LCD_HOFFSET
#define DispHSync0     LCD_HSYNC0
#define DispHSync1     LCD_HSYNC1
#define DispVCycle     LCD_VCYCLE
#define DispVOffset    LCD_VOFFSET
#define DispVSync0     LCD_VSYNC0
#define DispVSync1     LCD_VSYNC1
#define DispPCLK       LCD_PCLK
#define DispSwizzle    LCD_SWIZZLE
#define DispPCLKPol    LCD_PCLKPOL
#define DispCSpread    LCD_PCLK_CSPREAD
#define DispDither     LCD_DITHER

//////////////////////////
//FatFS config

/* Type of file to load from SDCard or Windows file system */
#define LOADIMAGE 1  //loadimage command takes destination address and options before the actual bitmap data
#define INFLATE 2    //inflate command takes destination address before the actual bitmap
#define LOAD 3       //load bitmaps directly

//////////////////////////////////////////////

/* Standard C libraries */
#include <stdio.h>
/* Standard Arduino libraries */
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <stdint.h>
typedef float float_t;

typedef boolean bool_t;
#define TRUE     (1)
#define FALSE    (0)

typedef char char8_t;
typedef unsigned char uchar8_t;
typedef signed char   schar8_t;

typedef PROGMEM const unsigned char  prog_uchar8_t;
typedef PROGMEM const char           prog_char8_t;
//typedef PROGMEM const uint8_t        prog_uint8_t;
//typedef PROGMEM const int8_t         prog_int8_t;
//typedef PROGMEM const uint16_t       prog_uint16_t;
//typedef PROGMEM const int16_t        prog_int16_t;
//typedef PROGMEM const uint32_t       prog_uint32_t;
//typedef PROGMEM const int32_t        prog_int32_t;
/* HAL inclusions */
#include "Gpu_Hal.h"
#include "Gpu.h"
#include "CoPro_Cmds.h"
#include "Hal_Utils.h"

#define FT800_SEL_PIN           FT800_CS

#endif /*_PLATFORM_H_*/
/* Nothing beyond this*/
