/*
   SFKTray - sfk status lights in the Windows system tray
   
   V 1.2.0 - Open Source under the BSD License

   Best use:

   -  create a folder C:\tools

   -  move sfktray.exe therein

   -  if you want more than 9 lights, create 3 exe files by
         copy sfktray.exe sfktray2.exe
         copy sfktray.exe sfktray3.exe

   -  run sfktray. without any options you get 9 lights per .exe.

      you can also use an option

         sfktray.exe -all

      which starts 3 executables, showing 27 lights.

   On the first run you have to move the sfktray icon manually
   from the "/\" area in the tray to the main tray.

   You also have to allow network I/O because sfk communicates
   with sfktray by UDP.

   compile with VC++ 14:
      cl -DSFKMINCORE sfktray.cpp sfktray.res sfk.cpp sfkext.cpp kernel32.lib user32.lib gdi32.lib shell32.lib comdlg32.lib ws2_32.lib advapi32.lib /link /SUBSYSTEM:WINDOWS

   build .res resource file:
      rc sfktray.rc

   to create icon files using imagemagick:
      -  create 16x16 32x32 48x48 transparent png files
      -  imgconv 48.png 32.png 16.png out.ico

   to build a GUID:
      x:\tools\vc10\SDK\Bin\guidgen.exe
*/

#define SFK_BRANCH   ""
#define SFK_VERSION  "1.2.0"
#define SFK_FIXPACK  ""
#define SFK_INFO     "StahlWorks Technologies, http://stahlworks.com/"
#define SFK_PREVER   ""

#define _WIN32_IE 0x600

// #include "windows.h"
#include "sfkbase.hpp"
#include "sfkext.hpp"
#include "Commctrl.h"

#define PRODNAME1    "SFKTray"
#define PRODNAME2    "sftray"
#define SFK_VERTYPE  "OSE"
#define MAX_STATE 10
#define MAX_POS   10

HINSTANCE ginst;

int gDeskW = 800;
int gDeskH = 600;

BOOL bHaveNotifyIcon = 0;

int iGivenPort = 0;

bool bGlblSilent = 0;
bool bGlblMinimize = 0;
bool bGlblMark = 0;
int  iGlblStartMore = 0;

#define VER_DAT_STR "date=" __DATE__

static const char *pszGlblVersion =
   "$version:vernum=" SFK_VERSION
   ",name=" PRODNAME2 ",title=" PRODNAME1 ","
   "os=windows-any,type=" SFK_VERTYPE ",fix=" SFK_FIXPACK "," VER_DAT_STR "$\0";

static GUID gGUID =
   { 0x79652791, 0x4607, 0x44c7, { 0x9d, 0x1e, 0xd5, 0xef, 0x97, 0xc0, 0xaf, 0xf5 } };
   // last bytes are changed by a hash of the .exe path.

uint crc32(uchar* data, uint length, uint inputCRC);

/*
   https://msdn.microsoft.com/en-us/library/windows/desktop/bb773352(v=vs.85).aspx

   reason if shell_notifyicon fails with code 1008:

   The binary file that contains the icon was moved. The path of the binary file is 
   included in the registration of the icon's GUID and cannot be changed. Settings 
   associated with the icon are preserved through an upgrade only if the file path and 
   GUID are unchanged. If the path must be changed, the application should remove any 
   GUID information that was added when the existing icon was registered. Once that 
   information is removed, you can move the binary file to a new location and 
   reregister it with a new GUID. Any settings associated with the original GUID 
   registration will be lost.
   This also occurs in the case of a side-by-side installation. When dealing with a 
   side-by-side installation, new versions of the application should update the GUID 
   of the binary file.
*/
void makeguid()
{
   char szOwnPath[512 + 10];
   mclear(szOwnPath);
   ::GetModuleFileName(NULL, szOwnPath, 512 + 2);
   uint n = crc32((uchar*)szOwnPath,strlen(szOwnPath),0);
   if (n == 0)
      return;
   gGUID.Data4[7] ^= (n >>  0);
   gGUID.Data4[6] ^= (n >>  8);
   gGUID.Data4[5] ^= (n >> 16);
   gGUID.Data4[4] ^= (n >> 24);
}

char *currentTime()
{
   static char szBuf[200];
   szBuf[0]='\0';

   mytime_t n = _time64(NULL);

   struct tm *pLocTime = _localtime64(&n);

   if (pLocTime)
   {
      strftime(szBuf, sizeof(szBuf)-10, "%Y-%m-%d %H:%M:%S", pLocTime);
   }

   return szBuf;
}

char *compactTime(char *psz,int &rpack)
{
   char *pnow=currentTime();

   if (rpack!=0) return psz+rpack;

   // 2017-07-31 22:24:01
   // 1234567890123456789
   if (!strncmp(pnow,psz,11)) {rpack=11; return psz+11;}
   if (!strncmp(pnow,psz,8))  {rpack=8; return psz+8;}
   if (!strncmp(pnow,psz,5))  {rpack=5; return psz+5;}
   return psz;
}

static uint crc32(uchar* data, uint length, uint inputCRC)
{
   uint i, j, q0, q1, q2, q3;
   uchar byte;
 
   uint crc = inputCRC; // default is 0xffffffff

   for (i = 0; i < length; i++)
   {
      byte = *data++;
      for (j = 0; j < 2; j++)
      {
         if (((crc >> 28) ^ (byte >> 3)) & 0x00000001) {
            q3 = 0x04C11DB7;
         } else {
            q3 = 0x00000000;
         }
         if (((crc >> 29) ^ (byte >> 2)) & 0x00000001) {
            q2 = 0x09823B6E;
         } else {
            q2 = 0x00000000;
         }
         if (((crc >> 30) ^ (byte >> 1)) & 0x00000001) {
            q1 = 0x130476DC;
         } else {
            q1 = 0x00000000;
         }
         if (((crc >> 31) ^ (byte >> 0)) & 0x00000001) {
            q0 = 0x2608EDB8;
         } else {
            q0 = 0x00000000;
         }
         crc = (crc << 4) ^ q3 ^ q2 ^ q1 ^ q0;
         byte >>= 4;
      }
   }
 
   return crc;
}

// ----- net -----

bool bGlblTCPInitialized = 0;

int prepareUDP()
{
   if (bGlblTCPInitialized)
      return 0;

   bGlblTCPInitialized = 1;

   WORD wVersionRequested = MAKEWORD(1,1);
   WSADATA wsaData;
   if (WSAStartup(wVersionRequested, &wsaData)!=0)
      return 9+perr("WSAStartup failed\n");

   return 0;
}

SOCKET nsock=0;

