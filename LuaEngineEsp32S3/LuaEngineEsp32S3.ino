// SD file browser + Lua runtime for ESP32-S3 Super Mini
// TFT ST7735 128x128  ->  HSPI  (MOSI=8, SCLK=7, CS=6, DC=5, RST=44)
// SD Card             ->  FSPI  (MISO=1, SCK=2, MOSI=3, CS=4)
// Buttons: DPAD + A + B + HOME  (INPUT_PULLUP, active LOW)

#include "wifi_server.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <stdarg.h>
#include <math.h>
#include "lua_api.h"

TFT_eSPI     tft    = TFT_eSPI();
TFT_eSprite  canvas = TFT_eSprite(&tft);

SPIClass fspi(1);

// SD card SPI pins
#define SD_MISO  1
#define SD_SCK   2
#define SD_MOSI  3
#define SD_CS    4

// Button pins
#define BTN_UP     9
#define BTN_DOWN  10
#define BTN_LEFT  11
#define BTN_RIGHT 12
#define BTN_A     13
#define BTN_B     43
#define BTN_HOME  44

// File browser layout
#define HEADER_H      14
#define FOOTER_H      13
#define ROW_H         11
#define VISIBLE_ROWS   9

// Max files and folder depth
#define MAX_FILES      48
#define MAX_DEPTH      16

// 5x5 pixel font bitmap — 59 characters starting from ASCII 32
const uint8_t FONT[59][5] PROGMEM = {
  {0x00,0x00,0x00,0x00,0x00},
  {0x04,0x04,0x04,0x00,0x04},
  {0x0A,0x0A,0x00,0x00,0x00},
  {0x0A,0x1F,0x0A,0x1F,0x0A},
  {0x0E,0x1C,0x0E,0x07,0x0E},
  {0x18,0x02,0x04,0x08,0x03},
  {0x0C,0x12,0x0C,0x15,0x12},
  {0x04,0x04,0x00,0x00,0x00},
  {0x02,0x04,0x04,0x04,0x02},
  {0x08,0x04,0x04,0x04,0x08},
  {0x00,0x0A,0x04,0x0A,0x00},
  {0x04,0x04,0x1F,0x04,0x04},
  {0x00,0x00,0x00,0x04,0x08},
  {0x00,0x00,0x1F,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x04},
  {0x01,0x02,0x04,0x08,0x10},
  {0x0E,0x13,0x15,0x19,0x0E},
  {0x04,0x0C,0x04,0x04,0x0E},
  {0x0E,0x01,0x06,0x08,0x1F},
  {0x0E,0x01,0x06,0x01,0x0E},
  {0x11,0x11,0x1F,0x01,0x01},
  {0x1F,0x10,0x1E,0x01,0x1E},
  {0x0F,0x10,0x1F,0x11,0x0E},
  {0x1F,0x01,0x02,0x04,0x04},
  {0x0E,0x11,0x0E,0x11,0x0E},
  {0x0E,0x11,0x0F,0x01,0x0E},
  {0x00,0x04,0x00,0x04,0x00},
  {0x00,0x04,0x00,0x04,0x08},
  {0x02,0x04,0x08,0x04,0x02},
  {0x00,0x1F,0x00,0x1F,0x00},
  {0x08,0x04,0x02,0x04,0x08},
  {0x0E,0x01,0x06,0x00,0x04},
  {0x0E,0x13,0x17,0x10,0x0E},
  {0x0E,0x11,0x1F,0x11,0x11},
  {0x1E,0x11,0x1E,0x11,0x1E},
  {0x0F,0x10,0x10,0x10,0x0F},
  {0x1E,0x11,0x11,0x11,0x1E},
  {0x1F,0x10,0x1F,0x10,0x1F},
  {0x1F,0x10,0x1F,0x10,0x10},
  {0x0F,0x10,0x13,0x11,0x0F},
  {0x11,0x11,0x1F,0x11,0x11},
  {0x1F,0x04,0x04,0x04,0x1F},
  {0x0F,0x01,0x01,0x11,0x0E},
  {0x11,0x12,0x1C,0x12,0x11},
  {0x10,0x10,0x10,0x10,0x1F},
  {0x11,0x1B,0x15,0x11,0x11},
  {0x11,0x19,0x15,0x13,0x11},
  {0x0E,0x11,0x11,0x11,0x0E},
  {0x1E,0x11,0x1E,0x10,0x10},
  {0x0E,0x11,0x15,0x12,0x0D},
  {0x1E,0x11,0x1E,0x12,0x11},
  {0x0F,0x10,0x0E,0x01,0x1E},
  {0x1F,0x04,0x04,0x04,0x04},
  {0x11,0x11,0x11,0x11,0x0E},
  {0x11,0x11,0x11,0x0A,0x04},
  {0x11,0x11,0x15,0x1B,0x11},
  {0x11,0x0A,0x04,0x0A,0x11},
  {0x11,0x0A,0x04,0x04,0x04},
  {0x1F,0x02,0x04,0x08,0x1F},
};

