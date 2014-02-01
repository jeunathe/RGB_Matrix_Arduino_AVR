/* Dependencies */
#include <MsTimer2.h>

/* Compile time constants */
static const byte NB_HORIZONTAL_MATRIX = 2;
static const byte NB_VERTICAL_MATRIX = 1;

static const byte NB_LINES_PER_MATRIX = 32;   // MUST be a power of 2
static const byte NB_COLUMNS_PER_MATRIX = 32; // MUST be a power of 2
static const byte MATRIX_SCANLINE_SIZE = 16;  // MUST be 8, 16 or 32

static const byte NB_LINES_COUNT = NB_VERTICAL_MATRIX * NB_LINES_PER_MATRIX;
static const byte NB_COLUMNS_COUNT = NB_HORIZONTAL_MATRIX * NB_COLUMNS_PER_MATRIX;

/* Pin mapping */
// R1, G1, B1, R2, G2, B2 hard-wired on PD2~PD7
// A, B, C, D hard-wired on PC0~PC3
// CLK, OE, LAT hard-wired on PB0~PB2

/** 
 * Framebuffer for RGB
 *
 * Array format : [scan line index] [interlaced pixels data stream R, G, B]
 * Data format : ... [R1 G1 B1][R2, G2, B2] ...
 */
static volatile byte framebuffer[MATRIX_SCANLINE_SIZE][(NB_LINES_COUNT / MATRIX_SCANLINE_SIZE) * (NB_COLUMNS_COUNT / 8) * 3];

/**
 * Possible color enumeration
 */
enum {
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_YELLOW,
  COLOR_CYAN,
  COLOR_PINK,
  COLOR_WHITE,
  COLOR_BLACK  
};

/**
 * Compute the linear offset in the matrix buffer for the given coordinates.
 *
 * @param x X position of the pixel.
 * @param y Y position of the pixel.
 * @return The linear offset in thematrix buffer for the given coordinates.
 */
static inline unsigned int matrixLinearOffset(byte x, byte y) {
  unsigned int xOffset = (x / 8) * (NB_LINES_PER_MATRIX / MATRIX_SCANLINE_SIZE) * 3;
  unsigned int yOffset = (y / NB_LINES_PER_MATRIX) * (NB_LINES_PER_MATRIX / MATRIX_SCANLINE_SIZE) * (NB_COLUMNS_COUNT / 8) * 3;
  unsigned int yInterlacedOffset = ((y & (NB_LINES_PER_MATRIX - 1)) / MATRIX_SCANLINE_SIZE) * 3;
  return yOffset + xOffset + yInterlacedOffset;
}

/**
 * Set the state of a pixel in the framebuffer.
 * 
 * @param x X position of the pixel.
 * @param y Y position of the pixel.
 * @param Color Color to set.
 * @param value Value of the pixel.
 */
static void setPixelAt(const byte x, const byte y, const byte color) {
  volatile byte* pixel = framebuffer[y & (MATRIX_SCANLINE_SIZE - 1)] + matrixLinearOffset(x, y);
  byte mask = 1 << (x & 7);
  switch(color) {
  case COLOR_RED:
    pixel[0] |= mask;
    pixel[1] &= ~mask;
    pixel[2] &= ~mask;
    break;
  case COLOR_GREEN:
    pixel[0] &= ~mask;
    pixel[1] |= mask;
    pixel[2] &= ~mask;
    break;
  case COLOR_BLUE:
    pixel[0] &= ~mask;
    pixel[1] &= ~mask;
    pixel[2] |= mask;
    break;
  case COLOR_YELLOW:
    pixel[0] |= mask;
    pixel[1] |= mask;
    pixel[2] &= ~mask;
    break;
  case COLOR_CYAN:
    pixel[0] &= ~mask;
    pixel[1] |= mask;
    pixel[2] |= mask;
    break;
  case COLOR_PINK:
    pixel[0] |= mask;
    pixel[1] &= ~mask;
    pixel[2] |= mask;
    break;
  case COLOR_WHITE:
    pixel[0] |= mask;
    pixel[1] |= mask;
    pixel[2] |= mask;
    break;
  default:
    pixel[0] &= ~mask;
    pixel[1] &= ~mask;
    pixel[2] &= ~mask;
    break;
  }
}

