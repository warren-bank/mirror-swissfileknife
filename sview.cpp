/*
   sfk snapview by stahlworks art & technology

   known issues:
   -  not all browser instances deleted on exit
      (however cleaned up implicitely by process end)
   -  general review of all things (not) copied
      from old to new view.

   0.3.1
   -  fix: mouse wheel lockup on high wheel speed.
   -  fix: cleanup of behaviour on changing views by rclick:
   -  fix: sometimes uneditable file mask.
   -  fix: always activate new view.
   -  fix: shift, ctrl key lock on view change.
   -  fix: uneditable mask on view change.
   -  fix: always go to edit mask mode on view change.

   0.3.0
   -  add: shift+lmouse now allows multi-word and line
           selection. on shift release, it's copied to clipboard.
   -  add: shift-click into top line copies top line.
   -  add: shift-click into bottom part copies mask.
   -  fix: shift+lmouse now also checks msg flag 0x4.
           this is needed on focus change from editor to viewer.
   -  fix: nClXOffs handling on shift-copy, result marking.

   0.2.8
   -  add: support for permanent wfree by ctrl+mbutton.
   -  add: rework of help text, added second help screen.
   -  add: local search scope support
   -  fix: multi*pattern*search improved, didn't find some lines.

   0.2.4
   -  add: wlock, wfree modes for multi-use of mouse wheel.
      the modes can be switched by middle button.
      in wfree mode, wheel behaves as in most text editors
      and now allows free scrolling.

   0.2.3
   -  add: horizontal scrolling using shift+left/right.
   -  fix: auto-selected masks now limited to 80 chars.
   -  fix: use HWND_TOP i/o TOPMOST to allow overlapping.

   0.2.1
   -  keyboard testing intervals now 50 msec.
   -  adapted help text, recommend -wrap=100 for any text.
   -  now testing for pending keyboard input during search,
      interrupting the search if so.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>

#define VER_NUM  "0.3.1"
#define VER_STR  "sfk snapview " VER_NUM " beta"
#define INFO_STR "f1:fullscr f8:help < sfk snapview " VER_NUM

#ifdef _WIN32
  #include <windows.h>
  #ifndef WM_MOUSEWHEEL
   #define WM_MOUSEWHEEL 0x020A
   #define WHEEL_DELTA      120
  #endif
  #include <sys/timeb.h>
  #include <time.h>
  #include <process.h>
  #define getpid() _getpid()
  #include <errno.h>
  #include <direct.h>
#else
  #include <unistd.h>
  #include <dirent.h>
  // #include <ndir.h>
  // #include <sys/ndir.h>
  // #include <sys/dir.h>
  // #define dirent direct
#endif

// #define WITH_MEM_TRACE

#ifdef _WIN32
 #ifdef WITH_MEM_TRACE
  #include "memdeb.cpp"
 #endif
#endif

#include "mtk/mtktrace.hpp"
#include "mtk/mtktrace.cpp"

#define _ mtklog("[%d]",__LINE__);

#define OWN_TOLOWER
// #define SHOW_SHIFT_CTRL

char *pszGlblHelp =
VER_STR "\n"
"realtime text browser\n"
"stahlworks art & technology\n"
"\n"
"F1: FULLSCREEN MODE ON/OFF \n"
"F2: move result focus up   \n"
"F3: move result focus down \n"
"F4: case-sensitive search  \n"
"F6: hide all viewer windows\n"
"F8: toggle help            \n"
"\n"
"Select-And-Search:  LeftMouseClick ON A WORD.\n"
"SAS in new view  : RightMouseClick ON A WORD.\n"
"Activate window:  LeftMouseClick ON EMPTY SPACE.\n"
"Create new view: RightMouseclick ON EMPTY SPACE.\n"
"\n"
"To quickly jump through the search results,\n"
"press SHIFT+CURSOR_DOWN, or use MOUSEWHEEL,\n"
"if in wlock mode. MidButton for wfree mode,\n"
"in which you can use the mouse as usual.   \n"
"SHIFT+MouseWheel always jumps thru results.\n"
"\n"
"To edit the search mask, type any character,\n"
"or Left/Right/Home/End/Insert. The search   \n"
"mask may contain * wildcards, for example:  \n"
"\"class *bar\" finds \"class CAnyFooBar\"\n"
"\n"
"CTRL+Home: jump to previous :file:  \n"
"CTRL+SHFT+Home: jump to top of file.\n"
"SHIFT+L/R, ENTER: horizontal motion.\n"
"        CTRL+TAB to change tab size.\n"
" To close latest view, press ESCAPE.\n"
" To close all,press ESC in 1st view.\n"
"\n"
"to copy words or lines to clipboard, keep SHIFT pressed, \n"
"then left-click into the first and last character, word, \n"
"or line of your choice. when done, release SHIFT to copy.\n"
"\n"
"Press F8 now for next help page.\n"
;

char *pszGlblHelp2 =
"sfk snapview is intended for developers and power users who\n"
"want to research all kinds of informations in tousands of  \n"
"ASCII files (.cpp, .java, .html, .txt, etc.). collect all  \n"
"your files into one large file using the swiss file knife: \n"
"\n"
"sfk snapto=all.txt . .txt\n"
"\n"
"now you have a \"snapfile\", a snapshot of files, containing  \n"
"thousands of subfile entries beginning with the line :file:.\n"
"load this into sfk snapview:\n"
"\n"
"            sview all.txt\n"
"\n"
"and perform realtime searches by selecting words with left\n"
"or right mouse button, or simply by typing word patterns. \n"
"\n"
"Extended Functionality:             \n"
"\n"
"F5: select local or gobal file scope. global is default,  \n"
"    which means all subfiles are searched. in local scope,\n"
"    you may limit the search range to specific subfiles.  \n"
"    local scope can be used only with sfk snapfiles.      \n"
"\n"
"With local scope active, press TAB key to switch between editing of\n"
"search mask and path mask. Path mask is a single, case-insensitive \n"
"expression, * is not supported. for example, \"bardriver\" selects   \n"
"all files in \"testfiles\\FooBank\\BarDriver\\\", but also files in     \n"
"\"testfiles\\ChokobarDriversystem\\\", if contained in the snapfile.   \n"
"\n"
"copy screen text    -> clipboard  :  CTRL+INSERT\n"
"copy clipboard line -> search mask: SHIFT+INSERT\n"
"copy selected line  -> clipboard  :  CTRL+LMOUSE\n"
"        Permanently activate wfree: CTRL+MBUTTON\n"
"\n"
"To configure some defaults via environment:\n"
"set SFK_SNAPVIEW=tabsize:3,case:0\n"
"\n"
"Press F8 now to exit help.\n"
;

#define IDR_MAINFRAME      128
#define IDD_XV_DIALOG      102
#define IDD_ABOUTBOX       103
#define IDS_APP_TITLE      103
#define IDM_ABOUT          104
#define IDM_EXIT           105
#define IDS_HELLO          106
#define IDI_XV             107
#define IDI_SMALL          108
#define IDC_XV             109
#define IDC_MYICON           2
#define IDC_STATIC          -1

// Next default values for new objects
#define _APS_NEXT_RESOURCE_VALUE        129
#define _APS_NEXT_COMMAND_VALUE         32771
#define _APS_NEXT_CONTROL_VALUE         1000
#define _APS_NEXT_SYMED_VALUE           110

#define MAX_LOADSTRING 100

#define uchar unsigned char
#define ulong unsigned long
#define bool  unsigned char

// Global Variables:
HINSTANCE hInst;                       // current instance
TCHAR *szTitle = "tmp01";
TCHAR *szWindowClass = "sfkview";
HFONT glblFont = 0;
HINSTANCE glblInstance = 0;
int   glblCmdShow = 0;
long  nGlblDeskWidth  = 800; // adapted on startup
long  nGlblDeskHeight = 600;
long  nGlblBaseX      = 20;
long  nGlblBaseY      = 20;
long  nGlblFullWidth  = 760;
long  nGlblThirdWidth = 200;
long  nGlblColHeight  = 500;
char  *pszGlblFile = "";

#ifdef _WIN32
 typedef __int64 num;
#else
 typedef long long num;
#endif

num getCurrentTime()
{
   #ifdef _WIN32
   return (num)GetTickCount();
   #else
   return 0;
   #endif
}

#define BCOL_MAX 200
#define BROW_MAX 100
#define MAX_LINE_LEN   4096
#define MAX_SUB_MASK     10
#define MAX_SNAPPRE_LEN 100

class Browser 
{
public:
   Browser  ( );
  ~Browser  ( );
   int   create      (Browser *pLeft, ulong nid, ulong x, ulong y, ulong w, ulong h, char *apLineTable[], ulong nLines);
   void  destroy     ( );
   void  setLine     (ulong nLine, char *pszText);
   void  setAttr     (ulong nLine, char *pszMask);
   void  update      ( );
   LRESULT process   (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
   void  setMask     (char *psz);
   void  setFileMask (char *psz);
   void  toggleWidth ( );
   void  copyConfig  (Browser *pSrc); // bool bStdSnapFile, bool bCluster, char *pszPrefix, bool bCase);
   void  config      (bool bsnap, bool bclust, char *pszPre);
   void  activate    ( );

private:
   void  setStatus   (char *pszStatus);
   void  fillRect    (HDC hdc,long x1,long y1,long x2,long y2,ulong nCol);
   void  updateMatchingFiles( );
   void  detab       (char *pszIn, char *pszOut, ulong lMaxOut);
   void  copyScreenToClipboard   ( );
   void  goLocalFileStart  ( );
   void  goLocalNextFile   ( );
   void  strippedLineToClip(char *psz);
   void  calcTopAndBot     (long &lrTop, long &lrBot);
   void  goPreviousMaskHit ( );
   void  goNextMaskHit     ( );
   void  setStringToLower  (char *psz);
   void  prepareMask       (char *pszMask);
   bool  matches           (char *pszStr, int *pHitIndex=0, int *pLen=0);
   bool  matchesFile       (char *pszFile);
   void  extendMaskByClass ( );
   void  handleTimer       ( );
   void  setMaskModified   ( );
   void  gotoFirstMaskHit  ( );
   void  updatePanel       ( );
   void  modifySearchMask  (UINT nKey);
   void  autoSelect        (char *pszLine, ulong ncol); // -> to szClSel
   void  putClipboard      (char *psz, bool bShowStatus=0);
   long  getClipboard      ( );  // returns no. of chars retrieved, if any
   void  drawBorder        (HDC hdc, RECT rt, ulong CBrTop, ulong CBrLeft, ulong CDark1, ulong CDark2, bool bActive);
   void  closeRight        ( );
   #ifdef OWN_TOLOWER
   void  prepareToLower    ( );
   #endif
   void  clipSelectHit     (ulong nLine, ulong ncol);
   void  clipSelCopy       ( );
   void  clipSetStatus     ( );

   Browser *pLeft;
   Browser *pRight;

   HWND  clWin;
   char  aText[BROW_MAX+2][BCOL_MAX+2];
   char  aAttr[BROW_MAX+2][BCOL_MAX+2];
   ulong wchar,hchar;
   char  **apClLines;
   ulong nClLines;
   ulong nClBotLines;
   ulong nClTopLine;
   ulong nClX,nClY;
   ulong nClPixWidth;
   ulong nClPixHeight;
   char  szClMask[MAX_LINE_LEN+10];
   char  szClFileMask[MAX_LINE_LEN+10];
   char  szClClassBuf[MAX_LINE_LEN+10];
   char  szClMaskBuf[MAX_LINE_LEN+10];
   char  szClSel[MAX_LINE_LEN+10];
   char  szClTitleBuf[MAX_LINE_LEN+10];
   char  szClMatchBuf[MAX_LINE_LEN+10];
   char  szClClipLine[MAX_LINE_LEN+10];
   char  szClStatusLine[MAX_LINE_LEN+10];
   char  szClClipTabBuf[MAX_LINE_LEN+10];
   char  szClTextBuf[BCOL_MAX+10];
   long  nClWheel;
   ulong nCurSel;
   long  iMaskPos;      // position of input cursor in mask field
   long  iFileMaskPos;  // position of input cursor in mask field
   ulong nClMaskHits;
   bool  bClSearching;  // 0, 1, 2
   num   nClSearchStart;
   num   nClSearchTime;
   bool  bClTimerSet;
   ulong nClID;
   char  szIDBuf[100];
   #ifdef OWN_TOLOWER
   char  aClLowerTab[256+10];
   #endif

   char  aClMask[MAX_SUB_MASK][1024];
   ulong nClMask;
   long  aClTopFix[3];  // possible lock positions for search cursor
   long  nClTopFixIdx;  // index into above
   long  nClTopFix;     // resulting value from index
   bool  bClStdSnapFile;   // using :file: syntax or not
   bool  bClStdCluster;
   char  szClSnapPrefix[MAX_SNAPPRE_LEN+10];
   char  *pszClCurSubFile;  // i.e. :file:
   bool  bClCaseSearch;
   static bool bClShift; // shift key pressed, global accross all viewers
   static bool bClCtrl;  //  ctrl key pressed, global accross all viewers
   bool  bClActive;
   long  nClHelpMode;
   ulong nClTabSize;
   long  nClXOffs;      // view offset, SHIFT+CRSR L/R
   long  nClMWMode;     // mouse wheel mode, 0 to 2
   bool  bClEditFileMask;  // edit filemask, else search mask
   bool  bClLocalScope;
   long  nClMatchingFiles; // on local scope, how many filenames do match
   long  nClFirstFileMatch;
   bool  bClClearBottom;

   long  nClClipLRow;   // absolute low  line of Shift+Button selection
   long  nClClipHRow;   // absolute high line of Shift+Button selection
   long  nClClipLCol;   // absolute left  column of Shift+Button selection
   long  nClClipRCol;   // absolute right column of Shift+Button selection
};

#define MAX_BROWSERS 100
Browser *apGlblBrowsers[MAX_BROWSERS];
ulong nGlblBrowsers = 0;
bool  bGlblMinimized = 0;
ulong nGlblSubFiles = 0;

bool Browser::bClShift = 0;
bool Browser::bClCtrl  = 0;

Browser::Browser() 
{
   clWin = 0;
   memset(aText, 0, sizeof(aText));
   memset(aAttr, 0, sizeof(aAttr));
   wchar = hchar = 0;
   apClLines = 0;
   nClLines = 0;
   nClBotLines = 3;
   nClTopLine = 0;
   nClPixWidth = 1;
   nClPixHeight = 1;
   pRight = 0;
   nClX = nClY = 0;
   memset(szClMask, 0, sizeof(szClMask));
   memset(szClFileMask, 0, sizeof(szClFileMask));
   memset(szClClassBuf, 0, sizeof(szClClassBuf));
   memset(szClMaskBuf, 0, sizeof(szClMaskBuf));
   memset(szClSel, 0, sizeof(szClSel));
   memset(szClMatchBuf, 0, sizeof(szClMatchBuf));
   memset(szClClipLine, 0, sizeof(szClClipLine));
   memset(szClStatusLine, 0, sizeof(szClStatusLine));
   memset(szClTextBuf, 0, sizeof(szClTextBuf));
   memset(szClSnapPrefix, 0, sizeof(szClSnapPrefix));
   memset(szClClipTabBuf, 0, sizeof(szClClipTabBuf));
   nClWheel = 0;
   nCurSel  = 0;
   iMaskPos = 0;
   iFileMaskPos = 0;
   nClMaskHits = 0;
   bClSearching = 0;
   bClTimerSet = 0;
   nClID    = 0;
   memset(szIDBuf, 0, sizeof(szIDBuf));
   memset(aClMask, 0, sizeof(aClMask));
   #ifdef OWN_TOLOWER
   memset(aClLowerTab, 0, sizeof(aClLowerTab));
   #endif
   nClMask = 0;
   aClTopFix[0] = aClTopFix[1] = aClTopFix[2] = 3;
   nClTopFixIdx = 1; // middle lock by default
   nClTopFix    = 3;
   bClStdSnapFile = 0;
   bClStdCluster  = 0;
   pszClCurSubFile = 0;
   bClCaseSearch  = 0;
   bClActive = 0;
   nClHelpMode = 0;
   nClSearchStart = 0;
   nClSearchTime = 0;
   #ifdef OWN_TOLOWER
   prepareToLower();
   #endif
   nClTabSize = 3;
   nClXOffs = 0;
   nClMWMode = 1; // wfree (temporary) by default
   bClEditFileMask = 0;
   bClLocalScope = 0;
   nClMatchingFiles = -1;
   nClFirstFileMatch = -1;
   bClClearBottom = 0;
   nClClipLRow = -1;
   nClClipHRow = -1;
   nClClipLCol = -1;
   nClClipRCol = -1;

   char *psz1 = getenv("SFK_SNAPVIEW");
   if (psz1) 
   {
      char *psz2 = strstr(psz1, "tabsize:");
      if (psz2)
         if (!(nClTabSize = atol(psz2+strlen("tabsize:"))))
            nClTabSize = 3;

      psz2 = strstr(psz1, "case:");
      if (psz2)
         bClCaseSearch = atol(psz2+strlen("case:"));
   }
}

Browser::~Browser()
{
}

char szCmpBuf1[4096];
char szCmpBuf2[4096];
static long mystrstri(char *psz1, char *psz2)
{
   long slen1 = strlen(psz1);
   if (slen1 > sizeof(szCmpBuf1)-10)
       slen1 = sizeof(szCmpBuf1)-10;

   long slen2 = strlen(psz2);
   if (slen2 > sizeof(szCmpBuf2)-10)
       slen2 = sizeof(szCmpBuf2)-10;

   for (long i1=0; i1<slen1; i1++)
      szCmpBuf1[i1] = tolower(psz1[i1]);
   szCmpBuf1[slen1] = '\0';

   for (long i2=0; i2<slen2; i2++)
      szCmpBuf2[i2] = tolower(psz2[i2]);
   szCmpBuf2[slen2] = '\0';

   return (strstr(szCmpBuf1, szCmpBuf2) != 0) ? 1 : 0;
}

void Browser::destroy()
{
   SetWindowLong(clWin, GWL_USERDATA, 0);
   DestroyWindow(clWin);
   clWin = 0;
}

void Browser::activate() {
   // SetFocus(clWin);
   SetActiveWindow(clWin);
}

void Browser::config(bool bsnap, bool bclust, char *pszPre)
{
   bClStdSnapFile = bsnap;
   bClStdCluster  = bclust;
   strncpy(szClSnapPrefix, pszPre, MAX_SNAPPRE_LEN);
   szClSnapPrefix[MAX_SNAPPRE_LEN] = '\0';
}

void Browser::copyConfig(Browser *pSrc)
{
   bClStdSnapFile = pSrc->bClStdSnapFile;
   bClStdCluster  = pSrc->bClStdCluster;
   strncpy(szClSnapPrefix, pSrc->szClSnapPrefix, MAX_SNAPPRE_LEN);
   szClSnapPrefix[MAX_SNAPPRE_LEN] = '\0';
   bClCaseSearch  = pSrc->bClCaseSearch;
   nClTabSize     = pSrc->nClTabSize;

   aClTopFix[0]   = pSrc->aClTopFix[0];
   aClTopFix[1]   = pSrc->aClTopFix[1];
   aClTopFix[2]   = pSrc->aClTopFix[2];
   nClTopFixIdx   = pSrc->nClTopFixIdx;
   nClTopFix      = pSrc->nClTopFix;

   bClLocalScope   = pSrc->bClLocalScope;

   strncpy(szClFileMask, pSrc->szClFileMask, MAX_LINE_LEN);
   szClFileMask[MAX_LINE_LEN] = '\0';
   iFileMaskPos    = pSrc->iFileMaskPos;
   bClEditFileMask = false;

   nClMatchingFiles  = pSrc->nClMatchingFiles;
   nClFirstFileMatch = pSrc->nClFirstFileMatch;

   bClClearBottom = 1; // just in case

   if (pSrc->nClMWMode == 2) nClMWMode = 2;
}

void Browser::setStatus(char *psz)
{
   int nLen = strlen(psz);
   if (nLen > MAX_LINE_LEN) nLen = MAX_LINE_LEN;
   strncpy(szClStatusLine, psz, nLen);
   szClStatusLine[nLen] = '\0';
}

void Browser::setMask(char *psz) 
{
   memset(szClMask, 0, sizeof(szClMask));
   strncpy(szClMask, psz, MAX_LINE_LEN);
   iMaskPos = strlen(szClMask);
   // gotoFirstMaskHit();
   setMaskModified();
}

void Browser::setFileMask(char *psz) 
{
   memset(szClFileMask, 0, sizeof(szClFileMask));
   strncpy(szClFileMask, psz, MAX_LINE_LEN);
   iFileMaskPos = strlen(szClFileMask);
   nClMatchingFiles = -1;
   nClFirstFileMatch = -1;
   setMaskModified();
}

void Browser::updateMatchingFiles()
{
   nClMatchingFiles = -1;
   nClFirstFileMatch = -1;

   num nTimeThresSize = 50;   // msec
   num nTimeThres = getCurrentTime()+nTimeThresSize;

   long lMatch =  0;
   long lFile  = -1;

   long nPreLen = strlen(szClSnapPrefix);
   for (ulong i=0; i<nClLines; i++)
   {
      if (!strncmp(apClLines[i], szClSnapPrefix, nPreLen))
         if ((nClLines > 0) && (i < nClLines-1))
            if (matchesFile(apClLines[i+1])) {
               lMatch++;
               if (lFile == -1)
                   lFile = i;
            }

      if (((i&1023)==1023) && (getCurrentTime() >= nTimeThres)) 
      {
         nTimeThres += nTimeThresSize;
         if (GetInputState()) {
            setMaskModified(); // re-run timer
            return; // search flags changed above
         }
      }
   }

   if (lMatch > 0) {
      nClMatchingFiles  = lMatch;
      nClFirstFileMatch = lFile;
   }
}

void Browser::setLine(ulong nLine, char *pszText) {
   // mtklog("Browser %p setLine %u %s",this,nLine,pszText);
   if (nLine >= BROW_MAX) {
      mtklog("ERROR: nLine %u\n", nLine);
      return;
   }
   strncpy(&aText[nLine][0], pszText, BCOL_MAX);
   aText[nLine][BCOL_MAX] = '\0';
}

void Browser::setAttr(ulong nLine, char *pszText) {
   if (nLine >= BROW_MAX) {
      mtklog("ERROR: setAttr: nLine %u\n", nLine);
      return;
   }
   memset(&aAttr[nLine][0], 0, sizeof(aAttr[nLine]));
   strncpy(&aAttr[nLine][0], pszText, BCOL_MAX);
}

int Browser::create(Browser *pLeftIn, ulong nid, ulong x, ulong y, ulong w, ulong h, char **apLineTable, ulong nLines)
{
   pLeft = pLeftIn;
   apGlblBrowsers[nGlblBrowsers++] = this;

   nClID     = nid;
   sprintf(szClTitleBuf, "%u", (nClID+1));

   apClLines = apLineTable;
   nClLines  = nLines;
   nClX      = x;
   nClY      = y;
   nClPixWidth = w;
   nClPixHeight = h;
   sprintf(szIDBuf, "%u", nid);
   clWin = CreateWindow(
      szWindowClass, szClTitleBuf,
      WS_POPUP,
      nClX,nClY,
      nClPixWidth, nClPixHeight,
      NULL, 
      NULL, glblInstance, NULL);
   if (!clWin) return 9;
   SetWindowLong(clWin, GWL_USERDATA, (long)this);
   ShowWindow(clWin, glblCmdShow);

   // consider active on start
   bClActive = 1;

   // the very first WM_PAINT will do an implicite updatePanel()
   update();

   return 0;
}

void Browser::update() {
   // mtklog("Browser %p update", this);
   InvalidateRect(clWin,0,0);
   UpdateWindow(clWin);
}

void Browser::putClipboard(char *pszStr, bool bShowStatus)
{
   if (!OpenClipboard(clWin))
      { MessageBox(0, "clipboard.1", "error", MB_OK); return; }
   
   if (!EmptyClipboard())
      { MessageBox(0, "clipboard.2", "error", MB_OK); return; }

   long nStrLen = strlen(pszStr);

   HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, nStrLen+10);
   if (hMem == NULL)
      { MessageBox(0, "clipboard.3", "error", MB_OK); return; }

   LPTSTR pCopy = (char*)GlobalLock(hMem);
   if (pCopy)
   {
      memcpy(pCopy, pszStr, nStrLen);
      pCopy[nStrLen] = 0;
   }
   GlobalUnlock(hMem);

   HANDLE hData = SetClipboardData(CF_TEXT, hMem);
   if (hData == NULL)
   {
      CloseClipboard();
      MessageBox(0, "clipboard.4", "error", MB_OK);
      return;
   }

   // System is now owner of hMem.

   CloseClipboard();

   if (bShowStatus) {
      sprintf(szClStatusLine, "clip: %.50s", pszStr);
      updatePanel();
      update();
   }
}

long Browser::getClipboard( )
{
   if (!IsClipboardFormatAvailable(CF_TEXT)) 
      return 0;

   if (!OpenClipboard(clWin)) 
      return 0;
 
   HGLOBAL hglb = GetClipboardData(CF_TEXT); 
   if (hglb != NULL)
   {
      char *pMem = (char*)GlobalLock(hglb); 
      if (pMem != NULL) 
      {
         strncpy(szClClipLine, pMem, MAX_LINE_LEN);
         szClClipLine[MAX_LINE_LEN] = '\0';
      }
      GlobalUnlock(hglb);
   }

   CloseClipboard(); 

   return strlen(szClClipLine);
}

char *pszAnim = "\\|/-";
ulong iAnim   = 0;

void Browser::detab(char *pszIn, char *pszOut, ulong lMaxOut)
{
   ulong nInsert=0, iout=0;
   for (int icol=0; (pszIn[icol]!=0) && (iout<lMaxOut-1); icol++)
   {
      char c1 = pszIn[icol];
      if (c1 == '\t') 
      {
         nInsert = nClTabSize - (iout % nClTabSize);
         for (ulong i2=0; i2<nInsert; i2++)
            pszOut[iout++] = ' ';
      }
      else
         pszOut[iout++] = c1;
   }
   pszOut[iout] = '\0';
}

void Browser::updatePanel() 
{
   prepareMask(szClMask);
   ulong ymax = BROW_MAX;
   if (hchar)
      ymax = (nClPixHeight-10-8)/hchar;
   else
      return;

   ulong yrel = 0;
   char aAttr[BCOL_MAX+10];

   if (bClStdSnapFile) 
   {
      pszClCurSubFile = 0;

      // search for previous :file:
      long i1;
      long nLen = strlen(szClSnapPrefix);
      for (i1 = nClTopLine; i1 >= 0; i1--)
         if (!strncmp(apClLines[i1], szClSnapPrefix, nLen))
            break;

      if (i1 >= 0) 
      {
         if (!bClStdCluster)
         {
            // snapfile
            i1++;
            if (i1 < nClLines) 
            {
               pszClCurSubFile = apClLines[i1];
               setLine(0, pszClCurSubFile);
   
               memset(aAttr, ' ', BCOL_MAX);
               aAttr[BCOL_MAX] = '\0';
               setAttr(0, aAttr);
   
               yrel = 1;
            }
         }
         else
         {
            // cluster
            pszClCurSubFile = apClLines[i1]+strlen(szClSnapPrefix);

            setLine(0, pszClCurSubFile);
   
            memset(aAttr, ' ', BCOL_MAX);
            aAttr[BCOL_MAX] = '\0';
            setAttr(0, aAttr);
   
            yrel = 1;
         }
      }
/*
      else {
         pszCurrentFile = pszGlblFile;
         setLine(0, pszCurrentFile);
         memset(aAttr, ' ', BCOL_MAX);
         aAttr[BCOL_MAX] = '\0';
         setAttr(0, aAttr);
         yrel = 1;
      }
*/
   }

   char szBuf[1024];
   memset(szBuf, 0, sizeof(szBuf));