int getReceivePort()
{
   // by -port option:
   if (iGivenPort > 0)
      return iGivenPort;

   // default: derive from .exe filename
   char szExePath[512 + 10]; mclear(szExePath);

   ::GetModuleFileName(NULL, szExePath, 512 + 2);

   uint nMyDefaultPort = 21343;

   if (strEnds(szExePath, "sfktray2.exe")) nMyDefaultPort = 21344;
   if (strEnds(szExePath, "sfktray3.exe")) nMyDefaultPort = 21345;

   return nMyDefaultPort;
}

int getInstance()
{
   char szExePath[512 + 10]; mclear(szExePath);

   ::GetModuleFileName(NULL, szExePath, 512 + 2);

   if (strEnds(szExePath, "sfktray2.exe")) return 2;
   if (strEnds(szExePath, "sfktray3.exe")) return 3;

   return 1;
}

int udpinit()
{
   prepareUDP();

   struct sockaddr_in oOwnAddr; mclear(oOwnAddr);

   oOwnAddr.sin_family      = AF_INET;
   oOwnAddr.sin_addr.s_addr = htonl(INADDR_ANY);

   oOwnAddr.sin_port        = htons(getReceivePort());

   nsock = socket(AF_INET, SOCK_DGRAM, 0);

   if (bind(nsock, (struct sockaddr *)&oOwnAddr, sizeof(oOwnAddr)) != 0)
      return 9;

   listen(nsock, 10);

   return 0;
}

char *ipAsString(struct sockaddr_in *pAddr)
{
   static char szBuf[200];

   uint uiIP  = pAddr->sin_addr.s_addr;
   int  iPort = ntohs(pAddr->sin_port);

   snprintf(szBuf, sizeof(szBuf)-10,
      "%u.%u.%u.%u",
      (uiIP >>  0) & 0xFFU,
      (uiIP >>  8) & 0xFFU,
      (uiIP >> 16) & 0xFFU,
      (uiIP >> 24) & 0xFFU
      );
   szBuf[sizeof(szBuf)-10]='\0';

   return szBuf;
}

char udpbuf[2000];
struct sockaddr_in inAddr;

char *fromaddr()
{
   return ipAsString(&inAddr);
}

int getudp()
{
   struct timeval tv;
   fd_set fdvar;
 
   tv.tv_sec  = 0;
   tv.tv_usec = 0;

   FD_ZERO(&fdvar);
   FD_SET(nsock, &fdvar);

   if (select(nsock+1, &fdvar, 0, 0, &tv) <= 0)
      return 0;

   mclear(udpbuf);

   socklen_t nadrlen = sizeof(inAddr);
   int nRead = recvfrom(nsock, (char*)udpbuf, sizeof(udpbuf)-100, 0, (struct sockaddr *)&inAddr, &nadrlen);

   return nRead;
}

// ----- main -----

#define COLOR_UNDEF 0xff000000

class State
{
public:
      State ( );
   void  setcolor(uint c);

   uint  color;
   uint  color2;
   uint  timecolor;
   uint  blink;
   uint  time;
   int   visible;
   char  text[80];
   char  timefirst[80];
   char  timerecent[80];
   char  ip[80];
   char  lurl[200];
   char  rurl[200];
};

State::State( )
{
   color = 0x555555;
   color2= color;
   timecolor=COLOR_UNDEF;
   blink = 0;
   time  = 0;
   visible=1;
   mclear(text);
   mclear(timefirst);
   mclear(timerecent);
   mclear(ip);
   mclear(lurl);
   mclear(rurl);
}

void State::setcolor(uint c)
{
   if (c != color2)
      strcpy(timefirst,currentTime());
   strcpy(timerecent,currentTime());
   color=c;
   color2=c;
}

class Pos
{
public:
      Pos ( );

   int   x;
   int   y;
   int   w;
   int   h;
   int   itool;
   // 0: outside
   // 1: inside first tick
   // 5: start showing text
};

Pos::Pos( )
{
   x=y=w=h=0;
}

class MainWindow
{
public:
      MainWindow  ( );

   int   init        ( );
   void  updateIcon  ( );
   void  step        ( );
   void  toggleShow  ( );
   void  renderStates(int w, int h);
   void  drawLine    (HDC hdc,int x1,int y1,int x2,int y2,uint ucol);
   void  fillRect    (HDC hdc,int x1,int y1,int x2,int y2,uint nCol,int bborder=0);
   void  drawSlots   (HDC hdc,int x,int y,int w,int h);
   void  errcol      (int istate);
   void  dobutton    (int ibutton, int ievent);
   void  drawToolTip (HDC hdc);

   void  startOthers ( );

   HWND  clwin;
   int   ilayout;    // 2,4,9
   int   isize;
   int   itick;
   int   itime;
   int   iscreen;
   int   btrace;
   int   bupdate;
   int   bshow;
   int   ballowhide;
   uint  cdef,ctopleft,cbotrite;

   HICON aicon[10];

   Pos   abutton[10];
   State astate[MAX_STATE];
   Pos   apos[MAX_POS];

   HBITMAP bmpMain;
   HBITMAP bmpMask;
   BITMAPINFOHEADER hdrMain;
   BITMAPINFOHEADER hdrMask;
   BYTE *pIconPix;
   BYTE *pIconMask;
   HICON holdIcon;

   char szmsg[20][500];
   int  imsg;
};

MainWindow::MainWindow( )
{
   iscreen = 0;
   btrace = 0;
   itime = 0;
   clwin = 0;
   isize = 32;
   itick = 0;
   bupdate = 0;

   bmpMain = 0;
   bmpMask = 0;
   mclear(hdrMain);
   mclear(hdrMask);
   pIconPix = 0;
   pIconMask = 0;
   holdIcon = 0;
   mclear(szmsg);
   imsg = 0;
   cdef     = 0x555555;
   ctopleft = 0x888888;
   cbotrite = 0x444444;
   ilayout=9;
   bshow=1;
   mclear(aicon);
}

MainWindow gwin;

long addmsg(const char *pszFormat, ...)
{
   char szBuf[300];

   va_list argList;
   va_start(argList, pszFormat);

   _vsnprintf(szBuf, sizeof(szBuf)-10, pszFormat, argList);

   cchar *pszerr="";
   char *psz=szBuf;
   if (!strncmp(psz,"[error] ",8)) {
      psz+=8;
      pszerr="[error] ";
   }

   snprintf(gwin.szmsg[gwin.imsg], sizeof(gwin.szmsg[gwin.imsg])-10, 
      "%s%s %s", pszerr, currentTime(), psz);

   gwin.imsg = (gwin.imsg + 1) % 20;

   return 0;
}

