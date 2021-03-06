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

#include "Adafruit_ST7735.h"
#include <limits.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>

inline uint16_t swapcolor(uint16_t x) { 
  return (x << 11) | (x & 0x07E0) | (x >> 11);
}

#if defined (SPI_HAS_TRANSACTION)
  static SPISettings mySPISettings;
#elif defined (__AVR__) || defined(CORE_TEENSY)
  static uint8_t SPCRbackup;
  static uint8_t mySPCR;
#endif

// Constructor when using hardware SPI.  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)
Adafruit_ST7735::Adafruit_ST7735(int8_t cs, int8_t dc, int8_t rst) 
  : Adafruit_GFX(ST7735_TFTWIDTH_128, ST7735_TFTHEIGHT_160) {
  _cs   = cs;
  _dc   = dc;
  _rst  = rst;
  hwSPI = true;
  _sid  = _sclk = -1;
}

inline void Adafruit_ST7735::spiwrite(uint8_t c) 
{
	#if defined (SPI_HAS_TRANSACTION)
		  SPI.transfer(c);
	#elif defined (__AVR__) || defined(CORE_TEENSY)
		  SPCRbackup = SPCR;
		  SPCR = mySPCR;
		  SPI.transfer(c);
		  SPCR = SPCRbackup;
	#elif defined (__arm__)
		  SPI.setClockDivider(21); //4MHz
		  SPI.setDataMode(SPI_MODE0);
		  SPI.transfer(c);
	#endif
}


void Adafruit_ST7735::writecommand(uint8_t c) {
	#if defined (SPI_HAS_TRANSACTION)
	//SPI.beginTransaction(mySPISettings);
	#endif
	  DC_LOW();
	  CS_LOW();

	  spiwrite(c);

	  CS_HIGH();
	#if defined (SPI_HAS_TRANSACTION)
	//SPI.endTransaction();
	#endif
}


void Adafruit_ST7735::writedata(uint8_t c) {
	#if defined (SPI_HAS_TRANSACTION)
	//SPI.beginTransaction(mySPISettings);
	#endif
	  DC_HIGH();
	  CS_LOW();
		
	  spiwrite(c);

	  CS_HIGH();
	#if defined (SPI_HAS_TRANSACTION)
	//SPI.endTransaction();
	#endif
}