_
   for (; yrel<ymax-2; yrel++) 
   {
      ulong iLine = nClTopLine+yrel;
      if (iLine < nClLines) 
      {
         memset(aAttr, ' ', BCOL_MAX);
         aAttr[BCOL_MAX] = '\0';

         char *pszLine = apClLines[iLine];
         detab(pszLine, szBuf, sizeof(szBuf)-10);
               pszLine = szBuf;

         // colorize selection hits in red
         bool bHit=0;
         int nMaskLen = strlen(szClMask);
         if (nMaskLen) 
         {
            int iHitIndex = 0;
            int iHitLen   = 0;
            matches(pszLine, &iHitIndex, &iHitLen);

            // this returns ABSOLUTE columns. relativize:
            if ((iHitIndex -= nClXOffs) < 0) {
               iHitLen += iHitIndex;
               iHitIndex = 0;
            }
            
            if (iHitLen > 0 && (iHitIndex+iHitLen) < BCOL_MAX)
            {
               memset(&aAttr[iHitIndex], 'x', iHitLen);
               bHit=1;
            }
         }

         // colorize shift+lbutton selections.

         // first RELATIVIZE the absolute columns:
         long nlcol = nClClipLCol - nClXOffs;
         long nrcol = nClClipRCol - nClXOffs;

         if (nClClipHRow == -1)
         {
           if ((nClClipLCol != -1) && (nClClipRCol != -1))
           {
            // single line selection
            if (nClClipLRow == iLine)
            {
               // mtklog("csh.2 nlcol %ld nrcol %ld\n", nlcol, nrcol);
               if (nlcol < 0) nlcol = 0;
               if (   (nlcol > -1) && (nlcol < BCOL_MAX)
                   && (nrcol >  0) && (nrcol < BCOL_MAX)
                   && (nlcol <= nrcol)
                  )
               {
                  long nLen = nrcol-nlcol;
                  if (!nLen) nLen = 1;
                  if (nlcol+nLen < BCOL_MAX) {
                     // mtklog("csh.3 nlcol %ld nlen %ld\n", nlcol, nLen);
                     memset(&aAttr[nlcol], 's', nLen);
                  }
               }
            }
           }
         }
         else  // multi line selection
         if ((iLine >= nClClipLRow) && (iLine <= nClClipHRow))
         {
            memset(aAttr, 's', BCOL_MAX);
         }

         if (nClXOffs < strlen(pszLine))
            setLine(yrel, pszLine+nClXOffs);
         else
            setLine(yrel, "");

         // the aAttr buffer contains RELATIVE columns
         if (nClXOffs < strlen(aAttr))
            setAttr(yrel, aAttr);
         else
            setAttr(yrel, "");

         if (bHit) mtklog("HIT \"%s\"", aAttr);
      } else {
         setLine(yrel, "");
         setAttr(yrel, "");
      }
   }
