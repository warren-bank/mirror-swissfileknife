
#define VER_NUM1 "1.2.0"
#define VER_NUM2 "120"
#define META_VERSION 120

/*
   Easy Image Viewer (iview)
   =========================
 
   (w) 2024 by J Thumm, www.stahlworks.com

   compile with vc 14:
      cl /Feiview.exe -DSFKPIC -DSFKMINCORE -I. iview.cpp sfk.cpp sfkext.cpp sfkpic.cpp iviewres.res kernel32.lib user32.lib gdi32.lib shell32.lib comdlg32.lib advapi32.lib /link /SUBSYSTEM:WINDOWS

   to rebuild resources:
      imgconv iviewres.bmp iviewres.ico
      (imagemagick, from 24 bit bmp)
      rc iviewres.rc

   1.2.0 19.10.24
      -  released as part of the sfk project on sourceforge

   1.1.9 28.08.24
      -  add: save second image index by shift+s, recall by shift+l

   1.1.8 15.04.24
      -  chg: no longer showing centered load status during reload

   1.1.7 11.04.24
      -  official release
*/


#define _WIN32_IE 0x600

#include "sfkbase.hpp"
#include "sfkint.hpp"

#include <shobjidl.h> 
#include <stdint.h>

#define PROG_NAME  "Easy Image Viewer"
#define CLASS_NAME "StwIView"

#define WITH_COLORMARK
#define WITH_BACK_RAND
#define WITH_ROTATE


#define MAX_FILES 300000

#define uchar  unsigned char
#define ushort unsigned short
#define uint   unsigned int
typedef unsigned char uint8_t;
typedef unsigned int  pixel_t;
typedef int inc;

int bverb = 0;
num tGlblStart = 0;

int iInfoFontHeight = 24;

int mbox(const char *psz, int n);
int getFileMD5(char *pszFile, uchar *abOut16);
bool isAbsolutePath(char *psz1);
void centerWithinArea(HWND hDlg);
bool strEnds(char *pszStr, cchar *pszPat);
extern bool bGlblEscape;
extern unsigned char abBuf[];

#ifndef SFK_V2
int getFileStat( // RC == 0 if exists anything
   char  *pszName,
   int   &rbIsDirectory,
   int   &rbCanRead,
   int   &rbCanWrite,
   num   &rlFileTime,
   num   &rlFileSize,
   num   *ppcatimes  = 0,  // optional: creation and access time
   void  *prawstat   = 0,  // optional: create copy of stat structure
   int    nrawstatmax= 0,  // size of above buffer
   uint   nmodeflags = 0   // bit 0: use alternative stat, if available
   );
#endif

#define REALM_DST 0
#define REALM_SRC 1  // unused

#define dst arealm[0]
#define src arealm[1]
#define cur arealm[clcurrealm]

char szGlblOwnDir[SFK_MAX_PATH+50];
char glblDialogURL[500];

char *absPath(char *pszFile)
{
   static char szPath[SFK_MAX_PATH+50];
   if (isAbsolutePath(pszFile))
      return pszFile;
   joinPath(szPath, sizeof(szPath)-10, szGlblOwnDir, pszFile);
   return szPath;
}

#define __

enum UIKeyIDs {
   IDLeft = 0x6c65,
   IDRite = 0x7269,
   IDUp   = 0x7570, // "up"
   IDDown = 0x646f, // "do"
   IDWithSub = 0x7773
};

void checkEscape()
{
   SHORT nksEsc = GetKeyState(VK_ESCAPE);
   bool bEsc = (nksEsc & (1 << 15)) ? 1 : 0;
   if (bEsc)
      exit(0);
}

void mystrcatf(char *pOut, int nOutMax, cchar *pszFormat, ...);

int myabs(int x)
{
   if (x < 0) return 0-x;
   return x;
}

double myabs(double x)
{
   if (x < 0.0) return 0.0-x;
   return x;
}

ulong aPaleCurve[16] = {
   100, 100, 100, 100, 100,
   100, 100, 100,  90,  80,
    70,  60,  50,  40,  20, 0
};

ulong paleColor(ulong ncol, ulong nperc)
{
   ulong nperc2 = 100 - aPaleCurve[(nperc / 7) & 15];
   ulong n1 = (ncol >> 16) & 0xFF;
   ulong n2 = (ncol >>  8) & 0xFF;
   ulong n3 = (ncol >>  0) & 0xFF;
   #if 1
   n1 = n1 - ((0xFF - n1) * nperc / 100);
   n2 = n2 - ((0xFF - n2) * nperc / 100);
   n3 = n3 - ((0xFF - n3) * nperc / 100);
   #else
   n1 = ((0xFF - n1) * nperc / 100) + n1;
   n2 = ((0xFF - n2) * nperc / 100) + n2;
   n3 = ((0xFF - n3) * nperc / 100) + n3;
   #endif
   ulong nmix = (n1 << 16) | (n2 << 8) | n3;
   return nmix;
}

#define DVTIMER_DISPLAY     2
#define DVTIMER_INFO        3
#define DVTIMER_ALERT       4
#define DVTIMER_MARKDEMO    5
#define DVTIMER_SHOWDEMO    6
#define DVTIMER_ANIMSCRIPT  7
#define DVTIMER_AUTORELOAD  8

static char szDVLogBuf[MAX_LINE_LEN+10];

#define MAX_DVLOG_LINES   100
#define MAX_DVLOG_LINELEN 200
#define DVLOG_SHOW_SEC      6
char  aLogLines[MAX_DVLOG_LINES+2][MAX_DVLOG_LINELEN];
num   aLogTimes[MAX_DVLOG_LINES+2];
uchar aLogFlags[MAX_DVLOG_LINES+2];
// bit 0 : temporary message, can be overwritten by next

long nGlblShowLog     = 0;
bool bGlblShowLogWide = 1;

void initdvlog()
{
   memset(aLogLines, 0, sizeof(aLogLines));
   memset(aLogTimes, 0, sizeof(aLogTimes));
   memset(aLogFlags, 0, sizeof(aLogFlags));
}

// defined prefixes:
// [error]  red error
// [info]   grey info even w/o showlog set
// [note]   orange note
// [blue]   blue note
// non-prefix lines are listed only w/ showlog set

void dvlog(const char *pszFormat, ...)
{
   va_list argList;
   va_start(argList, pszFormat);
   ::vsnprintf(szDVLogBuf, sizeof(szDVLogBuf)-10, pszFormat, argList);
   szDVLogBuf[sizeof(szDVLogBuf)-10] = '\0';

   // make sure there is no null string, as this flags EOD:
   if (!szDVLogBuf[0]) strcpy(szDVLogBuf, " ");

   // flatten some unprintable chars
   for (char *psz1=szDVLogBuf; *psz1; psz1++)
      switch (*psz1) {
         case '\r': case '\n': case '\t':
            *psz1 = ' ';
            break;
      }

   if (!aLogFlags[0]) {
      memmove(&aLogLines[1][0], &aLogLines[0][0], (MAX_DVLOG_LINES-1) * MAX_DVLOG_LINELEN);
      memmove(&aLogTimes[1]   , &aLogTimes[0]   , (MAX_DVLOG_LINES-1) * sizeof(num));
   }

   num nKeepTime = 0;
   uchar nflags  = 0;

   // check for prefix flags
   char *pszMsg = szDVLogBuf;
   if (pszMsg[0] == '~') {
      pszMsg++;
      nflags |= 1; // temporary
   }
   if (pszMsg[0] == '!') {
      pszMsg++;
      nKeepTime  = 10000;
   }
   if (!strncmp(pszMsg, "[error] ", 8))
      nKeepTime  =  5000;
   if (!strncmp(pszMsg, "[silent] ", 9)) {
      pszMsg += 9;
      nKeepTime -= 30000;
   }

   // if lines must be truncated, do it right-justified:
   char *psz = pszMsg;
   long nlen = strlen(psz);
   if (nlen > MAX_DVLOG_LINELEN-10)
         psz += nlen - (MAX_DVLOG_LINELEN-10);
   strcopy(aLogLines[0], psz);

   aLogTimes[0] = getCurrentTime() + nKeepTime;
   aLogFlags[0] = nflags;

   mtklog(("dvlog: %s", pszMsg));
}

static int perr(const char *pszFormat, ...)
{
   char szErrBuf[500];
   va_list argList;
   va_start(argList, pszFormat);
   ::vsnprintf(szErrBuf, sizeof(szErrBuf)-10, pszFormat, argList);
   szErrBuf[sizeof(szErrBuf)-10] = '\0';
   printf("error: %s\n", szErrBuf);
   return 0;
}

char *vtext(const char *pszFormat, ...)
{
   static char szBuf[4096];
   va_list argList;
   va_start(argList, pszFormat);
   ::vsnprintf(szBuf, sizeof(szBuf)-10, pszFormat, argList);
   szBuf[sizeof(szBuf)-10] = '\0';
   return szBuf;
}

char *mynumtohex(unum n)
{
   static char szBuf[100];
   sprintf(szBuf, "%016I64x", n);
   return szBuf;
}

#define IDI_CHECK          130
#define IDI_SMALL          108

HICON img1=0;
HICON img2=0;

HCURSOR glblScalePartCursor = 0;
HCURSOR glblNormCursor = 0;

#undef _mkdir

static int existsFile(char *pszName, int bOrDir=0)
{
   DWORD nAttrib = GetFileAttributes(pszName);

   if (nAttrib == 0xFFFFFFFF) // "INVALID_FILE_ATTRIBUTES"
      return 0;

   if (!bOrDir && (nAttrib & FILE_ATTRIBUTE_DIRECTORY))
      return 0; // is a dir, not a file

   return 1;
}

char *strend(char *psz, int n)
{
   int ilen=strlen(psz);
   if (ilen<=n) return psz;
   return psz+ilen-n;
}

// uses abBuf
int copyFileFlat(char *psrc, char *pdst, int iquiet=0)
{
   bool bCleanup = 0;

   FILE *fin = fopen(psrc, "rb");
   if (!fin) {
      if (iquiet < 1) {
         perr(0, "cannot open input file:");
         dvlog("[error] %s", psrc);
      }
      return 9;
   }

   FILE *fout = fopen(pdst, "wb");
   if (!fout) {
      if (iquiet < 2) {
         perr(0, "cannot open output file:");
         dvlog("[error] %s", pdst);
      }
      fclose(fin);
      return 9;
   }

   long nrc = 0;
   num ntotal = 0;

   while (1)
   {
      size_t nread  = fread(abBuf, 1, MAX_ABBUF_SIZE, fin);
      if (!nread) break; // EOD

      size_t nwrite = fwrite(abBuf, 1, nread, fout);
      if (nwrite != nread)
      {
         perr(0, "write error, probably disk full:");
         dvlog("[error] %s", pdst);
         bCleanup = 1;
         nrc = 9;
         break;
      }
      ntotal += nread;
   }

   fclose(fout);
   fclose(fin);
 
   if (bCleanup)
      remove(pdst);
 
   return nrc;
}

ulong myulfromucle(uchar *p)
{
   return   (((ulong)(*(p+0))) <<  0)
          | (((ulong)(*(p+1))) <<  8)
          | (((ulong)(*(p+2))) << 16)
          | (((ulong)(*(p+3))) << 24);
}

unum myunumfromucbe(uchar *p)
{
   return   (((unum)(*(p+7))) <<  0)
          | (((unum)(*(p+6))) <<  8)
          | (((unum)(*(p+5))) << 16)
          | (((unum)(*(p+4))) << 24)
          | (((unum)(*(p+3))) << 32)
          | (((unum)(*(p+2))) << 40)
          | (((unum)(*(p+1))) << 48)
          | (((unum)(*(p+0))) << 56)
          ;
}

uint8_t  red   (pixel_t c) { return (uint8_t)(c >>  0); }
uint8_t  grn   (pixel_t c) { return (uint8_t)(c >>  8); }
uint8_t  blu   (pixel_t c) { return (uint8_t)(c >> 16); }
uint8_t  alp   (pixel_t c) { return (uint8_t)(c >> 24); }

pixel_t topix  (uint8_t a,uint8_t r,uint8_t g,uint8_t b)
{
   return
         (((pixel_t)a) << 24)
      |  (((pixel_t)r) <<  0)
      |  (((pixel_t)g) <<  8)
      |  (((pixel_t)b) << 16)
      ;
}

int norm(int n)
{
   if (n<0) return 0;
   if (n>255) return 255;
   return n;
}

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)
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

bool isblankpix(pixel_t ppic)
{
   int r1=red(ppic);
   int g1=grn(ppic);
   int b1=blu(ppic);
   int a1=alp(ppic);
   if (r1>250 && g1>250 && b1>250)
      return 1;
   if (a1<200)
      return 1;
   return 0;
}

void splitcol(pixel_t c,
   int &r1,int &g1,int &b1,int &a1,
   double *h1=0,double *s1=0,double *v1=0)
{
   r1=red(c);
   g1=grn(c);
   b1=blu(c);
   a1=alp(c);
   if (h1 && s1 && v1)
      rgbtohsv(r1/255.0,g1/255.0,b1/255.0,h1,s1,v1);
}

double hue(pixel_t c)
{
   int    r1,g1,b1, a1,a2,a3, b2,b3;
   double h1,s1,v1;
   splitcol(c,r1,g1,b1,a1,&h1,&s1,&v1);
   return h1;
}

double dirFromDelta(double dx,double dy)
{
   //double dist=sqrt(dy+dx);
   double rad=atan2(dy,dx);
   double ang=rad*(180.0/3.141);
   ang = ang+90.0;
   if (ang<0.0) return ang+360.0;
   return ang;
}

double calcAngleDiff(double d1,double d2)
{
   double phi = (int)(myabs(d2-d1)) % 360;
   double dist = (phi > 180.0) ? 360.0-phi : phi;
   return dist;
}

bool samedir(double d1,double d2)
{
   if (calcAngleDiff(d1,d2) < 60.0) // critical
      return 1;
   return 0;
}

#ifndef INTERNAL
const char *aGlblImageList[3] = {
   "list1.txt", "list2.txt", "list3.txt"
   };
#endif

char szGlblListBuf[100000];

struct ViewPic
{
   char *name;
   num   time_write;
   num   size;
   uint  hashv1;
   unum  hashv2;
   bool  issrc;
   bool  isdst;
};

#define cur arealm[clcurrealm]

#define MAX_UI_ELEM 100

struct UIElem
{
   int    id;
   cchar  *pszid;
   int    x,y,w,h,hw,hh;
   int    flags;
   char   text[1024];
   num    tclickstart;
};