// Draws a single character using the FONT bitmap
void drawChar(int x, int y, char c, uint16_t color, int sz = 1) {
  if (c >= 'a' && c <= 'z') c -= 32;
  if (c < 32 || c > 90) c = 32;
  int idx = c - 32;
  for (int row = 0; row < 5; row++) {
    uint8_t bits = pgm_read_byte(&FONT[idx][row]);
    for (int col = 0; col < 5; col++) {
      if (bits & (0x10 >> col)) {
        if (sz == 1) canvas.drawPixel(x + col, y + row, color);
        else         canvas.fillRect(x + col*sz, y + row*sz, sz, sz, color);
      }
    }
  }
}

// Draws a string character by character
void drawText(int x, int y, const char* str, uint16_t color, int sz = 1) {
  int cx = x;
  while (*str) { drawChar(cx, y, *str, color, sz); cx += (5+1)*sz; str++; }
}

// printf-style text drawing
char _buf[64];
void drawTextF(int x, int y, uint16_t color, int sz, const char* fmt, ...) {
  va_list args; va_start(args, fmt);
  vsnprintf(_buf, sizeof(_buf), fmt, args); va_end(args);
  drawText(x, y, _buf, color, sz);
}

// Button instances
Button btnUp    = {BTN_UP,    false, false, false};
Button btnDown  = {BTN_DOWN,  false, false, false};
Button btnLeft  = {BTN_LEFT,  false, false, false};
Button btnRight = {BTN_RIGHT, false, false, false};
Button btnA     = {BTN_A,     false, false, false};
Button btnB     = {BTN_B,     false, false, false};
Button btnHome  = {BTN_HOME,  false, false, false};

// Hold-to-repeat timing
#define HOLD_DELAY_MS   400
#define HOLD_REPEAT_MS   80
unsigned long holdUpStart=0, lastHoldUp=0, holdDownStart=0, lastHoldDown=0;

// Reads all button states (pressed / held / prev)
void readButtons() {
  Button* btns[] = {&btnUp,&btnDown,&btnLeft,&btnRight,&btnA,&btnB,&btnHome};
  for (int i=0;i<7;i++){
    bool cur=(digitalRead(btns[i]->pin)==LOW);
    btns[i]->pressed=(cur&&!btns[i]->prev);
    btns[i]->held=cur; btns[i]->prev=cur;
  }
}

// Returns true on press or on hold repeat for UP
bool upAction() {
  if (btnUp.pressed){holdUpStart=millis();lastHoldUp=millis();return true;}
  if (btnUp.held){unsigned long n=millis();if(n-holdUpStart>HOLD_DELAY_MS&&n-lastHoldUp>HOLD_REPEAT_MS){lastHoldUp=n;return true;}}
  return false;
}

