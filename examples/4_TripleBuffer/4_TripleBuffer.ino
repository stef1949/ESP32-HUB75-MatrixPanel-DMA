/**
 Example uses the following configuration:  mxconfig.triple_buff = true;
 to enable triple buffering, which means display->flipDMABuffer(); is required.

 Bounce squares around the screen, doing the re-drawing in the background back-buffer.

 Triple buffering provides smoother animation than double buffering by having three frame 
 buffers instead of two. While one buffer is being displayed and another is being prepared 
 for display, a third buffer can be drawn to without any synchronization concerns.

 This allows for more complex rendering operations without affecting the display output,
 reducing tearing and providing ultra-smooth animations at the cost of using three times
 the frame buffer memory.

 Please note that triple buffering requires more memory and may not be suitable for all
 ESP32 configurations. Monitor your heap usage carefully.

 The buffer rotation works as follows: 0 -> 1 -> 2 -> 0 -> 1 -> 2 ...

**/


#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <array>

MatrixPanel_I2S_DMA *display = nullptr;

constexpr std::size_t color_num = 5;
using colour_arr_t = std::array<uint16_t, color_num>;

uint16_t myDARK, myWHITE, myRED, myGREEN, myBLUE;
colour_arr_t colours;

struct Square
{
  float xpos, ypos;
  float velocityx;
  float velocityy;
  boolean xdir, ydir;
  uint16_t square_size;
  uint16_t colour;
};

const int numSquares = 30; // More squares to demonstrate triple buffering advantage
Square Squares[numSquares];

void setup()
{
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);
  delay(200);

  Serial.println("...Starting Display with Triple Buffer");
  HUB75_I2S_CFG mxconfig;
  mxconfig.triple_buff = true; // <------------- Turn on triple buffer
  //mxconfig.double_buff = true; // <------------- Compare with double buffer performance

  // OK, now we can create our matrix object
  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();  // setup display with pins as pre-defined in the library

  myDARK = display->color565(64, 64, 64);
  myWHITE = display->color565(192, 192, 192);
  myRED = display->color565(255, 0, 0);
  myGREEN = display->color565(0, 255, 0);
  myBLUE = display->color565(0, 0, 255);

  colours = {{ myDARK, myWHITE, myRED, myGREEN, myBLUE }};

  // Create some random squares
  for (int i = 0; i < numSquares; i++)
  {
    Squares[i].square_size = random(2,8);
    Squares[i].xpos = random(0, display->width() - Squares[i].square_size);
    Squares[i].ypos = random(0, display->height() - Squares[i].square_size);
    Squares[i].velocityx = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    Squares[i].velocityy = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

    int random_num = random(5);
    Squares[i].colour = colours[random_num];
  }
}

void loop()
{

  // Flip to next buffer in the triple buffer rotation (0->1->2->0)
  display->flipDMABuffer(); 

  // SUPER IMPORTANT: Wait at least long enough to ensure that a "frame" has been displayed on the LED Matrix Panel before the next flip!
  delay(1000/display->calculated_refresh_rate);  

  // Now clear the back-buffer we are drawing to.
  display->clearScreen();   

  // More complex drawing routine to demonstrate triple buffering benefits
  // This simulates a more intensive rendering workload with actual computation
  // Simulate complex rendering operations with a small CPU-bound loop
  volatile uint32_t dummy = 0;
  for (int j = 0; j < 1000; ++j) {
    dummy += j * j;
  }

  for (int i = 0; i < numSquares; i++)
  {
    // Draw rect and then calculate
    display->fillRect(Squares[i].xpos, Squares[i].ypos, Squares[i].square_size, Squares[i].square_size, Squares[i].colour);

    // Add some trail effects to make rendering more intensive
    if (i % 3 == 0) {
      display->drawPixel(Squares[i].xpos - 1, Squares[i].ypos, Squares[i].colour);
      display->drawPixel(Squares[i].xpos + Squares[i].square_size, Squares[i].ypos, Squares[i].colour);
    }

    if (Squares[i].square_size + Squares[i].xpos >= display->width()) {
      Squares[i].velocityx *= -1;
    } else if (Squares[i].xpos <= 0) {
      Squares[i].velocityx = abs (Squares[i].velocityx);
    }

    if (Squares[i].square_size + Squares[i].ypos >= display->height()) {
      Squares[i].velocityy *= -1;
    } else if (Squares[i].ypos <= 0) {
      Squares[i].velocityy = abs (Squares[i].velocityy);
    }

    Squares[i].xpos += Squares[i].velocityx;
    Squares[i].ypos += Squares[i].velocityy;
  }
  
  // Simulate additional rendering work
  delay(5);
}