// .
class View
{
public:
      View  ( );

int   err   (cchar *pszMask, ...);  // low prio error
int   alert (cchar *pszTitle, cchar *pszFormat, ...);
int   errpop(cchar *pszTitle, cchar *pszMask, ...);
int   msg   (cchar *pszMask, ...);  // optional messages

void
      update            ( ),
      draw              ( ),
      closeView         ( ),
      setDefaultScale   ( ),
      copytoMoveFolder  ( ),
      moveToMoveFolder  ( ),
      askDelete         ( ),
      reset             ( ),
      toggleMinimize    ( ),
      updateMixPic      ( ),
      addToImageList     (char *pname);

int
      create      (HINSTANCE hInstance),
      loadpic     (char *pszFile=0, cchar *pszReason=0, bool bNoMetaLoad=0, int ipicrot=0),
      nextpic     (int idelta, bool bRequireMarked=0),
      scandir     (char *psz, int ilevel=0),
      loadfilelist      (char *pszFile),
      loadfilelist      (FILE *fin),
      gotoImageNumber   (int iOptIdx=-1);
bool
      mysystem          (char *pszCmd, long &rsubrc, bool bWaitForCompletion, bool bSilent=0);

int
      scalexout   (int x, bool bWithOffset),
      scaleyout   (int y, bool bWithOffset),
      scalexin    (int x, bool bWithOffset),
      scaleyin    (int y, bool bWithOffset);

void
      changeToFolder    (char *psz, bool breload=0),
      setdstdir         (char *psz),
      srcFromDst        (int &srcx,int &srcy,inc dstx,inc dsty),
      moveImageTo       (cchar *ppath),
      setwinpix         (int xc,int yc,pixel_t c);
char *getOutputPath     (cchar *pszFolder);
double
      distance          (double dx, double dy),
      percFromDist      (double distrefabs, double maxrange);
void
      gotoRandFile      ( );
pixel_t
      getwinpix         (int xc,int yc);

LRESULT process   (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int      clmixer;
int      clverbose;
int      clpendingkey;

// realm-independent display
HWND     clwin;
HFONT    clfont1, clfont2, clfont3, clfont4, clfont5, clfontl;

HDC      clhdc;
HBITMAP  clwinbm;
HGDIOBJ  clrecentbm;
pixel_t  *clwinpix;

int      clwinw;
int      clwinh;
int      clmaxwinpix;
int      clwinmousex;
int      clwinmousey;
int      clbuttondown;
int      clbuttonstartx;
int      clbuttonstarty;
int      clbuttoncurx;
int      clbuttoncury;
int      clshowui;
int      clclearscreen;
int      startupdone;
int      clshowinfo;
int      clmousemode;
int      cltracex;
int      cltracey;
int      clalphasort;
int      clwithsub;
int      cltestvalue;
int      clshowmanual;
int      clmanline;
int      clskipclearscreen;
int      clskipdrawmixpic;
int      clkeyfocus;
int      clquietmode;
int      clshowfastfile;
int      clshowingfastfile;
int      clremembered;
int      clautoreload;
num      clscanstart,clscantold;
int      clfilesscanned;
int      clinreload;

// single file list
char     *clfilenamelist;
int      clloadfilelist;
int      clstdinlist;
         // 2: read filenames then use
         // 1: just use
char     clidumpfile[200];
char     folder[SFK_MAX_PATH+10];
ViewPic  afile[MAX_FILES+10];
int      nfile;

SFKPic    src_mempic;
SFKPic    dst_mempic;
char      szTextTag[4096];
char      szTextCopy[4096];

int       clanimprog, clhotshotinfo, clmobilex;

char     exportpath[200];
char     exportname[200];
char     exportfullname[200];
char     imageeditor[SFK_MAX_PATH+10];

struct PicRealm
{
char     pathmask[200];
uint     aselfiles[MAX_FILES+10];
int      nselfiles;

SFKPic   mixpic;

int      metaver;
int      fileedition;

double   clcoordfactory; // factor of loaded versus real size
double   clcoordfactorx; // factor of loaded versus real size
int      clcoordxoff;    // in-memory x offset from left

int      clrotate;       // 0 90 180 270 applied directly after load

int      mempicw;
int      mempich;
int      memfocusx;
int      memfocusy;
int      startfocusx,endfocusx;
int      startfocusy,endfocusy;
int      hasalpha;
int      alphamode;
#define  ALPHAMODE_NONE    0
#define  ALPHAMODE_USE_SRC 1  // use alpha info from source

int      scanx1;
int      scany1;
int      scanw;
int      scanh;

int      memmousex;
int      memmousey;

double   viewscale;
double   startscale,endscale;
double   mdscale; // mark demo initial scale
double   hip;  // relative to SELECTED part
double   miw;  // milliwidth of loaded image
double   mid;  // millidiameter of loaded image
int      borderw;
int      borderh;

int      tloadpic;
int      loadrc;

int      curfile;
int      savepos; // saved index by ctrl+s
int      savepos2; // saved index by shift+s

char     szVariants[200]; // for curfile

#define MAX_RECENT_POS_MASK 127
uint      aclRecentPos[127+10];
int       clRecentPosIndex;

#define MAX_RECENT_RAND 1000
uint      aclRecentRand[1000+100];
uint      clRecentRandIndex;
}
arealm[2];

#define MAX_IO_BUF 100000

int      clcurrealm;
int      cleditrealm;
int      cleditedpix;
int      atold[10];
char     clmetadir[SFK_MAX_PATH+10];
char     clmovedir[SFK_MAX_PATH+10];

char     szClCryptKey[100];
char     cliobuf[MAX_IO_BUF+100];
int      clioidx;
int      clioerr;

int      initiobuf   ( );
int      addiobuf    (const char *pszMask, ...);
int      iobuferr    ( );

enum UIFlags {
   ShowSelectHighlight  = (1U << 0),
   UIStartScreen        = (1U << 10),
   UIStartButton        = (1U << 11),
   UILargeFont          = (1U << 12),
   UIJustText           = (1U << 13),
   UINextLine           = (1U << 14),
   UIMultiLineText      = (1U << 15),
   UILowCol             = (1U << 16),
};

// user interface
bool      isEditAreaFullyMarked ( );
bool      isOuterReferenceFullyMarked ( );
bool      issel (cchar *psz);
bool      inMarkAndShowEditMode ( );
bool      haveMetaDir ( );
void      clearui ( );
void      toggleui ( );
void      toggleMixerMode ( );
void      moveInManual (int idelta);
void      saveSettings( );
void      callImageEditor(bool bWithCopying);
void      loadSettings( );
int       provideMetaDir( );
bool      showui  ( );
void      addui   (cchar *id, int hw, int hh, cchar *text, int iflags);
void      makeui  ( );
void      drawui      (HDC hdc);
void      drawmanual  (HDC hdc);
void      clearscreen (HDC hdc);
int       getuielem     (int x, int y);
int       processUIElem (int x, int y, int ieventflags);
int       processKey    (int c, int iFlags=0);
void      stopAutoReload(int iFrom, int iInfo=0);
void      handleTimer   (ulong nid);
void      checkDisplayUpdates ( );
void      playVideo  (int64_t tstartpts, int64_t tendpts);
void      closeVideo ( );
int       numberOfPoints(int itype);
int       numberOfSrcPoints(int itype);
int       nextEditPoint(int itype);
bool      currentPicIsPartOfList(int ilist);

double    genw, genh; // generic width/height, 1/1000 of display width/height
UIElem    aelem   [MAX_UI_ELEM+2];
int       nelem;
int       uibasex, uicurx, uicury;
int       clstartscreen;
num       nClADUPending; // timestamp of last auto display update pending
int       nClLogTop;
int       nClLogLeft;
HINSTANCE clinstance;

#define MAX_SELECTED 10
cchar     *aclselected[MAX_SELECTED+4];
int       nclselected;

#if WITH_OPTIM_1
double    aclDeltaFromDir[3600][2];
#endif
double    aclRotate[360][2];

int       info(int nmsec, cchar *pszFormat, ...);
char      szClInfo[200];

num       clAlertStart;
char      szClAlertTitle[200];
char      szClAlertText[200];

};

View gview;

View::View( )
{
   memset(this, 0, sizeof(*this));

   for (int i=0; i<2; i++)
   {
      clcurrealm = i;

      cur.viewscale = 1.0;
      cur.endscale = 0.0;
      cur.savepos = -1;
      cur.savepos2 = -1;

      reset();
   }

   clcurrealm = 0;
   clwithsub = 1;
   clremembered = -1;
}

int View::err(cchar *pszFormat, ...)
{
   char szBuf[500];

   va_list argList;
   va_start(argList, pszFormat);
   ::vsnprintf(szBuf, sizeof(szBuf)-10, pszFormat, argList);
   szBuf[sizeof(szBuf)-10] = '\0';

   dvlog("[error] %s", szBuf);

   update();

   return 0;
}

int View::alert(cchar *pszTitle, cchar *pszFormat, ...)
{
   char szBuf[500];

   va_list argList;
   va_start(argList, pszFormat);
   ::vsnprintf(szBuf, sizeof(szBuf)-10, pszFormat, argList);
   szBuf[sizeof(szBuf)-10] = '\0';

   clAlertStart = getCurrentTime();

   // dvlog("[error] %s", pszTitle);
   // dvlog("[error] %s", szBuf);

   strcopy(szClAlertTitle, pszTitle);
   strcopy(szClAlertText, szBuf);

   SetTimer(clwin, DVTIMER_ALERT, 3000, 0);

   update();

   return 0;
}

int View::errpop(cchar *pszTitle, cchar *pszFormat, ...)
{
   char szBuf[500];

   va_list argList;
   va_start(argList, pszFormat);
   ::vsnprintf(szBuf, sizeof(szBuf)-10, pszFormat, argList);
   szBuf[sizeof(szBuf)-10] = '\0';

   MessageBox(0, szBuf, pszTitle, MB_OK);

   return 0;
}

int View::msg(cchar *pszFormat, ...)
{
   char szBuf[500];

   va_list argList;
   va_start(argList, pszFormat);
   ::vsnprintf(szBuf, sizeof(szBuf)-10, pszFormat, argList);
   szBuf[sizeof(szBuf)-10] = '\0';

   dvlog("%s", szBuf);

   return 0;
}

int View::info(int nmsec, cchar *pszFormat, ...)
{
   va_list argList;
   va_start(argList, pszFormat);
   ::vsnprintf(szClInfo, sizeof(szClInfo)-10, pszFormat, argList);
   szClInfo[sizeof(szClInfo)-10] = '\0';

   SetTimer(clwin, DVTIMER_INFO, nmsec, 0);

   return 0;
}

void View::toggleui( )
{__
   clshowui ^= 0x1;
   if (!clshowui && !nfile) {
      errpop("No images loaded so far", "Use F2 as soon as images are displayed.");
      clshowui = 1;
   }
   if (showui())
      clearui();
}

void View::clearui( )
{__
   memset(aelem, 0, sizeof(aelem));
   nelem = 0;
   uicurx = genw * 10;
   uicury = genh * 20;
   nclselected = 0;
   if (clscanstart && !clinreload)
   {
      int totalw = clwinw;
      int totalh = clwinh;
      uicurx = (totalw-genw*10)/2;
      uicury = (totalh-genh*(10+400))/2;
      uibasex = uicurx;
   }
}

// rc: index or -1
int View::getuielem(int x, int y)
{__
   if (!showui())
      return -1;
   for (int i=0; i<nelem; i++)
   {
      if (x<aelem[i].x || y<aelem[i].y
          || x>(aelem[i].x+aelem[i].w)
          || y>(aelem[i].y+aelem[i].h))
         continue;
      return i;
   }
   return -1;
}

bool View::haveMetaDir( )
{
   if (!clmetadir[0])
      return 0;
   return isDir(clmetadir);
}

int View::provideMetaDir( )
{
   if (!clmetadir[0]) {
      MessageBox(0,
         vtext("no meta dir name exists for:\n%s\n"
               "select a folder with '\\' in it's path"
               , folder),
         "Writing not possible", MB_OK);
      return 9;
   }
   if (isDir(clmetadir))
      return 0;
   #ifndef USE_MEM_FS
   int irc = 0;
      irc = MessageBox(0,
         vtext("A data folder is needed to save settings:\n\n"
               "%s\n\n"
               "Create this now?"
               , clmetadir),
         "Create meta data folder?",
         MB_YESNOCANCEL);
   if (irc != IDYES) {
      return 5;
   }
   #endif
   if (_mkdir(clmetadir)) {
      MessageBox(0, "cannot create folder", "error", MB_OK);
      clmixer = 0;
      return 10;
   }
   return 0;
}


void View::saveSettings( )
{
   if (provideMetaDir()) // saveSettings
      return;

   char szText[1024];

   sprintf(szText,
      "quietmode %d\n"
      "curimg %d\n"
      "curimg2 %d\n"
      , clquietmode
      , cur.savepos+1
      , cur.savepos2+1
      );

   char *pfile=vtext("%s\\zz-settings.txt",clmetadir);
   int rc = saveFile(pfile,(uchar*)szText,strlen(szText),"wb");
   if (rc)
      MessageBox(0, pfile, "Cannot write", MB_OK);
}

void View::loadSettings( )
{
   if (!clmetadir[0])
      return;

   char szText[1024];

   char *pfile=vtext("%s\\zz-settings.txt",clmetadir);
   char *ptext = loadFile(pfile, 1); // quiet
   if (!ptext) return;

   int iGotoImg=-1;

   if (ptext)
   {
      char *p = ptext;
      while (p && *p)
      {
         char *pnext = 0;
         char *p2 = p;
         while (*p2!=0 && *p2!='\r' && *p2!='\n') p2++;
         if (*p2!=0)
         {
            *p2++ = '\0';
            while (*p2!=0 && (*p2=='\r' || *p2=='\n')) p2++;
            pnext = p2;
         }
         // now 'p' is zero terminated
 
        do
        {
         if (!strncmp(p, "quietmode ", 10)) {
            clquietmode=atoi(p+10);
            break;
         }
         if (!strncmp(p, "curimg ", 7)) {
            int n=atoi(p+7);
            if (n>0 && n<=nfile)
               cur.savepos=n-1;
            break;
         }
         if (!strncmp(p, "curimg2 ", 8)) {
            int n=atoi(p+8);
            if (n>0 && n<=nfile)
               cur.savepos2=n-1;
            break;
         }
 
        } while (0);
 
         if (!pnext)
            break;
 
         p = pnext;
      }

      delete [] ptext;
      ptext = 0;
   }

   // if (iGotoImg!=-1)
   //   gotoImageNumber(iGotoImg);
}

void View::reset( )
{__
   cur.metaver = 0;     // wm121 missing
   cur.fileedition = 0; // reset() [2021131]
   cur.clrotate = 0;
}

// .
void View::makeui( )
{__
   clearui();

   if (nfile || clscanstart)
   {
      if (!clscanstart || clinreload)
         uibasex = 0;
      clstartscreen = 0;
   }
   else
   {
      uibasex = 0; // dummy, changed in addui
      clstartscreen = 1;

      addui(" ", 200, 60, PROG_NAME " " VER_NUM1,
         UIStartScreen|UILargeFont|UIJustText|UINextLine);
      // addui(" ", 200, 30, "easy lightweight image viewer", UIJustText|UINextLine);
      addui(" ", 200, 30, "www.StahlWorks.com", UIJustText|UINextLine);
      addui(" ", 200, 30, "", UIJustText|UINextLine);

      addui("o", 200, 130, "Open", UIStartButton|UILargeFont|UINextLine);

      addui("ws", 200, 30,
         vtext("[%c] include sub folders", clwithsub ? 'X':' '),
         UINextLine);
      addui(" ", 200, 30, "", UIJustText|UINextLine);
      addui("f1", 200, 30, "F1 to show help.", UIJustText|UINextLine);
      return;
   }

   addui(" ", 100, 10, "", UIJustText|UINextLine);

   if (clscanstart && !clinreload)
   {  
      if (getCurrentTime() - clscanstart > 200)
      {
         addui(" ", 100, 20, " Scanning folder ...", UIJustText|UINextLine);
         addui(" ", 100, 20, "", UIJustText|UINextLine);
         addui(" ", 100, 20, vtext(" %d files",clfilesscanned), UIJustText|UINextLine);
         addui(" ", 100, 20, vtext(" %d images",cur.curfile+1), UIJustText|UINextLine);
         addui(" ", 100, 20, "", UIJustText|UINextLine);
         addui(" ", 100, 20, " press Esc to exit", UIJustText|UINextLine);
         addui(" ", 100, 20, "", UIJustText|UINextLine);
      }
   }
   else
      addui(" ", 100, 20, vtext(" %d / %d",cur.curfile+1,nfile), UIJustText|UINextLine);

   if (clremembered>=0)
      addui(" ", 100, 20, vtext(" r %d",clremembered+1), UIJustText|UINextLine);

   if (nfile < 1)
      return;

   /*
      - current relative filename
      - var n of n
      - session duration and time
      - remembered info
   */
   char szShort[100];
   char *pszAbs = afile[cur.curfile].name;
   strcopy(szShort, pszAbs);
   char *pszDot = strrchr(szShort, '.');
   if (pszDot != 0 && pszDot > szShort)
      *pszDot = '\0';
   char *pszRel = strrchr(szShort, '\\');
   if (pszRel)
      pszRel++;
   else
      pszRel = szShort;
   if (strlen(pszRel) > 20)
      pszRel = pszRel+strlen(pszRel)-20;
   // addui(" ", 100, 20, vtext(" %s",pszRel), UIJustText|UINextLine);

   #ifndef INTERNAL
   const char *flag1 = currentPicIsPartOfList(0) ? "L1 " : "";
   const char *flag2 = currentPicIsPartOfList(1) ? "L2 " : "";
   const char *flag3 = currentPicIsPartOfList(2) ? "L3 " : "";
   #endif

   char szFlag[100];
   strcpy(szFlag, " ");

   if (flag1) strcat(szFlag, flag1);
   if (flag2) strcat(szFlag, flag2);
   if (flag3) strcat(szFlag, flag3);

   addui(" ", 100, 20, szFlag, UIJustText|UINextLine);

   int iSessionSec = (getCurrentTime()-tGlblStart)/1000;
   int iSessionHou = iSessionSec/3600;
       iSessionSec -= iSessionHou*3600;
   int iSessionMin = iSessionSec/60;

   char szNow[100]; szNow[0]='\0';
   num nnow = time(0);
   struct tm *pnow = localtime(&nnow);
   if (pnow)
      strftime(szNow, sizeof(szNow)-10, "%H:%M", pnow);

   if (iSessionHou > 0)
      addui(" ", 100, 20, vtext(" %s %02d:%02d",szNow,iSessionHou,iSessionMin), UILowCol|UIJustText|UINextLine);
   else if (iSessionMin > 0)
      addui(" ", 100, 20, vtext(" %s %dm",szNow,iSessionMin), UILowCol|UIJustText|UINextLine);
   else
      addui(" ", 100, 20, vtext(" %s",szNow), UILowCol|UIJustText|UINextLine);

   strcopy(szTextCopy,szTextTag);
   char *pstart = szTextCopy;

   char *pszSeed = strstr(pstart, ", Seed: ");
   if (pszSeed)
      pszSeed += strlen(", Seed: ");

   char *pszSubSeed = strstr(pstart, ", Variation seed: ");
   if (pszSubSeed)
      pszSubSeed += strlen(", Variation seed: ");

   char *pszSubStr = strstr(pstart, ", Variation seed strength: ");
   if (pszSubStr)
      pszSubStr += strlen(", Variation seed strength: ");

   char *pszID = strstr(pstart, " id=");
   if (pszID)
      pszID += strlen(" id=");

   addui(" ", 100, 20, "", UILowCol|UIJustText|UINextLine);

   char *ptmp=0;
   if (pszID) {
      ptmp=pszID; while (*ptmp!=0 && (isalnum(*ptmp)!=0 || *ptmp=='/')) ptmp++; *ptmp='\0';
      addui(" ", 100, 20, vtext(" id %s",pszID), UILowCol|UIJustText|UINextLine);
      addui(" ", 100, 20, "", UILowCol|UIJustText|UINextLine);
   }
   if (pszSeed) {
      ptmp=pszSeed; while (*ptmp!=0 && *ptmp!=',') ptmp++; *ptmp='\0';
      addui(" ", 100, 20, vtext(" sd %s",pszSeed), UILowCol|UIJustText|UINextLine);
   }
   if (pszSubSeed) {
      ptmp=pszSubSeed; while (*ptmp!=0 && *ptmp!=',') ptmp++; *ptmp='\0';
      addui(" ", 100, 20, vtext(" su %s",pszSubSeed), UILowCol|UIJustText|UINextLine);
   }
   if (pszSubStr) {
      ptmp=pszSubStr; while (*ptmp!=0 && *ptmp!=',') ptmp++; *ptmp='\0';
      addui(" ", 100, 20, vtext(" st %s",pszSubStr), UILowCol|UIJustText|UINextLine);
   }


   if (clautoreload) {
      addui(" ", 100, 20, "", UILowCol|UIJustText|UINextLine);
      addui(" ", 100, 20, " AutoReload (Ctrl+R)", UILowCol|UIJustText|UINextLine);
   }
}

bool View::currentPicIsPartOfList(int ilist)
{
   char *pszPic = afile[cur.curfile].name;
   if (!pszPic || !*pszPic) return false;

   const char *prelname = aGlblImageList[ilist];

   #ifndef INTERNAL
   if (!haveMetaDir()) // currentPicIsPartOfList
      return false;
   char *plname = vtext("%s\\%s",clmetadir,prelname);
   #endif

   memset(szGlblListBuf,0,sizeof(szGlblListBuf));

   FILE *fin=fopen(plname,"rb");
   if (!fin) return false;
   fread(szGlblListBuf,1,sizeof(szGlblListBuf)-10,fin);
   fclose(fin);

   if (!strstr(szGlblListBuf, pszPic))
      return false;   

   return true;
}

bool View::showui( )
{
   if (clshowui)
      return 1;

   if (clscanstart && !clinreload)
      return 1;

   return nfile ? 0 : 1;
}

void View::addui(cchar *pid, int hw, int hh, cchar *text, int iflags)
{__
   if (nelem >= MAX_UI_ELEM) return;

   int id = 0;
   cchar *psz=pid;
   for (; *psz; psz++)
       id = (id << 8) | ((int)*psz);

   int istepx = genw * 4;
   int istepy = genh * 4;

   if (iflags & UIStartScreen) // first element of start screen
   {
      int totalw = clwinw;
      int totalh = clwinh;
      uicurx = (totalw-genw*hw)/2;
      uicury = (totalh-genh*(hh+400))/2;
      uibasex = uicurx;
   }
   if (hw >= 400) // start help
   {
      uicurx -= genw*100;
   }

   aelem[nelem].x = uicurx;
   aelem[nelem].y = uicury;
   aelem[nelem].w = genw * hw - istepx;
   aelem[nelem].h = genh * hh - istepy;
   aelem[nelem].hw = hw;
   aelem[nelem].hh = hh;
   aelem[nelem].id = id;
   aelem[nelem].pszid = pid;
   aelem[nelem].flags = iflags;
   strcopy(aelem[nelem].text, text);

   if ((iflags & UINextLine)) {
      uicurx = uibasex;
      uicury += genh * hh;
   } else {
      uicurx += genw * hw;
   }

   nelem++;

   if ((iflags & ShowSelectHighlight) != 0)
   {
      if (nclselected<MAX_SELECTED)
         aclselected[nclselected++] = pid;
   }
}

void View::drawui(HDC hdc)
{__
   RECT orect;

   HBRUSH hbackunsel    = CreateSolidBrush(0x999999);
   HBRUSH hbacksel      = CreateSolidBrush(0xDDDDDD);
   HBRUSH hbacktext     = CreateSolidBrush(0);
   HBRUSH hhotback      = CreateSolidBrush(0xBBBBBB);
   HBRUSH houterrim     = CreateSolidBrush(0xFFFFFF);
   HBRUSH hinnerpassive = CreateSolidBrush(0x888888);
   HBRUSH hinneractive  = CreateSolidBrush(0x00FF00);

   HFONT  hOldFont = 0;

   for (int i=0; i<nelem; i++)
   {
      HBRUSH hinnerrim = hinnerpassive;
      HBRUSH hback = hbackunsel;

      char *ptext = aelem[i].text;
      if (!strlen(ptext))
         continue;

      int iflags = aelem[i].flags;
      if (iflags & ShowSelectHighlight)
         hback = hbacksel;

      num tclick = aelem[i].tclickstart;
      if (tclick != 0 && getCurrentTime()-tclick<500)
         hinnerrim = hinneractive;

      bool btext = (iflags & UIJustText) ? 1: 0;
      bool blowcol = (iflags & UILowCol) ? 1: 0;
      int bordx = (iflags & UIJustText) ? 0 : 2;
      int bordy = (iflags & UIJustText) ? 0 : 2;

      SetBkMode(hdc, TRANSPARENT);

      int xoffset=0;

      if (clstartscreen && btext)
      {
         SetTextColor(hdc, 0xFFFFFFUL);
      }
      else if (btext)
      {
         if (blowcol)
            SetTextColor(hdc, 0x666666UL);
         else
            SetTextColor(hdc, 0x999999UL);
         orect.left = aelem[i].x+bordx+1;
         orect.top = aelem[i].y+bordy+1;
         orect.right = aelem[i].x+aelem[i].w-bordx*2;
         orect.bottom = aelem[i].y+aelem[i].h-bordy*2;
         FillRect(hdc, &orect, hbacktext);
      }
      else
      {
         // normal button background
         SetTextColor(hdc, 0);
         orect.left = aelem[i].x+bordx+1;
         orect.top = aelem[i].y+bordy+1;
         orect.right = aelem[i].x+aelem[i].w-bordx*2;
         orect.bottom = aelem[i].y+aelem[i].h-bordy*2;
         FillRect(hdc, &orect, hback);
         // if there is a single char hotkey
         cchar *pid = aelem[i].pszid;
         if (aelem[i].hw >= 50 && strlen(pid) == 1 && pid[0] != ' ')
         {
            orect.left += 6;
            orect.top  += 2;
            orect.right = orect.left+20;
            orect.bottom -= 2;
            FillRect(hdc, &orect, hhotback);
            DrawText(hdc, pid, strlen(pid), &orect, DT_CENTER|DT_SINGLELINE|DT_VCENTER);
            xoffset = 26;
         }
      }

      uint ndrawflags = DT_SINGLELINE|DT_VCENTER;

      if (clstartscreen || (clscanstart && !clinreload))
           ndrawflags = DT_CENTER|DT_SINGLELINE|DT_VCENTER;

      if (iflags & UIMultiLineText)
      {
         ndrawflags = DT_LEFT|DT_WORDBREAK;
         bordx = 12;
         bordy = 10;
      }

      if (iflags & UILargeFont)
         { if (clfont2) hOldFont = (HFONT)SelectObject(hdc, clfont2); }
      else
         { if (clfont1) hOldFont = (HFONT)SelectObject(hdc, clfont1); }

      orect.left = aelem[i].x+bordx+xoffset;
      orect.top = aelem[i].y+bordy;
      orect.right = aelem[i].x+aelem[i].w-bordx*2;
      orect.bottom = aelem[i].y+aelem[i].h-bordy*2;
      if (iflags & UIStartScreen) {
         orect.left = 40;
         orect.right = clwinw-40;
      }
      if (iflags & UIStartButton)
         orect.bottom -= genh * 30;
      DrawText(hdc, ptext, strlen(ptext), &orect, ndrawflags);

      if (iflags & UIStartButton)
      {
         orect.bottom += genh * 30;
         SelectObject(hdc, clfont1);
         ptext = (char*)"all images from a folder.";
         orect.top += genh * 50;
         DrawText(hdc, ptext, strlen(ptext), &orect, ndrawflags);
      }

      if (hOldFont)
         { SelectObject(hdc, hOldFont); hOldFont = 0; }

      if ((iflags & UIJustText) == 0)
      {
         orect.left = aelem[i].x+1;
         orect.top = aelem[i].y+1;
         orect.right = aelem[i].x+aelem[i].w-2;
         orect.bottom = aelem[i].y+aelem[i].h-2;
         FrameRect(hdc, &orect, hinnerrim);
 
         orect.left = aelem[i].x+0;
         orect.top = aelem[i].y+0;
         orect.right = aelem[i].x+aelem[i].w-0;
         orect.bottom = aelem[i].y+aelem[i].h-0;
         FrameRect(hdc, &orect, houterrim);
      }
   }

   DeleteObject(hbackunsel);
   DeleteObject(hbacksel);
   DeleteObject(hbacktext);
   DeleteObject(hhotback);
   DeleteObject(houterrim);
   DeleteObject(hinnerpassive);
   DeleteObject(hinneractive);
}

char *help_abRawBlock =

"\n"
"#        Easy Image Viewer " VER_NUM1 "\n"
"\n                      www.StahlWorks.com\n"
"\n"
"loads all images from a folder, including all sub folders.\n"
"sorts them by modification time, and shows the most recent one.\n"
"walk back through images by 'a' or cursor left.\n"
"walk forward through images by 'd' or cursor right.\n"
"\n"
"   F1       show this help.\n"
"   F2       show current image number, session time etc.\n"
"   F3       show details of the currently viewed image.\n"
"   F4   w # minimize window, to get access to the desktop.\n"
"   F5   r   reload whole folder, and show newest image.\n"
"        t   toggle auto reload.\n"
"   Esc  +   exit the application.\n"
"   o        Open another folder.\n"
"\n"
"   1        or space jumps to a random image.\n"
"\n"
"   left a   previous (older) image. supports key repeat,\n"
"            i.e. if you keep cursor left pressed,\n"
"            you walk back through images automatically.\n"
"            shift+left: skip 10 images.\n"
"            ctrl+left: jump back to last position\n"
"                       before a '1' random jump.\n"
"\n"
"   right d  next (newer) image. supports key repeat.\n"
"            shift+right: skip 10 images.\n"
"            ctrl+right: jump forward in the history\n"
"                        of '1' random jumps.\n"
"\n"
"   up       rotate 90 degrees left  (in memory, not on disk)\n"
"   down     rotate 90 degrees right (in memory, not on disk)\n"
"   5        move image file to {imagefolder}\\zz-output\\5\n"
"   6        move image file to {imagefolder}\\zz-output\\6\n"
"   7        move image file to {imagefolder}\\zz-output\\7\n"
"   8        move image file to {imagefolder}\\zz-output\\8\n"
"   9        move image file to {imagefolder}\\zz-output\\9\n"
"   4        remember image for move (if ui is shown)\n"
"   0        copy current image filename to clipboard\n"
"   =        copy relative filename (if run from command line)\n"
"\n"
"   del      move image file to {imagefolder}\\zz-output\\trash\n"
"            shift+del fully deletes current file\n"
"   i        quiet mode, do not ask before move/delete image\n"
"   Ctrl+S   save current image number and settings to\n"
"            {imagefolder}\\zz-meta-dat\\zz-settings.txt\n"
"   l        recall saved image number\n"
"   Shift+S  save current image number to second slot\n"
"            and save settings to file\n"
"   Shift+L  recall current image number from second slot\n"
"   Shift+V  toggle verbose mode\n"
"   Ctrl+t   reset session time (shown by F2)\n"
"\n"
#ifndef INTERNAL
"   v        add image filename to {imagefolder}\\zz-meta-dat\\list1.txt\n"
"   b        add image filename to {imagefolder}\\zz-meta-dat\\list2.txt\n"
"   n        add image filename to {imagefolder}\\zz-meta-dat\\list3.txt\n"
"\n"
#endif
"   Ctrl+G   goto image number\n"
"   Ctrl+C   if showing PNG extended text, copy that to clipboard\n"
"   Ctrl+X   toggle mouse mode:\n"
"            left   mouse button = previous image\n"
"            right  mouse button = next image\n"
"            middle mouse button = random image\n"
"\n"
"   command line:\n"
"      \"iview [options] foldername\" shows all from that folder.\n"
"      \"iview list.txt\" shows all images listed in text file.\n"
"      possible options:\n"
"         -nosub   do not include sub folders\n"
"         -sub     include sub folders\n"
"         -quiet   activates quiet mode\n"
"         -i       read list of image filenames from stdin\n"
"         -showui  show F2 ui from start\n"
"      example:\n"
"         sfk list -late=100 -notime pic .png | iview -i\n"
"            show the most recent 100 images from folder pic.\n"
"            for more on the sfk tool, see below.\n"
"         sfk list -late=100 pic .png +iview\n"
"            the same, but shorter (with sfk 2.0)\n"
"\n"
"   environment:\n"
"      SET IVIEW_CONFIG=nosub,quiet,showui,mousemode\n"
"         to have these options active by default\n"
"\n"
"   beware of focus loss:\n"
"      if you have a multiple monitor setup, but the external\n"
"      monitor is off, it can happen that you click outside\n"
"      the fullscreen area unintentionally, and the keyboard\n"
"      no longer reacts.\n" // a short warning is shown then.\n"
"      if that happens, move your mouse back into the image\n"
"      and click to regain keyboard control.\n"
"\n"
"   =======================================================\n"
"   ===     If this is useful, you are encouraged to    ===\n"
"   ===             stahlworks.com/donate               ===\n"
"   ===                                                 ===\n"
"   ===     Also try these world wide unique tools:     ===\n"
"   =======================================================\n"
"\n"
"   1) Get Depeche View, the world's fastest text search tool.\n"
"      Fly over thousands of text files in a single window!\n"
"      Freeware version available.\n"
"\n"
"   2) Get the Swiss File Knife Multi Function command line tool.\n"
"      Find and replace text in files, find duplicate files,\n"
"      instant http/ftp server, run a command on all files,\n"
"      and 100 further functions. Free Open Source.\n"
"\n"
"                    www.stahlworks.com\n"
"\n"
   ;

void View::moveInManual(int idelta)
{
   int itotallines=0;
   int ihelplen=strlen(help_abRawBlock);
   for (int i=0;i<ihelplen;i++)
      if (help_abRawBlock[i]=='\n')
         itotallines++;

   int basey1 = 40;
   int basey2 = clwinh-40;
   int ydelta = basey2-basey1;

   int iscreenlines = idelta;

   if (idelta>0 && clmanline+iscreenlines<itotallines)
      clmanline += iscreenlines;

   if (idelta<0) {
      clmanline += iscreenlines;
      if (clmanline<0)
         clmanline = 0;
   }
}

void View::clearscreen(HDC hdc)
{
   HBRUSH hblack = CreateSolidBrush(0);

   RECT orect;
   orect.left   = 0;
   orect.top    = 0;
   orect.right  = clwinw;
   orect.bottom = clwinh;
   FillRect(hdc, &orect, hblack);

   DeleteObject(hblack);
}

void View::drawmanual(HDC hdc)
{__
   RECT orect;

   HBRUSH hbacktext = CreateSolidBrush(0);

   HFONT  hOldFont = (HFONT)SelectObject(hdc, clfont3);

   SetBkMode(hdc, TRANSPARENT);
   SetTextColor(hdc, 0xEEEEEEUL);

   int textw  = 900;
   int basex1 = clwinw/2-textw/2;
   int basey1 = 40;
   int basex2 = clwinw-40;
   int basey2 = clwinh-40;

   clearscreen(hdc);

   char *pcur = (char*)help_abRawBlock;
   char *pmax = pcur+strlen(pcur);

   int curx1 = basex1;
   int cury1 = basey1;
   int iskiplines = clmanline;

   while (pcur<pmax)
   {
      char *pnex=pcur;
      while (pnex<pmax && *pnex!='\r' && *pnex!='\n') pnex++;
      
      int ilen = pnex-pcur;

      if (pnex<pmax && *pnex=='\r') pnex++;
      if (pnex<pmax && *pnex=='\n') pnex++;
      if (iskiplines > 0)
         { iskiplines--; pcur=pnex; continue; }

      if (*pcur=='#') {
         pcur++; ilen--;
         SetTextColor(hdc, 0xFF9999UL);
         SelectObject(hdc, clfont4);
      } else {
         SetTextColor(hdc, 0xEEEEEEUL);
         SelectObject(hdc, clfont3);
      }

      char *puse=pcur; int nuse=ilen;

      if (ilen >= 3 && strncmp(puse, "===", 3) == 0)
         { }
      else
      {
         orect.left   = curx1;
         orect.top    = cury1;
         orect.right  = basex2;
         orect.bottom = cury1+60;
         DrawText(hdc, puse, nuse, &orect, DT_SINGLELINE|DT_NOPREFIX|DT_LEFT|DT_TOP);
      }

      cury1 += 28;
      if (cury1+10 >= basey2)
         break;

      pcur=pnex;
   }

   if (hOldFont)
      { SelectObject(hdc, hOldFont); hOldFont = 0; }

   DeleteObject(hbacktext);
}

// rc 0=miss 1=done
int View::processUIElem (int x, int y, int ieventflags)
{__
   if (!showui())
      return 0;

   int i = getuielem(x,y);
   if (i<0) {
      // printf("ui.proc miss %d %d\n",x,y);
      return 0;
   }

   UIElem *pelem = &aelem[i];

   // printf("ui.proc exec %d %d id %d\n",x,y,pelem->id);

   bool bleft  = (ieventflags & 1) ? 1 : 0;
   bool brite  = (ieventflags & 2) ? 1 : 0;
   bool bshift = (ieventflags & 4) ? 1 : 0;
   bool bctrl  = (ieventflags & 8) ? 1 : 0;

   int iupdate = 0;

   pelem->tclickstart = getCurrentTime();

   processKey(pelem->id,2); // from ui

   return 1;
}

char *View::getOutputPath(cchar *pszOutFolder)
{__
   int isrc = cur.curfile;

   if (clremembered>=0)
       isrc = clremembered;

   char *pdst = afile[isrc].name;
   if (!pdst)
      return 0;
 
   joinPath(exportpath, sizeof(exportpath)-10, folder, (char*)pszOutFolder);

   // always init target relname with curname
   char *prel = strrchr(pdst, '\\');
   if (prel) prel++;
   else prel = pdst;
   strcopy(exportname, prel);

   joinPath(exportfullname, sizeof(exportfullname)-10, exportpath, exportname);

   if (existsFile(exportfullname))
   {
      long nrc = MessageBox(0, exportfullname, "Replace this file on export?", MB_YESNO);
 
      if (nrc != IDYES)
         return 0;
   }

   return exportfullname;
}

void View::moveImageTo(cchar *pfolder)
{__
   int isrc = cur.curfile;

   if (clremembered>=0)
       isrc = clremembered;

   char *psrc = afile[isrc].name;
   if (!psrc)
      return;

   char *pdst = getOutputPath(pfolder);
   if (!pdst)
      return;
   // this checked if file exists in output.
   // we can overwrite an existing one.

   if (createOutDirTree(pdst)) {
      err("cannot create output folder");
      return;
   }

   if (clquietmode == 0)
   {
      int nrc = 0;

      if (clremembered>=0)
         nrc = MessageBox(0, psrc, "move the remembered image file?", MB_YESNO);
      else
         nrc = MessageBox(0, exportpath, "move image file to this folder?", MB_YESNO);
   
      if (nrc != IDYES)
         return;
   }

   if (existsFile(pdst))
      remove(pdst);

   int isubrc = rename(psrc,pdst);

   if (isubrc)
      err("move failed, rc=%d: %s", isubrc, pdst);
   else 
   {
      #if 1

      if (afile[isrc].name)
         delete [] afile[isrc].name;

      afile[isrc].name = strdup(pdst);

      #else

      int ipos = isrc;

      int iremain = nfile - ipos;

      if (iremain>0)
         memmove(&afile[ipos], &afile[ipos+1], iremain*sizeof(ViewPic));
      
      nfile--;

      cur.clRecentRandIndex = 0;

      if (ipos<nfile) {
         isrc = ipos;
      } else if (ipos>=nfile) {
         isrc = 0;
      }
      loadpic(0, "postmove");

      #endif
  
      updateMixPic();

      if (clremembered>=0)
         alert("moved remembered image to:", pdst);
      else
         alert("moved image to:", pdst);
   }

   clremembered = -1;
}

LRESULT CALLBACK MasterWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void View::updateMixPic( )
{
   if (clcurrealm==REALM_DST)
      cur.mixpic.copyFrom(&dst_mempic,0,0,cur.mempicw,cur.mempich,0,0,cur.mempicw,cur.mempich);
   else
      cur.mixpic.copyFrom(&src_mempic,0,0,cur.mempicw,cur.mempich,0,0,cur.mempicw,cur.mempich);
}

int View::create(HINSTANCE hInstance)
{__
   clinstance = hInstance;

   WNDCLASSEX wcex;
   wcex.cbSize = sizeof(WNDCLASSEX);

   // master window class
   wcex.style        = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc  = (WNDPROC)MasterWndProc;
   wcex.cbClsExtra   = 0;
   wcex.cbWndExtra   = 16;
   wcex.hInstance    = hInstance;
   wcex.hIcon        = LoadIcon(hInstance, (LPCTSTR)500);
   wcex.hCursor      = LoadCursor(NULL, IDC_ARROW);
   // wcex.hbrBackground= (HBRUSH)(COLOR_WINDOW+1);
   wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
   wcex.lpszMenuName = 0;
   wcex.lpszClassName= CLASS_NAME;
   wcex.hIconSm      = 0;
   RegisterClassEx(&wcex);

   HWND hDeskWin = ::GetDesktopWindow(); // backimg
   HDC  hdcDesk  = ::GetWindowDC(hDeskWin);
   if (hdcDesk == NULL) return 0;
   clwinw = GetDeviceCaps(hdcDesk, HORZRES);
   clwinh = GetDeviceCaps(hdcDesk, VERTRES);
   clmaxwinpix = clwinw * clwinh;
   genw = clwinw/1000.0;
   genh = clwinh/1000.0;
   ::ReleaseDC(hDeskWin, hdcDesk);

   // printf("window size %d x %d\n",clwinw,clwinh);

   // note this calls MasterWndProc and gview without clwin
   clwin = CreateWindow(
         CLASS_NAME,
         PROG_NAME,
         WS_POPUP, // WS_OVERLAPPEDWINDOW,
         0,0, clwinw,clwinh,
         NULL, NULL, hInstance, NULL
         );

   bool blores = (clwinw < 1600) ? 1 : 0;

   LOGFONT ofont;
   memset(&ofont, 0, sizeof(ofont));
   ofont.lfHeight = blores ? 12 : 20;
   ofont.lfWidth = 0;
   ofont.lfEscapement = 0;
   ofont.lfOrientation = 0;
   ofont.lfWeight = 1000;
   ofont.lfItalic = 0;
   ofont.lfUnderline = 0;
   ofont.lfStrikeOut = 0;
   ofont.lfCharSet = 0;
   ofont.lfOutPrecision = 0;
   ofont.lfClipPrecision = 0;
   ofont.lfQuality = 0;
   ofont.lfPitchAndFamily = 0;
   strcopy(ofont.lfFaceName, "Consolas");
   clfont1 = CreateFontIndirect(&ofont);

   ofont.lfHeight = blores ? 20 : 40;
   ofont.lfWidth = 0;
   ofont.lfEscapement = 0;
   ofont.lfOrientation = 0;
   ofont.lfWeight = 1000;
   ofont.lfItalic = 0;
   ofont.lfUnderline = 0;
   ofont.lfStrikeOut = 0;
   ofont.lfCharSet = 0;
   ofont.lfOutPrecision = 0;
   ofont.lfClipPrecision = 0;
   ofont.lfQuality = 0;
   ofont.lfPitchAndFamily = 0;
   strcopy(ofont.lfFaceName, "Consolas");
   clfont2 = CreateFontIndirect(&ofont);

   ofont.lfHeight = iInfoFontHeight;
   ofont.lfWidth = 0;
   ofont.lfEscapement = 0;
   ofont.lfOrientation = 0;
   ofont.lfWeight = 1000;
   ofont.lfItalic = 0;
   ofont.lfUnderline = 0;
   ofont.lfStrikeOut = 0;
   ofont.lfCharSet = 0;
   ofont.lfOutPrecision = 0;
   ofont.lfClipPrecision = 0;
   ofont.lfQuality = 0;
   ofont.lfPitchAndFamily = 0;
   strcopy(ofont.lfFaceName, "Consolas");
   clfont3 = CreateFontIndirect(&ofont);

   ofont.lfHeight = 40;
   ofont.lfWidth = 0;
   ofont.lfEscapement = 0;
   ofont.lfOrientation = 0;
   ofont.lfWeight = 1000;
   ofont.lfItalic = 0;
   ofont.lfUnderline = 0;
   ofont.lfStrikeOut = 0;
   ofont.lfCharSet = 0;
   ofont.lfOutPrecision = 0;
   ofont.lfClipPrecision = 0;
   ofont.lfQuality = 0;
   ofont.lfPitchAndFamily = 0;
   strcopy(ofont.lfFaceName, "Consolas");
   clfont4 = CreateFontIndirect(&ofont);


   // logo font
   ofont.lfHeight = 15;
   ofont.lfWidth = 0;
   ofont.lfEscapement = 0;
   ofont.lfOrientation = 0;
   ofont.lfWeight = 1000;
   ofont.lfItalic = 0;
   ofont.lfUnderline = 0;
   ofont.lfStrikeOut = 0;
   ofont.lfCharSet = 0;
   ofont.lfOutPrecision = 0;
   ofont.lfClipPrecision = 0;
   ofont.lfQuality = 0;
   ofont.lfPitchAndFamily = 0;
   strcopy(ofont.lfFaceName, "Consolas");
   clfontl = CreateFontIndirect(&ofont);

   HDC htmp = GetWindowDC(clwin);
   if (!htmp) return 9+perr("cannot get dc");

   if (!(clhdc = CreateCompatibleDC(htmp)))
      return 9+perr("cannot create dc");

   BITMAPINFO bmi;
   VOID *pvBits = 0;

   ZeroMemory(&bmi, sizeof(BITMAPINFO));

   bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bmi.bmiHeader.biWidth  = clwinw;
   bmi.bmiHeader.biHeight = clwinh;
   bmi.bmiHeader.biPlanes = 1;
   bmi.bmiHeader.biBitCount = 32;
   bmi.bmiHeader.biCompression = BI_RGB;
   bmi.bmiHeader.biSizeImage = clmaxwinpix * 4;

   if (!(clwinbm = CreateDIBSection(clhdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0)))
      return 9+perr("cannot alloc bitmap");

   clwinpix = (pixel_t *)pvBits;
   if (!clwinpix) return 9+perr("cannot get pix");

   clrecentbm = SelectObject(clhdc, clwinbm);
   if (!clrecentbm) perr("selectobject failed");

   return 0;
}

void View::toggleMinimize()
{
   WINDOWPLACEMENT oinfo;
   mclear(oinfo);
   GetWindowPlacement(clwin, &oinfo);
   if (oinfo.showCmd  == SW_MINIMIZE)
      ShowWindow(clwin, SW_RESTORE);
   else
      ShowWindow(clwin, SW_MINIMIZE);
}

void View::closeView( )
{__
   if (clhdc && clrecentbm)
   {
      SelectObject(clhdc, clrecentbm);
      clrecentbm = NULL;
   }
 
   if (clwinbm)
   {
      DeleteObject(clwinbm);
      clwinbm = NULL;
   }

   if (clhdc)
   {
      DeleteDC(clhdc);
      clhdc = 0;
   }
}

int View::loadpic(char *pszFile, cchar *pszReason, bool bNoMetaLoad, int ipicrot)
{__
   char szFastFile[800];
   mclear(szFastFile);

   clAlertStart = 0;
   clshowingfastfile = 0;

   // wm121 fix: wrong mix overwrite warnings
   cur.metaver = 0;
   cur.fileedition = 0; // loadpic [2021131]
   cur.hasalpha = 0;
   cur.alphamode = 0;

   szTextTag[0] = '\0';

   if (!pszFile)
   {
      pszFile = afile[cur.curfile].name;
      // printf("load #%d / %d : %s\n",cur.curfile,nfile,pszFile?pszFile:"<null>"); fflush(stdout);
   }
   if (!pszFile || !*pszFile)
   {
      err("loadpic: no selected file (%d/%d)",cur.curfile,nfile);
      return 9;
   }
   if (!existsFile(pszFile))
   {
      char  szOwnDir[SFK_MAX_PATH+50];
      err("cannot load, file not found: %s",pszFile);
      _getcwd(szOwnDir,sizeof(szOwnDir)-10);
      err("current dir is: %s", szOwnDir);
      return 9;
   }

   if (clshowfastfile)
   {
      // FROM: mydir\foo.png
      // TO  : mydir\fast\foo.jpg
      strcopy(szFastFile, pszFile);
      szFastFile[700] = '\0';
      char *psla1 = strrchr(pszFile, '\\');
      char *psla2 = strrchr(szFastFile, '\\');
      if (psla1 && psla2) {
         int ilen1=strlen(psla2+1);
         snprintf(psla2+1, 700-ilen1, "fast\\%s", psla1+1);
      } else {
         snprintf(szFastFile, 700, "fast\\%s", pszFile);
      }
      char *pdot = strrchr(szFastFile, '.');
      if (pdot)
         strcpy(pdot, ".jpg");
      if (existsFile(szFastFile)) {
         // printf("show fastfile: %s\n", szFastFile);
         clshowingfastfile=1;
      } else {
         // printf("no fastfile: %s\n", szFastFile);
         szFastFile[0] = '\0';
      }
   }

   // msg("loadpic #%d / %d : %s\n",cur.curfile,nfile,pszFile?pszFile:"<null>");
   // printf("loadpic #%d / %d : %s\n",cur.curfile,nfile,pszFile?pszFile:"<null>"); fflush(stdout); // °°

   num tstart = getCurrentTime();

   if (clcurrealm==REALM_DST)
      dst_mempic.freepix();
   else
      src_mempic.freepix();

   cur.loadrc = 0;
   cur.clcoordfactorx = 0.0;
   cur.clcoordfactory = 0.0;
   cur.clcoordxoff = 0;

   // cur.loadrc = dst_mempic.load(pszFile);
   char *pszToLoad = szFastFile[0] ? szFastFile : pszFile;
   uchar *pPack = 0, *pPack2 = 0;
   num    nPack = 0, nPack2 = 0;
   if (!(pPack = loadBinaryFlex(pszToLoad, nPack)))
      return 9;

   // no return without delete begin

   cur.loadrc = dst_mempic.load(pPack, nPack);

   if (cur.loadrc)
   {
      delete [] pPack;
      return cur.loadrc;
   }

   char szBase[SFK_MAX_PATH+50];
   char szPat1[SFK_MAX_PATH+50];
   char szPat2[SFK_MAX_PATH+50];
   char szFile2[SFK_MAX_PATH+50];
   char szFile3[SFK_MAX_PATH+50];

   /*
      look for loaded image variants:
      pic\a.png pic\a-v.png pic\a-edit.png
   */
   cur.szVariants[0] = '\0';
   {
      char *p = strrchr(pszFile, '\\');
      if (p)
         p++;
      else
         p = pszFile;
      strcopy(szBase, p);
      p = strrchr(szBase, '.');
      if (p) *p = '\0';

      // search for [lstart]base or \base
      strcopy(szPat1, szBase);
      snprintf(szPat2, SFK_MAX_PATH, "\\%s", szBase);
      int ilen1=strlen(szPat1);
      int ilen2=strlen(szPat2);

      const char *ppre = "";
      for (int i=0; i<nfile; i++)
      {
         if (strlen(cur.szVariants)+20 >= sizeof(cur.szVariants))
            break;
         char *pf = afile[i].name;
         // [lstart]base
         if (strncmp(pf, szPat1, ilen1) == 0 && pf[ilen1] == '-') {
            int k=0;
            for (; pf[ilen1+k]!=0 && pf[ilen1+k]!='.'; k++);
            mystrcatf(cur.szVariants, sizeof(cur.szVariants)-10, "%s-%.*s", ppre, k, pf+ilen1);
            ppre = " ";
            continue;
         }
         // search \base in mydir\base-v.png
         if ((pf = strstr(pf, szPat2)) == 0)
            continue;
         if (pf[ilen2] != '-')
            continue;
         {
            int k=0;
            for (; pf[ilen2+k]!=0 && pf[ilen2+k]!='.'; k++);
            if (k) k--;
            mystrcatf(cur.szVariants, sizeof(cur.szVariants)-10, "%s-%.*s", ppre, k, pf+ilen2+1);
            ppre = " ";
            continue;
         }
      }
   }

   for (int itry=0; itry<2; itry++)
   {
      // scan raw data for [4byteslen]tEXtparameters\0photo style, ...
      // so far we ignore  [4byteslen]tEXtsoftware\0paint.net
      uchar *ptag = itry ? pPack2 : pPack;
      uchar *ptagmax = itry ? pPack2+nPack2 : pPack+nPack;

      for (; ptag+50 < ptagmax; ptag++)
      {
         if (ptag[0] == 0 && ptag[1] == 0
             // && ptag[4]=='t' && ptag[5]=='E' // often
             // && ptag[4]=='i' && ptag[5]=='T' // sometimes
             && ptag[6]=='X' && ptag[7]=='t'
            )
         {
            uint tlen = (((uint)ptag[2])<<8) | ((uint)ptag[3]);
            if (tlen < 12) break;
            ptag += 8; // on "parameters\0"
            // accept only 'parameters', not 'software' etc.
            if (!strncmp((char*)ptag, "parameters", 10)) {
               ptag += 10;
               while (ptag < ptagmax && *ptag == 0) ptag++;
               int icopylen = ptagmax-ptag;
               if (icopylen+10 > sizeof(szTextTag))
                  icopylen = sizeof(szTextTag)-10;
               if (icopylen > 0) {
                  memcpy(szTextTag, ptag, icopylen);
                  szTextTag[icopylen] = '\0';
               }
            }
            break;
         }
      }

      if (szTextTag[0])
         break;

      if (itry == 0) 
      {  
         // best1\12345.jpg
         // -> best1\12345.txt
         strcopy(szFile2, pszFile);
         char *pdot=strrchr(szFile2, '.');
         if (pdot &&
             (!strcmp(pdot, ".png")
              || !strcmp(pdot, ".jpg")
              || !strcmp(pdot, ".jpeg")))
         {
            strcpy(pdot, ".txt");
            if (existsFile(szFile2)) {
               if (pPack2 = loadBinaryFlex(szFile2, nPack2)) {
                  // starts directly with: tEXtparameters:...
                  char *psrc=(char*)pPack2;
                  if (!strncmp(psrc, "tEXtparameters:", 15))
                     psrc += 15;
                  int icopylen=strlen(psrc);
                  if (icopylen+10 > sizeof(szTextTag))
                     icopylen = sizeof(szTextTag)-10;
                  if (icopylen > 0) {
                     memcpy(szTextTag, psrc, icopylen);
                     szTextTag[icopylen] = '\0';
                  }
                  break;
               }
            }
         }

         // best1\12345.jpg
         // -> best1\zzrawpng\12345.png
         strcopy(szFile2, pszFile);
         char *psla = strrchr(szFile2, '\\');
         if (!psla) break;
         *psla++ = '\0';
         char *pbase = psla;
         psla = strchr(psla, '.');
         if (!psla) break;
         *psla = '\0';
         snprintf(szFile3,SFK_MAX_PATH, "%s\\zzrawpng\\%s.png", szFile2, pbase);
         szFile3[SFK_MAX_PATH] = '\0';
         if (!existsFile(szFile3)) {
            snprintf(szFile3,SFK_MAX_PATH, "%s\\zzrawpng\\%s.txt", szFile2, pbase);
            szFile3[SFK_MAX_PATH] = '\0';
         }
         if (!(pPack2 = loadBinaryFlex(szFile3, nPack2)))
            break;
      }
   }

   checkEscape(); // loadpic

   // rotate due to live parm from cursor up/down
   if (ipicrot)
      dst_mempic.rotate(ipicrot);

   uint  nnumpix = 0;
   uint *ppixptr = 0;

   if (clcurrealm==REALM_DST) {
      cur.mempicw = dst_mempic.width();
      cur.mempich = dst_mempic.height();
      nnumpix = dst_mempic.getnumpix();
      ppixptr = dst_mempic.getpixptr();
   } else {
      cur.mempicw = src_mempic.width();
      cur.mempich = src_mempic.height();
      nnumpix = src_mempic.getnumpix();
      ppixptr = src_mempic.getpixptr();
   }

   // [202110243] check if image contains transparency
   cur.hasalpha=0;
   if (ppixptr) {
      for (int i=0;i<nnumpix;i++)
         if ((ppixptr[i]>>24)<255)
            { cur.hasalpha=1; break; }
   }

   cur.hip = cur.mempicw/1000.0; // temporary
   cur.miw = cur.mempicw/1000.0; // permanent

   setDefaultScale();

   cur.memfocusx = cur.mempicw/2;
   cur.memfocusy = cur.mempich/2;

   cur.mixpic.allocpix(cur.mempicw,cur.mempich);

   cur.tloadpic = getCurrentTime()-tstart;

   delete [] pPack;
   
   if (pPack2) delete [] pPack2;

   return cur.loadrc;
}

void View::askDelete( )
{__
   SHORT nksShift = GetKeyState(VK_SHIFT  );
   SHORT nksCtrl  = GetKeyState(VK_CONTROL);
   int bShift = (nksShift & (1 << 15)) ? 1 : 0;
   int bCtrl  = (nksCtrl  & (1 << 15)) ? 1 : 0;

   char *psrc = afile[cur.curfile].name;

   if (clquietmode == 0)
   {
      long lRC = MessageBox(0, psrc, "Delete this file?", MB_YESNO);
      if (lRC != IDYES)
         return;
   }

   remove(psrc);

   int ipos=cur.curfile;
   changeToFolder(0,1);
   if (ipos<nfile) {
      cur.curfile=ipos;
      loadpic(0, "postdel");
   }

   alert("deleted:", psrc);

   updateMixPic();
   update();
}

int View::initiobuf( )
{
   clioidx = 0;
   clioerr = 0;
   return 0;
}

int View::iobuferr( ) { return clioerr; }

int View::addiobuf(const char *pszFormat, ...)
{
   va_list argList;
   va_start(argList, pszFormat);
   char szBuf[1024];
   ::vsprintf(szBuf, pszFormat, argList);
   int ilen = strlen(szBuf);
   int iremain = MAX_IO_BUF-clioidx;
   if (ilen+10 > iremain)
      { clioerr=1; return -1; }
   memcpy(cliobuf+clioidx, szBuf, ilen);
   clioidx += ilen;
   cliobuf[clioidx] = '\0';
   return 0;
}

void View::setDefaultScale( )
{
   double scalew = clwinw*1.0/cur.mempicw;
   double scaleh = clwinh*1.0/cur.mempich;
   cur.viewscale = min(scalew,scaleh);
   cur.endscale = 0.0;
   // printf("set scale %1.3f\n",cur.viewscale); fflush(stdout);
   cur.startscale=cur.viewscale;
   cur.startfocusx=cur.memfocusx;
   cur.startfocusy=cur.memfocusy;
}

void View::draw( )
{__
   char szinfo[200];

   if (showui())
      makeui();

   PAINTSTRUCT ps;
   HDC hdc = BeginPaint(clwin, &ps);

   // printf("draw.begin\n"); fflush(stdout);

   RECT orect;
   GetClientRect(clwin, &orect);
   clwinw = orect.right-orect.left;
   clwinh = orect.bottom-orect.top;

   if (clscanstart 
       && !clinreload
       && getCurrentTime()-clscanstart < 200)
   {
      EndPaint(clwin, &ps);
      return;
   }

   if (clshowmanual)
   {
      drawmanual(hdc);
      EndPaint(clwin, &ps);
      return;
   }

   if (clclearscreen)
   {
      clclearscreen=0;
      clearscreen(hdc);
   }

   int iscancx,iscancy,iscanx2,iscany2;
   int iscanvieww,iscanviewh,iborderw,iborderh;

   // printf("mempicwh %d %d winwh %d %d scale %1.3f\n",
   //    cur.mempicw,cur.mempich, clwinw,clwinh, cur.viewscale);

   for (int itry=0; itry<2; itry++)
   {
      // calc area to take from memory
      iscancx = cur.memfocusx;
      iscancy = cur.memfocusy;
      cur.scanw  = min(cur.mempicw*1.0,round(clwinw *1.0/ cur.viewscale));
      cur.scanh  = min(cur.mempich*1.0,round(clwinh *1.0/ cur.viewscale));
      // printf("scanwh %d %d\n",cur.scanw,cur.scanh);
      cur.scanx1 = max(0,iscancx - cur.scanw/2);
      cur.scany1 = max(0,iscancy - cur.scanh/2);
      iscanx2 = cur.scanx1 + cur.scanw;
      iscany2 = cur.scany1 + cur.scanh;
   
      // calc area to display on screen
      iscanvieww = round(cur.scanw * cur.viewscale);
      iscanviewh = round(cur.scanh * cur.viewscale);
      iborderw = clwinw - iscanvieww;
      iborderh = clwinh - iscanviewh;

      // printf("scanviewwh %d %d border %d %d\n",
      //       iscanvieww,iscanviewh, iborderw,iborderh);
   
      if (iborderw>50 && iborderh>50) {
         // printf("apply default scale %1.3f %d %d\n",cur.viewscale,iborderw,iborderh);fflush(stdout);
         setDefaultScale();
         cur.memfocusx = cur.mempicw/2;
         cur.memfocusy = cur.mempich/2;
      } else {
         break;
      }
   }
   cur.borderw = iborderw/2;
   cur.borderh = iborderh/2;

   int x1dst = 0;
   int y1dst = 0;
   int wdst  = min(clwinw,iscanvieww);
   int hdst  = min(clwinh,iscanviewh);

   int x1src = 0;
   int y1src = 0;
   int wsrc  = cur.mempicw;
   int hsrc  = cur.mempich;

   // printf("scan for view %d %d %dx%d from mem %dx%d\n",
   //    cur.scanx1,cur.scany1,
   //    clwinw,clwinh, cur.mempicw,cur.mempich); fflush(stdout);

   int maxmemh = 0;

   // get partial area from mem for display.
   // apply scale, convert BGR and upside down.
   int imaxwinpix = clwinw*clwinh;
   if (clskipdrawmixpic)
   {
      // clskipdrawmixpic=0;
   }
   else
   for (int viewy=0; viewy<hdst; viewy++)
   {
      int imemy = cur.scany1 + round(viewy * 1.0 / cur.viewscale);
      if (imemy >= cur.mempich) break;
      maxmemh = imemy;
      hsrc = viewy+1;

      for (int viewx=0; viewx<wdst; viewx++)
      {
         int imemx = cur.scanx1 + round(viewx * 1.0 / cur.viewscale);
         if (imemx >= cur.mempicw) break;
         wsrc = viewx+1;
   
         pixel_t cmem = cur.mixpic.getpix(imemx,imemy);
   
         uint32_t r = (cmem >>16) & 0xFFU;
         uint32_t g = (cmem >> 8) & 0xFFU;
         uint32_t b = (cmem >> 0) & 0xFFU;

         cmem =      0xff000000UL
                  |  (r <<  0)
                  |  (g <<  8)
                  |  (b << 16);

         int ioffset = (clwinh-1-viewy)*clwinw+viewx;
         if (ioffset<0 || ioffset >=imaxwinpix) continue;
         clwinpix[ioffset] = cmem;
      }
   }

   wdst = min(wdst,wsrc);
   hdst = min(hdst,hsrc);

   // can we avoid flicker by full image copy?
   bool bFullImageCopy = 0;
   if (x1dst+cur.borderw==0 && y1dst+cur.borderh==0
       && wdst>=clwinw && hdst >= clwinh)
        bFullImageCopy = 1;

   RECT rt2;
   rt2.left   = 0;
   rt2.top    = 0;
   rt2.right  = clwinw;
   rt2.bottom = clwinh;

   // flicker: main reason is drawrect to blank screen
   if (bFullImageCopy==0 && clskipclearscreen==0) 
   {
      HBRUSH hBlank = CreateSolidBrush(0);
      if (hBlank != NULL) {
         FillRect(hdc, &rt2, hBlank);
         DeleteObject(hBlank);
      }
   }
   clskipclearscreen=0;

   BLENDFUNCTION bf;
   bf.BlendOp = AC_SRC_OVER;
   bf.BlendFlags = 0;
   bf.AlphaFormat = AC_SRC_ALPHA;  // use source alpha
   bf.SourceConstantAlpha = 0xff;

   int irc = 0;
   if (clskipdrawmixpic || clmobilex) 
   {
      clskipdrawmixpic=0;
      if (clmobilex)
         irc = BitBlt(hdc, 607,0,640,hdst, clhdc,clmobilex,0, SRCCOPY);
      else
         irc = BitBlt(hdc, 0,0,wdst,hdst, clhdc,0,0, SRCCOPY);
   } else {
      irc = GdiAlphaBlend(hdc, x1dst+cur.borderw,y1dst+cur.borderh,wdst,hdst, clhdc, x1src,y1src,wsrc,hsrc, bf);
   }

   if (showui())
      drawui(hdc);

   orect.left  = 10;
   orect.top   = clwinh - 70;
   orect.right = clwinw - 10;
   orect.bottom= clwinh - 10;

   if (clAlertStart != 0 && getCurrentTime() - clAlertStart > 3000)
      clAlertStart = 0;

   if (clAlertStart != 0)
   {
      orect.top   = (clwinh*3)/4;
      orect.bottom= orect.top + 100;

      HFONT  hOldFont = 0;
      if (clfont2) hOldFont = (HFONT)SelectObject(hdc, clfont2); 

      SetTextColor(hdc, 0xAAAAFF);
      SetBkColor(hdc, 0x666666);
      SetBkMode(hdc, OPAQUE);
      uint ndrawflags = DT_CENTER;

      DrawText(hdc, szClAlertTitle, strlen(szClAlertTitle), &orect, ndrawflags);
      orect.top += 50;
      DrawText(hdc, szClAlertText, strlen(szClAlertText), &orect, ndrawflags);

      if (hOldFont) SelectObject(hdc, hOldFont);
   }
   else
   if (szClInfo[0] || clshowinfo)
   {
      HFONT  hOldFont = 0;
      if (clfont3) hOldFont = (HFONT)SelectObject(hdc, clfont3);

      int iFontHeight = iInfoFontHeight;

      orect.top = clwinh - 100 + iFontHeight;

      SetTextColor(hdc, 0xFFFFFF);
      SetBkColor(hdc, 0x666666);
      SetBkMode(hdc, OPAQUE);
      uint ndrawflags = DT_CENTER; // |DT_VCENTER;
      char *pszDstPic = afile[dst.curfile].name;
      char *pszSrcPic = afile[src.curfile].name;

      char *ptext = szClInfo;

      char szBuf[4096];
      strcopy(szBuf, szTextTag);

      int nlines = 0;
      int imaxlinelen = 140;
      char *psz=szBuf,*pspc=0;
      int icur=0,ispc=0;
      for (; *psz; psz++)
      {
         if (*psz=='\n') {
            *psz = '\0';
            break;
         }
         if (*psz==' ')
            { pspc=psz; ispc=icur; }
         if (icur>imaxlinelen) {
            if (pspc!=0 && ispc>imaxlinelen/2) {
               *pspc='\n';
               psz = pspc;
            } else {
               *psz='\n';
            }
            icur=0; ispc=0; pspc=0;
            nlines++;
            orect.top -= iFontHeight;
            if (nlines >= 10) {
               *psz='\0';
               break;
            }
         } else {
            icur++;
         }
      }

      if (!ptext[0])
      {
         ptext = vtext(
            "%d/%d %s%dx%d %s %s%s%s\n"
            "%s\n"
            , cur.curfile+1, nfile
            , clshowingfastfile ? "fast ":""
            , dst.mempicw, dst.mempich
            , pszDstPic?pszDstPic:"-"
            , cur.szVariants[0] ? "(found ":""
            , cur.szVariants
            , cur.szVariants[0] ? ")":""
            , szBuf
            );
      }

      DrawText(hdc, ptext, strlen(ptext), &orect, ndrawflags);

      if (hOldFont) SelectObject(hdc, hOldFont);
   }

   // draw message log
   HDC hBack = hdc;
   int xleft = clwinw*2;
   int ybot  = clwinh-40;
   int xrite = clwinw-40;
   int hcharLog = 24;

   // from bottom up, newest entry first
   for (long ilog=0; ilog<MAX_DVLOG_LINES; ilog++)
   {
      char *plog = &aLogLines[ilog][0];
      if (!*plog) break;

      // check age of log entry
      long nage = (long)(getCurrentTime() - aLogTimes[ilog]) / 1000;
      long nAgePerc = 0;

      if (!nGlblShowLog) {
         nAgePerc = nage * 100 / DVLOG_SHOW_SEC;
         if (nAgePerc >= 100)
             continue; // skip, but there may be more recent entries above
         if (nAgePerc < 0)
             nAgePerc = 0;
      }

      // draw a single log entry
      RECT rt2;
      rt2.left   = xleft;
      rt2.top    = ybot - hcharLog * ilog;
      rt2.right  = xrite;
      rt2.bottom = rt2.top + hcharLog * 2;

      if (rt2.top < hcharLog * 4)
         break;   // reached top border

      long nLen  = strlen(plog);
      SetBkMode(hBack, nGlblShowLog ? OPAQUE : TRANSPARENT);

      if (!strncmp(plog, "[error] ", 7)) {
         SetTextColor(hBack, paleColor(0x0000FF, nAgePerc));
         plog += 7;
         nLen -= 7;
      }
      else
      if (!strncmp(plog, "[note] ",7)) {
         SetTextColor(hBack, paleColor(0x4080FF, nAgePerc));
         plog += 7;
         nLen -= 7;
      }
      else
      if (!strncmp(plog, "[blue] ",7)) {
         SetTextColor(hBack, paleColor(0xFF0000, nAgePerc));
         plog += 7;
         nLen -= 7;
      }
      else
      if (!strncmp(plog, "[info] ",7)) {
         SetTextColor(hBack, paleColor(0x909090, nAgePerc));
         plog += 7;
         nLen -= 7;
      }
      else
         SetTextColor(hBack, paleColor(0x909090, nAgePerc));

      SetBkColor(hBack, 0);

      SIZE osize; mclear(osize);
      if (GetTextExtentPoint32(hBack, plog, strlen(plog), &osize))
      {
         ulong nFlags = DT_SINGLELINE|DT_TOP|DT_RIGHT|DT_NOPREFIX;
         long nwidth = osize.cx;
         if (!bGlblShowLogWide && (nwidth > 400))
              nwidth = 400;
         if (nwidth >= rt2.right)
            rt2.left = 10;
         else
            rt2.left = rt2.right - nwidth;
         DrawText(hBack, plog, nLen, &rt2, nFlags);
      }
      else
      {
         ulong nFlags = DT_SINGLELINE|DT_TOP|DT_RIGHT|DT_NOPREFIX;
         DrawText(hBack, plog, nLen, &rt2, nFlags);
      }
      if (nClLogTop == 0 || rt2.top < nClLogTop)
         nClLogTop = rt2.top;
      if (nClLogLeft == 0 || rt2.left < nClLogLeft)
         nClLogLeft = rt2.left;
   }

   EndPaint(clwin, &ps);
}

LRESULT CALLBACK MasterWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   return gview.process(hWnd, msg, wParam, lParam);
}