// Returns true on press or on hold repeat for DOWN
bool downAction() {
  if (btnDown.pressed){holdDownStart=millis();lastHoldDown=millis();return true;}
  if (btnDown.held){unsigned long n=millis();if(n-holdDownStart>HOLD_DELAY_MS&&n-lastHoldDown>HOLD_REPEAT_MS){lastHoldDown=n;return true;}}
  return false;
}

// File and navigation structs
struct FileEntry { char name[24]; uint32_t size; bool isDir; };
struct NavHistory { char path[128]; int sel; int scroll; };

FileEntry fileList[MAX_FILES];
FileEntry tempDirs[MAX_FILES];
FileEntry tempFiles[MAX_FILES];

int  fileCount=0, fileSel=0, fileScroll=0;
NavHistory history[MAX_DEPTH];
int historyDepth=0;
char currentPath[128]="/";
bool sdOK=false;
float errAnim=0;

// HSV to RGB565 — used for rainbow/pulse effects
uint16_t hsv565(float h, float s, float v) {
  h=fmod(h,1.0f); if(h<0) h+=1.0f;
  float r,g,b; int i=(int)(h*6); float f=h*6-i;
  float p=v*(1-s), q=v*(1-f*s), t=v*(1-(1-f)*s);
  switch(i%6){case 0:r=v;g=t;b=p;break;case 1:r=q;g=v;b=p;break;
    case 2:r=p;g=v;b=t;break;case 3:r=p;g=q;b=v;break;
    case 4:r=t;g=p;b=v;break;default:r=v;g=p;b=q;break;}
  return tft.color565((uint8_t)(r*255),(uint8_t)(g*255),(uint8_t)(b*255));
}

// Returns just the filename from a full path string
const char* extractName(const char* fullPath) {
  const char* slash=strrchr(fullPath,'/');
  if(slash&&*(slash+1)!='\0') return slash+1;
  return fullPath;
}

// Loads directory contents into fileList — dirs first, then files
void loadDirectory(const char* path, int restoreSel=0, int restoreScroll=0) {
  fileCount=0; fileSel=restoreSel; fileScroll=restoreScroll;
  strncpy(currentPath,path,127); currentPath[127]='\0';
  int dc=0, fc=0;

  canvas.fillSprite(TFT_BLACK);
  drawText(30,55,"READING...",0x39E7,1);
  canvas.pushSprite(0,0);

  File dir=SD.open(path);
  if(!dir||!dir.isDirectory()){if(dir)dir.close();return;}

  File f=dir.openNextFile();
  while(f){
    const char* fname = extractName(f.name());
    bool isDir        = f.isDirectory();
    uint32_t fsize    = isDir ? 0 : (uint32_t)f.size();

    f = dir.openNextFile();

    if(strlen(fname)==0 || fname[0]=='.') continue;

    if(isDir && dc<MAX_FILES){
      strncpy(tempDirs[dc].name,fname,23); tempDirs[dc].name[23]='\0';
      tempDirs[dc].size=0; tempDirs[dc].isDir=true; dc++;
    } else if(!isDir && fc<MAX_FILES){
      strncpy(tempFiles[fc].name,fname,23); tempFiles[fc].name[23]='\0';
      tempFiles[fc].size=fsize; tempFiles[fc].isDir=false; fc++;
    }
  }
  dir.close();

  // Merge: directories first, then files
  for(int i=0;i<dc&&fileCount<MAX_FILES;i++) fileList[fileCount++]=tempDirs[i];
  for(int i=0;i<fc&&fileCount<MAX_FILES;i++) fileList[fileCount++]=tempFiles[i];
  if(fileSel>=fileCount) fileSel=max(0,fileCount-1);
  if(fileScroll>fileSel) fileScroll=fileSel;
}

