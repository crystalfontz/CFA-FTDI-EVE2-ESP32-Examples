# FTDI-ESP32-Examples-CFAF800480E0-050SC-A1
This is a collection of modified FTDI/Bridgetek EVE2 Examples for use with the Crystalfontz CFAF800480E0-050SC-A1-1 display, Crystalfontz CFA10098 breakout board and a Espressif ESP32 SoC.

Information on the Crystalfontz CFAF800480E0-050SC-A1-1 display may be [found here](https://www.crystalfontz.com/product/cfaf800480e0050sca11).
The Crystalfontz CFA10098 breakout board may be [found here](https://www.crystalfontz.com/product/cfa10098).

More EVE examples are available on the FTDI website, but those not listed here are not compatible with the EVE2 (FT91x) or were not programmed to run correctly on a display of this size (800x480 pixels).

## Getting Started
Our testing was performed using a NodeMCU ESP-32s development board.
If you are using a different development board, the pinout may be different, so be sure to double check the connections are correct.

Connections required are as follows:
* CFAF800480E0-050SC-A1 -> CFA10098 (using an appropriate flat-flex cable)
* CFA10098 3.3v -> 3.3v power supply (independent 3.3v 1A supply, NOT the ESP32 3.3v!)
* CFA10098 GND -> 3.3v power supply ground (as above)
* CFA10098 SCK -> NodeMCU-ESP32s pin P18
* CFA10098 MOSI -> NodeMCU-ESP32s pin P23
* CFA10098 MISO -> NodeMCU-ESP32s pin P19
* CFA10098 CS -> NodeMCU-ESP32s pin P5
* CFA10098 INT -> NodeMCU-ESP32s pin P4
* CFA10098 PD -> NodeMCU-ESP32s pin P0
* CFA10098 GND -> NodeMCU-ESP32s GND

To run the examples that use a microSD card, see the microSD Card Reader section below.
## Running an Example
* Download the examples from here to your PC
* Connect the CFAF800480E0-050SC-A1 display, CFA10098 breakout board, and ESP32 as above.
* Power on the LCD module with a 3.3v supply first
* Connect the ESP32 to your PC with a USB cable
* Run the Arduino software, and load one of the project INO files.
* Copy the files from the associated sd_contents directory (if it exists, otherwise this step is not required) to a microSD card, and insert it into the microSD card reader attached to the ESP32.
* Compile and upload the firmware to the ESP32

On completion of uploading complied firmware, the display should light, show a touch-screen calibration screen, and then proceed to the example.

## microSD Card Reader Connection
Some of of the examples use data stored on a microSD card. If your ESP32 board does not have a microSD card slot, you'll need to connect one.
The easiest way of doing so is by using a 3.3 volt microSD breakout board (make sure it does not have 5v level-shifters) connected to the ESP32 using jumper wires.
An example microSD breakout board [can be found here](https://www.sparkfun.com/products/544).

Connection is as follows:
* microSD 3.3V -> NodeMCU-ESP32s 3.3V
* microSD GND -> NodeMCU-ESP32s GND
* microSD SCLK -> NodeMCU-ESP32s pin P18
* microSD MISO -> NodeMCU-ESP32s pin P19
* microSD MOSI -> NodeMCU-ESP32s pin P23
* microSD CS/SS -> NodeMCU-ESP32s pin P15

## Further Information
For further information see:
* [Crystalfontz CFAF800480E0-050SC-A1-1 product page](https://www.crystalfontz.com/product/cfaf800480e0050sca11)
* [Crystalfontz CFA10098 product page](https://www.crystalfontz.com/product/cfa10098)
* [Crystalfontz Technical Forum](http://forum.crystalfontz.com/)
* [FTDI / Bridgetek EVE Example Projects](https://www.ftdichip.com/Support/SoftwareExamples/FT800_Projects.htm)