// Rather than a bazillion writecommand() and writedata() calls, screen
// initialization commands and arguments are organized in these tables
// stored in PROGMEM.  The table may look bulky, but that's mostly the
// formatting -- storage-wise this is hundreds of bytes more compact
// than the equivalent code.  Companion function follows.
#define DELAY 0x80
static const uint8_t PROGMEM
  Rcmd1[] = {                 // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      0xC8,                   //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color

  Rcmd2green[] = {            // Init for 7735R, part 2 (green tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 0
      0x00, 0x7F+0x02,        //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x01,             //     XSTART = 0
      0x00, 0x9F+0x01 },      //     XEND = 159

  Rcmd2red[] = {              // Init for 7735R, part 2 (red tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F },           //     XEND = 159

  Rcmd2green144[] = {              // Init for 7735R, part 2 (green 1.44 tab)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F },           //     XEND = 127

  Rcmd2green160x80[] = {              // Init for 7735R, part 2 (mini 160x80)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 79
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F },           //     XEND = 159


  Rcmd3[] = {                 // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100 };                  //     100 ms delay


// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in PROGMEM byte array.
void Adafruit_ST7735::commandList(const uint8_t *addr) {

  uint8_t  numCommands, numArgs;
  uint16_t ms;

  numCommands = pgm_read_byte(addr++);   // Number of commands to follow
  while(numCommands--) {                 // For each command...
    writecommand(pgm_read_byte(addr++)); //   Read, issue command
    numArgs  = pgm_read_byte(addr++);    //   Number of args to follow
    ms       = numArgs & DELAY;          //   If hibit set, delay follows args
    numArgs &= ~DELAY;                   //   Mask out delay bit
    while(numArgs--) {                   //   For each argument...
      writedata(pgm_read_byte(addr++));  //     Read, issue argument
    }

    if(ms) {
      ms = pgm_read_byte(addr++); // Read post-command delay time (ms)
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      delay(ms);
    }
  }
}


// Initialization code common to both 'B' and 'R' type displays
void Adafruit_ST7735::commonInit(const uint8_t *cmdList) {
  ystart = xstart = colstart  = rowstart = 0; // May be overridden in init func

  pinMode(_dc, OUTPUT);
  pinMode(_cs, OUTPUT);

#if defined(USE_FAST_IO)
  csport    = portOutputRegister(digitalPinToPort(_cs));
  dcport    = portOutputRegister(digitalPinToPort(_dc));
  cspinmask = digitalPinToBitMask(_cs);
  dcpinmask = digitalPinToBitMask(_dc);
#endif

#if defined (SPI_HAS_TRANSACTION)
    SPI.begin();
    mySPISettings = SPISettings(16000000, MSBFIRST, SPI_MODE0);

#endif

  // toggle RST low to reset; CS low so it'll listen to us
  CS_LOW();
  if (_rst != -1) {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
    delay(500);
    digitalWrite(_rst, LOW);
    delay(500);
    digitalWrite(_rst, HIGH);
    delay(500);
  }

  if(cmdList) commandList(cmdList);
  
  //test
  SPI.beginTransaction(mySPISettings);
}


// Initialization for ST7735R screens (green or red tabs)
void Adafruit_ST7735::initR(uint8_t options) {
	commonInit(Rcmd1);

	_height = ST7735_TFTHEIGHT_128;
	_width = ST7735_TFTWIDTH_128;
	commandList(Rcmd2green144);
	colstart = 2;
	commandList(Rcmd3);
	tabcolor = options;

	setRotation(0);
}


void Adafruit_ST7735::setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1,
 uint8_t y1) {

  writecommand(ST7735_CASET); // Column addr set
  writedata(0x00);
  writedata(x0+xstart);     // XSTART 
  writedata(0x00);
  writedata(x1+xstart);     // XEND

  writecommand(ST7735_RASET); // Row addr set
  writedata(0x00);
  writedata(y0+ystart);     // YSTART
  writedata(0x00);
  writedata(y1+ystart);     // YEND

  writecommand(ST7735_RAMWR); // write to RAM
}


void Adafruit_ST7735::pushColor(uint16_t color) {
	#if defined (SPI_HAS_TRANSACTION)
	//SPI.beginTransaction(mySPISettings);
	#endif

	  DC_HIGH();
	  CS_LOW();
	  spiwrite(color >> 8);
	  spiwrite(color);
	  CS_HIGH();

	#if defined (SPI_HAS_TRANSACTION)
	//SPI.endTransaction();
	#endif
}

void Adafruit_ST7735::drawPixel(int16_t x, int16_t y, uint16_t color) {

  uint8_t hi = color >> 8, lo = color;
  startDraw(x,y,x+1,y+1);
  drawFastPixel(hi,lo);
  endDraw();
}
//REPLACE THESE WITH writePixel/startWrite/endWrite, once all other functions are modified to take start/endwrite into account.
void Adafruit_ST7735::drawFastPixel(uint8_t hi_c,uint8_t lo_c)
{
	spiwrite(hi_c);
	spiwrite(lo_c);
}

void Adafruit_ST7735::startDraw(int16_t x, int16_t y, int16_t w, int16_t h)
{
	//x, y, x+w-1, y+h-1
  setAddrWindow(x,y,w,h); //ADDR set needs to be here, sets the area of the frame buffer to write to.
  #if defined (SPI_HAS_TRANSACTION)
  //SPI.beginTransaction(mySPISettings);
  #endif
  DC_HIGH();
  CS_LOW();
}

void Adafruit_ST7735::endDraw()
{
	CS_HIGH();

	#if defined (SPI_HAS_TRANSACTION)
	//SPI.endTransaction();
	#endif
}

void Adafruit_ST7735::drawFastBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color,uint16_t bg) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;
	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= _width) || (y >= _height)) return;
	if((x + w - 1) >= _width)  w = _width  - x;
	if((y + h - 1) >= _height) h = _height - y;
	
	uint8_t hi = color >> 8, lo = color;
	uint8_t hi_bg = bg >> 8, lo_bg = bg;
	
    startDraw(x,y,x+w-1,y+h-1);
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            if(byte & 0x80)
			{
				//if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;
				drawFastPixel(hi,lo);
			}
			else //Looks like we need to write a background color for FastBG, because we have a screen area that gets written to sequentially, not sure how to skip yet.
			{
				drawFastPixel(hi_bg,lo_bg);
			}
        }
    }
    endDraw();
}