void View::update()
{__
   InvalidateRect(clwin,0,0);
   UpdateWindow(clwin);

   // check if we have to retrigger timer-based updates
   checkDisplayUpdates();
}

void View::checkDisplayUpdates()
{
   // if a display update event is pending
   // in the near past (< 200 msec), do nothing.
   // otherwise, assume the event was lost.
   if (nClADUPending) {
      num nelapse = getCurrentTime() - nClADUPending;
      if (nelapse >= 0 && nelapse < 200) return;
      nClADUPending = 0;
   }

   // check if there is any need for timer-based redraw of panel
   long nMinPerc = 100;
   for (long ilog=0; ilog<MAX_DVLOG_LINES; ilog++)
   {
      char *plog = &aLogLines[ilog][0];
      if (!*plog) break;
      long nage = (long)(getCurrentTime() - aLogTimes[ilog]) / 1000;
      long nAgePerc = nage * 100 / DVLOG_SHOW_SEC;
      if (nAgePerc < nMinPerc)
         nMinPerc = nAgePerc;
   }

   if (   (nMinPerc < 100)
       // || bClMousePosDrawn
      )
   {
      // log entries must be painted further
      nClADUPending = getCurrentTime();
      SetTimer(clwin, DVTIMER_DISPLAY, 500, 0);
   }
}