// Saves current location to history and enters the selected folder
void enterFolder() {
  if(fileCount==0||!fileList[fileSel].isDir) return;
  if(historyDepth<MAX_DEPTH){
    strncpy(history[historyDepth].path,currentPath,127);
    history[historyDepth].sel=fileSel;
    history[historyDepth].scroll=fileScroll;
    historyDepth++;
  }
  char newPath[128];
  if(strcmp(currentPath,"/")==0) snprintf(newPath,sizeof(newPath),"/%s",fileList[fileSel].name);
  else snprintf(newPath,sizeof(newPath),"%s/%s",currentPath,fileList[fileSel].name);
  loadDirectory(newPath);
}

// Goes back one level in the navigation history
void goBack() {
  if(historyDepth==0) return;
  historyDepth--;
  loadDirectory(history[historyDepth].path,history[historyDepth].sel,history[historyDepth].scroll);
}

// Jumps straight to the root directory
void goHome() { historyDepth=0; loadDirectory("/"); }

// Animated error screen shown when SD card is missing
void renderSDError() {
  canvas.fillSprite(TFT_BLACK);
  errAnim+=0.07f;
  float pulse=0.3f+sinf(errAnim)*0.3f;
  uint16_t rc=hsv565(0.0f,1.0f,pulse);
  for(int i=0;i<3;i++) canvas.drawRect(i,i,128-i*2,128-i*2,rc);
  int sx=44,sy=20,sw=40,sh=48;
  canvas.fillRect(sx,sy,sw,sh,tft.color565(25,25,35));
  canvas.fillRect(sx,sy,12,12,TFT_BLACK);
  canvas.fillTriangle(sx,sy+12,sx+12,sy,sx+12,sy+12,tft.color565(25,25,35));
  canvas.drawRect(sx,sy,sw,sh,rc);
  canvas.drawLine(sx,sy+12,sx+12,sy,rc);
  int cx=sx+sw/2, cy=sy+sh/2;
  // Draw X mark over the card icon
  canvas.drawLine(cx-10,cy-10,cx+10,cy+10,0xF800);
  canvas.drawLine(cx-10,cy-9, cx+10,cy+11,0xF800);
  canvas.drawLine(cx+10,cy-10,cx-10,cy+10,0xF800);
  canvas.drawLine(cx+10,cy-9, cx-10,cy+11,0xF800);
  drawText(18,76,"NO SD CARD",0xFFFF,1);
  drawText(10,88,"CHECK CONNECTION",hsv565(0.12f,1,1),1);
  drawText(28,100,"FAT32 / SPI",hsv565(0.12f,1,0.7f),1);
  canvas.fillRoundRect(20,111,88,13,3,tft.color565(30,0,0));
  canvas.drawRoundRect(20,111,88,13,3,rc);
  drawText(22,115,"RESTART ESP32",0xFFFF,1);
  canvas.pushSprite(0,0);
}

