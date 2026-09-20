// App that reads images from a sd card and outputs them to a 128x128 LCD Screen

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <SD.h>

const int screenWidth = 128;
const int screenHeight = 128;

#define SCLPIN 13
#define MISOPIN 12
#define MOSIPIN 11
#define DISPLAYCS 10
#define RESPIN 9

#define SDCS 7

Adafruit_SSD1351 screen = Adafruit_SSD1351(screenWidth, screenHeight, DISPLAYCS, MOSIPIN, MISOPIN, SCLPIN, RESPIN);
SPISettings settings(SPI_HALF_SPEED, MSBFIRST, SPI_MODE0);
int SD_Buffer[128];
File dir;

// Open directory
File openFolder(char path[]) {
  File dir;

  if (SD.exists(path)) {
    Serial.println("Directory Found");
    dir = SD.open(path);

  } else {
    Serial.println("Not found");
    dir = SD.open("/");
  }

  return dir;
}

// Reads from a text file with list of RGB565 format colour string separated by a comma
// Converts the RGB565 to an integer
uint16_t getColour(File currentFile) {
  // Format
  // '0x': Defines as hex
  // 4 bytes for colour
  // ',': Delimiter

  uint16_t colourHex = 0;

  // Note that this include the string terminator \0
  char colourString[7] = "0x0000";

  for (int i = 0; i < 7; i++) {
    // Check end of file
    if (currentFile.peek() == -1) {
      break;
    }

    char chr = char(currentFile.read());

    if (chr != ',') {
      // File read is in ascii bytes, convert to char
      colourString[i] = chr;
    }
  }

  // Serial.println(colourString);

  // Type cast to unsigned int
  colourHex = (uint16_t)strtoul(colourString, nullptr, 16);
  // Serial.println(colourHex);

  return colourHex;
}

void displayImage(File dir) {
  int x = 0;
  int y = 0;

  SD.begin(SDCS);

  File imgFile = dir.openNextFile();
  uint16_t colour = 0;

  Serial.println(imgFile.name());
  Serial.println("Opening image");

  while (true) {
    // Read 128 bytes at a time
    SPI.beginTransaction(settings);
    digitalWrite(SDCS, LOW);
    // Calling SD.begin() and screen.begin() seems to turn off the other when either one is called
    // This is the only way I could figure out how to get them to work together by explicitly calling them when needed
    // Issue with this approach is that its slower and uses more ram than necessary
    // Possible causes may be an issue with the SD MOSI Pin not releasing properly or a voltage compatibility 
    SD.begin(SDCS); 
    for (int i = 0; i < screenWidth; i++) {
      if (imgFile.peek() != -1) {
        colour = getColour(imgFile);
      }
      SD_Buffer[i] = colour;
    }
    SPI.transfer(0x00); // Flush the MISO Pin
    digitalWrite(SDCS, HIGH);
    SPI.endTransaction();

    // Print to screen
    SPI.beginTransaction(settings);
    digitalWrite(DISPLAYCS, LOW);
    screen.begin();
    for (int x = 0; x < screenWidth; x++) {
      screen.drawPixel(x, y, SD_Buffer[x]);
      digitalWrite(DISPLAYCS, HIGH);
    }
    SPI.endTransaction();

    // Serial.print(x);
    // Serial.print(" ");
    // Serial.println(y);

    y++;

    if (y > screenHeight) {
      imgFile.close();
      break;
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Started");

  // Init SPI functionality
  pinMode(DISPLAYCS, OUTPUT);
  digitalWrite(DISPLAYCS, HIGH);
  pinMode(SDCS, OUTPUT);
  digitalWrite(SDCS, HIGH);

  SPI.begin();

  // Init screen
  SPI.beginTransaction(settings);
  screen.begin();
  screen.fillScreen(0x0000);
  digitalWrite(DISPLAYCS, HIGH);
  SPI.endTransaction();

  // Init SD card
  SPI.beginTransaction(settings);
  if (SD.begin(SDCS)) {
    Serial.println("SD Initalized");
  }
  char path[] = "/colmaps";
  dir = openFolder(path);
  digitalWrite(SDCS, HIGH);
  SPI.endTransaction();

  // Draw Image
  Serial.println("Begin drawing...");
  displayImage(dir);
  Serial.println("Drawing success");
}

void loop() {
  // put your main code here, to run repeatedly:

  displayImage(dir);
  delay(1000);

  screen.begin();
  screen.fillScreen(0x0000);

}