void View::handleTimer(ulong nid)
{__
   if (nid == DVTIMER_DISPLAY)
   {
      KillTimer(clwin, DVTIMER_DISPLAY);
      nClADUPending = 0;
      update();
   }
   if (nid == DVTIMER_INFO)
   {
      KillTimer(clwin, DVTIMER_INFO);
      szClInfo[0] = '\0';
      update();
   }
   if (nid == DVTIMER_ALERT)
   {
      KillTimer(clwin, DVTIMER_ALERT);
      clAlertStart = 0;
      update();
   }
   if (nid == DVTIMER_AUTORELOAD)
   {
      KillTimer(clwin, DVTIMER_AUTORELOAD);
      changeToFolder(0,1);
      updateMixPic();
      update();
      SetTimer(clwin, DVTIMER_AUTORELOAD, 3000, 0);
   }
}

void processPendingWinMsg()
{
   // process windows events?
   MSG msg;
   while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
   {
      if (GetMessage(&msg, NULL, 0, 0))
      {
         // if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
         {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         }
      }
   }
}

int View::nextpic(int idelta, bool bRequireMarked)
{__
   int ifile = cur.curfile;

   for (int itry=0; itry<nfile; itry++)
   {
      ifile += idelta;

      if (ifile < 0)
         ifile = nfile-1;
      else
      if (ifile >= nfile)
         ifile =0;

      // viewer mode contains ALL images
      if (!clmixer)
         break;

      if (ifile == cur.curfile) {
         if (clcurrealm==REALM_SRC && bRequireMarked==1)
            err("no further marked source file"); // issue: flicker
         else
            err("no further matching file");
         return 5;
      }

      if (clcurrealm==REALM_DST && afile[ifile].isdst)
         break;

      if (clcurrealm==REALM_SRC && afile[ifile].issrc)
         break;
   }

   // msg("nextpic: %d -> %d (m=%d,l=%d)",cur.curfile,ifile,clmixer,clcurrealm);

   cur.curfile = ifile;

   return 0;
}

