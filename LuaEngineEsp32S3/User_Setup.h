// TFT_eSPI config for ESP32-S3 Super Mini with ST7735 128x128 screen

#define USER_SETUP_LOADED

#define ST7735_DRIVER
#define TFT_WIDTH  128
#define TFT_HEIGHT 128
#define ST7735_GREENTAB3
#define TFT_RGB_ORDER TFT_BGR

// Screen SPI pins (HSPI)
#define USE_HSPI_PORT
#define TFT_CS    6
#define TFT_DC     5
#define TFT_RST    -1
#define TFT_MOSI  8
#define TFT_SCLK  7

#define SPI_FREQUENCY       27000000
#define SPI_READ_FREQUENCY  20000000
#define LOAD_FONT1