int MainWindow::init( )
{
   HWND hdesk = GetDesktopWindow();
   if (!hdesk) return 9;

   HDC hdc = GetDC(hdesk);
   if (!hdc) return 10;

   int irc = 12;

   do
   {
      bmpMain = CreateCompatibleBitmap(hdc,isize,isize);
      bmpMask = CreateCompatibleBitmap(hdc,isize,isize);
      if (!bmpMain || !bmpMask) return 11;
    
      hdrMain.biSize        = sizeof(BITMAPINFOHEADER);
      hdrMain.biWidth       = isize;
      hdrMain.biHeight      = isize;
      hdrMain.biPlanes      = 1;
      hdrMain.biBitCount    = 24;
      hdrMain.biCompression = BI_RGB;
    
      hdrMask.biSize        = sizeof(BITMAPINFOHEADER);
      hdrMask.biWidth       = isize;
      hdrMask.biHeight      = isize;
      hdrMask.biPlanes      = 1;
      hdrMask.biBitCount    = 24;
      hdrMask.biCompression = BI_RGB;
   
      GetDIBits(hdc, bmpMain, 0, isize, NULL, (BITMAPINFO*)&hdrMain, DIB_RGB_COLORS);
      if (!hdrMain.biSizeImage) break;

      GetDIBits(hdc, bmpMask, 0, isize, NULL, (BITMAPINFO*)&hdrMask, DIB_RGB_COLORS);
      if (!hdrMask.biSizeImage) break;
   
      pIconPix = (BYTE*)::GlobalAlloc(GMEM_FIXED,hdrMain.biSizeImage);
      if (!pIconPix)  break;
   
      pIconMask = (BYTE*)::GlobalAlloc(GMEM_FIXED,hdrMask.biSizeImage);
      if (!pIconMask) break;
    
      memset(pIconPix, 0xFF, hdrMain.biSizeImage);
      memset(pIconMask, 0xFF, hdrMask.biSizeImage);

      irc = 0;
   }
   while (0);

   ReleaseDC(hdesk,hdc);

   aicon[0] = LoadIcon(ginst, (LPCTSTR)490);
   aicon[1] = LoadIcon(ginst, (LPCTSTR)500);
   aicon[2] = LoadIcon(ginst, (LPCTSTR)501);
   aicon[3] = LoadIcon(ginst, (LPCTSTR)502);
   aicon[4] = LoadIcon(ginst, (LPCTSTR)503);
   aicon[5] = LoadIcon(ginst, (LPCTSTR)504);
   aicon[6] = LoadIcon(ginst, (LPCTSTR)505);

   return irc;
}

bool wordbeg(char *psz, char *pat)
{
   if (!strbeg(psz,pat)) return 0;
   psz += strlen(pat);
   if (*psz==0) return 1;
   if (*psz!=' ') return 0;
   return 1;
}

uint parseColor(char *psz,int &berr)
{
   berr = 0;

   if (wordbeg(psz, "grey"))   return 0x777777;
   if (wordbeg(psz, "gray"))   return 0x777777;
   if (wordbeg(psz, "Gray"))   return 0xaaaaaa;

   if (wordbeg(psz, "red"))    return 0x0000ff;
   if (wordbeg(psz, "green"))  return 0x00ff00;
   if (wordbeg(psz, "blue"))   return 0xff3333;
   if (wordbeg(psz, "yellow")) return 0x00ffff;
   if (wordbeg(psz, "orange")) return 0x2090ff;

   if (wordbeg(psz, "white"))  return 0xffffff;
   if (wordbeg(psz, "cyan"))   return 0xffff00;
   if (wordbeg(psz, "purple")) return 0xff00ff;
   if (wordbeg(psz, "black"))  return 0x444444;

   if (   isxdigit(psz[0])
       && isxdigit(psz[1])
       && isxdigit(psz[2])
       && (psz[3]==' ' || psz[3]==0)
      )
   {
      uint n=strtoul(psz,0,0x10);
      uint n1=((n>>8)&0xf); n1 |= (n1<<4);
      uint n2=((n>>4)&0xf); n2 |= (n2<<4);
      uint n3=((n>>0)&0xf); n3 |= (n3<<4);
      return (n3<<16)|(n2<<8)|n1;
   }

   if (   isxdigit(psz[0]) && isxdigit(psz[1])
       && isxdigit(psz[2]) && isxdigit(psz[3])
       && isxdigit(psz[4]) && isxdigit(psz[5])
       && (psz[6]==' ' || psz[6]==0)
      )
   {
      uint n=strtoul(psz,0,0x10);
      uint n1=((n>>16)&0xff);
      uint n2=((n>>8)&0xff);
      uint n3=((n>>0)&0xff);
      return (n3<<16)|(n2<<8)|n1;
   }

   berr = 1;

   return 0;
}

void MainWindow::errcol(int istate)
{
   if (istate >= MAX_STATE)
      return;
   astate[istate].color = 0x55aaff;
}

void MainWindow::drawToolTip(HDC hdc)
{
   char szBuf[200];
   szBuf[0] = '\0';

   for (int i=0; i<10; i++)
   {
      if (abutton[i].itool > 1 && abutton[i].itool < 7)
      {
         RECT rt;

         rt.left  = abutton[i].x - 200;
         rt.top   = abutton[i].y + 6;
         rt.right = abutton[i].x - 16;
         rt.bottom= rt.top + abutton[i].h;

         SetTextColor(hdc, 0xffffff);

         switch (i) 
         {
            case 0: strcpy(szBuf, "toggle message list"); break;
            case 1: 
               if (iscreen==1)
                  strcpy(szBuf, "toggle trace mode");
               else
                  strcpy(szBuf, "change layout");
               break;
            case 2: strcpy(szBuf, "minimize to tray"); break;
            case 3: strcpy(szBuf, "show help"); break;
         }

         int b=8;
         fillRect(hdc, rt.left-b, rt.top-b, rt.right-rt.left+b*2, rt.bottom-rt.top+b*2, 0x777777);
         fillRect(hdc, rt.left-b+2, rt.top-b+2, rt.right-rt.left+b*2-4, rt.bottom-rt.top+b*2-4, 0x999999);

         DrawText(hdc, szBuf, strlen(szBuf),
                  &rt, DT_CENTER|DT_VCENTER|DT_NOPREFIX);
      }
   }
}

char *aPatterns[] =
{
   "4up",    "1+2+3",
   "4down",  "2+3+4",
   "9up",    "2+4+5+6",
   "9down",  "4+5+6+8",

   "4left",  "1+3",
   "4right", "2+4",
   "9left",  "2+4+5+8",
   "9right", "2+5+6+8",

   "9fromleft",  "1+4+5+7",
   "9fromright", "3+5+6+9",

   "4top",    "1+2",
   "4bottom", "3+4",
   "9top",    "1+2+3",
   "9bottom", "7+8+9",

   "4all",    "1+2+3+4",
   "9all",    "1+2+3+4+5+6+7+8+9",

   0, 0
};