void View::changeToFolder(char *pszDst, bool breload)
{
   clinreload=breload;
   clscanstart=getCurrentTime();
   clscantold=clscanstart;
   clfilesscanned=0;

   if (pszDst==0) {
      // e.g. after delete file
      pszDst = folder;
      if (clverbose)
         msg("reread folder: %s",pszDst);
   } else {
      // initial folder selection
      if (clverbose)
         msg("set folder: %s",pszDst);
      setdstdir(pszDst);

      #ifdef FIND_META_BY_NAME
      scanmetadir();
      #endif
   }

   if (clfilenamelist) {
      if (clloadfilelist >= 2)
         loadfilelist(clfilenamelist);
      clloadfilelist = 1;
   }
   else if (clstdinlist) {
      if (clstdinlist >= 2)
         loadfilelist(stdin);
      clstdinlist = 1;
   } else {
      scandir(pszDst);
   }

   clscanstart=0;

   if (dst.curfile >= nfile)
      dst.curfile = 0;
   if (src.curfile >= nfile)
      src.curfile = 0;

   loadSettings();

   loadpic(0, "ctf");

   updateMixPic();
   clinreload=0;
}

void View::setdstdir(char *psz)
{
   clmetadir[0] = '\0';
   strcopy(folder,psz);

   joinPath(clmetadir, sizeof(clmetadir)-10, folder, "zz-meta-dat");
   if (clverbose)
      msg("meta dir: %s", clmetadir);
}

