#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include "lua_api.h"

// Globals from main sketch
extern TFT_eSPI    tft;
extern TFT_eSprite canvas;
extern Button btnUp, btnDown, btnLeft, btnRight, btnA, btnB, btnHome;
extern void readButtons();
extern void drawText(int x, int y, const char* str, uint16_t color, int sz);
extern void drawTextF(int x, int y, uint16_t color, int sz, const char* fmt, ...);
extern char currentPath[128];

#define WIFI_CFG_PATH   "/wifi.cfg"
#define WIFI_TIMEOUT_MS  15000

static char wifiSSID[64] = "";
static char wifiPASS[64] = "";
static bool wifiCfgLoaded = false;
static WebServer* webServer = nullptr;

// Shortcut to build a 565 color
static uint16_t C(uint8_t r, uint8_t g, uint8_t b) { return tft.color565(r,g,b); }

// Color palette
#define COL_BG        C(10,  10,  20)
#define COL_KEY_NORM  C(30,  30,  60)
#define COL_KEY_SEL   C(0,   90, 200)
#define COL_KEY_SPEC  C(20,  20,  45)
#define COL_KEY_OK    C(0,   80,  30)
#define COL_KEY_DEL   C(80,  15,  15)
#define COL_KEY_CAP   C(80,  70,   0)
#define COL_BORDER    C(50,  50, 100)
#define COL_SEL_BDR   C(80, 160, 255)
#define COL_TXT       C(200,200,230)
#define COL_TXT_SEL   C(255,255,255)
#define COL_TXT_DIM   C(100,100,140)
#define COL_INPUT_BG  C(15,  20,  45)
#define COL_INPUT_BDR C(0,   80, 180)
#define COL_HINT      C(60,  60,  90)
#define COL_NL        C(220,200,  0)

// Keyboard layout — fits 128x128
#define KB_KEY_W   11
#define KB_KEY_H   10
#define KB_GAP      1
#define KB_STEP    (KB_KEY_W + KB_GAP)
#define KB_START_Y  22