// Draws the file browser — header, file rows, scrollbar, footer
void renderBrowser() {
  canvas.fillSprite(TFT_BLACK);

  // Header bar
  canvas.fillRect(0,0,128,HEADER_H,tft.color565(0,15,35));
  canvas.drawFastHLine(0,HEADER_H,128,0x02DF);

  // Show current path (truncate if too long)
  char dispPath[20];
  int plen=strlen(currentPath);
  if(plen>18) snprintf(dispPath,sizeof(dispPath),"~%s",currentPath+plen-17);
  else strncpy(dispPath,currentPath,sizeof(dispPath));
  drawText(2,4,dispPath,0x07FF,1);

  // File counter top-right
  if(fileCount>0) drawTextF(90,4,0x39E7,1,"%d/%d",fileSel+1,fileCount);
  else drawText(104,4,"0/0",0x39E7,1);

  // File rows
  int visEnd=min(fileCount,fileScroll+VISIBLE_ROWS);
  for(int i=fileScroll;i<visEnd;i++){
    int row=i-fileScroll;
    int y=HEADER_H+1+row*ROW_H;
    bool sel=(i==fileSel);
    if(sel){canvas.fillRect(0,y,123,ROW_H,tft.color565(0,25,55));canvas.drawRect(0,y,123,ROW_H,0x04FF);}

    if(fileList[i].isDir){
      // Folder icon
      canvas.fillRect(2,y+3,9,6,hsv565(0.12f,0.9f,0.55f));
      canvas.fillRect(2,y+1,5,3,hsv565(0.12f,0.9f,0.55f));
      uint16_t nc=sel?0xFFFF:hsv565(0.12f,0.4f,1.0f);
      char dn[14]; strncpy(dn,fileList[i].name,13); dn[13]='\0';
      drawText(14,y+3,dn,nc,1);
      drawText(116,y+3,">",sel?0xFFFF:0x4208,1);
    } else {
      bool isLua=(strstr(fileList[i].name,".lua")!=nullptr||strstr(fileList[i].name,".LUA")!=nullptr);
      if(isLua){
        // Lua file icon with "L" label
        canvas.fillRect(2,y+1,8,9,tft.color565(0,25,50));
        canvas.drawRect(2,y+1,8,9,0x07FF);
        drawText(3,y+3,"L",0x07FF,1);
      } else {
        // Generic file icon with lines
        canvas.fillRect(2,y+1,8,9,tft.color565(20,20,40));
        canvas.drawRect(2,y+1,8,9,0x4208);
        canvas.drawFastHLine(3,y+3,6,0x2104);
        canvas.drawFastHLine(3,y+5,6,0x2104);
        canvas.drawFastHLine(3,y+7,4,0x2104);
      }
      uint16_t nc=sel?0xFFFF:0x7BEF;
      char fn[13]; strncpy(fn,fileList[i].name,12); fn[12]='\0';
      drawText(14,y+3,fn,nc,1);
      // File size on the right
      char szBuf[8];
      if(fileList[i].size<1024) snprintf(szBuf,sizeof(szBuf),"%luB",(unsigned long)fileList[i].size);
      else snprintf(szBuf,sizeof(szBuf),"%luK",(unsigned long)(fileList[i].size/1024));
      int sw2=strlen(szBuf)*6;
      drawText(122-sw2,y+3,szBuf,sel?0x39E7:0x2945,1);
    }
  }

  if(fileCount==0) drawText(22,55,"EMPTY FOLDER",0x4208,1);

  // Scrollbar
  if(fileCount>VISIBLE_ROWS){
    int trackH=128-HEADER_H-FOOTER_H;
    int barH=max(4,trackH*VISIBLE_ROWS/fileCount);
    int barY=HEADER_H+trackH*fileScroll/fileCount;
    canvas.fillRect(124,HEADER_H,4,trackH,tft.color565(10,10,20));
    canvas.fillRoundRect(124,barY,4,barH,2,0x04BF);
  }

  // Footer with button hints
  canvas.fillRect(0,128-FOOTER_H,128,FOOTER_H,tft.color565(0,10,20));
  canvas.drawFastHLine(0,128-FOOTER_H,128,0x3166);

  if(historyDepth>0){
    drawText(2,128-FOOTER_H+4,"A=OK",hsv565(0.35f,1,1),1);
    drawText(42,128-FOOTER_H+4,"B=BACK",hsv565(0.0f,0.8f,1),1);
    drawTextF(100,128-FOOTER_H+4,0x4208,1,"D%d",historyDepth);
  } else {
    drawText(2,128-FOOTER_H+4,"A=OPEN",hsv565(0.35f,1,1),1);
    drawText(60,128-FOOTER_H+4,"H=WiFi",tft.color565(0,160,220),1);
  }

  canvas.pushSprite(0,0);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== SD Explorer + Lua ===");
  Serial.printf("PSRAM: %u bytes\n", ESP.getPsramSize());
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());

  // Set all buttons as input with pull-up
  pinMode(BTN_UP,INPUT_PULLUP);   pinMode(BTN_DOWN,INPUT_PULLUP);
  pinMode(BTN_LEFT,INPUT_PULLUP); pinMode(BTN_RIGHT,INPUT_PULLUP);
  pinMode(BTN_A,INPUT_PULLUP);    pinMode(BTN_B,INPUT_PULLUP);
  pinMode(BTN_HOME,INPUT_PULLUP);

  // Init display and canvas sprite
  tft.init(); tft.setRotation(3); tft.fillScreen(TFT_BLACK);
  canvas.setColorDepth(16);
  canvas.createSprite(128, 128);

  // Splash screen while things start up
  canvas.fillSprite(TFT_BLACK);
  drawText(16,44,"SD EXPLORER",0x07FF,1);
  drawText(28,56,"+ LUA API",hsv565(0.55f,1,1),1);
  drawText(22,68,"STARTING...",0x39E7,1);
  canvas.pushSprite(0,0);
  delay(400);

  registerLuaAPI();

  // Init SD on FSPI, then bump speed to 40MHz
  fspi.begin(SD_SCK,SD_MISO,SD_MOSI,SD_CS);
  if(!SD.begin(SD_CS,fspi,400000)){
    Serial.println("SD: FAILED");
    sdOK=false;
  } else {
    fspi.setFrequency(40000000);
    Serial.println("SD: OK @ 40MHz");
    sdOK=true;
    loadDirectory("/");
    Serial.printf("Total entries: %d\n",fileCount);
  }

  if(sdOK) renderBrowser();
}