/**
 * Get the state of a pixel in the framebuffer.
 * 
 * @param x X position of the pixel.
 * @param y Y position of the pixel.
 * @param color Color buffer to use.
 * @return The state of the pixel.
 */
static byte getPixelAt(const byte x, const byte y, const byte color) {
  volatile byte* pixel = framebuffer[y & (MATRIX_SCANLINE_SIZE - 1)] + matrixLinearOffset(x, y);
  byte mask = 1 << (x & 7);
  byte r = !!(pixel[0] & mask);
  byte g = !!(pixel[1] & mask);
  byte b = !!(pixel[2] & mask);
  if(r && !g && !b)
    return COLOR_RED;
  if(!r && g && !b)
    return COLOR_GREEN;
  if(!r && !g && b)
    return COLOR_BLUE;
  if(r && g && !b)
    return COLOR_YELLOW;
  if(!r && g && b)
    return COLOR_CYAN;
  if(r && !g && b)
    return COLOR_PINK;
  if(r && g && b)
    return COLOR_WHITE;
  if(!r && !g && !b)
    return COLOR_BLACK;
}

/** Setup */
void setup() {

  // Setup pins
  pinMode(13, OUTPUT);
  DDRD = 0b11111100; // Data port
  PORTD = 0;

  DDRC = 0b1111; // Addr port
  PORTC = 0;

  DDRB = 0b100111; // Ctrl port + led pin13
  PORTB = 0b10;

  // Init frame buffers (all pixels black)
  memset((void*) framebuffer, 0, NB_LINES_COUNT * (NB_COLUMNS_COUNT / 8) * 3);

  // Init refresh timer
  MsTimer2::set(1, refreshDisplay);
  MsTimer2::start();
}

/** Loop */
void loop() {

  // Demo code
  static byte x = 0;
  static byte y = 0;
  static byte color = COLOR_RED;

  setPixelAt(x, y, color);

  if(++x == NB_COLUMNS_COUNT) {
    x = 0;

    if(++y == NB_LINES_COUNT) {
      y = 0;

      if(color == COLOR_BLACK)
        color = COLOR_RED;
      else
        ++color; 
    }
  }

  // No flood delay
  delay(10);
}