void MainWindow::step( )
{
   itick++;

   itime = (itick % 10);

   if (itime == 0)
   {
      for (int i=0;i<MAX_STATE; i++) 
      {
         if (gwin.astate[i].time > 0) 
         {
            gwin.astate[i].time--;
            if (gwin.astate[i].time == 0) 
            {
               if (gwin.astate[i].timecolor != COLOR_UNDEF)
                  gwin.astate[i].color = gwin.astate[i].timecolor;
               else
                  gwin.astate[i].color = cdef;
               gwin.astate[i].blink = 0;
               bupdate = 1;
            }
         }
      }
      // step tooltips
      for (int i=0; i<10; i++)
      {
         if (abutton[i].itool) {
            abutton[i].itool++;
            bupdate=1;
         }
      }
   }

   if (itick == 10)
   {
      if (udpinit()) 
      {
         if (bGlblSilent)
            exit(101); // already running

         iscreen=1;
         addmsg("[error] cannot receive on port %d. Make sure SFKTray is not running twice.",
            getReceivePort()
            );
      }
      else
      {
         if (iGlblStartMore)
            startOthers();

         if (bGlblMinimize)
            toggleShow();
      }
      if (bGlblMark)
      {
         int iInstance = getInstance();
         for (int i=0; i<iInstance; i++)
         {
            astate[i].color = 0x999999;
            astate[i].time = 60;
            astate[i].timecolor = cdef;
         }
      }
      bupdate = 1;
   }
   else
   while (1)
   {
      int n = getudp();
      if (n<=0)
         break;

      // :status v1 slot=1 color=red blink=slow text=compile error at 21:5
      // :status v1 slot=9 color=green timeout=60 confirm=62310 text=sql service running

      if (btrace)
         addmsg("%s %s", fromaddr(), udpbuf);

      int   s=0,bstop=0,istate=-1,berr=0;
      uint  nhave=0;
      char *psz=udpbuf;
      char *pcopy=0;

      while (psz!=0 && *psz!=0)
      {
         do
         {
            if (s==0 && strbeg(psz, ":status "))
               { bupdate=1; s=1; break; }

            if (s==1) 
            {
               if (strbeg(psz, "v1 "))
                  { s=2; break; }
               addmsg("got wrong protocol from %s: %s",fromaddr(),psz);
               bstop=1; break;
            }

            if (strbeg(psz,"layout="))
            {
               int i=atoi(psz+7);
               if (i==2 || i==4 || i==9) {
                  ilayout=i;
                  updateIcon();
                  InvalidateRect(clwin,0,0);
                  UpdateWindow(clwin);
               }
               break;
            }

            if (strbeg(psz,"slot=")) 
            {
               istate=atoi(psz+5)-1;
               if (istate<0 || istate>=MAX_STATE)
                  { addmsg("invalid slot from %s: %s",fromaddr(),psz); bstop=1; break; }
               astate[istate].color=cdef;
               astate[istate].blink=0;
               astate[istate].time=0;
               astate[istate].timecolor=COLOR_UNDEF;
               char *pszfrom=fromaddr();
               char *pszold=astate[istate].ip;
               if (pszold[0]!=0 && strcmp(pszold,pszfrom)!=0)
                  addmsg("slot %d source ip change: %s -> %s",istate+1,pszold,pszfrom);
               strcopy(astate[istate].ip,pszfrom);
               break;
            }

            if (strbeg(psz,"pat=")     // v1.1 pattern support
                || strbeg(psz,"pattern="))
            {
               char *ppat = psz+4;
               if (*ppat=='e')
                  ppat += 4;
               // up etc

               char szpat[50];
               mystrcopy(szpat+1, ppat, sizeof(szpat)-10);
               szpat[0] = '0'+ilayout;
               // 9up blabla - must ignore blabla

               int ipat=0;
               for (; aPatterns[ipat]!=0; ipat += 2)
                  if (!strncmp(szpat, aPatterns[ipat], strlen(aPatterns[ipat])))
                     break;
               if (aPatterns[ipat]!=0) 
               {
                  char *pmask = aPatterns[ipat+1];

                  // 2+4+5+6 take "2"
                  istate = atoi(pmask)-1;

                  // remember 4+5+6 for below
                  pmask++;
                  while (*pmask!=0 && isdigit(*pmask)==0)
                     pmask++;

                  pcopy = pmask;
               } else {
                  addmsg("unknown pattern from %s: %s",fromaddr(),psz); bstop=1;
               }

               break;
            }

            if (istate<0 || istate>=MAX_STATE)
               { addmsg("missing slot from %s: %s",fromaddr(),psz); bstop=1; break; }

            if (strbeg(psz,"color=")) 
            {
               uint ncol=parseColor(psz+6,berr);
               if (berr)
                  { addmsg("wrong color from %s: %s",fromaddr(),psz); errcol(istate); bstop=1; break; }
               astate[istate].setcolor(ncol);
               break;
            }
            if (strbeg(psz,"blink=")) 
            {
               char *pval=psz+6;
               if (strbeg(pval,"slow"))
                  { astate[istate].blink=1; break; }
               if (strbeg(pval,"fast"))
                  { astate[istate].blink=2; break; }
               if (strbeg(pval,"none"))
                  { astate[istate].blink=0; break; }
               { addmsg("wrong blink from %s: %s",fromaddr(),psz); errcol(istate); bstop=1; break; }
            }
            if (strbeg(psz,"timeout=")) 
            {
               // timeout=7,orange
               psz+=8;
               int n=atoi(psz);
               if (n<0)
                  { addmsg("wrong timeout from %s: %s",fromaddr(),psz); errcol(istate); bstop=1; break; }
               astate[istate].time=n;
               while (isdigit(*psz)) psz++;
               if (*psz==',') {
                  psz++;
                  uint ncol=parseColor(psz,berr); // must be terminated word
                  if (berr)
                     { addmsg("wrong timeout color from %s: %s",fromaddr(),psz); errcol(istate); bstop=1; break; }
                  astate[istate].timecolor=ncol;
               }
               break;
            }
            if (strbeg(psz,"text=")) 
            {
               char *pold=psz;
               if (nhave&4)
                  { addmsg("text given twice from %s: %s",fromaddr(),psz); errcol(istate); bstop=1; break; }
               nhave |= 4;
               char *pval=psz+5;
               if (strlen(pval)<2)
                  { addmsg("invalid text from %s: %s",fromaddr(),psz); errcol(istate); bstop=1; break; }
               char csep=*pval++;
               char *pend=pval;
               while (*pend!=0 && *pend!=csep)
                  pend++;
               if (*pend!=csep)
                  { addmsg("missing separators from %s: %s",fromaddr(),psz); errcol(istate); bstop=1; break; }
               int ilen=pend-pval;
               if (ilen>sizeof(astate[istate].text)-10)
                   ilen=sizeof(astate[istate].text)-10;
               memcpy(astate[istate].text,pval,ilen);
               astate[istate].text[ilen]='\0';
               psz=pend+1;
               if (*psz!=0 && *psz!=' ')
                  { addmsg("missing text delimiters '' from %s: %s",fromaddr(),pold); errcol(istate); bstop=1; break; }
               break;
            }
            { addmsg("unexpected keyword from %s: %s",fromaddr(),psz); errcol(istate); bstop=1; break; }
         }
         while (0);

         if (bstop)
            break;

         while (*psz!=0 && *psz!=' ')
            psz++;
         while (*psz==' ')
            psz++;
      }

      // v1.1 pattern copy
      if (istate>=0 && istate<MAX_STATE)
      {
         while (pcopy!=0 && *pcopy!=0)
         {
            int istate2 = atoi(pcopy)-1;
   
            if (istate2>=0 && istate2<MAX_STATE)
               memcpy(&astate[istate2], &astate[istate], sizeof(astate[istate]));
   
            pcopy++;
            while (*pcopy!=0 && isdigit(*pcopy)==0)
               pcopy++;
         }
      }
   }

   for (int i=0;i<ilayout && i<MAX_STATE;i++)
      if (gwin.astate[i].blink)
         bupdate=1;

   if (bupdate)
   {
      updateIcon();
   }

   if (bshow != 0 && bupdate == 1)
   {
      InvalidateRect(clwin,0,0);
      UpdateWindow(clwin);
   }
}