_
// setLine(ymax-3, "------------------------------------------------------------------------------------------------------------------------------------------");

   long iMaskBase = 7;
   long iFMskBase = 7;

   char aAttrMask[BCOL_MAX+2];

   memset(szBuf, 0, sizeof(szBuf));
   if (bClLocalScope)
      sprintf(szBuf, "path: >%s< - local search over %lu files.", szClFileMask, (nClMatchingFiles!=-1)?nClMatchingFiles:0);
   else
   if (nGlblSubFiles > 0)
      sprintf(szBuf, "global search over %lu files.", nGlblSubFiles);
   else
      sprintf(szBuf, "global search.");

   setLine(ymax-1, szBuf);

   if (bClLocalScope)
      memset(aAttrMask, ' ', BCOL_MAX);
   else
      memset(aAttrMask, 'g', BCOL_MAX);
   aAttrMask[BCOL_MAX] = '\0';

   if (bClEditFileMask)
      aAttrMask[iFileMaskPos+iFMskBase] = 'x';

   setAttr(ymax-1, aAttrMask);

   strcpy(szBuf, "mask: >");
   strncpy(&szBuf[iFMskBase], szClMask, sizeof(szBuf)-10);
   char *pszMWMode = "wlock";
   if (nClMWMode==1) pszMWMode = "wfree";
   if (nClMWMode==2) pszMWMode = "WFREE";
   if (!nClMaskHits) {
      if (bClSearching)
         sprintf(&szBuf[strlen(szBuf)], "< - %c", (bClSearching<2)?'-':'*');
      else
         sprintf(&szBuf[strlen(szBuf)], "< - no hit, %s, %s.",bClCaseSearch?"case":"nocase",pszMWMode);
   } else
      sprintf(&szBuf[strlen(szBuf)], "< - %u hits, %s, %s.",nClMaskHits,bClCaseSearch?"case":"nocase",pszMWMode);

   // append status, if any.
   if (szClStatusLine[0]) {
      strcat(szBuf, " ");
      strncat(szBuf, szClStatusLine, sizeof(szBuf)-10-strlen(szBuf));
      szClStatusLine[0] = '\0';
   }
_
   memset(aAttrMask, ' ', BCOL_MAX);

   ulong nAbsFocLine = nClTopLine+nClTopFix;
_
   // append statistics in grey, if in fullscreen.
   ulong iOffs = strlen(szBuf);
   if ((nClPixWidth == nGlblFullWidth) && (iOffs < sizeof(szBuf)-100)) {
      sprintf(&szBuf[strlen(szBuf)], " tab=%lu, ", (ulong)nClTabSize);
      sprintf(&szBuf[strlen(szBuf)], "line %lu", nAbsFocLine); // (ulong)nClLines);
      if (nClSearchTime > 0)
         sprintf(&szBuf[strlen(szBuf)], ", %lu msec.", (ulong)nClSearchTime);
      else
         strcat(szBuf, ".");
      if (nClWheel != 0)
         sprintf(&szBuf[strlen(szBuf)], " mw %ld ",(long)nClWheel);
      sprintf(&szBuf[strlen(szBuf)], "%s%s",bClShift?"S":"",bClCtrl?"C":"");
      ulong nGrey = strlen(&szBuf[iOffs]);
      if (iOffs+nGrey < sizeof(szBuf)-10)
         memset(&aAttrMask[iOffs], 'g', nGrey);
   }

   long iInfo = strlen(szBuf);
   if (wchar)
   {
      long xmax = nClPixWidth/wchar;
      if (iInfo < (xmax - (long)strlen(INFO_STR) - 3))
      {
         // mtklog("iinfo %d xmax %d sl %d",iInfo,xmax,strlen(INFO_STR));
         for (long i=iInfo; (i<xmax) && (i<BCOL_MAX);i++)
            szBuf[i] = ' ';
         iInfo = xmax-strlen(INFO_STR)-3;
         strncpy(&szBuf[iInfo], INFO_STR, strlen(INFO_STR));
      }
   }
_
   setLine(ymax-2, szBuf);
_
   // mark the position of mask input cursor
   aAttrMask[BCOL_MAX] = '\0';
   if (!bClEditFileMask)
      aAttrMask[iMaskPos+iFMskBase] = 'x';

   // add grey status infos
   long nInfoLen = strlen(INFO_STR);
   for (long n=0; (n<nInfoLen) && (iInfo+n<BCOL_MAX); n++)
      aAttrMask[iInfo+n] = 'g';

   setAttr(ymax-2, aAttrMask);
}

// Forward declarations of functions included in this code module:
ATOM           MyRegisterClass(HINSTANCE hInstance);
BOOL           InitInstance(HINSTANCE, int);
LRESULT CALLBACK  WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK  About(HWND, UINT, WPARAM, LPARAM);

