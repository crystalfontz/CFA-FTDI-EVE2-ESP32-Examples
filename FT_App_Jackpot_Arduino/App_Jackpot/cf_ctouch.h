//============================================================================
// 
// Add-on FT9xx library for original FTDI/BridgeTek EVE Arduino examples
//
// This source-code and minor modifications to the original FTDI examples
// is meant for use with the Crystalfontz CFA10100 based LCD modules which
// use a Goodix GT911 cap touch screen controller.
//
// 2019-06-01 Mark Williams / Crystalfontz
// 2018-11-21 Brent A. Crosby / Crystalfontz
//
//---------------------------------------------------------------------------
//
//This is free and unencumbered software released into the public domain.
//
//Anyone is free to copy, modify, publish, use, compile, sell, or
//distribute this software, either in source code form or as a compiled
//binary, for any purpose, commercial or non-commercial, and by any
//means.
//
//In jurisdictions that recognize copyright laws, the author or authors
//of this software dedicate any and all copyright interest in the
//software to the public domain. We make this dedication for the benefit
//of the public at large and to the detriment of our heirs and
//successors. We intend this dedication to be an overt act of
//relinquishment in perpetuity of all present and future rights to this
//software under copyright law.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
//OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//OTHER DEALINGS IN THE SOFTWARE.
//
//For more information, please refer to <http://unlicense.org/>
//============================================================================

#ifndef _CF_CTOUCH_H
#define _CF_CTOUCH_H

#include "Platform.h"

#ifdef FT81X_CTOUCH

#include "App_Common.h"
#include "Gpu.h"
#include "Hal_Config.h"

//FTDI defs miss this
#define REG_TOUCH_CONFIG        (3154280UL) //   0x302168
//Capacitive Touch
#define REG_CTOUCH_MODE         (3154180UL) //   0x302104
#define CTOUCH_MODE_CONTINUOUS     ( 3UL) // 0x000003
#define CTOUCH_MODE_ONESHOT        ( 1UL) // 0x000001
#define REG_CTOUCH_EXTEND       (3154184UL) //   0x302108
#define CTOUCH_EXTEND_COMPATIBILITY (1UL) // 0x000001
#define CTOUCH_EXTEND_EXTENDED      (0UL) // 0x000000
#define REG_CTOUCH_TOUCH0_XY    (3154212UL) //   0x302124
#define REG_CTOUCH_TOUCH1_XY    (3154204UL) //   0x30211C
#define REG_CTOUCH_TOUCH2_XY    (3154316UL) //   0x30218C
#define REG_CTOUCH_TOUCH3_XY    (3154320UL) //   0x302190
#define REG_CTOUCH_TOUCH4_X     (3154284UL) //   0x30216C
#define REG_CTOUCH_TOUCH4_Y     (3154208UL) //   0x302120

void FT81x_Init_CTouch(Gpu_Hal_Context_t *phost);

#endif

#endif