//Draw fast bitmap, non-transparent
void Adafruit_ST7735::drawFastColorBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t colorIndex[], const uint16_t pal[],bool flipH, bool flipV) {
	drawCBMPsectionRLE(x,y,w,h,colorIndex,emptyTiles,pal,w,h,0,flipH,flipV);
}
//FLIP image vertically, can just draw last line first, then go up
//RGB 4-4-4 mode, 1byte, 1 pixel. 3AH = 03h
//16 bit write mode?	
//set inverted mode instead of rotate
//Draw image section, like a tilemap
//tileDraw - 55ms. 
//improved to 47ms.
//tileDraw while skipping blanks - 25-26ms - removing bitshifting only removed 1-2ms.
//improved to 22-23ms.
//generate rects for redraw tilemap?
//w/ inefficient ifs -> 2-3,sometimes 4. w/o 1-2, sometimes 3.
//getting rid of bitshifting helps.

//DRAW 1-bit bmp section, for use with fonts. ADAfruits GFX font routine is HUGE.

//need to compact CIDX to 4bit
void Adafruit_ST7735::drawCBMPsection(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t colorIndex[], const uint16_t pal[], uint8_t imageW, uint8_t imageH, uint8_t sectionID, bool flipH, bool flipV, uint8_t bitDepth) {

	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= _width) || (y >= _height)) return;
	
	//1-bit vars
	int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;
	uint16_t monoCol = pgm_read_word(&pal[0]);
	uint16_t monoBG = pgm_read_word(&pal[1]);
	uint8_t hi = monoCol >> 8, lo = monoCol;
	uint8_t hi_bg = monoBG >> 8, lo_bg = monoBG;
	uint16_t startAddr = sectionID * (w*h);
	//end 1-bit vars
	

	//get correct address position of the tile we want
	uint8_t line = ((sectionID * w) / imageW);
	uint16_t iterator = ((sectionID * w) % imageW) + ((h*line)*imageW);
	
	//4bitvals
	uint16_t finalColor;
	uint8_t color;
	
	int itXAdder = 1;
	int itYAdder = imageW - w;
	
	if (flipV)
	{
		itXAdder = -1;
		itYAdder = -(imageW - w);

		iterator = w*h;//this  will only work for non tiles - imageW same as w.
		//iterator -= h;
	}
	if(flipH)
	{
		itXAdder = -1;
		itYAdder = w*2;
		iterator = w;
		iterator -= h;
	}
	unsigned char digit[] = {0xff, 0xc3, 0x81, 0xbd, 0xbd, 0x81, 0xc3, 0xff};
    startDraw(x,y,x+w-1,y+h-1);
    for(uint8_t j=0; j<h; j+=1, y++) {
        for(uint8_t i=0; i<w; i++) {
			switch(bitDepth)
			{
				case 4: 
				color = pgm_read_byte(&colorIndex[iterator]);
				
				
				//move to a var- should also do bit shifting on load.
				finalColor = pgm_read_word(&pal[color]);
				drawFastPixel(finalColor >> 8, finalColor);
				break;
				
				case 1:
				if((i+startAddr)& 7) byte <<= 1;
				else      byte   = pgm_read_byte(&colorIndex[(j * byteWidth + (i+startAddr)/ 8)]);
				if(byte & 0x80)
				{
					//if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;
					drawFastPixel(hi,lo);
				}
				else //Looks like we need to write a background color for FastBG, because we have a screen area that gets written to sequentially, not sure how to skip yet.
				{
					drawFastPixel(hi_bg,lo_bg);
				}
				break;
				iterator +=itXAdder;
			};
			
        }
		iterator += itYAdder;
		//iterator+= imageW - w;//}
    }
    endDraw();
}