void MainWindow::renderStates(int w, int h)
{
   int ncols=2;
   int nrows=1;
   switch (ilayout)
   {
      case 2: ncols=2; nrows=2; break;
      case 4: ncols=2; nrows=2; break;
      case 9: ncols=3; nrows=3; break;
   }
   int colw=w/ncols;
   int rowh=h/nrows;

   int icol=0,irow=0;
   for (int istate=0; istate<ilayout; istate++)
   {
      irow=istate/ncols;
      icol=(istate-irow*ncols);

      if (ilayout == 2 && istate == 1) {
         irow=1;
         icol=1;
      }

      if (irow>=nrows) break;

      apos[istate].x = icol*colw+0;
      apos[istate].y = irow*rowh+0;
      apos[istate].w = colw-0;
      apos[istate].h = rowh-0;

      if (ilayout==2) 
      {
         apos[istate].y += rowh/8;
         apos[istate].h -= rowh/4;
      }

      if (istate >= MAX_STATE)
         continue;

      int iblink = 0;
      if (astate[istate].blink==1)
         iblink = (itick/13)&1;
      else
      if (astate[istate].blink==2)
         iblink = (itick/4)&1;

      astate[istate].visible = iblink ? 0 : 1;
   }
}

void MainWindow::updateIcon( )
{
   HWND hdesk=GetDesktopWindow();
   if (!hdesk) return;

   HDC hdc = GetDC(hdesk);
   if (!hdc)   return;

   // NO RETURN WITHOUT ReleaseDC BEGIN

   uchar *ppix = (uchar*)pIconPix;
   uchar *pmsk = (uchar*)pIconMask;

   memset(pIconPix, 0xFF, hdrMain.biSizeImage);
   memset(pIconMask, 0xFF, hdrMask.biSizeImage);

   renderStates(isize,isize);

   uchar c1,c2,c3,cmask;

   for (int istate=0; istate<ilayout; istate++)
   {
      if (istate < MAX_STATE)
      {
         c1    = astate[istate].color >> 16;
         c2    = astate[istate].color >> 8;
         c3    = astate[istate].color >> 0;
         cmask = astate[istate].visible ? 0 : 0xFF;
      }
      else
      {
         c1    = cdef >> 16;
         c2    = cdef >> 8;
         c3    = cdef >> 0;
         cmask = 0;
      }

      int l = apos[istate].x;
      int t = apos[istate].y;
      int w = apos[istate].w;
      int h = apos[istate].h;
      for (int xr=0;xr<w;xr++)
      {
         for (int yr=0;yr<h;yr++)
         {
            int x=l+xr;
            int y=isize-1-(t+yr);
            ppix[y*isize*3+x*3+0] = c1;
            ppix[y*isize*3+x*3+1] = c2;
            ppix[y*isize*3+x*3+2] = c3;
         }
      }

      for (int xr=1;xr<w-2;xr++)
      {
         for (int yr=1;yr<h-2;yr++)
         {
            int x=l+xr;
            int y=isize-1-(t+yr);
            pmsk[y*isize*3+x*3+0] = cmask;
            pmsk[y*isize*3+x*3+1] = cmask;
            pmsk[y*isize*3+x*3+2] = cmask;
         }
      }
   }

   SetDIBits(hdc, bmpMain, 0, isize, pIconPix, (BITMAPINFO*)&hdrMain, DIB_RGB_COLORS);
   SetDIBits(hdc, bmpMask, 0, isize, pIconMask, (BITMAPINFO*)&hdrMask, DIB_RGB_COLORS);

   ICONINFO inf;
   mclear(inf);

   inf.fIcon=TRUE;
   inf.hbmMask=bmpMask;
   inf.hbmColor=bmpMain;

   HICON hnew = CreateIconIndirect(&inf);

   if (hnew)
   {
      NOTIFYICONDATA nid;
      memset(&nid,0,sizeof(nid));

      nid.cbSize     = sizeof(nid);
      nid.hWnd       = clwin;
      nid.uFlags     = NIF_ICON | NIF_TIP | NIF_GUID | NIF_MESSAGE;
      nid.uID        = 1;
      nid.guidItem   = gGUID;
      nid.uCallbackMessage = WM_APP+1;
      nid.hIcon      = hnew;

      if (holdIcon != 0)
      {
         if (bHaveNotifyIcon) {
            if (Shell_NotifyIcon(NIM_MODIFY, &nid))
               ballowhide=1;
         }
         DestroyIcon(holdIcon);
      }
      else
      {
         bHaveNotifyIcon = Shell_NotifyIcon(NIM_ADD, &nid);
         if (bHaveNotifyIcon == 0) {
            iscreen=1;
            addmsg("[error] Cannot create tray icon (%u). Make sure SFKTray is not running twice.", GetLastError());
            // continue limited
         } else {
            ballowhide=1;
         }
      }

      holdIcon = hnew;
   }

   // NO RETURN WITHOUT ReleaseDC END

   ReleaseDC(clwin, hdc);
}

void MainWindow::drawLine(HDC hBack, int x1abs, int y1abs, int x2, int y2, uint ucol)
{
   HGDIOBJ hpen    = CreatePen(PS_SOLID, 1, ucol);
   HGDIOBJ hpenOld = SelectObject(hBack, hpen);
 
   MoveToEx(hBack, x1abs,y1abs, 0);
   LineTo(hBack, x2,y2);
 
   SelectObject(hBack, hpenOld);
   DeleteObject(hpen);
}