int View::scandir(char *pszdir, int ilevel)
{__
   char szDirPat[510];
   memset(szDirPat,0,sizeof(szDirPat));

   char szPicnamer[510];
   memset(szPicnamer,0,sizeof(szPicnamer));

   // if (ilevel==0 && bverb) printf("scandir realm=%d\n",clcurrealm);

   bool brescan=0;

   if (pszdir) {
      if (ilevel==0)
         strcopy(folder,pszdir);
   } else {
      brescan=1;
      pszdir = folder;
      if (!*pszdir) return 9;
   }

   if (ilevel==0)
   {
      nfile = 0;
      cur.clRecentRandIndex = 0;
   }

   snprintf(szDirPat, 500, "%s\\*", pszdir);

   // printf("scan: %s\n",szDirPat);

   SFKFindData myfdat;
   memset(&myfdat, 0, sizeof(myfdat));

   intptr_t otrav = _findfirst64(szDirPat, &myfdat);

   while (nfile<MAX_FILES)
   {
      if ((nfile & 15) == 0)
      {
         processPendingWinMsg();
         checkEscape(); // scandir

         if (getCurrentTime()-clscantold >= 500)
         {
            clscantold=getCurrentTime();
            update();
         }
      }

      if (otrav == -1) break;

      clfilesscanned++;

      if (myfdat.attrib & 0x10)
      {
         // subdir
         if (clwithsub)
         {
            if (!strcmp(myfdat.name, ".")
                || !strcmp(myfdat.name, ".."))
               { }
            else if (ilevel<100)
            {
               snprintf(szDirPat, 500, "%s\\%s", pszdir, myfdat.name);
               scandir(szDirPat, ilevel+1);
            }
         }
      }
      else
      if (striEnds(myfdat.name,".jpg")
          || striEnds(myfdat.name,".jpeg")
          || striEnds(myfdat.name,".png")
          )
      {
         snprintf(szDirPat, 500, "%s\\%s", pszdir, myfdat.name);

         int iins = 0;
         for (; iins<nfile; iins++)
         {
            if (clalphasort==1 && stricmp(szDirPat, afile[iins].name) > 0)
               break;
            if (clalphasort==0 && myfdat.time_write < afile[iins].time_write)
               break;
         }

         int iremain = nfile-iins;

         // printf("insert %d %u < %u rem=%d %s\n",
         //    iins, (uint)afile[iins].time_write, (uint)myfdat.time_write, 
         //    iremain, szDirPat);

         if (iremain>0)
            memmove(&afile[iins]+1, &afile[iins], iremain*sizeof(ViewPic));

         afile[iins].name = strdup(szDirPat);
         afile[iins].time_write = myfdat.time_write;
         afile[iins].size = myfdat.size;
         afile[iins].hashv1 = 0;
         afile[iins].hashv2 = 0; // on demand getFileHashV2(szDirPat);

         nfile++;
         if (!brescan)
            cur.curfile = nfile-1;
      }

      int nrc = _findnext64(otrav, &myfdat);
      if (nrc) break;
   }

   if (otrav != -1)
      _findclose(otrav);

   #if 0
   if (ilevel == 0)
   {
      printf("--- full list: ---\n");
      for (int i=0; i<nfile; i++)
         printf("%04u size=%u %s\n",i,(uint)afile[i].size,afile[i].name);
      fflush(stdout);
   }
   #endif

   return 0;
}

int View::loadfilelist(char *pszFile)
{
   FILE *f=fopen(pszFile,"rb");
   if (!f) return 9;
   loadfilelist(f);
   fclose(f);
   return 0;
}

int View::loadfilelist(FILE *fin)
{__
   char szFile[510];
   memset(szFile,0,sizeof(szFile));

   bool brescan=0;

   strcopy(folder, ".");

   nfile = 0;
   cur.clRecentRandIndex = 0;
   cur.curfile = 0;

   FILE *fdump=0;
   if (clidumpfile[0])
      fdump=fopen(clidumpfile, "w");

   if (fin == stdin) {
      myfgets_init(1); // from stdin console handle
      fin = 0;
   } else {
      myfgets_init(0); // from file
   }

   while (myfgets(szFile, sizeof(szFile)-10, fin))
   {
      if (fdump)
         fwrite(szFile, 1, strlen(szFile), fdump);

      removeCRLF(szFile);

      if (userInterrupt()) break;

      if (!strncmp(szFile, "# ", 2))
         continue;

      if (striEnds(szFile,".jpg")
          || striEnds(szFile,".jpeg")
          || striEnds(szFile,".png"))
         { }
      else 
      {
         printf("skip, no valid image: %s\n", szFile);
         continue;
      }

      int bIsDir    = 0;
      int bCanRead  = 1;
      int bCanWrite = 1;
      num  nFileTime = 0;
      num  nFileSize = 0;
      num aExtTimes[2];
      memset(aExtTimes, 0, sizeof(aExtTimes));

      if (getFileStat(szFile, bIsDir, bCanRead, bCanWrite,
         nFileTime, nFileSize, aExtTimes, 0, 0))
      {
         printf("skip, no such file: %s\n", szFile);
         continue;
      }

      int iins = nfile;
      #if 0
      for (; iins<nfile; iins++)
      {
         if (clalphasort==1 && stricmp(szFile, afile[iins].name) > 0)
            break;
         if (clalphasort==0 && nFileTime < afile[iins].time_write)
            break;
      }
      #endif

      int iremain = nfile-iins;

      if (iremain>0)
         memmove(&afile[iins]+1, &afile[iins], iremain*sizeof(ViewPic));

      afile[iins].name = strdup(szFile);
      afile[iins].time_write = nFileTime;
      afile[iins].size = nFileSize;
      afile[iins].hashv1 = 0;
      afile[iins].hashv2 = 0;

      nfile++;

      // cur.curfile = nfile-1;
   }

   if (fdump)
      fclose(fdump);

   #if 0
   if (ilevel == 0)
   {
      printf("--- full list: ---\n");
      for (int i=0; i<nfile; i++)
         printf("%04u size=%u %s\n",i,(uint)afile[i].size,afile[i].name);
      fflush(stdout);
   }
   #endif

   return 0;
}