void Adafruit_ST7735::drawCBMPsectionRLE(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t colorIndex[], const uint16_t tileAddr[], const uint16_t pal[], uint8_t imageW, uint8_t imageH, uint8_t sectionID, bool flipH, bool flipV) {

	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= _width) || (y >= _height)) return;

	//uint8_t palHi[palSize] = palH;
	//uint8_t palLo[palSize] = palL;
	uint8_t tiles =pgm_read_byte(&tileAddr[0]);
	
    startDraw(x,y,x+w-1,y+h-1);

	uint8_t rLength = 0;
	uint8_t colorID = 0;
	//uint8_t linehi[w]; // for flip
	//uint8_t linelo[w];
	uint8_t lineCtr = 0;
	uint16_t imageSz = w*h;
	uint16_t startAddr = 0;
	uint16_t pixelsDrawn=0;
	if(tiles >= sectionID)
	{
		startAddr = pgm_read_word(&tileAddr[sectionID+1]);
	}
	for(uint16_t j=startAddr; pixelsDrawn<imageSz;j++)
	{
		uint8_t color = pgm_read_byte(&colorIndex[j]);
		//start rle read
		//rle_4_bit(color,rLength,colorID);
		rLength = color & 0xF;
		colorID = (color >> 4) & 0xF;
		//end rle read
		uint16_t finalColor = pgm_read_word(&pal[colorID]);
		uint8_t hi = finalColor >> 8, lo = finalColor;
		
		for(uint8_t i = 0; i<rLength+1;i++)
		{
			/*if(flipH)
			{
				linehi[lineCtr] = hi;
				linelo[lineCtr] = lo;
				lineCtr++;
				if(lineCtr >= w)
				{
					lineCtr = 0;
					for(uint8_t k=w;k>0;k--)
					{
						drawFastPixel(linehi[k], linelo[k]);
					}
				}
			}*/
			//else
			//{
				drawFastPixel(hi, lo);
			//}
		}
		pixelsDrawn+=rLength+1;
		//will probably need checks to see if rectFill exceeds graphic width
		//pixels drawn var
		
	}

    endDraw();
	
}

//Draw Slow Color BMPs, with transparency.
//At present im not sure its possible to draw transparent BMPs with setAddrWindow set to the size of the graphic.
//setAddrWindow needs to be provided with data to fill the entire space, it doesnt have a 'skip pixel' byte im aware of.
void Adafruit_ST7735::drawColorBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h, const uint8_t colorIndex[], const uint16_t pal[], uint16_t bg) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;
	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= _width) || (y >= _height)) return;
	if((x + w - 1) >= _width)  w = _width  - x;
	if((y + h - 1) >= _height) h = _height - y;
	
	
	uint8_t hi_bg = bg >> 8, lo_bg = bg;
	uint16_t iterator = 0;

    //startDraw(x,y,x+w-1,y+h-1);
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
			uint8_t color = pgm_read_byte(&colorIndex[iterator]);
			iterator++;
            if(byte & 0x80)
			{
				
				uint16_t finalColor = pgm_read_word(&pal[color]);//convert 8 to 16 iterator
				//drawFastPixel(x+i, y, finalColor >> 8, finalColor);
				//drawPixel(finalColor);
			}
        }
    }
}

void Adafruit_ST7735::drawFont(uint8_t x, uint8_t y,String text)
{
	//A=65: tileID=10:
	//0=48: tileID=0:
	//can put special chars between 57&65(6chars) to get rid of if statement.
	uint8_t tileID;
	//uint8_t asciiOffset = 55; // adjust from ascii code to tileID
	uint8_t xOffset;
	uint8_t len = text.length();
	for(uint8_t i =0; i< len; i++)
	{
		tileID = text[i] - 48;
		if(tileID > 50)
		{
			tileID = 11;//space
		}
		drawCBMPsection(x+xOffset, y, FONT_TILESZ, FONT_TILESZ, tileFont, fontCol, FONT_WIDTH, FONT_HEIGHT, tileID, false, false,1);
		//drawFastBitmap(x+xOffset, y,tileFont, 8, 8, fontCol[0],fontCol[1]);
		xOffset += 8;
	}
}

uint8_t Adafruit_ST7735::rle_4_bit(uint8_t &input, uint8_t &outputColor, uint8_t &outputLength)
{
	outputLength = (input >> 4) & 0xF;
	outputColor = input & 0xF;
}

uint8_t Adafruit_ST7735::rle_1_bit(uint8_t &input, uint8_t &outputColor, uint8_t &outputLength)
{
	outputLength = (input >> 4) & 0xF;
	outputColor = input & 0xF;
}