void loop() {
  readButtons();

  // Show SD error screen if card is not mounted
  if(!sdOK){ renderSDError(); delay(16); return; }

  bool changed=false;

  // Navigate up/down with hold repeat
  if(upAction()&&fileSel>0){
    fileSel--; if(fileSel<fileScroll) fileScroll=fileSel; changed=true;
  }
  if(downAction()&&fileSel<fileCount-1){
    fileSel++; if(fileSel>=fileScroll+VISIBLE_ROWS) fileScroll=fileSel-VISIBLE_ROWS+1; changed=true;
  }

  if(btnA.pressed){
    if(fileCount>0&&fileList[fileSel].isDir){
      enterFolder();
    } else if(fileCount>0&&!fileList[fileSel].isDir){
      const char* name=fileList[fileSel].name;
      bool isLua=(strstr(name,".lua")!=nullptr||strstr(name,".LUA")!=nullptr);

      if(isLua){
        char scriptPath[128];
        if(strcmp(currentPath,"/")==0) snprintf(scriptPath,sizeof(scriptPath),"/%s",name);
        else snprintf(scriptPath,sizeof(scriptPath),"%s/%s",currentPath,name);

        // Don't clear the canvas here — runLuaScript needs it to show the loading screen
        runLuaScript(scriptPath);

        // Recreate sprite in case the script messed with it, then redraw browser
        canvas.deleteSprite();
        canvas.createSprite(128, 128);
        renderBrowser();
        return;
      } else {
        Serial.printf("File: %s/%s\n",currentPath,name);
      }
    }
    changed=true;
  }

  if(btnB.pressed)    { goBack();  changed=true; }
  if(btnHome.pressed) {
    if(historyDepth == 0) {
      // At root -> launch WiFi file server
      runWifiServer();
      renderBrowser();
      return;
    } else {
      goHome();
      changed = true;
    }
  }

  // Left/right jump a full page
  if(btnLeft.pressed){
    fileSel=max(0,fileSel-VISIBLE_ROWS);
    fileScroll=max(0,fileScroll-VISIBLE_ROWS); changed=true;
  }
  if(btnRight.pressed){
    fileSel=min(fileCount-1,fileSel+VISIBLE_ROWS);
    if(fileSel>=fileScroll+VISIBLE_ROWS) fileScroll=fileSel-VISIBLE_ROWS+1; changed=true;
  }

  if(changed) renderBrowser();
  delay(16);
}