static const char* ROW_LO[] = { "qwertyuiop", "asdfghjkl", "zxcvbnm" };
static const char* ROW_HI[] = { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
static const char* ROW_NUM  =   "1234567890";

// Returns number of keys in each row
static int kbRowLen(int row) {
    if (row == 0) return 10;
    if (row == 1) return 10;
    if (row == 2) return 7;
    if (row == 3) return 10;
    if (row == 4) return 3;
    if (row == 5) return 1;
    return 0;
}

static int kbRowY(int row) {
    return KB_START_Y + row * (KB_KEY_H + KB_GAP);
}

// Centers the row horizontally on the 128px screen
static int kbRowX(int row) {
    int n = kbRowLen(row);
    int total = n * KB_KEY_W + (n - 1) * KB_GAP;
    return (128 - total) / 2;
}

static int kbKeyX(int row, int col) {
    return kbRowX(row) + col * KB_STEP;
}

// Returns the character for a given key position
static char kbChar(int row, int col, bool shift) {
    if (row == 0) return shift ? ROW_HI[0][col] : ROW_LO[0][col];
    if (row == 1 && col < 9) return shift ? ROW_HI[1][col] : ROW_LO[1][col];
    if (row == 1 && col == 9) return 0;
    if (row == 2) return shift ? ROW_HI[2][col] : ROW_LO[2][col];
    if (row == 3) return ROW_NUM[col];
    return 0;
}

// Draws the keyboard frame and all keys to canvas
static void kbDraw(const char* label, const char* buf, int len,
                   bool masked, int sRow, int sCol, bool shift) {

    canvas.fillSprite(COL_BG);

    // Input field at the top
    canvas.fillRect(0, 0, 128, 21, COL_INPUT_BG);
    canvas.drawFastHLine(0, 20, 128, COL_INPUT_BDR);
    drawText(2, 2, label, COL_TXT_DIM, 1);
    const char* show = buf;
    if (len > 19) show = buf + len - 19;
    if (masked) {
        char m[21]; int ml = strlen(show);
        for (int i = 0; i < ml; i++) m[i] = '*'; m[ml] = '\0';
        drawText(2, 12, m, COL_TXT, 1);
        canvas.drawFastVLine(2 + ml*6, 11, 7, COL_SEL_BDR);
    } else {
        drawText(2, 12, show, COL_TXT, 1);
        canvas.drawFastVLine(2 + (int)strlen(show)*6, 11, 7, COL_SEL_BDR);
    }

    // Draw letter/number rows
    for (int row = 0; row <= 3; row++) {
        int n = kbRowLen(row);
        int y = kbRowY(row);
        for (int col = 0; col < n; col++) {
            bool sel  = (sRow == row && sCol == col);
            bool isNn = (row == 1 && col == 9);
            uint16_t bg  = sel ? COL_KEY_SEL  : COL_KEY_NORM;
            uint16_t bdr = sel ? COL_SEL_BDR  : COL_BORDER;
            uint16_t fg  = sel ? COL_TXT_SEL  : (isNn ? COL_NL : COL_TXT);
            int x = kbKeyX(row, col);
            canvas.fillRect(x, y, KB_KEY_W, KB_KEY_H, bg);
            canvas.drawRect(x, y, KB_KEY_W, KB_KEY_H, bdr);
            char lbl[3];
            if (isNn) { lbl[0]='n'; lbl[1]='~'; lbl[2]='\0'; }
            else if (row == 1 && col < 9 && shift) { lbl[0]=ROW_HI[1][col]; lbl[1]='\0'; }
            else if (row == 0) { lbl[0]=shift?ROW_HI[0][col]:ROW_LO[0][col]; lbl[1]='\0'; }
            else if (row == 2) { lbl[0]=shift?ROW_HI[2][col]:ROW_LO[2][col]; lbl[1]='\0'; }
            else if (row == 3) { lbl[0]=ROW_NUM[col]; lbl[1]='\0'; }
            else { lbl[0]=ROW_LO[1][col]; lbl[1]='\0'; }
            int tw = (strlen(lbl)==1) ? 5 : 11;
            int tx = x + (KB_KEY_W - tw) / 2;
            int ty = y + (KB_KEY_H - 5) / 2;
            drawText(tx, ty, lbl, fg, 1);
        }
    }

    // Special keys row: SHIFT, SPACE, BACKSPACE
    {
        int y = kbRowY(4);
        struct { int x; int w; const char* lbl; uint16_t bg; uint16_t fg; } sp[] = {
            { 3,  38, shift?"CAPS*":"CAPS", shift?COL_KEY_CAP:COL_KEY_SPEC, shift?C(255,220,0):COL_TXT_DIM },
            { 43, 40, "SPACE",              COL_KEY_SPEC,                   COL_TXT_DIM                    },
            { 85, 38, "DEL",                COL_KEY_DEL,                    C(255,110,110)                  },
        };
        for (int c = 0; c < 3; c++) {
            bool sel = (sRow == 4 && sCol == c);
            uint16_t bg  = sel ? COL_KEY_SEL  : sp[c].bg;
            uint16_t bdr = sel ? COL_SEL_BDR  : COL_BORDER;
            uint16_t fg  = sel ? COL_TXT_SEL  : sp[c].fg;
            canvas.fillRect(sp[c].x, y, sp[c].w, KB_KEY_H, bg);
            canvas.drawRect(sp[c].x, y, sp[c].w, KB_KEY_H, bdr);
            int tw = strlen(sp[c].lbl) * 6 - 1;
            int tx = sp[c].x + (sp[c].w - tw) / 2;
            drawText(tx, y + (KB_KEY_H-5)/2, sp[c].lbl, fg, 1);
        }
    }

    // OK / confirm button
    {
        int y   = kbRowY(5);
        bool sel = (sRow == 5 && sCol == 0);
        uint16_t bg  = sel ? C(0,150,50) : COL_KEY_OK;
        uint16_t bdr = sel ? C(0,255,100): C(0,120,40);
        canvas.fillRect(20, y, 88, KB_KEY_H+1, bg);
        canvas.drawRect(20, y, 88, KB_KEY_H+1, bdr);
        drawText(20+(88-12)/2, y+(KB_KEY_H-4)/2, "OK", COL_TXT_SEL, 1);
    }

    canvas.pushSprite(0, 0);
}

// Keyboard input loop — returns false if cancelled
static bool runKeyboard(const char* label, char* outBuf, int maxLen, bool masked = false) {
    char buf[64] = ""; int len = 0;
    int  sRow = 0, sCol = 0;
    bool shift = false, done = false, cancelled = false;

    while (!done && !cancelled) {
        readButtons();
        int n = kbRowLen(sRow);

        if (btnUp.pressed) {
            sRow--; if (sRow < 0) sRow = 5;
            int nn = kbRowLen(sRow); if (sCol >= nn) sCol = nn - 1;
        }
        if (btnDown.pressed) {
            sRow++; if (sRow > 5) sRow = 0;
            int nn = kbRowLen(sRow); if (sCol >= nn) sCol = nn - 1;
        }
        if (btnLeft.pressed)  { sCol--; if (sCol < 0)  sCol = n - 1; }
        if (btnRight.pressed) { sCol++; if (sCol >= n)  sCol = 0; }

        if (btnA.pressed) {
            if (sRow == 5) {
                done = true;
            } else if (sRow == 4) {
                if (sCol == 0) { shift = !shift; }
                else if (sCol == 1 && len < maxLen-1) { buf[len++]=' '; buf[len]='\0'; }
                else if (sCol == 2 && len > 0) { buf[--len]='\0'; }
            } else if (sRow == 1 && sCol == 9) {
                // Insert ñ (UTF-8 two-byte)
                if (len + 2 < maxLen) {
                    buf[len++] = '\xC3';
                    buf[len++] = shift ? '\x91' : '\xB1';
                    buf[len]   = '\0';
                    if (shift) shift = false;
                }
            } else {
                char c = kbChar(sRow, sCol, shift);
                if (c && len < maxLen - 1) {
                    buf[len++] = c; buf[len] = '\0';
                    if (shift) shift = false;
                }
            }
        }
        if (btnHome.pressed) done = true;
        if (btnB.pressed)    cancelled = true;

        kbDraw(label, buf, len, masked, sRow, sCol, shift);
        delay(25);
    }

    if (!cancelled) { strncpy(outBuf, buf, maxLen-1); outBuf[maxLen-1]='\0'; return true; }
    return false;
}

// Reads SSID and password from /wifi.cfg on SD
static void loadWifiCfg() {
    if (wifiCfgLoaded) return;
    File f = SD.open(WIFI_CFG_PATH, FILE_READ);
    if (!f) return;
    int i = 0;
    // Read SSID line
    while (f.available() && i < 63) { char c=f.read(); if(c=='\n'||c=='\r')break; wifiSSID[i++]=c; } wifiSSID[i]='\0';
    // Skip line endings between SSID and password
    while (f.available()) { char c=f.peek(); if(c!='\n'&&c!='\r')break; f.read(); }
    // Read password line
    i=0;
    while (f.available() && i < 63) { char c=f.read(); if(c=='\n'||c=='\r')break; wifiPASS[i++]=c; } wifiPASS[i]='\0';
    f.close();
    wifiCfgLoaded=true;
}

// Saves SSID and password to /wifi.cfg
static void saveWifiCfg() {
    SD.remove(WIFI_CFG_PATH);
    File f=SD.open(WIFI_CFG_PATH,FILE_WRITE); if(!f)return;
    f.println(wifiSSID); f.println(wifiPASS); f.close();
}

// Menu to enter/edit WiFi credentials before connecting
static bool wifiCredentialsMenu() {
    loadWifiCfg();
    int sel=0; bool changed=true;
    while (true) {
        readButtons();
        if (btnUp.pressed)   { sel=(sel+3)%4; changed=true; }
        if (btnDown.pressed) { sel=(sel+1)%4; changed=true; }
        if (btnB.pressed)    return false;
        if (btnA.pressed) {
            if      (sel==0) { char t[64]=""; if(runKeyboard("Network SSID:",t,64,false)) strncpy(wifiSSID,t,63); changed=true; }
            else if (sel==1) { char t[64]=""; if(runKeyboard("Password:",t,64,true))  strncpy(wifiPASS,t,63);  changed=true; }
            else if (sel==2) { saveWifiCfg(); return true; }
            else             { return false; }
        }
        if (changed) {
            canvas.fillSprite(COL_BG);
            canvas.fillRect(0,0,128,14,C(0,15,40));
            canvas.drawFastHLine(0,14,128,C(0,80,180));
            drawText(18,4,"WIFI CONFIG",C(0,180,255),1);

            bool s0=(sel==0);
            canvas.fillRect(2,18,124,22,s0?C(0,25,60):C(12,12,28));
            canvas.drawRect(2,18,124,22,s0?C(0,120,255):C(30,30,60));
            drawText(6,21,"SSID:",C(80,140,220),1);
            char sb[22]; strncpy(sb,wifiSSID[0]?wifiSSID:"(empty)",21); sb[21]='\0';
            drawText(6,30,sb,0xFFFF,1);

            bool s1=(sel==1);
            canvas.fillRect(2,43,124,22,s1?C(0,25,60):C(12,12,28));
            canvas.drawRect(2,43,124,22,s1?C(0,120,255):C(30,30,60));
            drawText(6,46,"PASSWORD:",C(80,140,220),1);
            char pb[22]=""; int pl=strlen(wifiPASS);
            if(pl==0) strcpy(pb,"(empty)");
            else { int sh=min(pl,19); for(int i=0;i<sh;i++) pb[i]='*'; pb[sh]='\0'; }
            drawText(6,55,pb,0xFFFF,1);

            bool s2=(sel==2);
            canvas.fillRect(6,69,116,15,s2?C(0,100,0):C(0,40,0));
            canvas.drawRect(6,69,116,15,s2?C(0,220,0):C(0,70,0));
            drawText(6+(116-strlen("CONNECT AND START")*6)/2,74,"CONNECT AND START",s2?0xFFFF:C(0,180,0),1);

            bool s3=(sel==3);
            canvas.fillRect(6,87,116,15,s3?C(80,0,0):C(30,0,0));
            canvas.drawRect(6,87,116,15,s3?C(220,50,50):C(70,0,0));
            drawText(6+(116-strlen("CANCEL")*6)/2,92,"CANCEL",s3?0xFFFF:C(180,60,60),1);

            drawText(10,108,"UP/DOWN=move  A=select",COL_HINT,1);
            if(wifiCfgLoaded&&wifiSSID[0]) drawText(20,118,"config saved on SD",C(0,100,60),1);
            canvas.pushSprite(0,0); changed=false;
        }
        delay(16);
    }
}

// Connects to WiFi and shows progress on screen
static bool connectWifi() {
    WiFi.mode(WIFI_STA); WiFi.begin(wifiSSID,wifiPASS);
    unsigned long t0=millis(); int dots=0;
    while (WiFi.status()!=WL_CONNECTED) {
        if (millis()-t0>WIFI_TIMEOUT_MS) {
            canvas.fillSprite(COL_BG);
            drawText(14,50,"NO CONNECTION",C(220,60,60),1);
            drawText(6,62,"Check SSID/PASS",C(180,100,100),1);
            canvas.pushSprite(0,0); delay(2000);
            WiFi.disconnect(true); WiFi.mode(WIFI_OFF); return false;
        }
        canvas.fillSprite(COL_BG);
        canvas.fillRect(0,0,128,14,C(0,15,40));
        drawText(22,4,"CONNECTING...",C(0,180,255),1);
        drawText(4,26,"Network:",COL_TXT_DIM,1); drawText(4,36,wifiSSID,0xFFFF,1);
        int bw=(int)(122.0f*(millis()-t0)/WIFI_TIMEOUT_MS);
        canvas.fillRect(3,52,122,7,C(15,15,35)); canvas.fillRect(3,52,bw,7,C(0,100,200));
        canvas.drawRect(3,52,122,7,C(0,60,120));
        char db[5]=""; for(int i=0;i<dots%4;i++) db[i]='.'; db[dots%4]='\0';
        drawText(60,64,db,C(0,200,255),1); dots++;
        canvas.pushSprite(0,0); delay(300);
    }
    // Show IP once connected
    canvas.fillSprite(COL_BG);
    canvas.fillRect(0,0,128,14,C(0,30,0));
    drawText(28,4,"CONNECTED!",C(0,220,0),1);
    char ip[32]; snprintf(ip,sizeof(ip),"%s",WiFi.localIP().toString().c_str());
    drawText(4,26,"Server IP:",COL_TXT_DIM,1); drawText(4,38,ip,C(0,200,255),1);
    drawText(4,54,"Open in your browser:",COL_TXT_DIM,1);
    drawText(4,64,"http://",C(0,160,255),1); drawText(46,64,ip,C(0,160,255),1);
    drawText(4,82,"B = stop server",COL_HINT,1);
    canvas.pushSprite(0,0); delay(2500); return true;
}

// Returns MIME type based on file extension
static const char* getMime(const char* n){const char* e=strrchr(n,'.');if(!e)return"application/octet-stream";if(strcasecmp(e,".lua")==0||strcasecmp(e,".txt")==0)return"text/plain";if(strcasecmp(e,".json")==0)return"application/json";if(strcasecmp(e,".html")==0||strcasecmp(e,".htm")==0)return"text/html";if(strcasecmp(e,".png")==0)return"image/png";if(strcasecmp(e,".jpg")==0||strcasecmp(e,".jpeg")==0)return"image/jpeg";return"application/octet-stream";}

// HTML-escapes a string for safe output
static String he(const char* s){String o;while(*s){if(*s=='<')o+="&lt;";else if(*s=='>')o+="&gt;";else if(*s=='&')o+="&amp;";else if(*s=='"')o+="&quot;";else o+=*s;s++;}return o;}

// Gets 'path' query param from request, defaults to "/"
static String getP(){if(webServer->hasArg("path")){String p=webServer->arg("path");if(!p.length())p="/";return p;}return "/";}

// Serves the SD directory browser page
static void handleRoot(){
    String path=getP();File dir=SD.open(path.c_str());
    if(!dir||!dir.isDirectory()){webServer->send(404,"text/plain","Not found");if(dir)dir.close();return;}
    String h="<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>SD Explorer</title><style>body{font-family:monospace;background:#0a0a14;color:#ccc;margin:0;padding:12px}h2{color:#4af;margin:0 0 8px}a{color:#8cf;text-decoration:none}a:hover{color:#fff}.bar{background:#111;border:1px solid #223;padding:6px 8px;margin-bottom:8px;border-radius:4px}.entry{display:flex;align-items:center;padding:4px 8px;border-bottom:1px solid #1a1a2e}.entry:hover{background:#0d1a33}.icon{width:20px;color:#888;flex-shrink:0}.name{flex:1}.size{color:#555;font-size:.85em;margin-left:8px}.actions{margin-left:10px;display:flex;gap:6px}.btn{background:#1a1a33;border:1px solid #334;color:#aac;padding:2px 8px;border-radius:3px;cursor:pointer;font-size:.8em}.btn:hover{background:#2a2a55;color:#fff}.btn.del{border-color:#522;color:#f88}.btn.del:hover{background:#400}.upload{margin-top:12px;padding:8px;background:#0d1a0d;border:1px solid #1a3a1a;border-radius:4px}input[type=file]{color:#aaa}input[type=submit]{background:#1a3a1a;border:1px solid #2a5a2a;color:#8f8;padding:4px 12px;border-radius:3px;cursor:pointer}input[type=submit]:hover{background:#2a5a2a;color:#fff}.crumb{color:#678;font-size:.85em;margin-bottom:8px}.crumb a{color:#57a}</style></head><body>";
    h+="<h2>&#128190; SD Explorer</h2><div class='bar crumb'><a href='/?path=/'>root</a>";
    String p=path,acc="";
    if(p!="/"){int st=1;for(int i=1;i<=(int)p.length();i++){if(i==(int)p.length()||p[i]=='/'){String pt=p.substring(st,i);acc+="/"+pt;h+=" / <a href='/?path="+acc+"'>"+he(pt.c_str())+"</a>";st=i+1;}}}
    h+="</div>";
    if(path!="/"){int ls=path.lastIndexOf('/');String par=(ls==0)?"/":path.substring(0,ls);h+="<div class='bar'><a href='/?path="+par+"'>&larr; Up</a></div>";}
    File entry=dir.openNextFile();
    while(entry){
        const char* fn=entry.name();
        if(fn[0]!='.'){
            bool id=entry.isDirectory();
            String fp=(path=="/"?"":path)+"/"+String(fn),sn=he(fn);
            // String(...) avoids adding two const char[] directly
            h+=String("<div class='entry'><span class='icon'>")+( id?"&#128193;":"&#128196;")+"</span>";
            if(id)h+="<span class='name'><a href='/?path="+fp+"'>"+sn+"</a></span>";
            else{
                uint32_t sz=(uint32_t)entry.size();
                String ss;
                if(sz<1024)ss=String(sz)+"B";
                else if(sz<1048576)ss=String(sz/1024)+"KB";
                else ss=String(sz/1048576)+"MB";
                h+="<span class='name'><a href='/download?path="+fp+"'>"+sn+"</a></span><span class='size'>"+ss+"</span>";
            }
            h+="<div class='actions'>";
            if(!id)h+="<button class='btn' onclick=\"copyFile('"+fp+"')\">Copy</button>";
            // String(...) avoids adding two const char[] directly
            h+=String("<button class='btn del' onclick=\"")+(id?"delDir":"delFile")+"('"+fp+"')\">Delete</button>";
            h+="</div></div>";
        }
        entry=dir.openNextFile();
    }
    dir.close();
    h+="<div class='upload'><b>Upload to: "+he(path.c_str())+"</b><br><br><form method='POST' action='/upload' enctype='multipart/form-data'><input type='hidden' name='path' value='"+path+"'><input type='file' name='file'><br><br><input type='submit' value='Upload'></form></div>";
    h+="<script>function delFile(p){if(!confirm('Delete '+p+'?'))return;fetch('/delete?path='+encodeURIComponent(p),{method:'POST'}).then(r=>r.text()).then(t=>{alert(t);location.reload();})}function delDir(p){if(!confirm('Delete folder '+p+'?'))return;fetch('/delete?path='+encodeURIComponent(p),{method:'POST'}).then(r=>r.text()).then(t=>{alert(t);location.reload();})}function copyFile(p){var d=prompt('Copy to:',p);if(!d)return;fetch('/copy?src='+encodeURIComponent(p)+'&dst='+encodeURIComponent(d),{method:'POST'}).then(r=>r.text()).then(t=>{alert(t);location.reload();})}</script></body></html>";
    webServer->send(200,"text/html",h);
}

// Sends a file from SD as a download
static void handleDownload(){String path=getP();File f=SD.open(path.c_str(),FILE_READ);if(!f||f.isDirectory()){webServer->send(404,"text/plain","Not found");if(f)f.close();return;}const char* name=strrchr(path.c_str(),'/');name=name?name+1:path.c_str();webServer->sendHeader("Content-Disposition",String("attachment; filename=\"")+name+"\"");webServer->streamFile(f,getMime(name));f.close();}

// Deletes a file or folder from SD
static void handleDelete(){String path=getP();if(SD.remove(path.c_str())||SD.rmdir(path.c_str()))webServer->send(200,"text/plain","Deleted: "+path);else webServer->send(500,"text/plain","Error deleting");}

// Copies a file on SD from src to dst
static void handleCopy(){if(!webServer->hasArg("src")||!webServer->hasArg("dst")){webServer->send(400,"text/plain","Missing params");return;}String src=webServer->arg("src"),dst=webServer->arg("dst");File fs=SD.open(src.c_str(),FILE_READ);if(!fs||fs.isDirectory()){webServer->send(404,"text/plain","Source not found");if(fs)fs.close();return;}File fd=SD.open(dst.c_str(),FILE_WRITE);if(!fd){fs.close();webServer->send(500,"text/plain","Could not create destination");return;}uint8_t buf[512];while(fs.available()){int n=fs.read(buf,sizeof(buf));fd.write(buf,n);}fs.close();fd.close();webServer->send(200,"text/plain","Copied to: "+dst);}

static File uploadFile;static String uploadDir;

// Handles incoming file upload, chunk by chunk
static void handleUpload(){HTTPUpload& u=webServer->upload();if(u.status==UPLOAD_FILE_START){uploadDir=webServer->hasArg("path")?webServer->arg("path"):"/";String fp=(uploadDir=="/"?"":uploadDir)+"/"+String(u.filename.c_str());SD.remove(fp.c_str());uploadFile=SD.open(fp.c_str(),FILE_WRITE);}else if(u.status==UPLOAD_FILE_WRITE){if(uploadFile)uploadFile.write(u.buf,u.currentSize);}else if(u.status==UPLOAD_FILE_END){if(uploadFile)uploadFile.close();webServer->sendHeader("Location","/?path="+uploadDir);webServer->send(303);}}
static void handleUploadDone(){}

// Shows the "server is running" screen with the IP address
static void drawServerScreen(const char* ip){
    canvas.fillSprite(COL_BG);
    canvas.fillRect(0,0,128,14,C(0,30,0)); canvas.drawFastHLine(0,14,128,C(0,100,0));
    drawText(14,4,"SERVER ACTIVE",C(0,220,60),1);
    drawText(2,22,"Connect at:",COL_TXT_DIM,1);
    drawText(2,34,"http://",C(0,160,255),1); drawText(44,34,ip,C(0,200,255),1);
    canvas.fillRect(2,48,124,1,C(20,40,20));
    drawText(2,54,"You can browse, upload,",COL_TXT_DIM,1);
    drawText(2,64,"delete and copy files",COL_TXT_DIM,1);
    drawText(2,74,"from any device on the",COL_TXT_DIM,1);
    drawText(2,84,"same network.",COL_TXT_DIM,1);
    canvas.fillRect(8,100,112,16,C(60,0,0)); canvas.drawRect(8,100,112,16,C(180,40,40));
    drawText(14,107,"B = STOP SERVER",C(255,120,120),1);
    canvas.pushSprite(0,0);
}

// Entry point: shows credentials menu, connects, starts web server
void runWifiServer() {
    if (!wifiCredentialsMenu()) return;
    if (!connectWifi()) return;
    String ip=WiFi.localIP().toString();
    if(webServer){webServer->stop();delete webServer;}
    webServer=new WebServer(80);
    webServer->on("/",        HTTP_GET,  handleRoot);
    webServer->on("/download",HTTP_GET,  handleDownload);
    webServer->on("/delete",  HTTP_POST, handleDelete);
    webServer->on("/copy",    HTTP_POST, handleCopy);
    webServer->on("/upload",  HTTP_POST, handleUploadDone,handleUpload);
    webServer->begin();
    drawServerScreen(ip.c_str());
    // Keep handling requests until B is pressed
    while(true){webServer->handleClient();readButtons();if(btnB.pressed)break;delay(5);}
    webServer->stop();delete webServer;webServer=nullptr;
    WiFi.disconnect(true);WiFi.mode(WIFI_OFF);
    canvas.fillSprite(COL_BG);
    drawText(18,56,"WIFI OFF",C(180,80,80),1);
    canvas.pushSprite(0,0);delay(1000);
}