void Adafruit_ST7735::drawFastVLine(int16_t x, int16_t y, int16_t h,
 uint16_t color) {

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((y+h-1) >= _height) h = _height-y;
  setAddrWindow(x, y, x, y+h-1);

  uint8_t hi = color >> 8, lo = color;
    
#if defined (SPI_HAS_TRANSACTION)
  //SPI.beginTransaction(mySPISettings);
#endif

  DC_HIGH();
  CS_LOW();
  while (h--) {
    spiwrite(hi);
    spiwrite(lo);
  }
  CS_HIGH();

#if defined (SPI_HAS_TRANSACTION)
  //SPI.endTransaction();
#endif
}


void Adafruit_ST7735::drawFastHLine(int16_t x, int16_t y, int16_t w,
  uint16_t color) {

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((x+w-1) >= _width)  w = _width-x;
  setAddrWindow(x, y, x+w-1, y);

  uint8_t hi = color >> 8, lo = color;

#if defined (SPI_HAS_TRANSACTION)
  //SPI.beginTransaction(mySPISettings);
#endif

  DC_HIGH();
  CS_LOW();
  while (w--) {
    spiwrite(hi);
    spiwrite(lo);
  }
  CS_HIGH();

#if defined (SPI_HAS_TRANSACTION)
  //SPI.endTransaction();
#endif
}



void Adafruit_ST7735::fillScreen(uint16_t color) {
  fillRect(0, 0,  _width, _height, color);
}



// fill a rectangle
void Adafruit_ST7735::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
  uint16_t color) {

  // rudimentary clipping (drawChar w/big text requires this)
  if((x >= _width) || (y >= _height)) return;
  if((x + w - 1) >= _width)  w = _width  - x;
  if((y + h - 1) >= _height) h = _height - y;

  setAddrWindow(x, y, x+w-1, y+h-1);

  uint8_t hi = color >> 8, lo = color;
    
#if defined (SPI_HAS_TRANSACTION)
  //SPI.beginTransaction(mySPISettings);
#endif

  DC_HIGH();
  CS_LOW();
  for(y=h; y>0; y--) {
    for(x=w; x>0; x--) {
      spiwrite(hi);
      spiwrite(lo);
    }
  }
  CS_HIGH();

#if defined (SPI_HAS_TRANSACTION)
  //SPI.endTransaction();
#endif
}

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

void Adafruit_ST7735::setRotation(uint8_t m) {

  writecommand(ST7735_MADCTL);
  rotation = m % 4; // can't be higher than 3
  switch (rotation) {
   case 0:
	writedata(MADCTL_MX | MADCTL_MY | MADCTL_BGR);


	_height = ST7735_TFTHEIGHT_128;
	_width  = ST7735_TFTWIDTH_128;
     
     break;
   case 1:

       writedata(MADCTL_MY | MADCTL_MV | MADCTL_BGR);



       _width = ST7735_TFTHEIGHT_128;
       _height = ST7735_TFTWIDTH_128;

     break;
  case 2:

       writedata(MADCTL_BGR);
       _height = ST7735_TFTHEIGHT_128;
       _width  = ST7735_TFTWIDTH_128;

     break;
   case 3:

       writedata(MADCTL_MX | MADCTL_MV | MADCTL_BGR);

       _width = ST7735_TFTHEIGHT_128;
       _height = ST7735_TFTWIDTH_128;
     
     
     break;
  }
  ystart = colstart;
  xstart = rowstart;
}


void Adafruit_ST7735::invertDisplay(boolean i) {
  writecommand(i ? ST7735_INVON : ST7735_INVOFF);
}


/******** low level bit twiddling **********/


inline void Adafruit_ST7735::CS_HIGH(void) {
#if defined(USE_FAST_IO)
  *csport |= cspinmask;
#else
  digitalWrite(_cs, HIGH);
#endif
}

inline void Adafruit_ST7735::CS_LOW(void) {
#if defined(USE_FAST_IO)
  *csport &= ~cspinmask;
#else
  digitalWrite(_cs, LOW);
#endif
}

inline void Adafruit_ST7735::DC_HIGH(void) {
#if defined(USE_FAST_IO)
  *dcport |= dcpinmask;
#else
  digitalWrite(_dc, HIGH);
#endif
}

inline void Adafruit_ST7735::DC_LOW(void) {
#if defined(USE_FAST_IO)
  *dcport &= ~dcpinmask;
#else
  digitalWrite(_dc, LOW);
#endif
}