void rgbtohsv( double r, double g, double b, double *h, double *s, double *v )
{
	double dmin, dmax, delta;
	dmin = min( min(r, g), min(g, b) );
	dmax = max( max(r, g), max(g, b) );
	*v = dmax;				// v
	delta = dmax - dmin;
	if( dmax != 0 )
		*s = delta / dmax;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0;
		*h = -1;
		return;
	}
	if( r == dmax )
		*h = ( g - b ) / delta;		// between yellow & magenta
	else if( g == dmax )
		*h = 2 + ( b - r ) / delta;	// between cyan & yellow
	else
		*h = 4 + ( r - g ) / delta;	// between magenta & cyan
	*h *= 60;				// degrees
	if( *h < 0 )
		*h += 360;
}

void hsvtorgb( double *r, double *g, double *b, double h, double s, double v )
{
	int i;
	double f, p, q, t;
	if( s == 0 ) {
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}
	h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i ) {
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
	}
}

uint brite(uint n) 
{
   uint r1=(n>>16)&0xFFU;
   uint g1=(n>> 8)&0xFFU;
   uint b1=(n>> 0)&0xFFU;
   if (r1<=0xe0) r1 += 0x10;
   if (g1<=0xe0) g1 += 0x10;
   if (b1<=0xe0) b1 += 0x10;
   return   (((uint)r1)<<16)
         |  (((uint)g1)<< 8)
         |  (((uint)b1)<< 0);
}

uint dark(uint n)
{
   uint r1=(n>>16)&0xFFU;
   uint g1=(n>> 8)&0xFFU;
   uint b1=(n>> 0)&0xFFU;
   if (r1>=0x10) r1 -= 0x10;
   if (g1>=0x10) g1 -= 0x10;
   if (b1>=0x10) b1 -= 0x10;
   return   (((uint)r1)<<16)
         |  (((uint)g1)<< 8)
         |  (((uint)b1)<< 0);
}

void MainWindow::fillRect(HDC hdc,int x1,int y1,int w,int h,uint nCol,int bborder)
{
   RECT rt2;
   rt2.left   = x1;
   rt2.top    = y1;
   rt2.right  = x1+w;
   rt2.bottom = y1+h;
   HBRUSH hBlank = CreateSolidBrush(nCol);
   if (hBlank == NULL)
      return;
   FillRect(hdc, &rt2, hBlank);
   DeleteObject(hBlank);

   /*
   if (bborder) 
   {
      ctopleft=brite(nCol);
      cbotrite=dark(nCol);
      for (int i=0;i<5;i++)
      {
         drawLine(hdc,x1-i,y1-i,x1+w+i,y1-i,ctopleft);
         drawLine(hdc,x1-i,y1-i,x1-i,y1+h+i,ctopleft);
         drawLine(hdc,x1+w+i,y1-i,x1+w+i,y1+h+i,cbotrite);
         drawLine(hdc,x1-i,y1+h+i,x1+w+i+1,y1+h+i,cbotrite);
      }
   }
   */
}

void MainWindow::drawSlots(HDC hdc,int x,int y,int w,int h)
{
   char szBuf[300];

   int b = 10;

   renderStates(w-b*2,h-b*2);

   x += b;
   y += b;

   SetBkMode(hdc, TRANSPARENT);

   for (int istate=0;istate<ilayout;istate++)
   {
      if (istate >= MAX_STATE)
      {
         fillRect(hdc,
            x+apos[istate].x+b,
            y+apos[istate].y+b,
            apos[istate].w-b*2,
            apos[istate].h-b*2,
            cdef, 1);
         continue;
      }

      if (astate[istate].visible)
      {
         fillRect(hdc,
            x+apos[istate].x+b,
            y+apos[istate].y+b,
            apos[istate].w-b*2,
            apos[istate].h-b*2,
            astate[istate].color, 1);
      }

      if (astate[istate].blink)
         SetTextColor(hdc, 0x999999);
      else
         SetTextColor(hdc, 0);

      {
         char *psz=astate[istate].text;

         RECT rt;

         rt.left=x+apos[istate].x+b;
         rt.top=y+apos[istate].y+b
                +(apos[istate].h-b*2)/2-20;
         rt.right=rt.left+apos[istate].w-b*2;
         rt.bottom=rt.top+apos[istate].h-b*2;

         if (psz[0])
         {
            DrawText(hdc, psz, strlen(psz),
                     &rt, DT_CENTER|DT_VCENTER|DT_NOPREFIX);
         }
         else if (astate[istate].timefirst[0]==0)
         {
            rt.top=y+apos[istate].y+b
                   +(apos[istate].h-b*2)/2-8;
            sprintf(szBuf, "%u", istate+1);
            SetTextColor(hdc, 0x888888);
            DrawText(hdc, szBuf, strlen(szBuf),
                     &rt, DT_CENTER|DT_VCENTER|DT_NOPREFIX);
         }

         char *pf=astate[istate].timefirst;
         char *pr=astate[istate].timerecent;

         if (pf[0])
         {
            int ipack=0;
            char *pt1=compactTime(pf,ipack);
            char *pt2=compactTime(pr,ipack);
            snprintf(szBuf,sizeof(szBuf)-10,
               "%s - %s",pt1,pt2);
            rt.top=y+apos[istate].y+b
                   +(apos[istate].h-b*2)/2+4;
            DrawText(hdc, szBuf, strlen(szBuf),
                     &rt, DT_CENTER|DT_VCENTER|DT_NOPREFIX);
         }

         char *pszip=astate[istate].ip;
         if (pszip[0]!=0 && strcmp(pszip,"127.0.0.1")!=0)
         {
            rt.top += 16;
            DrawText(hdc, pszip, strlen(pszip),
                     &rt, DT_CENTER|DT_VCENTER|DT_NOPREFIX);
         }
      }
   }
}

void MainWindow::toggleShow()
{
   bshow ^= 0x1;
   if (bshow) {
      SetForegroundWindow(clwin); // V1.1.1
      SetWindowPos(clwin, HWND_TOP,
         0, 0, 0, 0,
         SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE
         );
      ShowWindow(clwin, SW_RESTORE);
   } else {
      ShowWindow(clwin, SW_HIDE);
   }
}

