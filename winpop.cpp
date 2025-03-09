#ifdef LOCALTEST
 #include <stdio.h>
 #include <string.h>
 #include <windows.h>
#endif

#include "poprawpic.cpp"

const char *msgAsString(long nMsg) {
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
   case WM_KEYFIRST: return "WM_KEYFIRST";
// case WM_KEYDOWN: return "WM_KEYDOWN";
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
// case WM_MOUSEWHEEL: return "WM_MOUSEWHEEL";
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

char *pszGlblInfo = "";

bool  bHavePic = 0;
uchar abPopupPic[24000];

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   int wmId, wmEvent;
   PAINTSTRUCT ps;
   HDC hdc;

   // printf("wndproc %02x %s\n", message, msgAsString(message));

   switch (message) 
   {
      case WM_PAINT:
      {
         hdc = BeginPaint(hWnd, &ps);
         RECT rt;
         GetClientRect(hWnd, &rt);
         // printf("%d %d %d %d\n",rt.left,rt.top,rt.right,rt.bottom);

         #define CMiddle 0xD00000
         #define CBrTop  0xFF6060
         #define CBrLeft 0xFFAAAA
         #define CDark1  0xA00000
         #define CDark2  0x900000

         if (pszGlblInfo)
         {
            HBRUSH hBlue = CreateSolidBrush(CMiddle);
            FillRect(hdc, &rt, hBlue);
            DeleteObject(hBlue);

            long n=4; // border thickness
            for (long x1=0; x1<rt.right; x1++) {
               for (long d=0; d<n; d++) {
                  SetPixel(hdc, x1, d, CBrTop);
                  SetPixel(hdc, x1, rt.bottom-d, CDark2);
               }
            }
            for (long y1=0; y1<rt.bottom; y1++) {
               for (long d=0; d<n; d++) {
                  if (y1 >= d && y1 < rt.bottom-d)
                     SetPixel(hdc, d, y1, CBrLeft);
                  if (y1 >= d && y1 < rt.bottom-d)
                     SetPixel(hdc, rt.right-d, y1, CDark1);
               }
            }

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, 0xFFFFFF);
            DrawText(hdc, pszGlblInfo, strlen(pszGlblInfo), &rt,
               DT_SINGLELINE|DT_CENTER|DT_VCENTER);
            EndPaint(hWnd, &ps);
         }
         else
         {
            if (!bHavePic) {
               bHavePic = 1;
               fskGetBlock(abPopupPic, 24000);
            }
   
            long nWidth  = 200;
            long nHeight =  40;
            for (long x1=0; x1<nWidth; x1++)
               for (long y1=0; y1<nHeight; y1++) {
                  long lColR = abPopupPic[y1*nWidth*3+x1*3+0];
                  long lColG = abPopupPic[y1*nWidth*3+x1*3+1];
                  long lColB = abPopupPic[y1*nWidth*3+x1*3+2];
                  long lCol  = (lColB<<16)|(lColG<<8)|lColR;
                  if (lCol)
                     SetPixel(hdc, x1, y1, lCol);
               }
         }
      }
      break;

      // case WM_DESTROY:
      //   PostQuitMessage(0);
      //   break;

      case WM_ERASEBKGND:
      case WM_NCPAINT:
         if (pszGlblInfo)
            return DefWindowProc(hWnd, message, wParam, lParam);
         break;

      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
        WNDCLASSEX wcex;

        wcex.cbSize = sizeof(WNDCLASSEX); 

        wcex.style         = CS_NOCLOSE; // CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc   = (WNDPROC)WndProc;
        wcex.cbClsExtra    = 0;
        wcex.cbWndExtra    = 0;
        wcex.hInstance     = hInstance;
        wcex.hIcon         = 0; // LoadIcon(hInstance, (LPCTSTR)IDI_DAPROJ);
        wcex.hCursor       = 0; // LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszMenuName  = 0; // "DaMenu"; // (LPCSTR)IDC_DAPROJ;
        wcex.lpszClassName = "MyPopWin";
        wcex.hIconSm       = 0; // LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

        return RegisterClassEx(&wcex);
}

HWND glblPopWin = 0;

long showInfoPopup(char *pszMsg) 
{
   pszGlblInfo = pszMsg;

   HWND hDesk = GetDesktopWindow();
   RECT rDesk;
   GetWindowRect(hDesk, &rDesk);

   long nDeskWidth  = rDesk.right-rDesk.left;
   long nDeskHeight = rDesk.bottom-rDesk.top;
   long nDeskXMid   = nDeskWidth/2;
   long nDeskYMid   = nDeskHeight/2;

   if (!glblPopWin)
   {
      // first call: create popup window with default size
      MyRegisterClass(0);
    
      glblPopWin = CreateWindow("MyPopWin", 0,
         WS_VISIBLE|WS_POPUP,
         nDeskXMid-100,nDeskYMid-20,200,40,
         NULL,
         NULL, 0, NULL);
      if (!glblPopWin) { printf("err1\n"); return 1; }
   }

   // resize given window to match current message
   long nWidth = 0;
   long nPosX  = 0;
   long nPosY  = (nDeskHeight-40)/2;

   if (pszGlblInfo) 
   {
      // display text message popup, static
      nWidth = 100 + strlen(pszGlblInfo) * 15;
      if (nWidth > nDeskWidth*80/100) nWidth = nDeskWidth*80/100;
      nPosX = (nDeskWidth - nWidth) / 2;

      SetWindowPos(glblPopWin, HWND_TOPMOST, nPosX,nPosY,nWidth,40,
         SWP_SHOWWINDOW|SWP_NOACTIVATE
         );

      UpdateWindow(glblPopWin);
      Sleep(1500);
      ShowWindow(glblPopWin, SW_HIDE);
   }
   else 
   {
      // display graphical popup, blink
      nWidth = 200;
      nPosX  = (nDeskWidth - 100) / 2;

      for (long i=0; i<3; i++)
      {
         SetWindowPos(glblPopWin, HWND_TOPMOST, nPosX,nPosY,nWidth,40,
            SWP_SHOWWINDOW|SWP_NOACTIVATE
            );
      
         UpdateWindow(glblPopWin);
      
         Sleep(600);
         ShowWindow(glblPopWin, SW_HIDE);
         Sleep(600);
      }
   }

   return 0;
}

void winCleanupGUI()
{
   if (glblPopWin != 0)
   {
      CloseWindow(glblPopWin);
      UnregisterClass("MyPopWin", 0);
   }
}

#ifdef LOCALTEST
int main(int argc, char *argv[]) {
   showInfoPopup("TEST-POPUP");
   return 0;
}
#endif