/** Send a whole column pixels bundle */
static void sendColumnBundle(byte **lineBufferPtr) {

  // Hardcoded 1/16 scanline with 32x32 matrix

  // Get RGB values for line N and N + 16
  byte b2 = *(*lineBufferPtr);
  byte g2 = *(--(*lineBufferPtr));
  byte r2 = *(--(*lineBufferPtr));
  byte b1 = *(--(*lineBufferPtr));
  byte g1 = *(--(*lineBufferPtr));
  byte r1 = *(--(*lineBufferPtr));
  --(*lineBufferPtr);

  // MSB FIRST
  PORTD = (PORTD & 0b11) | (!!(r1 & 128) << 2) | (!!(g1 & 128) << 3) | (!!(b1 & 128) << 4) | (!!(r2 & 128) << 5) | (!!(g2 & 128) << 6) | (!!(b2 & 128) << 7);
  PINB = 1;
  PINB = 1; // CLK pulse
  PORTD = (PORTD & 0b11) | (!!(r1 & 64) << 2) | (!!(g1 & 64) << 3) | (!!(b1 & 64) << 4) | (!!(r2 & 64) << 5) | (!!(g2 & 64) << 6) | (!!(b2 & 64) << 7);
  PINB = 1;
  PINB = 1; // CLK pulse
  PORTD = (PORTD & 0b11) | (!!(r1 & 32) << 2) | (!!(g1 & 32) << 3) | (!!(b1 & 32) << 4) | (!!(r2 & 32) << 5) | (!!(g2 & 32) << 6) | (!!(b2 & 32) << 7);
  PINB = 1;
  PINB = 1; // CLK pulse
  PORTD = (PORTD & 0b11) | (!!(r1 & 16) << 2) | (!!(g1 & 16) << 3) | (!!(b1 & 16) << 4) | (!!(r2 & 16) << 5) | (!!(g2 & 16) << 6) | (!!(b2 & 16) << 7);
  PINB = 1;
  PINB = 1; // CLK pulse
  PORTD = (PORTD & 0b11) | (!!(r1 & 8) << 2) | (!!(g1 & 8) << 3) | (!!(b1 & 8) << 4) | (!!(r2 & 8) << 5) | (!!(g2 & 8) << 6) | (!!(b2 & 8) << 7);
  PINB = 1;
  PINB = 1; // CLK pulse
  PORTD = (PORTD & 0b11) | (!!(r1 & 4) << 2) | (!!(g1 & 4) << 3) | (!!(b1 & 4) << 4) | (!!(r2 & 4) << 5) | (!!(g2 & 4) << 6) | (!!(b2 & 4) << 7);
  PINB = 1;
  PINB = 1; // CLK pulse
  PORTD = (PORTD & 0b11) | (!!(r1 & 2) << 2) | (!!(g1 & 2) << 3) | (!!(b1 & 2) << 4) | (!!(r2 & 2) << 5) | (!!(g2 & 2) << 6) | (!!(b2 & 2) << 7);
  PINB = 1;
  PINB = 1; // CLK pulse
  PORTD = (PORTD & 0b11) | (!!(r1 & 1) << 2) | (!!(g1 & 1) << 3) | (!!(b1 & 1) << 4) | (!!(r2 & 1) << 5) | (!!(g2 & 1) << 6) | (!!(b2 & 1) << 7);
  PINB = 1;
  PINB = 1; // CLK pulse
}

/** Display scaline refresh routine */
void refreshDisplay() {

  // Scan line index
  static byte scanlineIndex = 0;

  // Setup control lines and address lines
  PORTB = 0b100110;
  PORTC = scanlineIndex;

  // Get line buffer
  byte *lineBuffer = (byte*) framebuffer[scanlineIndex];
  lineBuffer += (NB_LINES_COUNT / MATRIX_SCANLINE_SIZE) * (NB_COLUMNS_COUNT / 8) * 3 - 1;

  // For each pixels bundle of matrix 4/4, 3/3, 2/2 or 1/1
  sendColumnBundle(&lineBuffer);
  sendColumnBundle(&lineBuffer);
  sendColumnBundle(&lineBuffer);
  sendColumnBundle(&lineBuffer);

  // For each pixels bundle of matrix 3/4, 2/3, 1/2
  if (NB_HORIZONTAL_MATRIX * NB_VERTICAL_MATRIX >= 2) {
    sendColumnBundle(&lineBuffer);
    sendColumnBundle(&lineBuffer);
    sendColumnBundle(&lineBuffer);
    sendColumnBundle(&lineBuffer);
  }

  // For each pixels bundle of matrix 2/4, 1/3
  if (NB_HORIZONTAL_MATRIX * NB_VERTICAL_MATRIX >= 3) {
    sendColumnBundle(&lineBuffer);
    sendColumnBundle(&lineBuffer);
    sendColumnBundle(&lineBuffer);
    sendColumnBundle(&lineBuffer);
  }

  // For each pixels bundle of matrix 1/4
  if (NB_HORIZONTAL_MATRIX * NB_VERTICAL_MATRIX >= 4) {
    sendColumnBundle(&lineBuffer);
    sendColumnBundle(&lineBuffer);
    sendColumnBundle(&lineBuffer);
    sendColumnBundle(&lineBuffer);
  }

  // Trigger latch
  PORTB = 0;

  // Handle scan line overflow
  if (++scanlineIndex == MATRIX_SCANLINE_SIZE) {

    // Reset scan line index
    scanlineIndex = 0;
  }
}