LRESULT View::process(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

   // clwin may NOT exist yet.
   if (clwin == NULL)
      return DefWindowProc(hWnd, msg, wParam, lParam);


   int bKeyRepeat = 0;

   if (nfile > 1 && clmousemode)
   {
      int ideltax=abs(clbuttoncurx-clbuttonstartx);
      int ideltay=abs(clbuttoncury-clbuttonstarty);
      int bmoved = (ideltax>3 || ideltay>3);

      switch (msg)
      {
         case WM_LBUTTONUP:
            if (!bmoved) {
               clbuttondown=0;
               processKey(IDLeft,1); 
               return 0;
            }
            break;

         case WM_RBUTTONUP: 
            clbuttondown=0;
            processKey(IDRite,1); 
            return 0;

         case WM_MBUTTONUP: // random jump to a pic
            clbuttondown=0;
            gotoRandFile();
            bool bNoMetaLoad = clmixer ? 0 : 1;
            loadpic(0, "random", bNoMetaLoad); // rndjump
            updateMixPic();
            update();
            return 0;
      }
   }

   switch (msg)
   {
      case WM_ERASEBKGND:
         return 1;

      case WM_KILLFOCUS:
         #ifdef WARN_FOCUS_LOST
         alert("Keyboard Focus Lost!", "Click into image to regain control.");
         #endif
         clkeyfocus = 2;
         return 0;

      case WM_SETFOCUS:
         #ifdef WARN_FOCUS_LOST
         if (clkeyfocus == 2)
            alert("Keyboard Focus Regained.", "");
         #endif
         clkeyfocus = 1;
         return 0;
      
      case WM_DESTROY:
         PostQuitMessage(0);
         break;

      case WM_CHAR:
         processKey(wParam);
         break;

      case WM_SETCURSOR:
      {
         if (glblNormCursor) SetCursor(glblNormCursor);
         return 1;
      }

      case WM_PAINT:
      {
         draw();
         break;
      }

      case WM_SIZE:
      {
         RECT orect;
         memset(&orect, 0, sizeof(orect));
         if (GetClientRect(clwin, &orect))
         {
            clwinw = orect.right-orect.left;
            clwinh = orect.bottom-orect.top;
            genw = clwinw/1000.0;
            genh = clwinh/1000.0;
            // if (bverb) printf("resize to %d %d from %d %d %d %d\n",
            //   clwinw,clwinh, orect.left,orect.right,orect.top,orect.bottom);
         }
         if (wParam == SIZE_MINIMIZED)
            clkeyfocus = 0;
         break;
      }

      case WM_LBUTTONDOWN:
      {
         clbuttondown=1;
         clbuttoncurx = LOWORD(lParam);
         clbuttoncury = HIWORD(lParam);
         clbuttonstartx = clbuttoncurx;
         clbuttonstarty = clbuttoncury;
         break;
      }

      case WM_LBUTTONUP:
         clbuttondown=0;
      case WM_RBUTTONUP:
      {
         SHORT nksShift = GetKeyState(VK_SHIFT  );
         SHORT nksCtrl  = GetKeyState(VK_CONTROL);
         int bShift = (nksShift & (1 << 15)) ? 1 : 0;
         int bCtrl  = (nksCtrl  & (1 << 15)) ? 1 : 0;

         int xwin = LOWORD(lParam);
         int ywin = HIWORD(lParam);

         if (!processUIElem(xwin,ywin,
                  ((msg == WM_LBUTTONUP) ? 1 : 0)
               |  ((msg == WM_RBUTTONUP) ? 2 : 0)
               |  (bShift ? 4 : 0)
               |  (bCtrl ? 8 : 0)
               ))
         {
            if (nGlblShowLog==1
                && clwinmousex > (clwinw*4)/5
                && clwinmousey > (clwinh*9)/10)
               initdvlog();
         }

         break;
      }

      case WM_MBUTTONUP:
      {
         int xwin = LOWORD(lParam);
         int ywin = HIWORD(lParam);
         // if (clcurrealm == REALM_DST)
         //   editPix(cur.memmousex,cur.memmousey,255,1);
         break;
      }

      case WM_MOUSEMOVE:
      {
         clwinmousex = LOWORD(lParam);
         clwinmousey = HIWORD(lParam);

         cur.memmousex = round((clwinmousex-cur.borderw) / cur.viewscale) + cur.scanx1;
         cur.memmousey = round((clwinmousey-cur.borderh) / cur.viewscale) + cur.scany1;

         if (clbuttondown)
         {
            int dmousex = (clwinmousex - clbuttoncurx) / cur.viewscale;
            int dmousey = (clwinmousey - clbuttoncury) / cur.viewscale;
            clbuttoncurx = clwinmousex;
            clbuttoncury = clwinmousey;

            int ifocusx = cur.memfocusx;
            int ifocusy = cur.memfocusy;
            ifocusx -= dmousex;
            ifocusy -= dmousey;

            int iscanw = min(cur.mempicw*1.0,round(clwinw *1.0/ cur.viewscale));
            int iscanh = min(cur.mempich*1.0,round(clwinh *1.0/ cur.viewscale));

            ifocusx = max(0,ifocusx);
            ifocusx = min(ifocusx,cur.mempicw-iscanw/2);

            ifocusy = max(0,ifocusy);
            ifocusy = min(ifocusy,cur.mempich-iscanh/2);

            if (   ifocusx != cur.memfocusx
                || ifocusy != cur.memfocusy)
            {
               cur.memfocusx = ifocusx;
               cur.memfocusy = ifocusy;
               clskipclearscreen=1;
               update();
            }

            break;
         }

         int iOldShow = nGlblShowLog;
         if (clwinmousex > (clwinw*4)/5
             && clwinmousey > (clwinh*9)/10)
            nGlblShowLog = 1;
         else
            nGlblShowLog = 0;
         if (iOldShow != nGlblShowLog)
            update();
         else
         if (clshowinfo)
            update();

         break;
      }

      case WM_MOUSEWHEEL: // .
      {
         short zDelta = (short)HIWORD(wParam);

         SHORT nksShift = GetKeyState(VK_SHIFT  );
         SHORT nksCtrl  = GetKeyState(VK_CONTROL);
         int bShift = (nksShift & (1 << 15)) ? 1 : 0;
         int bCtrl  = (nksCtrl  & (1 << 15)) ? 1 : 0;

         if (clshowmanual) {
            moveInManual(zDelta>0 ? -3 : 3);
            update();
            break;
         }

         double dscale = cur.viewscale;
         int ifocusx = cur.memfocusx;
         int ifocusy = cur.memfocusy;

         if (zDelta > 0)
            dscale = min(128.0, dscale * 1.20);
         else
            dscale = max(0.05, dscale / 1.20);

         double deltamousex = cur.memmousex - ifocusx;
         double deltamousey = cur.memmousey - ifocusy;

         ifocusx += (deltamousex/10.0);
         ifocusy += (deltamousey/10.0);

         int iscanw  = round(clwinw *1.0/ dscale);
         int iscanh  = round(clwinh *1.0/ dscale);

         if (ifocusx - iscanw/2 < 0)
            ifocusx = iscanw/2;
         if (ifocusx + iscanw/2 > cur.mempicw)
            ifocusx = cur.mempicw - iscanw/2;

         if (ifocusy - iscanh/2 < 0)
            ifocusy = iscanh/2;

         if (ifocusy + iscanh/2 > cur.mempich)
            ifocusy = cur.mempich - iscanh/2;

         ifocusx = max(0,ifocusx);
         ifocusx = min(ifocusx,cur.mempicw-1);
         ifocusy = max(0,ifocusy);
         ifocusy = min(ifocusy,cur.mempich-1);

         if (cur.viewscale != dscale
             || ifocusx != cur.memfocusx
             || ifocusy != cur.memfocusy)
         {
            cur.viewscale = dscale;
            cur.memfocusx = ifocusx;
            cur.memfocusy = ifocusy;
            update();
         }

         break;
      }

      case WM_KEYDOWN:
      {
         UINT nKey = (UINT)wParam;

         SHORT nksShift = GetKeyState(VK_SHIFT  );
         SHORT nksCtrl  = GetKeyState(VK_CONTROL);
         int bShift = (nksShift & (1 << 15)) ? 1 : 0;
         int bCtrl  = (nksCtrl  & (1 << 15)) ? 1 : 0;

         if (bCtrl) break;

         switch (nKey)
         {
            case VK_LEFT  : processKey(IDLeft,1); break;
            case VK_RIGHT : processKey(IDRite,1); break;

            case 0x21: // PAGE UP
               if (clshowmanual) {
                  for (int i=0; i<15; i++)
                     moveInManual(-1);
                  update();
               }
               break;

            case 0x22: // PAGE DOWN
               if (clshowmanual) {
                  for (int i=0; i<15; i++)
                     moveInManual(1);
                  update();
               }
               break;
         }

         break;
      }
      
      case WM_KEYUP:
      {
         UINT nKey = (UINT)wParam;

         SHORT nksShift = GetKeyState(VK_SHIFT  );
         SHORT nksCtrl  = GetKeyState(VK_CONTROL);
         int bShift = (nksShift & (1 << 15)) ? 1 : 0;
         int bCtrl  = (nksCtrl  & (1 << 15)) ? 1 : 0;

         switch (nKey)
         {
            case VK_F1:
               clshowmanual ^= 0x1;
               if (!clshowui)
                  toggleui();
               update();
               break;

            case VK_F2:
               toggleui();
               update();
               break;

            case VK_F3:
               clshowinfo ^= 0x1;
               update();
               break;

            case VK_LEFT  : processKey(IDLeft); break;
            case VK_RIGHT : processKey(IDRite); break;
            case VK_UP    : processKey(IDUp); break;
            case VK_DOWN  : processKey(IDDown); break;

            #ifndef USE_MEM_FS
            case VK_DELETE:
               if (bShift) {
                  askDelete();
               } else {
                  moveImageTo("zz-output\\trash");
                  update();
               }
               break;
            #endif

            case VK_F4:
               toggleMinimize();
               break;

            case VK_F5:
               changeToFolder(0,1);
               updateMixPic();
               update();
               break;
         }

         break;
      }

      case WM_TIMER:
         handleTimer(wParam);
         break;


      default:
         return DefWindowProc(hWnd, msg, wParam, lParam);
   }

   return 0;
}

char *getLoadPath(char *ptitle);

void View::gotoRandFile( )
{
   #ifdef WITH_BACK_RAND
   cur.aclRecentPos[cur.clRecentPosIndex]=cur.curfile;
   cur.clRecentPosIndex = (cur.clRecentPosIndex+1) & MAX_RECENT_POS_MASK;
   #endif

   if (clmixer)
   {
      uint nlimit = cur.nselfiles;
      if (nlimit<1) return;
   
      uint rnow=0,k=0,itry=0;
   
      for (itry=0; itry<10; itry++)
      {
         // random start index
         rnow = (rand() % nlimit);
   
         bool bfail=0;
         for (int i=0; i<cur.clRecentRandIndex && i<nlimit; i++)
         {
            if (cur.aclRecentRand[i]==rnow)
               { bfail=1; break; }
         }
         if (!bfail)
            break;
      }
   
      cur.aclRecentRand[cur.clRecentRandIndex] = rnow;
      cur.clRecentRandIndex++;
   
      // if showed ALL images once
      // or if recent limit reached
      if (cur.clRecentRandIndex >= nlimit
          || cur.clRecentRandIndex >= MAX_RECENT_RAND)
      {
         cur.clRecentRandIndex = 0;
      }
   
      // from virtual to global index
      uint iindex = cur.aselfiles[rnow];
      if (iindex >= nfile)
         { err("int. error in file index"); iindex=0; }
      cur.curfile = iindex;
   }
   else
   {
      uint nlimit = nfile;
      if (nlimit<1) return;
 
      uint rnow=0,k=0,itry=0;
 
      for (itry=0; itry<10; itry++)
      {
         // random start index
         rnow = (rand() % nlimit);
 
         bool bfail=0;
         for (int i=0; i<cur.clRecentRandIndex && i<nlimit; i++)
         {
            if (cur.aclRecentRand[i]==rnow)
               { bfail=1; break; }
         }
         if (!bfail)
            break;
      }
 
      cur.aclRecentRand[cur.clRecentRandIndex] = rnow;
      cur.clRecentRandIndex++;
 
      // if showed ALL images once
      // or if recent limit reached
      if (cur.clRecentRandIndex >= nlimit
          || cur.clRecentRandIndex >= MAX_RECENT_RAND)
      {
         cur.clRecentRandIndex = 0;
      }
 
      // we have global index
      if (rnow >= nfile)
         { err("int. error in global file index"); rnow=0; }
      cur.curfile = rnow;
   }
}

bool View::inMarkAndShowEditMode( )
{
   return 0;
}

int putClipboard(char *pszStr);

#define IDC_GOTO_IMAGE_TEXT 601
#define IDC_GOTO 602
#define IDC_CANCEL 603

LRESULT CALLBACK myAskINumProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
      case WM_INITDIALOG:
      {
         centerWithinArea(hDlg);
      }
         return TRUE;
 
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDCANCEL:
            case IDC_GOTO:
            case IDC_CANCEL:
            {
               // retrieve text from edit control
               HWND hedit = FindWindowEx(hDlg, 0, "Edit", 0);
               if (hedit)
                  GetWindowText(hedit, glblDialogURL, sizeof(glblDialogURL)-10);

               // caller does setfocus afterwards
               EndDialog(hDlg, LOWORD(wParam));
               return TRUE;
            }
               break;
         }
      }
         break;
   }
   return FALSE;
}


void View::addToImageList(char *prelname)
{
   #ifndef INTERNAL
   if (provideMetaDir()) // addToImageList
      return;
   char *plname = vtext("%s\\%s",clmetadir,prelname);
   #endif

   memset(szGlblListBuf,0,sizeof(szGlblListBuf));

   FILE *fin=fopen(plname,"rb");
   if (fin) {
      fread(szGlblListBuf,1,sizeof(szGlblListBuf)-10,fin);
      fclose(fin);
   }
   char *pname=afile[cur.curfile].name;
   int ipos=strlen(szGlblListBuf);
   int ilen=strlen(pname);
   if (ipos+ilen+100 > sizeof(szGlblListBuf)) {
      dvlog("[error] image list too long, cannot add");
      return;
   }
   strcpy(szGlblListBuf+ipos,pname);
   strcat(szGlblListBuf,"\n");
   saveFile(plname,(uchar*)szGlblListBuf,strlen(szGlblListBuf),"wb");
   dvlog("added to %s",plname);
   update();
}

int View::gotoImageNumber(int iOptIdx)
{__
   mclear(glblDialogURL);

   int inum = iOptIdx;

   if (inum==-1)
   {
      long nrc = DialogBox(clinstance, (LPCTSTR)600, clwin, (DLGPROC)myAskINumProc);
   
      // re-enable keyboard input
      SetFocus(clwin);

      inum = atoi(glblDialogURL);
         
      if (nrc != IDC_GOTO) return 1;
     
      if (inum > 0) inum--;
   }

   if (inum >= 0 && inum < nfile)
   {
      cur.curfile = inum;
      int irc = loadpic(0, "<");
      updateMixPic();
      update();
   }

   return 0;
}

void View::stopAutoReload(int iFrom, int iInfo)
{
   // printf("stopAutoReload %d %d\n",iFrom,iInfo);
   if (clautoreload) {
      clautoreload=0;
      KillTimer(clwin, DVTIMER_AUTORELOAD);
   }
}