void MainWindow::dobutton(int ibutton, int ievent)
{
   if (ievent == WM_LBUTTONUP)
   {
      abutton[ibutton].itool = 10; // block tooltip

      switch (ibutton)
      {
         case 0:
            iscreen ^= 1;
            InvalidateRect(clwin,0,0);
            UpdateWindow(clwin);
            break;
   
         case 1:
            if (iscreen==1) {
               btrace ^= 1;
               addmsg("[trace mode %s]",btrace?"on":"off");
            } else {
               switch (ilayout)
               {
                  case 9: ilayout=2; break;
                  case 4: ilayout=9; break;
                  case 2: ilayout=4; break;
               }
               updateIcon();
            }
            InvalidateRect(clwin,0,0);
            UpdateWindow(clwin);
            break;
   
         case 2:
            toggleShow();
            break;

         case 3:

            iscreen = 1;
            mclear(szmsg);
            imsg = 0;

            addmsg("=== click the stripe button to hide this short help ===");
            addmsg(" ");

            addmsg("on first use you have to pull the sfktray icon");
            addmsg("from the tray '/\\' area to make it visible.");
            addmsg("then you can use 'sfk status' to toggle status lights:");
            addmsg(" ");
            addmsg(" sfk status 127.0.0.1 \"slot=1 color=green\"");
            addmsg(" ");

            addmsg("startup options (for desktop icon's target field):");
            addmsg("-min     hide the main window");
            addmsg("-port=n  use a different port");
            addmsg("-2x      also run sktray2.exe");
            addmsg("-all     also run sktray2/3.exe");
            addmsg("  (to use -2x you have to copy sfktray.exe sfktray2.exe)");
            addmsg(" ");

            addmsg("with 3 instances running you can use:");
            addmsg("  sfk status local:2nd \"slot=1 color=green\"");
            addmsg("  sfk status local:3rd \"pat=up color=green blink=fast timeout=10\"");
            addmsg("  sfk sel ... +sft -notify=local:3rd cput");

            break;
      }
   }

   if (ievent == WM_MOUSEMOVE)
   {
      for (int i=0;i<10;i++) {
         if (i != ibutton)
            abutton[i].itool = 0;
      }
      if (abutton[ibutton].itool==0) {
         abutton[ibutton].itool = 1;
      }
   }
}

LRESULT CALLBACK MasterWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   gwin.bupdate = 0;

   switch (msg)
   {
      case WM_DESTROY:
      {
        PostQuitMessage(WM_QUIT);
        break;
      }

      case WM_TIMER:
      {
         KillTimer(gwin.clwin, 1);
         gwin.step();
         SetTimer(gwin.clwin, 1, 100, 0);
         break;
      }

      case WM_APP+1:
      {
         if (lParam == 0x202) // lbutton
         {
            gwin.toggleShow();
         }
         if (lParam == 0x205) // rbutton
         {
            // PostQuitMessage(WM_QUIT);
         }
         break;
      }

      case WM_LBUTTONUP:
      case WM_RBUTTONUP:
      case WM_MOUSEMOVE:
      {
         UINT x = LOWORD(lParam);
         UINT y = HIWORD(lParam);
         int brite = (msg==WM_RBUTTONUP) ? 1:0;

         bool bany=0;

         for (int i=0; i<4; i++)
         {
            if (   x>=gwin.abutton[i].x
                && y>=gwin.abutton[i].y
                && x<gwin.abutton[i].x+gwin.abutton[i].w
                && y<gwin.abutton[i].y+gwin.abutton[i].h)
            {
               bany=1;
               gwin.dobutton(i,msg);
            }
         }

         if (!bany)
         {
            bool bchange=0;
            for (int i=0; i<10; i++) {
               if (gwin.abutton[i].itool)
                  bchange=1;
               gwin.abutton[i].itool = 0;
            }
            if (bchange) {
               InvalidateRect(gwin.clwin,0,0);
               UpdateWindow(gwin.clwin);
            }
         }

         if (msg == WM_LBUTTONUP || msg == WM_RBUTTONUP)
         {
            for (int i=0;i<MAX_POS;i++) 
            {
               if (   x>=gwin.apos[i].x
                   && y>=gwin.apos[i].y
                   && x<gwin.apos[i].x+gwin.apos[i].w
                   && y<gwin.apos[i].y+gwin.apos[i].h
                   && i<MAX_STATE
                  )
               {
                  // reset slot color on click
                  if (gwin.astate[i].color != gwin.cdef) {
                     gwin.astate[i].color = gwin.cdef;
                     gwin.astate[i].blink = 0;
                     gwin.updateIcon();
                     InvalidateRect(gwin.clwin,0,0);
                     UpdateWindow(gwin.clwin);
                  }
                  // internal: open given url
                  char *purl = brite ? gwin.astate[i].rurl : gwin.astate[i].lurl;
                  if (purl[0])
                     ShellExecute(NULL, "open", purl, NULL, NULL, SW_SHOWNORMAL);
               }
            }
         }

         break;
      }

      case WM_PAINT:
      {
         PAINTSTRUCT ps;
         HDC hdc = BeginPaint(hWnd, &ps);

         // icon init
         if (gwin.itick >= 1 && gwin.holdIcon == 0)
            gwin.updateIcon();

         RECT rt;
         GetClientRect(hWnd, &rt);

         int b = 10;

         // . --- slots ---

         int w=rt.right-rt.left;
         int h=rt.bottom-rt.top;
         gwin.fillRect(hdc,0,0,w,h,0x666666);

         gwin.abutton[0].x = w-b-25;
         gwin.abutton[0].y = b*2-3;
         gwin.abutton[0].w = 20;
         gwin.abutton[0].h = 20;

         gwin.abutton[1].x = gwin.abutton[0].x;
         gwin.abutton[1].y = gwin.abutton[0].y+30+b;
         gwin.abutton[1].w = gwin.abutton[0].w;
         gwin.abutton[1].h = gwin.abutton[0].h;

         if (gwin.ballowhide) {
            gwin.abutton[2].x = gwin.abutton[1].x;
            gwin.abutton[2].y = gwin.abutton[1].y+30+b;
            gwin.abutton[2].w = gwin.abutton[1].w;
            gwin.abutton[2].h = gwin.abutton[1].h;
         } else {
            mclear(gwin.abutton[2]);
         }

         gwin.abutton[3].x = gwin.abutton[1].x;
         // gwin.abutton[3].y = gwin.abutton[1].y+30+b+30+b;
         gwin.abutton[3].y = rt.bottom - b*2 - 20 - 6;
         gwin.abutton[3].w = gwin.abutton[1].w;
         gwin.abutton[3].h = gwin.abutton[1].h;

         if (gwin.iscreen==0)
         {
            gwin.drawSlots(hdc,0,0,w-20,h);
         }

         if (gwin.iscreen==1)
         {
            // --- messages ---
            rt.left += 20;
            rt.top  += 20;

            SetBkMode(hdc, TRANSPARENT);
   
            int i = gwin.imsg;
            for (int k=0; k<20; k++)
            {
               i = (i<19) ? (i+1) : 0;
               char *psz=gwin.szmsg[i];
               if (psz[0]==0)
                  continue;
               if (!strncmp(psz,"[error] ",8)) {
                  psz+=8;
                  SetTextColor(hdc, 0x0000ff);
               } else {
                  SetTextColor(hdc, 0xffffff);
               }
               DrawText(hdc, psz, strlen(psz),
                  &rt, DT_SINGLELINE|DT_TOP|DT_LEFT|DT_NOPREFIX);
               rt.top += 15;
            }
         }

         if (gwin.aicon[1])
            DrawIcon(hdc,gwin.abutton[0].x,gwin.abutton[0].y,gwin.aicon[1]);

         if (gwin.iscreen==1) {
            if (gwin.aicon[5])
               DrawIcon(hdc,gwin.abutton[1].x,gwin.abutton[1].y,gwin.aicon[5]);
         } else {
            if (gwin.aicon[6])
               DrawIcon(hdc,gwin.abutton[1].x,gwin.abutton[1].y,gwin.aicon[6]);
         }

         if (gwin.aicon[2] && gwin.abutton[2].x)
            DrawIcon(hdc,gwin.abutton[2].x,gwin.abutton[2].y,gwin.aicon[2]);

         if (gwin.aicon[3] && gwin.abutton[3].x)
            DrawIcon(hdc,gwin.abutton[3].x,gwin.abutton[3].y,gwin.aicon[3]);

         gwin.drawToolTip(hdc);

         EndPaint(hWnd, &ps);
         break;
      }

      default:
         return DefWindowProc(hWnd, msg, wParam, lParam);
   }

   return DefWindowProc(hWnd, msg, wParam, lParam);
}