const char *msgAsString(long nMsg) 
{
   switch (nMsg) {
   case WM_NULL: return "WM_NULL";
   case WM_CREATE: return "WM_CREATE";
   case WM_DESTROY: return "WM_DESTROY";
   case WM_MOVE: return "WM_MOVE";
   case WM_SIZE: return "WM_SIZE";
   case WM_ACTIVATE: return "WM_ACTIVATE";
   case WM_SETFOCUS: return "WM_SETFOCUS";
   case WM_KILLFOCUS: return "WM_KILLFOCUS";
   case WM_ENABLE: return "WM_ENABLE";
   case WM_SETREDRAW: return "WM_SETREDRAW";
   case WM_SETTEXT: return "WM_SETTEXT";
   case WM_GETTEXT: return "WM_GETTEXT";
   case WM_GETTEXTLENGTH: return "WM_GETTEXTLENGTH";
   case WM_PAINT: return "WM_PAINT";
   case WM_CLOSE: return "WM_CLOSE";
   case WM_QUERYENDSESSION: return "WM_QUERYENDSESSION";
   case WM_QUIT: return "WM_QUIT";
   case WM_QUERYOPEN: return "WM_QUERYOPEN";
   case WM_ERASEBKGND: return "WM_ERASEBKGND";
   case WM_SYSCOLORCHANGE: return "WM_SYSCOLORCHANGE";
   case WM_ENDSESSION: return "WM_ENDSESSION";
   case WM_SHOWWINDOW: return "WM_SHOWWINDOW";
   case WM_WININICHANGE: return "WM_WININICHANGE";
// case WM_SETTINGCHANGE: return "WM_SETTINGCHANGE";
   case WM_DEVMODECHANGE: return "WM_DEVMODECHANGE";
   case WM_ACTIVATEAPP: return "WM_ACTIVATEAPP";
   case WM_FONTCHANGE: return "WM_FONTCHANGE";
   case WM_TIMECHANGE: return "WM_TIMECHANGE";
   case WM_CANCELMODE: return "WM_CANCELMODE";
   case WM_SETCURSOR: return "WM_SETCURSOR";
   case WM_MOUSEACTIVATE: return "WM_MOUSEACTIVATE";
   case WM_CHILDACTIVATE: return "WM_CHILDACTIVATE";
   case WM_QUEUESYNC: return "WM_QUEUESYNC";
   case WM_GETMINMAXINFO: return "WM_GETMINMAXINFO";
   case WM_PAINTICON: return "WM_PAINTICON";
   case WM_ICONERASEBKGND: return "WM_ICONERASEBKGND";
   case WM_NEXTDLGCTL: return "WM_NEXTDLGCTL";
   case WM_SPOOLERSTATUS: return "WM_SPOOLERSTATUS";
   case WM_DRAWITEM: return "WM_DRAWITEM";
   case WM_MEASUREITEM: return "WM_MEASUREITEM";
   case WM_DELETEITEM: return "WM_DELETEITEM";
   case WM_VKEYTOITEM: return "WM_VKEYTOITEM";
   case WM_CHARTOITEM: return "WM_CHARTOITEM";
   case WM_SETFONT: return "WM_SETFONT";
   case WM_GETFONT: return "WM_GETFONT";
   case WM_SETHOTKEY: return "WM_SETHOTKEY";
   case WM_GETHOTKEY: return "WM_GETHOTKEY";
   case WM_QUERYDRAGICON: return "WM_QUERYDRAGICON";
   case WM_COMPAREITEM: return "WM_COMPAREITEM";
   case WM_COMPACTING: return "WM_COMPACTING";
   case WM_COMMNOTIFY: return "WM_COMMNOTIFY";
   case WM_WINDOWPOSCHANGING: return "WM_WINDOWPOSCHANGING";
   case WM_WINDOWPOSCHANGED: return "WM_WINDOWPOSCHANGED";
   case WM_POWER: return "WM_POWER";
   case WM_COPYDATA: return "WM_COPYDATA";
   case WM_CANCELJOURNAL: return "WM_CANCELJOURNAL";
   case WM_NOTIFY: return "WM_NOTIFY";
   case WM_INPUTLANGCHANGEREQUEST: return "WM_INPUTLANGCHANGEREQUEST";
   case WM_INPUTLANGCHANGE: return "WM_INPUTLANGCHANGE";
   case WM_TCARD: return "WM_TCARD";
   case WM_HELP: return "WM_HELP";
   case WM_USERCHANGED: return "WM_USERCHANGED";
   case WM_NOTIFYFORMAT: return "WM_NOTIFYFORMAT";
   case WM_CONTEXTMENU: return "WM_CONTEXTMENU";
   case WM_STYLECHANGING: return "WM_STYLECHANGING";
   case WM_STYLECHANGED: return "WM_STYLECHANGED";
   case WM_DISPLAYCHANGE: return "WM_DISPLAYCHANGE";
   case WM_GETICON: return "WM_GETICON";
   case WM_SETICON: return "WM_SETICON";
   case WM_NCCREATE: return "WM_NCCREATE";
   case WM_NCDESTROY: return "WM_NCDESTROY";
   case WM_NCCALCSIZE: return "WM_NCCALCSIZE";
   case WM_NCHITTEST: return "WM_NCHITTEST";
   case WM_NCPAINT: return "WM_NCPAINT";
   case WM_NCACTIVATE: return "WM_NCACTIVATE";
   case WM_GETDLGCODE: return "WM_GETDLGCODE";
   case WM_NCMOUSEMOVE: return "WM_NCMOUSEMOVE";
   case WM_NCLBUTTONDOWN: return "WM_NCLBUTTONDOWN";
   case WM_NCLBUTTONUP: return "WM_NCLBUTTONUP";
   case WM_NCLBUTTONDBLCLK: return "WM_NCLBUTTONDBLCLK";
   case WM_NCRBUTTONDOWN: return "WM_NCRBUTTONDOWN";
   case WM_NCRBUTTONUP: return "WM_NCRBUTTONUP";
   case WM_NCRBUTTONDBLCLK: return "WM_NCRBUTTONDBLCLK";
   case WM_NCMBUTTONDOWN: return "WM_NCMBUTTONDOWN";
   case WM_NCMBUTTONUP: return "WM_NCMBUTTONUP";
   case WM_NCMBUTTONDBLCLK: return "WM_NCMBUTTONDBLCLK";
// case WM_KEYFIRST: return "WM_KEYFIRST";
   case WM_KEYDOWN: return "WM_KEYDOWN";
   case WM_KEYUP: return "WM_KEYUP";
   case WM_CHAR: return "WM_CHAR";
   case WM_DEADCHAR: return "WM_DEADCHAR";
   case WM_SYSKEYDOWN: return "WM_SYSKEYDOWN";
   case WM_SYSKEYUP: return "WM_SYSKEYUP";
   case WM_SYSCHAR: return "WM_SYSCHAR";
   case WM_SYSDEADCHAR: return "WM_SYSDEADCHAR";
   case WM_KEYLAST: return "WM_KEYLAST";
   case WM_IME_STARTCOMPOSITION: return "WM_IME_STARTCOMPOSITION";
   case WM_IME_ENDCOMPOSITION: return "WM_IME_ENDCOMPOSITION";
   case WM_IME_COMPOSITION: return "WM_IME_COMPOSITION";
// case WM_IME_KEYLAST: return "WM_IME_KEYLAST";
   case WM_INITDIALOG: return "WM_INITDIALOG";
   case WM_COMMAND: return "WM_COMMAND";
   case WM_SYSCOMMAND: return "WM_SYSCOMMAND";
   case WM_TIMER: return "WM_TIMER";
   case WM_HSCROLL: return "WM_HSCROLL";
   case WM_VSCROLL: return "WM_VSCROLL";
   case WM_INITMENU: return "WM_INITMENU";
   case WM_INITMENUPOPUP: return "WM_INITMENUPOPUP";
   case WM_MENUSELECT: return "WM_MENUSELECT";
   case WM_MENUCHAR: return "WM_MENUCHAR";
   case WM_ENTERIDLE: return "WM_ENTERIDLE";
   case WM_CTLCOLORMSGBOX: return "WM_CTLCOLORMSGBOX";
   case WM_CTLCOLOREDIT: return "WM_CTLCOLOREDIT";
   case WM_CTLCOLORLISTBOX: return "WM_CTLCOLORLISTBOX";
   case WM_CTLCOLORBTN: return "WM_CTLCOLORBTN";
   case WM_CTLCOLORDLG: return "WM_CTLCOLORDLG";
   case WM_CTLCOLORSCROLLBAR: return "WM_CTLCOLORSCROLLBAR";
   case WM_CTLCOLORSTATIC: return "WM_CTLCOLORSTATIC";
   case WM_MOUSEFIRST: return "WM_MOUSEFIRST";
// case WM_MOUSEMOVE: return "WM_MOUSEMOVE";
   case WM_LBUTTONDOWN: return "WM_LBUTTONDOWN";
   case WM_LBUTTONUP: return "WM_LBUTTONUP";
   case WM_LBUTTONDBLCLK: return "WM_LBUTTONDBLCLK";
   case WM_RBUTTONDOWN: return "WM_RBUTTONDOWN";
   case WM_RBUTTONUP: return "WM_RBUTTONUP";
   case WM_RBUTTONDBLCLK: return "WM_RBUTTONDBLCLK";
   case WM_MBUTTONDOWN: return "WM_MBUTTONDOWN";
   case WM_MBUTTONUP: return "WM_MBUTTONUP";
   case WM_MBUTTONDBLCLK: return "WM_MBUTTONDBLCLK";
   case WM_MOUSEWHEEL: return "WM_MOUSEWHEEL";
// case WM_MOUSELAST: return "WM_MOUSELAST";
// case WM_MOUSELAST: return "WM_MOUSELAST";
   case WM_PARENTNOTIFY: return "WM_PARENTNOTIFY";
   case WM_ENTERMENULOOP: return "WM_ENTERMENULOOP";
   case WM_EXITMENULOOP: return "WM_EXITMENULOOP";
   case WM_NEXTMENU: return "WM_NEXTMENU";
   case WM_SIZING: return "WM_SIZING";
   case WM_CAPTURECHANGED: return "WM_CAPTURECHANGED";
   case WM_MOVING: return "WM_MOVING";
   case WM_POWERBROADCAST: return "WM_POWERBROADCAST";
   case WM_DEVICECHANGE: return "WM_DEVICECHANGE";
   case WM_IME_SETCONTEXT: return "WM_IME_SETCONTEXT";
   case WM_IME_NOTIFY: return "WM_IME_NOTIFY";
   case WM_IME_CONTROL: return "WM_IME_CONTROL";
   case WM_IME_COMPOSITIONFULL: return "WM_IME_COMPOSITIONFULL";
   case WM_IME_SELECT: return "WM_IME_SELECT";
   case WM_IME_CHAR: return "WM_IME_CHAR";
   case WM_IME_KEYDOWN: return "WM_IME_KEYDOWN";
   case WM_IME_KEYUP: return "WM_IME_KEYUP";
   case WM_MDICREATE: return "WM_MDICREATE";
   case WM_MDIDESTROY: return "WM_MDIDESTROY";
   case WM_MDIACTIVATE: return "WM_MDIACTIVATE";
   case WM_MDIRESTORE: return "WM_MDIRESTORE";
   case WM_MDINEXT: return "WM_MDINEXT";
   case WM_MDIMAXIMIZE: return "WM_MDIMAXIMIZE";
   case WM_MDITILE: return "WM_MDITILE";
   case WM_MDICASCADE: return "WM_MDICASCADE";
   case WM_MDIICONARRANGE: return "WM_MDIICONARRANGE";
   case WM_MDIGETACTIVE: return "WM_MDIGETACTIVE";
   case WM_MDISETMENU: return "WM_MDISETMENU";
   case WM_ENTERSIZEMOVE: return "WM_ENTERSIZEMOVE";
   case WM_EXITSIZEMOVE: return "WM_EXITSIZEMOVE";
   case WM_DROPFILES: return "WM_DROPFILES";
   case WM_MDIREFRESHMENU: return "WM_MDIREFRESHMENU";
// case WM_MOUSEHOVER: return "WM_MOUSEHOVER";
// case WM_MOUSELEAVE: return "WM_MOUSELEAVE";
   case WM_CUT: return "WM_CUT";
   case WM_COPY: return "WM_COPY";
   case WM_PASTE: return "WM_PASTE";
   case WM_CLEAR: return "WM_CLEAR";
   case WM_UNDO: return "WM_UNDO";
   case WM_RENDERFORMAT: return "WM_RENDERFORMAT";
   case WM_RENDERALLFORMATS: return "WM_RENDERALLFORMATS";
   case WM_DESTROYCLIPBOARD: return "WM_DESTROYCLIPBOARD";
   case WM_DRAWCLIPBOARD: return "WM_DRAWCLIPBOARD";
   case WM_PAINTCLIPBOARD: return "WM_PAINTCLIPBOARD";
   case WM_VSCROLLCLIPBOARD: return "WM_VSCROLLCLIPBOARD";
   case WM_SIZECLIPBOARD: return "WM_SIZECLIPBOARD";
   case WM_ASKCBFORMATNAME: return "WM_ASKCBFORMATNAME";
   case WM_CHANGECBCHAIN: return "WM_CHANGECBCHAIN";
   case WM_HSCROLLCLIPBOARD: return "WM_HSCROLLCLIPBOARD";
   case WM_QUERYNEWPALETTE: return "WM_QUERYNEWPALETTE";
   case WM_PALETTEISCHANGING: return "WM_PALETTEISCHANGING";
   case WM_PALETTECHANGED: return "WM_PALETTECHANGED";
   case WM_HOTKEY: return "WM_HOTKEY";
   case WM_PRINT: return "WM_PRINT";
   case WM_PRINTCLIENT: return "WM_PRINTCLIENT";
   case WM_HANDHELDFIRST: return "WM_HANDHELDFIRST";
   case WM_HANDHELDLAST: return "WM_HANDHELDLAST";
   case WM_AFXFIRST: return "WM_AFXFIRST";
   case WM_AFXLAST: return "WM_AFXLAST";
   case WM_PENWINFIRST: return "WM_PENWINFIRST";
   case WM_PENWINLAST: return "WM_PENWINLAST";
   case WM_APP: return "WM_APP";
   case WM_USER: return "WM_USER";
   }
   return "?";
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
   WNDCLASSEX wcex;

   wcex.cbSize = sizeof(WNDCLASSEX); 

   wcex.style        = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc  = (WNDPROC)WndProc;
   wcex.cbClsExtra   = 0;
   wcex.cbWndExtra   = 16;
   wcex.hInstance    = hInstance;
   wcex.hIcon        = 0; // LoadIcon(hInstance, (LPCTSTR)IDI_XV);
   wcex.hCursor      = LoadCursor(NULL, IDC_ARROW);
   wcex.hbrBackground= (HBRUSH)(COLOR_WINDOW+1);
   wcex.lpszMenuName = 0;
   wcex.lpszClassName= szWindowClass;
   wcex.hIconSm      = 0; // LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

   return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
   hInst = hInstance; // Store instance handle in our global variable
   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   Browser *pBrowser = (Browser *)GetWindowLong(hWnd, GWL_USERDATA);

   // mtklog("wndproc %02x %s %p\n", message, msgAsString(message), pBrowser);
   if (pBrowser != 0)
   {
      long lRC = pBrowser->process(hWnd, message, wParam, lParam);
      return lRC;
   }
   else
      return DefWindowProc(hWnd, message, wParam, lParam);
}

static char *pszBlankBuf =
   "                                                  "
   "                                                  "
   "                                                  "
   "                                                  "
   ;

void Browser::drawBorder(HDC hdc, RECT rt, ulong CBrTop, ulong CBrLeft, ulong CDark1, ulong CDark2, bool bActive)
{
   ulong CWhite = 0xFFFFFFUL;

   ulong CBrTop2  = bActive ? CBrTop  : CWhite;
   ulong CBrLeft2 = bActive ? CBrLeft : CWhite;
   ulong CDark12  = bActive ? CDark1  : CWhite;
   ulong CDark22  = bActive ? CDark2  : CWhite;

   // ulong n1=6, n2=4;
   ulong n1=5, n2=3;

   for (long x1=0; x1<rt.right; x1++) 
   {
      if (x1 > 20 && x1 < rt.right-20)
         continue;

      for (long d=0; d<n1; d++) 
      {
         if (x1 >= d && x1 <= rt.right-d) {
            SetPixel(hdc, x1, d, (d<n2)?CBrTop:CBrTop2);
            SetPixel(hdc, x1, rt.bottom-d, (d<n2)?CDark2:CDark22);
         }
      }
   }
   fillRect(hdc, 20,  0, rt.right-20, n2, CBrTop );
   fillRect(hdc, 20, n2, rt.right-20, n1, CBrTop2);
   fillRect(hdc, 20, rt.bottom-n2+1, rt.right-20, rt.bottom   +0, CDark2 );
   fillRect(hdc, 20, rt.bottom-n1+1, rt.right-20, rt.bottom-n2+1, CDark22);

   for (long y1=0; y1<rt.bottom; y1++)
   {
      if (y1 > 20 && y1 < rt.bottom-20)
         continue;

      for (long d=0; d<n1; d++) 
      {
         if (y1 >= d && y1 < rt.bottom-d) {
            SetPixel(hdc, d, y1, (d<n2)?CBrLeft:CBrLeft2);
            SetPixel(hdc, rt.right-d, y1, (d<n2)?CDark1:CDark12);
         }
      }
   }
   fillRect(hdc,  0, 20, n2, rt.bottom-20, CBrLeft);
   fillRect(hdc, n2, 20, n1, rt.bottom-20, CBrLeft2);
   fillRect(hdc, rt.right-n2+1, 20, rt.right     , rt.bottom-20, CDark1 );
   fillRect(hdc, rt.right-n1+1, 20, rt.right-n2+1, rt.bottom-20, CDark12);
}