int View::processKey(int c, int iFlags)
{__
   SHORT nksShift = GetKeyState(VK_SHIFT  );
   SHORT nksCtrl  = GetKeyState(VK_CONTROL);
   int bShift = (nksShift & (1 << 15)) ? 1 : 0;
   int bCtrl  = (nksCtrl  & (1 << 15)) ? 1 : 0;

   int brep = (iFlags & 1) ? 1 : 0; // repeat
   int bfromui = (iFlags & 2) ? 1 : 0; // by click

   if (c == 0x1b  // VK_ESCAPE
       || c == '+')
   {
      // exit(0);
      PostQuitMessage(0);
      return 0;
   }

   if (c != 't' && c != 18) // ctrl+r
      stopAutoReload(1,c);

   // dvlog("key 0x%02x ctrl=%d\n",c,bCtrl);

   /*
      Ctrl+M becomes 0x0D.
      Ctrl+s becomes 0x13.
   */

   int iupdate = 0;

   switch (c)
   {
      case 'o':
      {
         if (clcurrealm!=REALM_DST)
            break;
         char *psz = getLoadPath("open images from a folder");
         if (!psz) break;
         changeToFolder(psz); // interactive selection
         iupdate=2;
         break;
      }

      case IDWithSub:
         clwithsub ^= 0x1;
         iupdate=1;
         break;

      case IDLeft :
      {
         if (bfromui) brep = 1;
         #ifdef WITH_BACK_RAND
         if (brep==0 && bCtrl==1) {
            cur.aclRecentPos[cur.clRecentPosIndex]=cur.curfile;
            if (cur.clRecentPosIndex>0)
               cur.clRecentPosIndex--;
            else
               cur.clRecentPosIndex = MAX_RECENT_POS_MASK;
            cur.curfile = cur.aclRecentPos[cur.clRecentPosIndex];
            if (cur.curfile<0 || cur.curfile>nfile) cur.curfile=0;
            loadpic(0, "<");
            iupdate=2;
         }
         else
         #endif
         if (brep==1)
         {
            if (bShift) {
               for (int i=0;i<10;i++)
                  nextpic(-1); // left
            } else {
               nextpic(-1); // left
            }
            loadpic(0, "<");
            iupdate=2;
         }
         break;
      } 

      case IDRite:
      {
         if (bfromui) brep = 1;
         #ifdef WITH_BACK_RAND
         if (brep==0 && bCtrl==1) {
            cur.clRecentPosIndex = (cur.clRecentPosIndex+1) & MAX_RECENT_POS_MASK;
            cur.curfile = cur.aclRecentPos[cur.clRecentPosIndex];
            if (cur.curfile<0 || cur.curfile>nfile) cur.curfile=0;
            loadpic(0, "<");
            iupdate=2;
         }
         else
         #endif
         if (brep==1)
         {
            if (bShift) {
               for (int i=0;i<10;i++)
                  nextpic(1); // rite
            } else {
               nextpic(1); // rite
            }
            loadpic(0, ">");
            iupdate=2;
         }
         break;
      }

      case '0':
      case '=':
      {
         if (!afile[cur.curfile].name)
            break;
         char *pszFile = afile[cur.curfile].name;
         if (c == '0')
            pszFile = absPath(pszFile);
         if (!pszFile)
            break;
         putClipboard(pszFile);
         dvlog("%s - name to clip",pszFile);
         iupdate=1;
         break;
      }

      case 'w':
      case '#': 
         toggleMinimize();
         break;

      case 'a': processKey(IDLeft,1); break;
      case 'd': processKey(IDRite,1); break;

      case 't':
         if (clautoreload) {
            stopAutoReload(2);
            // clclearscreen=1;
         } else {
            clautoreload=1;
            SetTimer(clwin, DVTIMER_AUTORELOAD, 1000, 0);
         }
         iupdate=1;
         break;

      case '4': 
         if (!showui())
            break;
         if (cur.curfile==clremembered)
             clremembered=-1;
         else
             clremembered=cur.curfile;
         iupdate=1;
         break;

      case '5': moveImageTo("zz-output\\5"); iupdate=1; break;
      case '6': moveImageTo("zz-output\\6"); iupdate=1; break;
      case '7': moveImageTo("zz-output\\7"); iupdate=1; break;
      case '8': moveImageTo("zz-output\\8"); iupdate=1; break;
      case '9': moveImageTo("zz-output\\9"); iupdate=1; break;

      case ' ': // random jump to a pic
      case '1': // random jump to a pic
      {
         gotoRandFile();
         bool bNoMetaLoad = clmixer ? 0 : 1;
         loadpic(0, "random", bNoMetaLoad); // rndjump
         iupdate=2;
         break;
      }

      case 'i':
         clquietmode ^= 1;
         if (clquietmode)
            alert("quiet mode on", "will not ask before image move/delete");
         else
            alert("quiet mode off", "will ask before image move/delete");
         break;

      case IDUp:
         if (bfromui) bShift = 1;
         if (clshowmanual) {
            moveInManual(-1);
            iupdate=1;
         }
         #ifdef WITH_ROTATE
         else if (!bfromui) {
            // user intention: rotate 90 degrees
            int ipicrot = cur.clrotate;
            if (ipicrot >= 90)
               ipicrot -= 90;
            else
               ipicrot = 270;
            if (!loadpic(0, "rotate", 1, ipicrot)) {
               cur.clrotate = ipicrot;
               iupdate=3; // with meta save
            }
         } 
         #endif
         break;
   
      case IDDown:
         if (bfromui) bShift = 1;
         if (clshowmanual) {
            moveInManual(1);
            iupdate=1;
         }
         #ifdef WITH_ROTATE
         else if (!bfromui) {
            // user intention: rotate 90 degrees
            int ipicrot = cur.clrotate;
            if (ipicrot <= 180)
               ipicrot += 90;
            else
               ipicrot = 0;
            if (!loadpic(0, "rotate", 1, ipicrot)) {
               cur.clrotate = ipicrot;
               iupdate=3; // with meta save
            }
         }
         #endif
         break;

      case 0x07:   // CTRL+G (W=0x17)
         if (bCtrl)
            gotoImageNumber();
         break;

      case 0x03:   // CTRL+C
         if (bCtrl && clshowinfo && szTextTag[0])
         {
            putClipboard(szTextTag);
            dvlog("text copied to clip");
            iupdate=1;
            break;
         }
         break;

      case 0x18:  // CTRL+X
         if (bCtrl) {
            clmousemode ^= 1;
            dvlog("mouse mode %s", clmousemode ? "activated":"deactivated");
            iupdate=1;
         }
         break;

      case 0x12:  // CTRL+R
         if (bCtrl) {
            if (clautoreload) {
               stopAutoReload(3);
            } else {
               clautoreload=1;
               SetTimer(clwin, DVTIMER_AUTORELOAD, 1000, 0);
            }
            iupdate=1;
         }
         break;

      case 0x14:  // CTRL+T
         if (bCtrl) {
            tGlblStart=getCurrentTime();
            iupdate=1;
         }
         break;

      case 0x13:  // CTRL+S
         if (bCtrl) {
            cur.savepos = cur.curfile;
            msg("index = %d", cur.savepos+1);
            iupdate=1;
            saveSettings();
         }
         break;


      case 'V':
         if (clverbose) {
            msg("verbose mode off");
            clverbose = 0;
         } else {
            clverbose = 1;
            msg("verbose mode on");
         }
         iupdate=1;
         break;

      case 'v': addToImageList((char*)aGlblImageList[0]); break;
      case 'b': addToImageList((char*)aGlblImageList[1]); break;
      case 'n': addToImageList((char*)aGlblImageList[2]); break;

      // case 's':
      //    cur.savepos = cur.curfile;
      //    msg("index = %d", cur.savepos+1);
      //    iupdate=1;
      //    break;

      case 'S':
         cur.savepos2 = cur.curfile;
         msg("index2 = %d", cur.savepos2+1);
         iupdate=1;
         saveSettings();
         break;

      case 'l':
         if (cur.savepos != -1)
            gotoImageNumber(cur.savepos);
         else
            msg("no saved position");
         break;

      case 'L':
         if (cur.savepos2 != -1)
            gotoImageNumber(cur.savepos2);
         else
            msg("no saved position2");
         break;

      case 'r':
         if (clloadfilelist || clstdinlist)
         {
            if (nfile>0)
               cur.curfile=nfile-1;
            loadpic(0, "r");
         }
         else
         {
            changeToFolder(0,1);
         }
         updateMixPic();
         update();
         break;

   }

   if (iupdate >= 2) updateMixPic();
   if (iupdate >= 1) update();

   return 0;
}

#define IDC_EDIT_HIGHLIGHT 501

#define IDC_EDIT_FILEPATH  511
#define IDC_EDIT_FILENAME  512

long nGlblBMDiaX = -1;
long nGlblBMDiaY = -1;

void centerWithinArea(HWND hDlg)
{
   long nareaw = gview.clwinw;
   long nareah = gview.clwinh;
 
   RECT rown;
   GetWindowRect(hDlg, &rown);
 
   long nownw  = rown.right  - rown.left;
   long nownh  = rown.bottom - rown.top;
 
   long nspacex = nareaw - nownw;
   long nspacey = nareah - nownh;
 
   long xabs = rown.left;
   long yabs = rown.top;

   if (nspacex > 0 || nspacey > 0)
   SetWindowPos(hDlg, HWND_TOP,
      xabs, yabs, 0, 0,
      SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE
      );
}

// --- 32-bit folder selection begin ---

char  szGlblAbsBrowseFolder[SFK_MAX_PATH+50];
TCHAR szGlblBrowsePath[SFK_MAX_PATH+50];

void setBrowseFolder(char *psz, bool bFromFilePath=0) // dv172
{
   if (psz == 0) {
      mclear(szGlblAbsBrowseFolder);
      _getcwd(szGlblAbsBrowseFolder,SFK_MAX_PATH);
   } else {
      strcopy(szGlblAbsBrowseFolder,psz);
      if (bFromFilePath) {
         char *psz = strrchr(szGlblAbsBrowseFolder, glblPathChar);
         if (psz)
            *psz='\0';
      }
   }
}

extern bool endsWithPathChar(char *pszPath, bool bAcceptFWSlash);

// make sure that browse path is relative to cwd
void normalizeBrowsePath()
{
   if (!szGlblBrowsePath[0])
      return;

   mclear(szGlblOwnDir);
   _getcwd(szGlblOwnDir,sizeof(szGlblOwnDir)-10);

   if (!szGlblOwnDir[0])
      return;

   // fix: dv165: incomplete comparison lead to truncation
   if (!endsWithPathChar(szGlblOwnDir, 0))
      strcat(szGlblOwnDir, "\\");

   char *pszRaw = szGlblBrowsePath;

   // if chosen own working dir, make it "."
   if (!strcmp(pszRaw, szGlblOwnDir))
   {
      strcpy(szGlblBrowsePath, ".");
   }
   else
   // if below working dir, strip that
   if (strBegins(pszRaw, szGlblOwnDir))
   {
      // x:\mydir\foo\text.txt -> foo\text.txt
      pszRaw += strlen(szGlblOwnDir);
      if (*pszRaw == '\\')
         pszRaw++;

      // readjust incl. terminator
      memmove(szGlblBrowsePath, pszRaw, strlen(pszRaw)+1);
   }
}

int CALLBACK myBrowseFolderCallback(HWND hDlg, UINT uMsg, LPARAM lParam, LPARAM lpData)
{__
   // printf("wndproc2 hwnd=%x msg=%03x %s l=%x\n", (uint)hDlg, uMsg, msgAsString(uMsg), lParam); fflush(stdout);
   switch (uMsg)
   {
      case BFFM_INITIALIZED:
      {
         if (szGlblAbsBrowseFolder[0])
            SendMessage(hDlg, BFFM_SETSELECTION, 1, (LPARAM)szGlblAbsBrowseFolder);

         long xabs = 40;
         long yabs = 40;

         SetWindowPos(hDlg, HWND_TOP,
            xabs, yabs, 0, 0,
            SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE
            );

         break;
      }

      case BFFM_VALIDATEFAILED: {
         break;
      }
   }
   return 0;
}

char *getLoadPath(char *ptitle)
{__
   mclear(szGlblBrowsePath);

   BROWSEINFO     bi;
   ITEMIDLIST     *piidlist;
   LPMALLOC       lpMalloc;

   bi.lpszTitle = ptitle;
   bi.hwndOwner = gview.clwin;
   bi.pidlRoot  = NULL;
   bi.pszDisplayName = NULL;
   bi.ulFlags   = BIF_BROWSEINCLUDEFILES | BIF_RETURNONLYFSDIRS;
   bi.ulFlags  |= BIF_USENEWUI;  // flag not yet available under wine
   bi.lpfn      = myBrowseFolderCallback; // for centering
   bi.lParam    = 0;
   bi.iImage    = 0;

   piidlist = (ITEMIDLIST *)SHBrowseForFolder(&bi);

   if (piidlist)
   {
      if (!SHGetPathFromIDList(piidlist, szGlblBrowsePath))
         szGlblBrowsePath[0] = '\0';
      SHGetMalloc(&lpMalloc);
      lpMalloc->Free(piidlist);
      if (szGlblBrowsePath[0])
         setBrowseFolder(szGlblBrowsePath); // dv172 open dir
   }

   // fixcwd("glp"); // post SHBrowseForFolder

   normalizeBrowsePath();

   return szGlblBrowsePath[0] ? szGlblBrowsePath : 0;
}

int mbox(const char *psz, int n)
{
   char szBuf[500];
   sprintf(szBuf, "%s\n\nstatus code : %d", psz, n);
   MessageBox(0, szBuf, "startup error", MB_OK);
   return 0;
}

char *getLoadPathNew(char *ptitle)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | 
        COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog *pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
                IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem *pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        MessageBoxW(NULL, pszFilePath, L"File Path", MB_OK);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return 0;
}

// --- 32-bit folder selection end ---

int isopt(char *psz, char *pat)
{
   char szBuf[100];
   sprintf(szBuf, "%s ", pat);
   if (!strcmp(psz, pat)) return 1;
   if (!strncmp(psz, szBuf, strlen(szBuf))) return 1;
   return 0;
}

// .
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
   char aFile[3][SFK_MAX_PATH+10];
   memset(aFile,0,sizeof(aFile));

   mclear(szGlblOwnDir);
   _getcwd(szGlblOwnDir,sizeof(szGlblOwnDir)-10);

   int nsubrc=0;
   if ((nsubrc = OleInitialize(0)) != S_OK)
      mbox("OleInitialize failed", nsubrc);

   tGlblStart = getCurrentTime();

   // read defaults from environment
   // IVIEW_CONFIG=quiet,iedit=c:\app\paint.exe
   char *pcfg = getenv("IVIEW_CONFIG");
   if (pcfg)
   {
      char *p1=0,*p2=0;
      if (strstr(pcfg, "nosub"))
         gview.clwithsub = 0;
      if (strstr(pcfg, "quiet"))
         gview.clquietmode = 1;
      if (strstr(pcfg, "showui"))
         gview.clshowui = 1;
      if (strstr(pcfg, "mousemode"))
         gview.clmousemode = 1;
      if (p1=strstr(pcfg,"iedit:")) {
         p1+=strlen("iedit:");
         p2=p1;
         while (*p2!=0 && *p2!=',') p2++;
         int ilen=p2-p1;
         if (ilen<SFK_MAX_PATH) {
            memcpy(gview.imageeditor,p1,ilen);
            gview.imageeditor[ilen]='\0';
         }
      }
   }

   char *pszArgs = (char*)lpCmdLine;

   // todo: support quotes
   int nfile=0;
   char *psz = pszArgs;
   while (psz && *psz)
   {
      char *pend=psz;

      // consider "c:\the blank pic"
      bool bquoted=0;
      if (*pend=='\"') {
         bquoted=1;
         psz++; pend++;
         while (*pend!=0 && *pend!='\"')
            pend++;
      } else {
         while (*pend!=0 && *pend!=' ')
            pend++;
      }
      int ilen=pend-psz;
      if (bquoted!=0 && *pend=='\"')
         pend++;

      if (ilen<1) break;
      do
      {
         if (isopt(psz, "-nosub")) {
            gview.clwithsub = 0;
            break;
         }
         if (isopt(psz, "-sub") || isopt(psz, "-withsub")) {
            gview.clwithsub = 1;
            break;
         }
         if (isopt(psz, "-quiet")) {
            gview.clquietmode = 1;
            break;
         }
         if (isopt(psz, "-showui")) {
            gview.clshowui = 1;
            break;
         }
         if (isopt(psz, "-i")) {
            gview.clstdinlist=2;
            break;
         }
         if (!strncmp(psz, "-idump=", 7)) {
            strcopy(gview.clidumpfile, psz+7);
            for (int i=0;i<sizeof(gview.clidumpfile)-10;i++) {
               if (gview.clidumpfile[i]==' ') {
                  gview.clidumpfile[i]='\0';
                  break;
               }
            }
            break;
         }
         if (isopt(psz, "-v")) {
            gview.clverbose=1;
            break;
         }
         if (isopt(psz, "-a")) {
            gview.clalphasort=1;
            break;
         }
         memcpy(aFile[nfile],psz,ilen);
         nfile++;
         if (nfile>2)
            break;
      }
      while (0);
      psz=pend;
      while (*psz==' ') psz++;
   }

   char *pszDst=aFile[0];
   char *pszMove=aFile[1];

   srand(time(0)); // getCurrentTime());

   if (pszMove[0])
      strcopy(gview.clmovedir,pszMove);

   // important to get real pixels on GetSystemMetrics etc.
   ::SetProcessDPIAware();

   glblScalePartCursor = LoadCursor(NULL, IDC_SIZEALL);
   glblNormCursor      = LoadCursor(NULL, IDC_ARROW);

   if (pszDst[0] || gview.clstdinlist)
      gview.clscanstart = getCurrentTime();

   if (gview.create(hInstance))
      return 9;

   ShowWindow(gview.clwin, nCmdShow);
   // UpdateWindow(gview.clwin);

   if (pszDst[0] && strEnds(pszDst,".txt"))
   {
      // load file list
      gview.clfilenamelist = pszDst;
      gview.clloadfilelist = 2;
      gview.changeToFolder(".");
   }
   else if (pszDst[0])
   {
      // TargetDir by CommandLine
      DWORD nAttrib = GetFileAttributes(pszDst);
      if (nAttrib & FILE_ATTRIBUTE_DIRECTORY)
      {
         gview.changeToFolder(pszDst); // cmdline
      }
      else
      {
         gview.clscanstart = 0;
         gview.afile[0].name = strdup(pszDst);
         gview.nfile=1;
         gview.loadpic(0, "ctf");
         gview.updateMixPic();
      }
   }
   else if (gview.clstdinlist)
   {
      gview.changeToFolder(".");
   }

   gview.update();

   gview.startupdone=1;

   MSG msg;
   BOOL fGotMessage;
   while ((fGotMessage = GetMessage(&msg, (HWND) NULL, 0, 0)) != 0 
           && fGotMessage != -1) 
   { 
      TranslateMessage(&msg); 
      DispatchMessage(&msg); 
   } 

   gview.closeView();
   DestroyWindow(gview.clwin);
   OleUninitialize();

   return 0;
}
 