int myFileExists(char *pszName)
{
   DWORD nAttrib = GetFileAttributes(pszName);
   if (nAttrib == 0xFFFFFFFF) // "INVALID_FILE_ATTRIBUTES"
      return 0;
   return 1;
}

void MainWindow::startOthers()
{
   char szExePath[512 + 10]; mclear(szExePath);
   char szCmd[512 + 10]; mclear(szCmd);

   ::GetModuleFileName(NULL, szExePath, 512 + 2);
   // ... sfktray.exe

   char *psz = strstr(szExePath, "sfktray.exe");
   if (!psz) return;

   for (int i=1; i<=iGlblStartMore; i++)
   {
      sprintf(psz, "sfktray%d.exe", i+1);
      if (!myFileExists(szExePath))
         break;

      int iport = 21343 + i;

      snprintf(szCmd, sizeof(szCmd)-10,
         "start %s -port=%d -min -mark -silent"
         , szExePath
         , iport);

      int isubrc = system(szCmd);

      addmsg("starting %s - rc %d", psz, isubrc);
   }
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     pszCmdLine,
                     int       nCmdShow)
{
   ginst = hInstance;

   bool bstartmsg=1;

   char *psz=pszCmdLine;
   while (psz && *psz)
   {
      if (strbeg(psz,"-port=")) {
         psz+=6;
         iGivenPort = atoi(psz);
         while (*psz!=0 && *psz!=' ')
            psz++;
         while (*psz==' ')
            psz++;
         continue;
      }
      if (strbeg(psz,"-all")
          || strbeg(psz,"-2x")
          || strbeg(psz,"-3x")
         )
      {
         if (strbeg(psz,"-all")) {
            psz+=4;
            iGlblStartMore=2;
         }
         else if (strbeg(psz,"-2x")) {
            psz+=3;
            iGlblStartMore=1;
         }
         else if (strbeg(psz,"-3x")) {
            psz+=3;
            iGlblStartMore=2;
         }
         bGlblMark=1;
         while (*psz!=0 && *psz!=' ')
            psz++;
         while (*psz==' ')
            psz++;
         continue;
      }
      if (strbeg(psz,"-mark")) {
         psz+=5;
         bGlblMark=1;
         while (*psz!=0 && *psz!=' ')
            psz++;
         while (*psz==' ')
            psz++;
         continue;
      }
      if (strbeg(psz,"-min")) {
         psz+=4;
         bGlblMinimize=1;
         while (*psz!=0 && *psz!=' ')
            psz++;
         while (*psz==' ')
            psz++;
         continue;
      }
      if (strbeg(psz,"-silent")) {
         psz+=7;
         bGlblSilent=1;
         while (*psz!=0 && *psz!=' ')
            psz++;
         while (*psz==' ')
            psz++;
         continue;
      }
      break;
   }

   HWND hDeskWin = ::GetDesktopWindow();
   HDC  hdcDesk  = ::GetWindowDC(hDeskWin);
   if (hdcDesk) {
      gDeskW = GetDeviceCaps(hdcDesk, HORZRES);
      gDeskH = GetDeviceCaps(hdcDesk, VERTRES);
      ReleaseDC(hDeskWin, hdcDesk);
   }

   WNDCLASSEX wcex;
   wcex.cbSize       = sizeof(WNDCLASSEX);
   wcex.style        = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc  = (WNDPROC)MasterWndProc;
   wcex.cbClsExtra   = 0;
   wcex.cbWndExtra   = 16;
   wcex.hInstance    = hInstance;
   wcex.hIcon        = LoadIcon(hInstance, (LPCTSTR)490);
   wcex.hCursor      = LoadCursor(NULL, IDC_ARROW);
   wcex.hbrBackground= (HBRUSH)(COLOR_WINDOW+1);
   wcex.lpszMenuName = 0;
   wcex.lpszClassName= "SFKTrayClass";
   wcex.hIconSm      = 0;
   RegisterClassEx(&wcex);

   int y1 = (gDeskH - 40) - 400; if (y1<0) y1=0;
   int x1 = (gDeskW - 800) / 2;  if (x1<0) x1=0;

   makeguid();

   if (bstartmsg)
   {
      addmsg("listening on port %u.", getReceivePort());
   }

   gwin.clwin = CreateWindow(
         "SFKTrayClass",
         PRODNAME1 " " SFK_VERSION,
         WS_OVERLAPPEDWINDOW,
         x1,y1,800,400,
         NULL, NULL, ginst, NULL
         );
   ShowWindow(gwin.clwin, nCmdShow);
   UpdateWindow(gwin.clwin);

   int irc = gwin.init();
   if (irc) {
      printf("error.1.%d\n",irc);
      return 0;
   }

   SetTimer(gwin.clwin, 1, 100, 0);

   MSG msg;
   BOOL fGotMessage;
   while ((fGotMessage = GetMessage(&msg, (HWND) NULL, 0, 0)) != 0 && fGotMessage != -1)
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   if (bHaveNotifyIcon)
   {
      NOTIFYICONDATA nid;
      memset(&nid,0,sizeof(nid));
      nid.cbSize     = sizeof(nid);
      nid.hWnd       = gwin.clwin;
      nid.uFlags     = NIF_ICON | NIF_TIP | NIF_GUID | NIF_MESSAGE;
      nid.uID        = 1;
      nid.guidItem   = gGUID;
      nid.uCallbackMessage = WM_APP+1;
      nid.hIcon      = 0;
      Shell_NotifyIcon(NIM_DELETE, &nid);
   }

   DestroyWindow(gwin.clwin);

   return 0;
}
