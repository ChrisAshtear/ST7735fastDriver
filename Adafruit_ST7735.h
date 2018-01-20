/***************************************************
  This is a library for the Adafruit 1.8" SPI display.

This library works with the Adafruit 1.8" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/358
The 1.8" TFT shield
  ----> https://www.adafruit.com/product/802
The 1.44" TFT breakout
  ----> https://www.adafruit.com/product/2088
as well as Adafruit raw 1.8" TFT display
  ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#ifndef _ADAFRUIT_ST7735H_
#define _ADAFRUIT_ST7735H_

#include "Arduino.h"
#include "Print.h"
#include <Adafruit_GFX.h>

#if defined(__AVR__) || defined(CORE_TEENSY)
  #include <avr/pgmspace.h>
  #define USE_FAST_IO
  typedef volatile uint8_t RwReg;
#elif defined(ARDUINO_STM32_FEATHER)
  typedef volatile uint32 RwReg;
  #define USE_FAST_IO
#elif defined(ARDUINO_FEATHER52)
  typedef volatile uint32_t RwReg;
  #define USE_FAST_IO
#elif defined(ESP8266)
  #include <pgmspace.h>
#elif defined(__SAM3X8E__)
  #undef __FlashStringHelper::F(string_literal)
  #define F(string_literal) string_literal
  #include <include/pio.h>
  #define PROGMEM
  #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
  #define pgm_read_word(addr) (*(const unsigned short *)(addr))
  typedef unsigned char prog_uchar;
#endif

// some flags for initR() :(
#define INITR_GREENTAB   0x0
#define INITR_REDTAB     0x1
#define INITR_BLACKTAB   0x2

#define INITR_18GREENTAB    INITR_GREENTAB
#define INITR_18REDTAB      INITR_REDTAB
#define INITR_18BLACKTAB    INITR_BLACKTAB
#define INITR_144GREENTAB   0x1
#define INITR_MINI160x80    0x4


// for 1.44 and mini
#define ST7735_TFTWIDTH_128  128
// for mini
#define ST7735_TFTWIDTH_80   80
// for 1.44" display
#define ST7735_TFTHEIGHT_128 128
// for 1.8" and mini display
#define ST7735_TFTHEIGHT_160  160

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Color definitions
#define	ST7735_BLACK   0x0000
#define	ST7735_BLUE    0x001F
#define	ST7735_RED     0xF800
#define	ST7735_GREEN   0x07E0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW  0xFFE0
#define ST7735_WHITE   0xFFFF


#ifndef pgm_read_byte
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
 #define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
 #define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif

uint8_t fontWidth = 152;
uint8_t fontHeight = 16;
uint8_t fTileSize = 8;

const unsigned char PROGMEM tileFont[] =
{
0xff, 0xc3, 0x81, 0xbd, 0xbd, 0x81, 0xc3, 0xff, 0xff, 0xff, 0xbb, 0x81, 0x81, 0xbf, 0xff, 0xff, 0xff, 0x9b, 0x8d, 0x8d, 0xa1, 0xa1, 0xb3, 0xff, 0xff, 0xbd, 0xb5, 0xb5, 0x81, 0x81, 0xcb, 0xff, 0xff, 0xc3, 0xd9, 0xdd, 0x81, 0x81, 0xcf, 0xff, 0xff, 0xd1, 0xb1, 0xb5, 0x85, 0x85, 0xcf, 0xff, 0xff, 0xc3, 0x81, 0xb5, 0xb5, 0x85, 0xcf, 0xff, 0xff, 0xfd, 0x9d, 0x8d, 0x85, 0xf1, 0xf9, 0xff, 0xff, 0xcb, 0xb5, 0xb5, 0x81, 0x81, 0xcb, 0xff, 0xff, 0xf3, 0xad, 0xad, 0x81, 0x81, 0xc3, 0xff, 0xff, 0x83, 0xed, 0xed, 0x81, 0x81, 0x83, 0xff, 0xff, 0x81, 0x81, 0xb5, 0xb5, 0x81, 0xcb, 0xff, 0xff, 0xc3, 0x81, 0xbd, 0xbd, 0x99, 0xdb, 0xff, 0xff, 0x81, 0xbd, 0xbd, 0x81, 0x81, 0xc3, 0xff, 0xff, 0x81, 0x81, 0xb5, 0xb5, 0xb5, 0xbd, 0xff, 0xff, 0x81, 0x81, 0xed, 0xed, 0xed, 0xfd, 0xff, 0xff, 0xc3, 0x81, 0xbd, 0xad, 0x89, 0x8b, 0xff, 0xff, 0x81, 0xf7, 0xf7, 0xf7, 0x81, 0x81, 0xff, 0xff, 0xff, 0xbd, 0x81, 0x81, 0xbd, 0xff, 0xff, 
0xff, 0xcf, 0x9d, 0x81, 0xc1, 0xfd, 0xff, 0xff, 0xff, 0x81, 0x81, 0xe7, 0xc3, 0x99, 0xbd, 0xff, 0xff, 0x81, 0x81, 0xbf, 0xbf, 0xbf, 0xbf, 0xff, 0xff, 0x81, 0xf3, 0xe7, 0xf3, 0x81, 0x81, 0xff, 0xff, 0x81, 0xf3, 0xe7, 0xcf, 0x81, 0x81, 0xff, 0xff, 0xc3, 0x81, 0xbd, 0xbd, 0x81, 0xc3, 0xff, 0xff, 0x81, 0x81, 0xed, 0xed, 0xe1, 0xf3, 0xff, 0xff, 0xc3, 0x81, 0xbd, 0xad, 0xdd, 0xa3, 0xff, 0xff, 0x81, 0x81, 0xed, 0xcd, 0xa1, 0xb3, 0xff, 0xff, 0xdb, 0xb1, 0xb5, 0x85, 0x85, 0xcf, 0xff, 0xff, 0xfd, 0xfd, 0x81, 0x81, 0xfd, 0xfd, 0xff, 0xff, 0xc1, 0xbf, 0xbf, 0x9f, 0x81, 0xc1, 0xff, 0xff, 0xe1, 0xdf, 0xbf, 0x9f, 0xc1, 0xe1, 0xff, 0xff, 0x81, 0xcf, 0xe7, 0xcf, 0x81, 0x81, 0xff, 0xff, 0xbd, 0x99, 0xc3, 0xc3, 0x99, 0xbd, 0xff, 0xff, 0xf9, 0xf1, 0x87, 0x87, 0xf1, 0xf9, 0xff, 0xff, 0x9d, 0x8d, 0x85, 0xa1, 0xb1, 0xb9, 0xff, 0xff, 0xbb, 0xd7, 0xef, 0xd7, 0xbb, 0xff, 0xff, 0xff, 0xff, 0x9f, 0x9f, 0xff, 0xff, 0xff, 0xff, 
};

class Adafruit_ST7735 : public Adafruit_GFX {

 public:

  Adafruit_ST7735(int8_t CS, int8_t RS, int8_t SID, int8_t SCLK, int8_t RST = -1);
  Adafruit_ST7735(int8_t CS, int8_t RS, int8_t RST = -1);

  //need to update fill/hline/vline/etc to use these.
  /*override void startWrite(int16_t x, int16_t y, int16_t w, int16_t h);
  override void endWrite(void);
  override void writePixel(int16_t x, int16_t y, uint16_t color);*/
  
  void     initB(void),                             // for ST7735B displays
           initR(uint8_t options = INITR_GREENTAB), // for ST7735R
           setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1),
           pushColor(uint16_t color),
           fillScreen(uint16_t color),
           drawPixel(int16_t x, int16_t y, uint16_t color),
		   drawFastPixel(int16_t x, int16_t y, uint8_t hi_c,uint8_t lo_c)/*NEED TO USE startDraw/endDraw before & after this function*/,
		   startDraw(int16_t x, int16_t y, int16_t w, int16_t h),
		   drawFont(uint8_t x, uint8_t y, uint8_t length, const char text[], const uint8_t font, uint16_t fontSize,const uint16_t pal[]), //Tilemap Font
		   drawFastBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color,uint16_t bg)/*DRAWS STANDALONE BITMAP. IF DRAWING TILES USE */,
		   drawFastColorBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t colorIndex[], const uint16_t pal[],bool flipH,bool FlipV)/*DRAWS STANDALONE BITMAP. IF DRAWING TILES USE */,
		   drawColorBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, const uint8_t colorIndex[], const uint16_t pal[], uint16_t bg)/*DRAWS STANDALONE BITMAP. IF DRAWING TILES USE */,
		   //drawSurface(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t colorIndex[], const uint16_t pal[], uint8_t imageW, uint8_t imageH, uint8_t sectionID)/*MUST USE START/END DRAW WITH THIS*/,
		   drawCBMPsection(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t colorIndex[], const uint16_t pal[], uint8_t imageW, uint8_t imageH, uint8_t sectionID,bool flipH,bool FlipV),
		   drawCBMPsectionRLE(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t colorIndex[], uint8_t RLEsize, const uint16_t pal[], uint8_t imageW, uint8_t imageH, uint8_t sectionID, bool flipH, bool flipV),
		   endDraw(),
           drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color),
           drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color),
           fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
             uint16_t color),
           setRotation(uint8_t r),
           invertDisplay(boolean i);
  uint16_t Color565(uint8_t r, uint8_t g, uint8_t b);
  
  int RLE_Uncompress( unsigned char *in, RLE_data *out, unsigned int insize ); //uncompress encoded bitmap/tilemap
  
  uint8_t rle_4_bit(uint8_t &input, uint8_t &outputColor, uint8_t &outputLength); //uncompress 16-color RLE bitmaps
  uint8_t rle_1_bit(uint8_t &input, uint8_t &outputColor, uint8_t &outputLength); //uncompress monochrome RLE bitmaps

  /* These are not for current use, 8-bit protocol only!
  uint8_t  readdata(void),
           readcommand8(uint8_t);
  uint16_t readcommand16(uint8_t);
  uint32_t readcommand32(uint8_t);
  void     dummyclock(void);
  */

 private:
  uint8_t  tabcolor;

  void     spiwrite(uint8_t),
           writecommand(uint8_t c),
           writedata(uint8_t d),
           commandList(const uint8_t *addr),
           commonInit(const uint8_t *cmdList);
//uint8_t  spiread(void);


  inline void CS_HIGH(void);
  inline void CS_LOW(void);
  inline void DC_HIGH(void);
  inline void DC_LOW(void);

  boolean  hwSPI;

  int8_t  _cs, _dc, _rst, _sid, _sclk;
  uint8_t colstart, rowstart, xstart, ystart; // some displays need this changed

#if defined(USE_FAST_IO)
  volatile RwReg  *dataport, *clkport, *csport, *dcport;

  #if defined(__AVR__) || defined(CORE_TEENSY)  // 8 bit!
    uint8_t  datapinmask, clkpinmask, cspinmask, dcpinmask;
  #else    // 32 bit!
    uint32_t  datapinmask, clkpinmask, cspinmask, dcpinmask;
  #endif
#endif

};




#endif
