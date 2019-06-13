
# FTDI-Arduino-Examples-CFAF800480E0-050SC-A1
This is a collection of modified FTDI/Bridgetek EVE Examples for use with the Crystalfontz CFAF800480E0-050SC-A1-2 display kit.
The kit includes a CFAF800480E0-050SC-A1 display module, a CFA10098 breakout board and a Seeeduino v4.x. 

Information on the Crystalfontz CFAF800480E0-050SC-A1-2 kit may be [found here](https://www.crystalfontz.com/product/cfaf800480e0050sca12-800x480-accelerated-tft-dev-kit).

More EVE examples are available on the FTDI website, but those not listed here are not compatible with the EVE2 (FT91x) or were not programmed to run correctly on a display of this size (800x480 pixels).

## Getting Started

See the "Getting Started" PDF located in this repository for a guide on powering-up the Crystalfontz CFAF800480E0-050SC-A1-2 kit.

Running some of the examples requires connection of an extra (not supplied in the kit) microSD card reader & SD card (more information below).

## Running an Example
* Download the examples from here to your PC
* Connect the CFAF800480E0-050SC-A1-2 kit as in the "Getting Started" PDF
* Power on the LCD module with a 3.3v supply first
* Connect the Seeeduino to your PC with a USB cable
* Run the Arduino software, and load one of the project INO files.
* Copy the files from the associated sd_contents directory (if it exists, otherwise this step is not required) to a microSD card, and insert it into the microSD card reader attached to the Seeeduino.
* Compile and upload the firmware to the Seeeduino

On completion of uploading complied firmware, the display should light, show a touch-screen calibration screen, and then proceed to the example.

## microSD Card Reader Connection
Some of of the examples use data stored on a microSD card. As the Seeeduino does not have a microSD slot, one must be provided by the user.
The easiest way of doing so is by using a 3.3 volt microSD breakout board (make sure it does not have 5v level-shifters) connected to the Seeeduino using jumper wires.
An example microSD breakout board [can be found here](https://www.sparkfun.com/products/544).

Connection is as follows:
* microSD 3.3V -> Seeeduino 3.3V
* microSD GND -> Seeeduino GND
* microSD SCLK -> Seeeduino pin 13
* microSD MISO -> Seeeduino pin 12
* microSD MOSI -> Seeeduino pin 11
* microSD CS/SS -> Seeeduino pin 4

## Further Information
For further information see:
* [Crystalfontz CFAF800480E0-050SC-A1-2 product page](https://www.crystalfontz.com/product/cfaf800480e0050sca12-800x480-accelerated-tft-dev-kit)
* [Crystalfontz Technical Forum](http://forum.crystalfontz.com/)
* [FTDI / Bridgetek EVE Example Projects](https://www.ftdichip.com/Support/SoftwareExamples/FT800_Projects.htm)

