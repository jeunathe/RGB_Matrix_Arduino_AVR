/* Dependencies */
#include <util/delay.h> /* For delay */
#include <stdint.h>     /* For hardcoded type */
#include "RgbMatrix.h"  /* For the RGB matrix driver API */
#include <stdlib.h>

/** Program entry point */
int main(void) {
int xx=0;

  /* Setup hardware */
  setupMatrixDriver();
  
  /* Main loop */
  for(;;) {
  
    // Demo code: fill the matrix with gradient colors
    static uint8_t r = 0;
    static uint8_t g = 0;
    static uint8_t b = 0;

	/* For each line */
	for(uint8_t y = 0; y < NB_LINES_COUNT; ++y) {
	
	  /* For each column */
	  for(uint8_t x = 0; x < NB_COLUMNS_COUNT; ++x) {
	
		/* Draw color pattern */
/*
#define COLOR_STEP 15
	    if((r += COLOR_STEP) >= 256 - COLOR_STEP) {
          r = 0;
		  
	      if((g += COLOR_STEP) >= 256 - COLOR_STEP) {
            g = 0;
		  
		    if((b += COLOR_STEP) >= 256 - COLOR_STEP) {
              b = 0;
            }
          }
        }
*/
	    setPixelAt(x, y, xx, rand()%128, rand()%128);
//	    setPixelAt(x, y, (x+xx) & 0xFF, (y+xx) & 0xFF, (x+y+xx) & 0xFF);
xx++;
		
		// Add some delay to clearly see the drawing artifact when not using double buffering
//		_delay_us(150);
	  }
	}
//xx++;
	
#if defined(USE_DOUBLE_BUFFERING) || defined(USE_TRIPLE_BUFFERING)
    // Rotate buffer if double or triple buffering is used
    rotateFramebuffer();
#endif
	
    // Inter frame delay
//    _delay_ms(200);
  }
  
  /* Compiler fix */
  return 0;
}
