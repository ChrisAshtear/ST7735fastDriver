# Fast ST7735 Arduino Display Driver - orig. from Adafruit

The PDQ library does not work in arduino projects that use more than one file,
and the original Adafruit lib was incredibly slow.

I took a look and part of the problem is that for every pixel drawn for bitmaps, the SPI connection is reset.
Correcting that gave me a tremendous speed boost, and along with that I added the ability to draw 256-color BMPs.