LRESULT Browser::process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   int wmId, wmEvent;
   PAINTSTRUCT ps;
   HDC hdc;
   TCHAR *szHello = "hellou";

   mtklog("Browser %p proc %02x %s w %x l %x act %d", this, message, msgAsString(message), (ulong)wParam, (ulong)lParam, bClActive);

   ulong nbasex  = 10;
   ulong nbasey  =  8;

   switch (message) 
   {
      case WM_COMMAND:
      {
         wmId    = LOWORD(wParam); 
         wmEvent = HIWORD(wParam); 
         // Parse the menu selections:
         switch (wmId)
         {
            case IDM_EXIT:
               DestroyWindow(hWnd);
               break;
            default:
               return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
         break;

      case WM_PAINT:
      {
_
         hdc = BeginPaint(hWnd, &ps);
         RECT rt;
         GetClientRect(hWnd, &rt);
         HFONT hfOld = (HFONT)SelectObject(hdc, glblFont);

         // DrawText(hdc, szHello, strlen(szHello), &rt, DT_CENTER);

         ulong CMiddle, CBrTop, CBrLeft, CDark1, CDark2;

/*
         if (bClActive)
         {
            CMiddle = 0xFFFFFFUL;
            CBrTop  = 0xBB9999UL;
            CBrLeft = 0xDDBBBBUL;
            CDark1  = 0x775555UL;
            CDark2  = 0x553333UL;
         }
         else 
*/
         {
            CMiddle = 0xFBFBFBUL;
            CBrTop  = 0xAAAAAAUL;
            CBrLeft = 0xCCCCCCUL;
            CDark1  = 0x666666UL;
            CDark2  = 0x444444UL;
         }

         // SetBkColor(hdc, CMiddle);

         {
            drawBorder(hdc,rt,CBrTop,CBrLeft,CDark1,CDark2,bClActive);
_
            SetBkMode(hdc, OPAQUE); // TRANSPARENT);
_
            if (!wchar)
            {
               TEXTMETRIC tm;
               GetTextMetrics(hdc, &tm);
               wchar = tm.tmAveCharWidth;
               hchar = tm.tmHeight;
               mtklog("hchar %u", hchar);
               if (hchar) {
                  aClTopFix[0] = 3;
                  aClTopFix[1] = ((nClPixHeight/hchar)-2)*1/2;
                  aClTopFix[2] = ((nClPixHeight/hchar)-2)-5;
                  nClTopFix    = aClTopFix[nClTopFixIdx];
               }
               // on very first WM_PAINT, create text buffer implicitely
               if (wchar)
                  updatePanel();
            }

            // mtklog("rt top %d bot %d basey %d wc %d hc %d",rt.top,rt.bottom,nbasey,wchar,hchar);
_
            ulong nMaxCol = (rt.right-rt.left-nbasex-8)/wchar;
            ulong nMaxRow = (rt.bottom-rt.top-nbasey-8)/hchar;

            if (nMaxCol > BCOL_MAX-1) nMaxCol = BCOL_MAX-1;
            if (nMaxRow > BROW_MAX-1) nMaxRow = BROW_MAX-1;

            ulong nColText = 0x000000UL;
            if (!bClActive)
                  nColText = 0x000000UL;

            RECT rt2;

            if (bClClearBottom)
            {
               bClClearBottom = 0;
               fillRect(hdc,
                     nbasex, nbasey + hchar * (nMaxRow-2),
                     rt.right - nbasex, rt.bottom - 4,
                     0xFFFFFFUL);
            }

            char *pszHelp = (nClHelpMode==2) ? pszGlblHelp2 : pszGlblHelp;

            for (ulong y=0; y<nMaxRow; y++)
            {
             if (y == nMaxRow-2)
             {
               // init separate bottom area
               int ypix = nbasey + y * hchar + 2;
               nbasey += 4;
               fillRect(hdc, 5,ypix,rt.right-5,ypix+1, CBrTop);
               // for (long x1=5; x1<rt.right-5; x1++)
               //   SetPixel(hdc, x1, ypix, CBrTop);
               // if in GLOBAL mode, hide the lowest line
               if (!bClLocalScope)
                 nbasey += hchar/2;
             }
             else
             if (y == nMaxRow-1)
             {
               if (!bClLocalScope)
                  break;
             }

             for (ulong x=0; x<nMaxCol; x++)
             {
               char c = aText[y][x];
               if (c)
               {
                  rt2.left   = nbasex + x * wchar;
                  rt2.top    = nbasey + y * hchar;
                  rt2.right  = rt2.left + wchar*2;
                  rt2.bottom = rt2.top + hchar*2;

                  ulong nFlags = DT_SINGLELINE|DT_TOP|DT_LEFT|DT_NOPREFIX;
  
                  if ((y==0) && pszClCurSubFile) {
                     SetTextColor(hdc, 0xBB0000);
                     // long ypix = nbasey + y * hchar + hchar;
                     // for (long x1=5; x1<rt.right-5; x1++)
                     //    SetPixel(hdc, x1, ypix, CBrTop);
                  }
                  else
                  switch (aAttr[y][x]) {
                     case 'x': SetTextColor(hdc, 0x0000AA); break;
                     case 'g': SetTextColor(hdc, 0xAAAAAA); break;
                     case 's': SetTextColor(hdc, 0xFF0000); break;
                     default : SetTextColor(hdc, nColText); break;
                  }
   
                  DrawText(hdc,
                     &c, 1,
                     &rt2, nFlags
                     );
               }
               else
               if (x < nMaxCol)
               {
                  // draw blank rest of line, maybe including help
                  rt2.left   = nbasex + x * wchar;
                  rt2.top    = nbasey + y * hchar;
                  rt2.right  = rt.right;
                  rt2.bottom = rt2.top + hchar*2;

                  SetTextColor(hdc, 0x555555);

                  long nrlen = nMaxCol-x;
                  memset(szClTextBuf, ' ', nrlen);
                  szClTextBuf[nrlen] = '\0';

                  if (nClHelpMode && (nGlblFullWidth==nClPixWidth) && pszHelp)
                  {
                     char *psz2 = strchr(pszHelp, '\n');
                     long nhlen = 0;
                     if (psz2) { nhlen = psz2-pszHelp; psz2++; }
                         else  nhlen = strlen(pszHelp);
                     if (nhlen < nrlen) {
                        strncpy(&szClTextBuf[nrlen-nhlen], pszHelp, nhlen);
                     }
                     pszHelp = psz2; // can be NULL
                  }

                  DrawText(hdc,
                     szClTextBuf, nrlen,
                     &rt2, DT_SINGLELINE|DT_TOP|DT_LEFT|DT_NOPREFIX
                     );

                  break; // continue at next line                 
               }
               else
                  break;
             } // endfor x

             #ifdef WITH_GREY_LINES
             long nt2y = nbasey + y * hchar;
             for (long x1=5; x1<rt.right-5; x1++)
                SetPixel(hdc, x1, nt2y, 0xCCCCCCUL);
             #endif

            }  // endfor y
         }

         SelectObject(hdc, hfOld);
         EndPaint(hWnd, &ps);
      }
         break;

      case WM_DESTROY:
      {
         PostQuitMessage(0);
      }
         break;

      case WM_SETCURSOR:
      {
         UINT nHittest = LOWORD(lParam);  // hit-test code 
         UINT wMouseMsg = HIWORD(lParam); // mouse-message identifier 
         // mtklog(" nhit %x wmm %x", nHittest, wMouseMsg);
         return DefWindowProc(hWnd, message, wParam, lParam);
      }

      case WM_LBUTTONDOWN:
      {
         UINT xPos = LOWORD(lParam);  // horizontal position of cursor 
         UINT yPos = HIWORD(lParam);  // vertical position of cursor 
         UINT fwKeys = wParam;        // key flags 
         mtklog("LBUTTON %u %u %x",xPos,yPos,fwKeys);

         ulong xrel = xPos-nbasex;  // UNDERFLOW possible
         ulong yrel = yPos-nbasey;  // UNDERFLOW possible

         ulong nrow = hchar ? (yrel/hchar) : 0;
         ulong ncol = wchar ? (xrel/wchar) : 0;
         ulong ymax = BROW_MAX;
         if (hchar) ymax = nClPixHeight/hchar;
_
         if (bClCtrl || (fwKeys & 8) != 0)  // 4 == SHIFT, 8 == CTRL
         {
            if ((nrow==0) && (pszClCurSubFile!=0) && (strlen(pszClCurSubFile)>1))
               strippedLineToClip(pszClCurSubFile);
            else 
            if (nrow >= 1 && nrow < ymax-2)
            {
               char *pszLine = &aText[nrow][0];
               if (strlen(pszLine) > 1)
                  strippedLineToClip(pszLine);
            }
         }
         else
         if (bClShift || (fwKeys & 4) != 0)  // 4 == SHIFT, 8 == CTRL
         {
            /*
            if (nrow >= 1 && nrow < ymax-2)
            {
               // auto-select a path mask: strip filename
               char szBuf[1024];
               char *pszPath = &aText[nrow][0];
               strncpy(szBuf, pszPath, sizeof(szBuf)-10);
               szBuf[sizeof(szBuf)-10] = '\0';
               pszPath = szBuf;
               char *pszRear = strrchr(pszPath, '\\');
               if (pszRear) {
                  *pszRear = '\0';
                  // then set path of file as default
                  if (strlen(pszPath) > 0) {
                     // bClEditFileMask = 1;
                     bClLocalScope = 1;
                     setFileMask(pszPath);
                  }
               }
            }
            */
            if (nrow == 0) {
               // SHIFT+click into top line: copy filename
               if (strlen(aText[0]))
                  putClipboard(aText[0], true);
            }
            else
            if (nrow >= ymax-3) {
               // SHIFT+click into bottom area:
               /*
               nrow = hchar ? ((yrel+6)/hchar) : 0;
               if (bClLocalScope && nrow == ymax-3) {
                  if (strlen(szClMask))
                     putClipboard(szClMask, true);
               }
               else
               if (bClLocalScope && nrow == ymax-2) {
                  if (strlen(szClFileMask))
                     putClipboard(szClFileMask, true);
               }
               else
               */
               putClipboard(szClMask, true);
            }
            else {
               // SHIFT+click into main area:
               clipSelectHit(nClTopLine+nrow, nClXOffs+ncol);
            }
         }
         else
         if (nrow >= 1 && nrow < ymax-2)
         {
_           autoSelect(&aText[nrow][0], ncol);
            if (strlen(szClSel) > 0) {
               setMask(szClSel);
            }
         }
_
         // return DefWindowProc(hWnd, message, wParam, lParam);
      }
         break;

      case WM_MBUTTONDOWN:
      {
         if (bClCtrl) {
            if (nClMWMode==0) nClMWMode=2; // WFREE (permanent)
            else
            if (nClMWMode==1) nClMWMode=2;
            else
            if (nClMWMode==2) {
               if (nClMaskHits > 1) nClMWMode=0;
               else                 nClMWMode=1;
            }
         } else {
            if (nClMWMode==0) nClMWMode=1; // wfree (temporary)
            else
            if (nClMWMode==1) nClMWMode=0; // wlock (temporary)
            else
            if (nClMWMode==2) nClMWMode=0; // wlock (temporary)
         }
         updatePanel();
         update();
      }
         break;

      case WM_RBUTTONDOWN:
      {
         UINT xPos = LOWORD(lParam);  // horizontal position of cursor 
         UINT yPos = HIWORD(lParam);  // vertical position of cursor 
         mtklog("RBUTTON %u %u",xPos,yPos);

         ulong xrel = xPos-nbasex;
         ulong yrel = yPos-nbasey;

         ulong nrow = hchar ? (yrel/hchar) : 0;
         ulong ncol = wchar ? (xrel/wchar) : 0;

         autoSelect(aText[nrow], ncol);

         {
            // right-click leading to autoselect:

            // deactivate fullscreen, if active
            if (nClPixWidth == nGlblFullWidth)
               toggleWidth();
   
            if (!pRight)
            {
               pRight = new Browser();
               HWND hDesk = GetDesktopWindow();
               RECT rDesk;
               GetWindowRect(hDesk, &rDesk);
               long nDeskWidth  = rDesk.right-rDesk.left;
               long nDeskHeight = rDesk.bottom-rDesk.top;
               ulong nXNew = nClX+nClPixWidth;
               ulong nYNew = nClY;
               if (nXNew > (nDeskWidth - nClPixWidth)) {
                  nXNew  = nGlblBaseX;
                  nYNew += 20;
               }
               if (pRight->create(this, nClID+1, nXNew, nYNew, nGlblThirdWidth, nGlblColHeight, apClLines, nClLines)) {
                  mtklog("ERROR: cannot create child browser");
                  delete pRight;
                  pRight = 0;
               }
            }
   
            if (pRight)
            {
               pRight->copyConfig(this);

               if (strlen(szClSel) > 0)
               {
                  pRight->setMask(szClSel);
               }
               else
               {
                  pRight->updatePanel();
                  pRight->update();
               }

               pRight->activate();
            }
         }

         return DefWindowProc(hWnd, message, wParam, lParam);
      }

      case WM_KEYUP:
      {
         UINT nKey = (UINT)wParam;
         switch (nKey)
         {   
            case 0x10:
               bClShift = 0;
               mtklog("SHIFT UP");
               if (nClClipLRow != -1)
               {
                  // end clip selection
                  clipSelCopy();
                  clipSetStatus();
                  nClClipLRow = -1;
                  nClClipHRow = -1;
                  nClClipLCol = -1;
                  nClClipRCol = -1;
                  updatePanel();
                  update();
               } else {
                  #ifdef SHOW_SHIFT_CTRL
                  updatePanel();
                  update();
                  #endif
               }
               break;

            case 0x11:
               bClCtrl  = 0;
               mtklog("CTRL UP");
               #ifdef SHOW_SHIFT_CTRL
               updatePanel();
               update();
               #endif
               break;
         }
      }
         break;

      case WM_KEYDOWN:
      {
         UINT nKey = (UINT)wParam;
         ulong nmax = BROW_MAX;
         if (hchar) nmax = nClPixHeight/hchar;
         ulong nOldTop = nClTopLine;
         switch (nKey) 
         {
            case 0x10:
               bClShift = 1;
               mtklog("SHIFT DOWN");
               #ifdef SHOW_SHIFT_CTRL
               updatePanel();
               update();
               #endif
               break;

            case 0x11:
               bClCtrl  = 1;
               mtklog("CTRL DOWN");
               #ifdef SHOW_SHIFT_CTRL
               updatePanel();
               update();
               #endif
               break;

            case 0x12:
               // windows bug: sends CTRL DOWN if you press right Alt Key!
               // but it also sends 0x12 afterwards.
               bClCtrl  = 0;
               mtklog("CTRL UP FIX");
               #ifdef SHOW_SHIFT_CTRL
               updatePanel();
               update();
               #endif
               break;

            case 33: // page up
               if (nClTopLine >= nmax)
                  nClTopLine -= nmax;
               else
                  nClTopLine = 0;
               break;

            case 34: // page down
               if ((nClTopLine+nmax*2) < nClLines+nClBotLines)
                  nClTopLine += nmax;
               else
               if (nClLines+nClBotLines >= nmax)
                  nClTopLine = nClLines+nClBotLines-nmax;
               break;

            case 38: // crsr up
               if (strlen(szClMask) && bClShift)
                  goPreviousMaskHit();
               else
               if (bClShift && (nClTopLine >= 5))
                  nClTopLine -= 5;
               else
               if (nClTopLine > 0)
                  nClTopLine--;
               break;

            case 40: // crsr down
               if (strlen(szClMask) && bClShift)
                  goNextMaskHit();
               else
               if (bClShift && ((nClTopLine+nmax+5) < nClLines+nClBotLines))
                  nClTopLine += 5;
               else
               if ((nClTopLine+nmax) < nClLines+nClBotLines)
                  nClTopLine++;
               break;

            case 36: // home, pos1
               if (bClCtrl) {
                  if (bClShift)
                     nClTopLine = 0;
                  else {
                     if (bClStdSnapFile)
                        goLocalFileStart();
                     else
                        nClTopLine = 0;
                  }
               }
               else
               if (bClEditFileMask) {
                  if (iFileMaskPos > 0) {
                     iFileMaskPos=0;
                     updatePanel();
                     update();
                  }
               } else {
                  if (iMaskPos > 0) {
                     iMaskPos=0;
                     updatePanel();
                     update();
                  }
               }
               break;

            case 35: // end
               if (bClCtrl) {
                  if (bClShift || !bClStdSnapFile) {
                     ulong ymax = BROW_MAX;
                     if (hchar) ymax = nClPixHeight/hchar;
                     if (nClLines+nClBotLines > ymax)
                        nClTopLine = nClLines+nClBotLines-ymax;
                  } else {
                     goLocalNextFile();
                  }
               }
               else
               if (bClEditFileMask) {
                  int nLen = strlen(szClFileMask);  
                  if (iFileMaskPos < nLen) {
                     iFileMaskPos = nLen;
                     updatePanel();
                     update();
                  }
               } else {
                  int nLen = strlen(szClMask);  
                  if (iMaskPos < nLen) {
                     iMaskPos = nLen;
                     updatePanel();
                     update();
                  }
               }
               break;

            case VK_ESCAPE: // esc
               if (!pLeft)
                  PostQuitMessage(0);
               else
               if (!pRight)
               {
                  // call parent to close ourselves
                  pLeft->closeRight();
                  // 'this' is now INVALID!
                  return 0;
                  // NO further message processing done
               }
               else {
                  sprintf(szClStatusLine, "select first view to exit.");
                  updatePanel();
                  update();
               }
               break;

            case VK_LEFT:
               if (bClShift) {
                  if (nClXOffs >= 10)
                      nClXOffs -= 10;
                  else
                      nClXOffs = 0;
                  updatePanel();
                  update();
               }
               else
               if (bClEditFileMask) {
                  if (iFileMaskPos > 0) {
                     iFileMaskPos--;
                     updatePanel();
                     update();
                  }
               } else {
                  if (iMaskPos > 0) {
                     iMaskPos--;
                     updatePanel();
                     update();
                  }
               }
               break;

            case VK_RIGHT:
               if (bClShift) {
                  nClXOffs += 10;
                  updatePanel();
                  update();
               }
               else 
               if (bClEditFileMask) {
                   int nLen = strlen(szClFileMask);  
                   if (iFileMaskPos < nLen) {
                     iFileMaskPos++;
                     updatePanel();
                     update();
                   }
               } else {
                   int nLen = strlen(szClMask);  
                   if (iMaskPos < nLen) {
                     iMaskPos++;
                     updatePanel();
                     update();
                   }
               }
               break;

            case VK_RETURN:
               nClXOffs = 0;
               updatePanel();
               update();
               break;

            case VK_DELETE:
            {
               modifySearchMask(nKey+1000);
            }
               break;

            case VK_F1:
               toggleWidth();
               break;

            case VK_F8:
               nClHelpMode = (nClHelpMode+1)%3;
               update();
               break;

            case VK_INSERT:
               if (bClShift) {
                  if (getClipboard() > 0)
                     if (bClEditFileMask)
                        setFileMask(szClClipLine);
                     else
                        setMask(szClClipLine);
               }
               else
               if (bClCtrl) {
                  copyScreenToClipboard();
               }
               else
               if (bClEditFileMask)
                  setFileMask("");
               else
                  setMask("");
               break;

            case VK_F2:
            {
               // move focus line up
               long nOldFocus = (long)(nClTopLine + nClTopFix);

               if (nClTopFixIdx > 0)
                  nClTopFixIdx--;
               nClTopFix = aClTopFix[nClTopFixIdx];

               long nNewFocus = (long)(nClTopLine + nClTopFix);
               long lDiff     = nNewFocus-nOldFocus;
               long lNewTop   = ((long)nClTopLine) - lDiff;
               if (lNewTop < 0)
                   nClTopLine = 0;
               else
               if (lNewTop < nClLines-5)
                   nClTopLine = lNewTop;

               updatePanel();
               update();
            }
               break;
               

            case VK_F3:
            {
               // move focus line down
               long nOldFocus = (long)(nClTopLine + nClTopFix);

               if (nClTopFixIdx < 2)
                  nClTopFixIdx++;
               nClTopFix = aClTopFix[nClTopFixIdx];

               long nNewFocus = (long)(nClTopLine + nClTopFix);
               long lDiff     = nNewFocus-nOldFocus;
               long lNewTop   = ((long)nClTopLine) - lDiff;
               if (lNewTop < 0)
                   nClTopLine = 0;
               else
               if (lNewTop < nClLines-5)
                   nClTopLine = lNewTop;

               updatePanel();
               update();
            }
               break;

            case VK_F4:
               bClCaseSearch ^= 0x1;
               setMaskModified();
               break;

            case VK_F7:
               extendMaskByClass();
               break;

            case VK_F5:
               bClLocalScope ^= 0x1;
               if (bClLocalScope)
                  bClEditFileMask = 1;
               else
                  bClEditFileMask = 0;
               bClClearBottom = 1;
               setMaskModified();
               break;

            case VK_F6:
            {
               if (bGlblMinimized) {
                  for (ulong i=0; i<nGlblBrowsers; i++)
                     ShowWindow(apGlblBrowsers[i]->clWin, SW_RESTORE);
                  bGlblMinimized = 0;
               } else { 
                  for (ulong i=0; i<nGlblBrowsers; i++)
                     ShowWindow(apGlblBrowsers[i]->clWin, SW_MINIMIZE);
                  bGlblMinimized = 1;
               }
            }
               break;

            case VK_TAB:
               if (bClCtrl) {
                  // change current tab size
                  switch (nClTabSize) {
                     case 2: nClTabSize=3; break;
                     case 3: nClTabSize=4; break;
                     case 4: nClTabSize=8; break;
                     case 8: nClTabSize=2; break;
                  }
               } else {
                  if (bClLocalScope) {
                     // switch between search and file mask
                     bClEditFileMask ^= 0x1;
                  }
               }
               updatePanel();
               update();
               break;

            default:
               break;
         }
         if (nClTopLine != nOldTop) {
            mtklog("KEY: %u, new topline %u", nKey, nClTopLine);
            updatePanel();
            update();
         } else {
            mtklog("KEY: %u %x, no change", nKey, nKey);
            // return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
         break;

      case WM_CHAR:
      {
         UINT nChar = (TCHAR)wParam;
         mtklog("WM_CHAR.2 %x", nChar);
         // treat as free text, add to search mask
         if (nChar != VK_TAB)
            modifySearchMask(nChar);
      }
         break;

      case WM_MOUSEWHEEL:
      {        
         short zDelta = (short)HIWORD(wParam);
         // mtklog("WHEEL %d", zDelta);
         short nStep  = WHEEL_DELTA;
         nClWheel += zDelta;
         // NOTE: zDelta can also be MULTIPLES of WHEEL_DELTA,
         //       if user turns wheel rapidly under high load!

         // calc currently displayed top and bottom lines
         long ymax = hchar ? nClPixHeight/hchar : BROW_MAX;
         long nTopLine = nClTopLine;
         long nBotLine = nClTopLine+ymax;
         if (nBotLine >= nClLines-1) nBotLine = nClLines ? nClLines-1:nClLines;

         if (nClWheel >= nStep)
         {
            // mouse wheel up
            if (!nClMWMode || bClShift) {
               // independent from wheel speed, always step just ONE hit:
               nClWheel = 0;
               goPreviousMaskHit();
            } else {
               // in case of hectic wheel turns, we step farther:
               while (nClWheel > 0) {
                  nClWheel -= nStep;
                  if (nClTopLine >= 5)
                     nClTopLine -= 5;
                  else
                     nClTopLine = 0;
               }
               updatePanel();
               update();
            }
         }
         else
         if (nClWheel <= -nStep) 
         {
            // mouse wheel down
            if (!nClMWMode || bClShift) {
               // independent from wheel speed, always step just ONE hit:
               nClWheel = 0;
               goNextMaskHit();
            } else {
               // in case of hectic wheel turns, we step farther:
               while (nClWheel < 0) {
                  nClWheel += nStep;
                  if ((nClTopLine+ymax) < nClLines+nClBotLines-2)
                     nClTopLine += 5;
               }
               updatePanel();
               update();
            }
         }
         return DefWindowProc(hWnd, message, wParam, lParam);
      }
         break;

      case WM_TIMER:
         handleTimer();
         break;

      case WM_ACTIVATE:
      {
         bool bOldActive = bClActive;
         UINT fActive = LOWORD(wParam);

         if (fActive == WA_INACTIVE)
         {
            bClActive = 0;
            if (bOldActive)
               update();
         }
         else
         {
            bClActive = 1;

            SetWindowPos(
               hWnd, HWND_TOP,
               0,0,0,0,
               SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE
               );
   
            if (bGlblMinimized) {
               bGlblMinimized = 0;
               for (ulong i=0; i<nGlblBrowsers; i++) {
                  HWND h2 = apGlblBrowsers[i]->clWin;
                  if (h2 != hWnd)
                     ShowWindow(h2, SW_RESTORE);
               }
            }

            if (!bOldActive)
               update();
         }
      }
         break;

      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}

void Browser::fillRect(HDC hdc,long x1,long y1,long x2,long y2,ulong nCol)
{
   RECT rt2;
   rt2.left   = x1;
   rt2.top    = y1;
   rt2.right  = x2;
   rt2.bottom = y2;
   HBRUSH hBlank = CreateSolidBrush(nCol);
   FillRect(hdc, &rt2, hBlank);
   DeleteObject(hBlank);
}

long fileSize(char *pszFile) {
   struct stat sinfo;
   if (stat(pszFile, &sinfo))
      return -1;
   return sinfo.st_size;
}

char *loadFile(char *pszFile, int nLine) 
{
   long lFileSize = fileSize(pszFile);
   if (lFileSize < 0)
      return 0;
   char *pOut = new char[lFileSize+10];
   // printf("loadFile %p %d\n", pOut, nLine);
   FILE *fin = fopen(pszFile, "rb");
   if (!fin) { fprintf(stderr, "error: cannot read: %s\n", pszFile); return 0; }
   long nRead = fread(pOut, 1, lFileSize, fin);
   fclose(fin);
   if (nRead != lFileSize) {
      fprintf(stderr, "error: cannot read: %s (%d %d)\n", pszFile, nRead, lFileSize);
      delete [] pOut;
      return 0;
   }
   pOut[lFileSize] = '\0';
   return pOut;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
   MSG msg;
   HACCEL hAccelTable;

/*
HFONT CreateFont(
  int nHeight,             // logical height of font
  int nWidth,              // logical average character width

  int nEscapement,         // angle of escapement
  int nOrientation,        // base-line orientation angle
  int fnWeight,            // font weight

  DWORD fdwItalic,         // italic attribute flag
  DWORD fdwUnderline,      // underline attribute flag
  DWORD fdwStrikeOut,      // strikeout attribute flag
  DWORD fdwCharSet,        // character set identifier

  DWORD fdwOutputPrecision,  // output precision
  DWORD fdwClipPrecision,  // clipping precision
  DWORD fdwQuality,        // output quality
  DWORD fdwPitchAndFamily,  // pitch and family

  LPCTSTR lpszFace         // pointer to typeface name string
);
*/

   HWND hDesk = GetDesktopWindow();
   RECT rDesk;
   GetWindowRect(hDesk, &rDesk);
   nGlblDeskWidth  = rDesk.right-rDesk.left;
   nGlblDeskHeight = rDesk.bottom-rDesk.top;

   nGlblBaseX      = 20;
   nGlblBaseY      = 20;
   nGlblFullWidth  = nGlblDeskWidth-nGlblBaseX*2;
   nGlblThirdWidth = nGlblFullWidth/3;
   nGlblColHeight  = (((nGlblDeskHeight-140)/15)+1)*15;
   // fixedsys hchar is 15
   nGlblColHeight += 10+8+2;

   glblFont = CreateFont(
      8, 0,
      0, 0, 0,
      0, 0, 0, 0, 
      0, 0, 0, 0, 
      "Fixedsys");

   // Initialize global strings
   // LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
   // LoadString(hInstance, IDC_XV, szWindowClass, MAX_LOADSTRING);

   MyRegisterClass(hInstance);

   // Perform application initialization:
   glblInstance = hInstance;
   glblCmdShow  = nCmdShow;
   if (!InitInstance (hInstance, nCmdShow)) 
   {
      MessageBox(0, "init#1", "error", MB_OK);
      return FALSE;
   }

   // load text

   if (!lpCmdLine || (strlen(lpCmdLine) < 1))
      { MessageBox(0, "supply a filename for reading:\nsview filename.txt", "error", MB_OK); return FALSE; }

   pszGlblFile = lpCmdLine;
   char *pszText = loadFile(lpCmdLine, __LINE__);
   if (!pszText) { MessageBox(0, "cannot load file", "error", MB_OK); return FALSE; }
   ulong nLines = 0;
   ulong i;
   for (i=0; pszText[i]; i++) {
      if (pszText[i] == '\n')
         nLines++;
   }

   char **apLineTab = new char*[nLines+10];
   memset(apLineTab, 0, sizeof(apLineTab));
   nLines = 0;
   char *psz1 = pszText;
   char *psz2 = psz1;
   while (*psz1)
   {
      if (*psz1 == '\n') {
         apLineTab[nLines++] = psz2;
         *psz1++ = '\0';
         psz2 = psz1;
      }
      else
      if (*psz1 == '\r')
         *psz1++ = ' '; // trick: allow easy search for expressions at line end
      else
         psz1++;
   }

   // check text format by header line
   // :snapfile sfk,1.0.7,lprefix=:file:
   // :cluster sfk,1.0.7,prefix=:
   bool bStdSnapFile = 0;
   bool bCluster = 0;
   char *pszHead = apLineTab[0];
   char szSnapPrefix[MAX_SNAPPRE_LEN+10];
   szSnapPrefix[0] = '\0';
   if (pszHead) 
   {
      char *pszPrefix = strstr(pszHead, ",lprefix=");
      if (pszPrefix) {
         // process snapfile prefix
         pszPrefix += strlen(",lprefix=");
         char *psz2 = pszPrefix;
         while (*psz2 && *psz2 != '\n' && *psz2 != ',')
            psz2++;
         long nLen = psz2 - pszPrefix;
         if (nLen > MAX_SNAPPRE_LEN) nLen = MAX_SNAPPRE_LEN;
         strncpy(szSnapPrefix, pszPrefix, nLen);
         szSnapPrefix[nLen] = '\0';
         bStdSnapFile = 1;
      }
      else
      if (!strncmp(pszHead, ":cluster sfk,", strlen(":cluster sfk,")))
      {
         pszPrefix = strstr(pszHead, ",prefix=");
         if (pszPrefix) {
            pszPrefix += strlen(",prefix=");
            char *psz2 = pszPrefix;
            while (*psz2 && *psz2 != '\n' && *psz2 != ',')
               psz2++;
            long nLen = psz2 - pszPrefix;
            if (nLen > MAX_SNAPPRE_LEN) nLen = MAX_SNAPPRE_LEN;
            strncpy(szSnapPrefix, pszPrefix, nLen);
            szSnapPrefix[nLen] = '\0';
            strcat(szSnapPrefix, "create ");
            bStdSnapFile = 1;
            bCluster = 1;
         }
      }
   }

   // count number of local files contained
   long nPreLen = strlen(szSnapPrefix);
   for (ulong i2=0; i2<nLines; i2++)
      if (!strncmp(apLineTab[i2], szSnapPrefix, nPreLen))
         nGlblSubFiles++;

   Browser b1;
   if (b1.create(0, 0, nGlblBaseX, nGlblBaseY, nGlblFullWidth, nGlblColHeight, apLineTab, nLines))
   {
      MessageBox(0, "unable to create view", "error", MB_OK);
      return FALSE;
   }
   b1.config(bStdSnapFile, bCluster, szSnapPrefix);
   b1.update();

   // hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_XV);
   // ACCEL a[10];
   // hAccelTable = CreateAcceleratorTable(a, 5);

   // Main message loop:
   while (GetMessage(&msg, NULL, 0, 0)) 
   {
      // if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   // cleanup
   delete [] apLineTab;
   delete [] pszText;

   #ifdef WITH_MEM_TRACE
   if (anyMemoryLeaks())
   {
      FILE *fmemlog = fopen("mem.log","w");
      long lRC = listMemoryLeaks(fmemlog);
      fclose(fmemlog);
      if (lRC) MessageBox(0, "memleak detected\nsee mem.log", "error", MB_OK|MB_TOPMOST);
   }
   #endif

   return msg.wParam;
}

void Browser::modifySearchMask(UINT nKey) 
{
   mtklog("modsm key %x", nKey);

   if (bClEditFileMask)
   {
      // edit local file filter mask
      int nLen = strlen(szClFileMask);
      bool bDone = 0;
   
      switch (nKey) 
      {
         case VK_DELETE+1000: // messes up with "."
         {
            // remove at maskpos
            for (int i=iFileMaskPos; i<=nLen; i++)
               szClFileMask[i] = szClFileMask[i+1];
            bDone=1;
         }
            break;
   
         case VK_BACK:
         {
            // remove char at left, if any
            if (iFileMaskPos > 0) {
               iFileMaskPos--;
               for (int i=iFileMaskPos; i<=nLen; i++)
                  szClFileMask[i] = szClFileMask[i+1];
               bDone=1;
            }
         }
            break;
   
         case VK_ESCAPE:
         case VK_RETURN:
            break;
   
         default:
         {
            if (nLen < MAX_LINE_LEN-2) 
            {
               // insert at maskpos
               for (int i=nLen; i>=iFileMaskPos; i--)
                  szClFileMask[i+1] = szClFileMask[i];
               szClFileMask[iFileMaskPos] = (char)nKey;
               iFileMaskPos++;
               bDone=1;
            }
         }
            break;
      }
   
      if (bDone) {
         nClMatchingFiles = -1;
         setMaskModified();
      }
   }
   else
   {
      // edit search mask
      int nLen = strlen(szClMask);
      bool bDone = 0;
   
      switch (nKey) 
      {
         case VK_DELETE+1000: // messes up with "."
         {
            // remove at maskpos
            for (int i=iMaskPos; i<=nLen; i++)
               szClMask[i] = szClMask[i+1];
            bDone=1;
         }
            break;
   
         case VK_BACK:
         {
            // remove char at left, if any
            if (iMaskPos > 0) {
               iMaskPos--;
               for (int i=iMaskPos; i<=nLen; i++)
                  szClMask[i] = szClMask[i+1];
               bDone=1;
            }
         }
            break;
   
         case VK_ESCAPE:
         case VK_RETURN:
            break;
   
         default:
         {
            if (nLen < MAX_LINE_LEN-2) 
            {
               // insert at maskpos
               for (int i=nLen; i>=iMaskPos; i--)
                  szClMask[i+1] = szClMask[i];
               szClMask[iMaskPos] = (char)nKey;
               iMaskPos++;
               bDone=1;
            }
         }
            break;
      }
   
      if (bDone)
         setMaskModified();
   }
}

void Browser::extendMaskByClass() 
{
   if (!strncmp(szClMask, "class ", strlen("class ")))
      return;
   sprintf(szClClassBuf, "class %s ", szClMask);
   strcpy(szClMask, szClClassBuf);
   iMaskPos = strlen("class ");
   setMaskModified();
}

void Browser::setMaskModified() 
{
   if (bClTimerSet)
      KillTimer(clWin, 1);
   SetTimer(clWin, 1, bClCaseSearch ? 600 : 800, 0);
   bClTimerSet = 1;
   // immediately show new (temporary) mask status
   nClMaskHits = 0;
   bClSearching   = 1;  // show "-" status
   nClSearchStart = getCurrentTime();
   nClSearchTime  = 0;
   updatePanel();
   update();
   // also update window title
   sprintf(szClTitleBuf, "%s", szClMask);
   SetWindowText(clWin, szClTitleBuf);
}

void Browser::handleTimer() 
{
   KillTimer(clWin, 1);
   bClTimerSet = 0;
   if (bClStdSnapFile && (nClMatchingFiles == -1))
      updateMatchingFiles();
   gotoFirstMaskHit();
}

#ifdef _WIN32
long getKeyPress()
{
   DWORD dwNumEvents, dwEventsPeeked, dwInputEvents;
   DWORD iEvent, iPrevEvent;
   INPUT_RECORD aInputBuffer[1];

   HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

   if (!GetNumberOfConsoleInputEvents(hStdIn, &dwNumEvents))
      return -1;

   if (dwNumEvents <= 0)
      return -1;

   if (!PeekConsoleInput(hStdIn, aInputBuffer, 1, &dwEventsPeeked))
      return -1;
   
   if (!ReadConsoleInput(hStdIn, &aInputBuffer[0], 1, &dwInputEvents))
      return -1;

   if (dwInputEvents != 1)
      return -1;

   if (aInputBuffer[0].EventType == KEY_EVENT)
      return aInputBuffer[0].Event.KeyEvent.wVirtualKeyCode;

   return -1;
}
#endif

void Browser::gotoFirstMaskHit()
{
   nClSearchStart = getCurrentTime();
   bClSearching   = 2;  // show animated status
   nClMaskHits = 0;
   updatePanel();
   update();

   nClSearchStart = getCurrentTime();

   num nTimeThresSize = 50;   // msec
   num nTimeThres = getCurrentTime()+nTimeThresSize;
   long nPreLen = strlen(szClSnapPrefix);
   char *pszCurFile = "";
   bool bFileMatch = 0;
   long nFileHits = 0;

   prepareMask(szClMask);
   if (strlen(szClMask) > 0)
   for (ulong i=0; i<nClLines; i++)
   {
      if (!strncmp(apClLines[i], szClSnapPrefix, nPreLen))
         if (i<nClLines) {
            pszCurFile = apClLines[i+1];
            bFileMatch = matchesFile(pszCurFile);
            if (bFileMatch)
               nFileHits++;
         }

      if (bFileMatch && matches(apClLines[i]))
      {
         nClMaskHits++;
         if (nClMaskHits == 1) {
            nCurSel = i;
            if (i > nClTopFix)
               nClTopLine = i-nClTopFix;
            else
            if (i > 3)
               nClTopLine = i-3;
            else
               nClTopLine = 0;
         }
         // else continue counting
      }

      if (((i&1023)==1023) && (getCurrentTime() >= nTimeThres)) 
      {
         nTimeThres += nTimeThresSize;
         if (GetInputState()) {
            // user touched keyboard or clicked the mouse.
            // interrupt search in case he wants
            // to change the search mask.
            setMaskModified(); // re-run timer
            return; // search flags changed above
         }
      }
   }

   if ((nClMaskHits == 0) && bClLocalScope) {
      // no mask hits but in local mode: jump to first file
      if (nClFirstFileMatch) {
         long i2 = nClFirstFileMatch;
         if (i2 > nClTopFix)
            nClTopLine = i2-nClTopFix;
         else
         if (i2 > 3)
            nClTopLine = i2-3;
         else
            nClTopLine = 0;
      }
   }

   // auto-switch mouse wheel lock depending on the number
   // of search results: lock only if at least 2 hits.
   if (nClMaskHits > 1) {
      if (nClMWMode==1) nClMWMode=0; // wfree->wlock (temporary)
      // in mode 2, we have permanent wfree, which isn't changed.
   } else {
      if (nClMWMode==0) nClMWMode=1; // wlock->wfree (temporary)
   }

   bClSearching   = 0;
   nClSearchTime  = getCurrentTime()-nClSearchStart;
   nClSearchStart = 0;
   updatePanel();
   update();
}

void Browser::clipSelectHit(ulong nLine, ulong ncol)
{
   mtklog("csh %lu %lu xoffs %d\n", nLine, ncol, nClXOffs);

   // user click-selected another char.
   if (nLine >= nClLines)
      return;

   char *pszLine = apClLines[nLine];
   detab(pszLine, szClClipTabBuf, MAX_LINE_LEN);
   pszLine = szClClipTabBuf;

   // initial set of lower line on first click:
   if (nClClipLRow == -1)
      nClClipLRow = nLine;

   // still in the same line?
   if (nClClipLRow != nLine)
   {
      // no: reset selection columns
      nClClipLCol = -1;
      nClClipRCol = -1;

      // go into, or continue, line selection mode
      nClClipHRow = nLine;
      if (nClClipHRow < nClClipLRow) {
         nClClipHRow = nClClipLRow;
         nClClipLRow = nLine;
      }
   }

   if (nClClipHRow == -1)
   {
      // single line mode: select words within line
      bool bFixLCol = 0;
      bool bFixRCol = 0;
   
      // raw-adapt left and right selection border.
      if (nClClipLCol == -1) {
         nClClipLCol = ncol;
         bFixLCol = 1;
      }
   
      if (nClClipRCol == -1) {
         nClClipRCol = ncol;
         bFixRCol = 1;
      }
   
      if (ncol < nClClipLCol) {
         nClClipLCol = ncol;
         bFixLCol = 1;
      }
   
      if (ncol > nClClipRCol) {
         nClClipRCol = ncol;
         bFixRCol = 1;
      }
   
      // extend borders to end of alnum words.
      if (bFixLCol)
      for (; nClClipLCol > 0; nClClipLCol--) {
         char c = pszLine[nClClipLCol];
         if (isalnum(c) || c=='_')
            continue;
         else {
            if (nClClipLCol < ncol)
               nClClipLCol++;
            break;
         }
      }
   
      if (bFixRCol) {
         for (; pszLine[nClClipRCol]; nClClipRCol++) {
            char c = pszLine[nClClipRCol];
            if (isalnum(c) || c=='_')
               continue;
            else
               break;
         }
         if (nClClipRCol == ncol)
            nClClipRCol++;
      }
   }

   updatePanel();
   update();
}

void Browser::clipSelCopy()
{
   if (nClClipLRow == -1 && nClClipHRow == -1)
      return;

   // copy selected part to clipboard
   if (nClClipHRow == -1)
   {
      // single line mode
      if (nClClipLRow >= nClLines)
         return;

      char *pszLine = apClLines[nClClipLRow];
      detab(pszLine, szClClipTabBuf, MAX_LINE_LEN);
      pszLine = szClClipTabBuf;

      // copy selected words
      long nMaxLen = MAX_LINE_LEN;

      // using the ABSOLUTE columns as they are.
      long nlcol = nClClipLCol;
      long nrcol = nClClipRCol;

      if (   (nlcol > -1) && (nlcol < nMaxLen)
          && (nrcol > -1) && (nrcol < nMaxLen)
          && (nlcol <= nrcol)
         )
      {
         long nLen = nrcol - nlcol;
         if (!nLen) nLen = 1;
         if (nlcol+nLen < nMaxLen) {
            strncpy(szClClipLine, &pszLine[nlcol], nLen);
            szClClipLine[nLen] = '\0';
         }
         putClipboard(szClClipLine);
         clipSetStatus();
      }
   }
   else
   {
      // multi line mode: collect lines
      char *pszTmp = new char[10000];
      if (!pszTmp) return;
      pszTmp[0] = '\0';
      ulong iout=0;
      for (ulong iLine=nClClipLRow; iLine<=nClClipHRow; iLine++)
      {
         if (iLine >= nClLines)
            break;

         char *psz = apClLines[iLine];
         ulong nLen = strlen(psz);
         if (iout+nLen < 10000-100) 
         {
            strcat(pszTmp, psz);
            strcat(pszTmp, "\r\n");
            iout = strlen(pszTmp);
         }
         else
            break;
      }
      putClipboard(pszTmp);
      delete [] pszTmp;
   }
}

void Browser::clipSetStatus()
{
   if (nClClipHRow == -1)
      sprintf(szClStatusLine, "clip: %.50s", szClClipLine);
   else
      sprintf(szClStatusLine, "clip: block copy.");
}

void Browser::autoSelect(char *pszLine, ulong ncol)
{
_  szClSel[0] = '\0';
   ulong nLineLen = strlen(pszLine);
   if (ncol >= nLineLen)
      return;
 
   char *psz1 = pszLine+ncol;
   // search start and end of expression
   char *pszs = psz1;
   char *psze = psz1;
   for (; pszs > pszLine; pszs--) {
      char c = *pszs;
      if (isalnum(c) || c=='_')
         continue;
      else {
         pszs++;
         break;
      }
   }
   for (; *psze; psze++) {
      char c = *psze;
      if (isalnum(c) || c=='_')
         continue;
      else
         break;
   }
   int nLen = psze-pszs;
   if (nLen > 0) {
      // if (nLen > MAX_LINE_LEN) nLen = MAX_LINE_LEN;
      if (nLen > 80) nLen = 80;
      strncpy(szClSel,pszs,nLen);
      szClSel[nLen] = '\0';
   }
   // doesn't change any display yet,
_  // we only set the szClSel field.
}

void Browser::toggleWidth() 
{
   if (nClPixWidth == nGlblThirdWidth) 
   {
      nClPixWidth = nGlblFullWidth;
      SetWindowPos(clWin, HWND_TOP,
         nGlblBaseX,nClY,
         nClPixWidth,nClPixHeight,
         SWP_SHOWWINDOW|SWP_NOACTIVATE
         );
   }
   else 
   {
      nClPixWidth = nGlblThirdWidth;
      SetWindowPos(clWin, HWND_TOP,
         nClX,nClY,
         nClPixWidth,nClPixHeight,
         SWP_SHOWWINDOW|SWP_NOACTIVATE
         );
   }
   updatePanel();
   update();
}

// uses: szClMaskBuf, also used in matches()
void Browser::prepareMask(char *pszMask)
{
   mtklog("prepareMask \"%s\"", pszMask);

   strncpy(szClMaskBuf, pszMask, MAX_LINE_LEN);
   pszMask = szClMaskBuf;

   memset(aClMask, 0, sizeof(aClMask));
   nClMask = 0;

   int nSubMaskSize = sizeof(aClMask[0])-2;
   
   // split class*FooBar*{ into 3 submasks
   char *psz1 = pszMask;
   while (*psz1) 
   {
      char *psz2 = psz1+1;
      while (*psz2 && *psz2 != '*')
         psz2++;
      if (*psz2 == '*') 
      {
         *psz2++ = '\0';
         strncpy(aClMask[nClMask], psz1, nSubMaskSize);
         if (!bClCaseSearch)
            setStringToLower(aClMask[nClMask]);
         mtklog(" submask %u \"%s\"", nClMask, aClMask[nClMask]);
         nClMask++;
      }
      else
      if (strlen(psz1))
      {
         strncpy(aClMask[nClMask], psz1, nSubMaskSize);
         if (!bClCaseSearch)
            setStringToLower(aClMask[nClMask]);
         mtklog(" submask %u \"%s\"", nClMask, aClMask[nClMask]);
         nClMask++;
      }
      psz1 = psz2;
   }
_
}

#ifdef OWN_TOLOWER
const char *pszGlblHi = "ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ";
const char *pszGlblLo = "abcdefghijklmnopqrstuvwxyzäöü";

void Browser::prepareToLower()
{
   for (ulong u1=0; u1<256; u1++)
   {
      uchar u2 = (uchar)u1;
      char *psz1 = strchr(pszGlblHi, (char)u1);
      if (psz1 != 0) {
         ulong i2 = psz1-pszGlblHi;
         if (i2 < strlen(pszGlblLo))
             u2   = pszGlblLo[i2];
      }
      aClLowerTab[u1] = (char)u2;
   }
}
#endif

void Browser::setStringToLower(char *psz)
{
   ulong nLen = strlen(psz);
   for (ulong i=0; i<nLen; i++) {
      #ifdef OWN_TOLOWER
      psz[i] = aClLowerTab[(uchar)psz[i]];
      #else
      psz[i] = tolower(psz[i]);
      #endif
   }
}

bool Browser::matchesFile(char *pszFile)
{
   if (!bClLocalScope)
      return true;   // global scope, all files match

   // compare filenames ALWAYS case-insensitive
   if (mystrstri(pszFile, szClFileMask))
      return true;

   // mtklog("NO matchesfile %s %s",pszFile, szClFileMask);

   return false;
}

bool Browser::matches(char *pszLine, int *pIndex, int *pLen)
{
   // requires previous prepareMask run
   char *pszSubHitOld = 0;
   int nSubHitOldLen = 0;
   char *pszSubHit = 0;
   char *pszSubHitFirst = 0;
   int iHitIndex = -1;

   if (!bClCaseSearch)
   {
      // NOTE: this case-insensitive search is TEN TIMES slower
      //       then case-sensitive search. there may be much space
      //       for optimization.
      long nLen = strlen(pszLine);
      if (nLen > MAX_LINE_LEN) nLen = MAX_LINE_LEN;
      strncpy(szClMatchBuf, pszLine, nLen);
      szClMatchBuf[nLen] = '\0';
      setStringToLower(szClMatchBuf);
      pszLine = szClMatchBuf;
   }

   char *pszBase = pszLine;
   for (ulong i=0; i<nClMask; i++) 
   {
      char *pszSubMask = aClMask[i];

      pszSubHit = strstr(pszBase, pszSubMask);

      if (!pszSubHit)
         return false;

      pszBase = pszSubHit;

      if (iHitIndex == -1) {
         iHitIndex = pszSubHit-pszLine;
         pszSubHitFirst = pszSubHit;
      }

      // submasks must appear in correct sequence
      if (pszSubHitOld && (pszSubHit < pszSubHitOld))
         return false;

      pszSubHitOld = pszSubHit;
      nSubHitOldLen = strlen(pszSubMask);
   }
_
   int iHitLen = pszSubHitOld+nSubHitOldLen-pszSubHitFirst;

   mtklog("mask hitlen %u", iHitLen);

   if (pIndex) *pIndex = iHitIndex;
   if (pLen)   *pLen   = iHitLen;
_
   return true;
}

void Browser::calcTopAndBot(long &lrTop, long &lrBot)
{
   // calc currently displayed top and bottom lines
   long ymax = hchar ? nClPixHeight/hchar : BROW_MAX;
   long nTopLine = nClTopLine;
   long nBotLine = nClTopLine+ymax;
   if (nBotLine >= nClLines-1) nBotLine = nClLines ? nClLines-1:nClLines;
   lrTop = nTopLine;
   lrBot = nBotLine;
}

void Browser::goPreviousMaskHit() 
{
   long nTopLine,nBotLine;
   calcTopAndBot(nTopLine,nBotLine);

   // jump to previous mask hit
   prepareMask(szClMask);
  if (nCurSel > 0)
  {
   // is last selected line off screen?
   if (nCurSel > nBotLine)
       nCurSel = nBotLine;

   // we assume current location is a file match
   long nPreLen = strlen(szClSnapPrefix);
   char *pszCurFile = "";
   bool bFileMatch = 1;

   ulong i=0;
   for (i=nCurSel-1; i>0; i--) 
   {
      if (!strncmp(apClLines[i], szClSnapPrefix, nPreLen))
      {
         // reached top of local file. not sure if still matching.
         bFileMatch = false;
         // search next header.
         for (ulong i2=i; i2>0; i2--)
            if (!strncmp(apClLines[i2], szClSnapPrefix, nPreLen)) {
               if (i2<nClLines) {
                  pszCurFile = apClLines[i2+1];
                  bFileMatch = matchesFile(pszCurFile);
               }
               break;
            }
      }

      if (bFileMatch && matches(apClLines[i])) 
      {
         nCurSel = i;
         if (i > nClTopFix)
            nClTopLine = i-nClTopFix;
         else
         if (i > 3)
            nClTopLine = i-3;
         else
            nClTopLine = 0;
         mtklog("WHEEL new topline %u sel %u msk \"%s\"", nClTopLine, nCurSel, szClMask);
         updatePanel();
         update();
         break;
      }
   }

   if (i==0) {
      setStatus("no previous hits.");
      updatePanel();
      update();
   }
  }
}

void Browser::goNextMaskHit() 
{
   long nTopLine,nBotLine;
   calcTopAndBot(nTopLine,nBotLine);

   // jump to next mask hit
   prepareMask(szClMask);

   // is last selected line off screen?
   if (nCurSel < nTopLine)
       nCurSel = nTopLine;

   // we assume current location is a file match
   long nPreLen = strlen(szClSnapPrefix);
   char *pszCurFile = "";
   bool bFileMatch = 1;

   for (ulong i=nCurSel+1; i<nClLines; i++) 
   {
      if (!strncmp(apClLines[i], szClSnapPrefix, nPreLen))
         if (i<nClLines) {
            pszCurFile = apClLines[i+1];
            bFileMatch = matchesFile(pszCurFile);
         }

      if (bFileMatch && matches(apClLines[i])) 
      {
         nCurSel = i;
         if (i > nClTopFix)
            nClTopLine = i-nClTopFix;
         else
         if (i > 3)
            nClTopLine = i-3;
         else
            nClTopLine = 0;
         updatePanel();
         update();
         break;
      }
   }

   if (i==nClLines) {
      setStatus("no further hits.");
      updatePanel();
      update();
   }
}

void Browser::strippedLineToClip(char *psz)
{
   char szBuf[1024+10];
   strncpy(szBuf, psz, 1024);
   szBuf[1024] = '\0';
   while (szBuf[0] && szBuf[strlen(szBuf)-1]==' ')
      szBuf[strlen(szBuf)-1] = '\0'; // cut trailing blanks
   putClipboard(szBuf);
   sprintf(szClStatusLine, "clip: %.50s", szBuf);
   updatePanel();
   update();
}

void Browser::goLocalFileStart()
{
   // search local file start
   long i1;
   long nLen = strlen(szClSnapPrefix);
   for (i1 = nClTopLine; i1 >= 0; i1--)
      if (!strncmp(apClLines[i1], szClSnapPrefix, nLen))
         break;

   if (i1 >= 0)
   {
      i1++;
      if (i1 < nClLines) 
      {
         if (i1 > 2)
            nClTopLine = i1-2;
         else
            nClTopLine = i1;
      }
   }
}

void Browser::goLocalNextFile()
{
   long i1;
   long nLen = strlen(szClSnapPrefix);
   for (i1 = nClTopLine+2; i1 < nClLines-3; i1++)
      if (!strncmp(apClLines[i1], szClSnapPrefix, nLen))
         break;

   if (i1 < nClLines-3)
   {
      i1++;
      if (i1 < nClLines) 
      {
         if (i1 > 2)
            nClTopLine = i1-2;
         else
            nClTopLine = i1;
      }
   }
   else {
      // jump to global file end
      ulong ymax = BROW_MAX;
      if (hchar) ymax = nClPixHeight/hchar;
      if (nClLines+nClBotLines > ymax)
         nClTopLine = nClLines+nClBotLines-ymax;
   }
}

void Browser::copyScreenToClipboard() 
{
   ulong ymax = BROW_MAX;
   if (hchar)
      ymax = nClPixHeight/hchar;

   // collect visible lines.
   char *pszTmp = new char[10000];
   if (!pszTmp) return;
   pszTmp[0] = '\0';
   ulong iout=0;
   for (long yrel=0; yrel<ymax; yrel++) 
   {
      ulong iLine = nClTopLine+yrel;
      if (iLine >= nClLines)
         break;
      char *psz = apClLines[iLine];
      ulong nLen = strlen(psz);
      if (iout+nLen < 10000-100) 
      {
         strcat(pszTmp, psz);
         strcat(pszTmp, "\r\n");
         iout = strlen(pszTmp);
      }
      else
         break;
   }
   putClipboard(pszTmp);
   delete [] pszTmp;

   sprintf(szClStatusLine, "clip: screen copy, %u bytes.", iout);
   updatePanel();
   update();
}

void Browser::closeRight()
{
   if (!pRight)
      return;

   if (apGlblBrowsers[nGlblBrowsers-1] == pRight)
   {
      // stop WinMsg processing
      pRight->destroy();

      // delete instance
      delete pRight;

      pRight = 0;
      apGlblBrowsers[nGlblBrowsers-1] = 0;
      nGlblBrowsers--;
   }

   activate();
}
