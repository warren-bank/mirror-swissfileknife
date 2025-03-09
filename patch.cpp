/*
   sfk dynamic source file patching.

   originally written as a separate tool,
   then integrated into sfk with the least possible effort.

   changes:
   1.3.2
   - add: it is now allowed to insert any number of blanks
          after most commands, e.g.
          :mkdir   thedir
   - fix: added error checks
*/

#include "sfkbase.hpp"

#ifndef USE_SFK_BASE
 #if defined(WINFULL) && defined(_MSC_VER)
  #define SFK_MEMTRACE
 #endif
#endif

#define MAX_CMD_LINES   10000 // max lines per :file ... :done block
#define MAX_CACHE_LINES 10000 // max lines per :from ... :to pattern
#define MAX_CMD         500   // max number of :from commands per patchfile
#define MAX_OUT_LINES   50000 // max lines per target file

FILE *fpatch  = 0;
char *pszRoot = 0;
char szCmdBuf[MAX_LINE_LEN];
int  bGlblRevoke = 0;
int  bGlblBackup = 0;
int  bGlblSimulate = 0;
static int bGlblTouchOnRevoke = 1;
int  nCmdFileLineEndings = -1;
int  nGlblLine = 0;
int  bGlblIgnoreWhiteSpace = 1;
int  bGlblAlwaysSimulate = 0;
int  bGlblVerbose = 0;
int  bGlblQuickSum = 0;
int  bGlblUnixOutput = 0;
int  nGlblPatchedFiles = 0;
int  nGlblRevokedFiles = 0;
char *pszGlblRelWorkDir = 0;
int  bGlblAnySelRep = 0;
int  bGlblCheckSelRep = 0;
int  bGlblStats = 0;
int  nGlblDetabOutput = 0;
int  bGlblVerify = 0;
int  bGlblNoPID  = 0;
int  bGlblIgnoreRoot = 0;

char **apOut = 0; // [MAX_OUT_LINES];

// select-replace table over all targets
#define MAX_GLOBAL_CHANGES 50
char *apGlobalChange[MAX_GLOBAL_CHANGES][3];
int anGlobalChange[MAX_GLOBAL_CHANGES];
int iGlobalChange = 0;

// select-replace table local to current target
#define MAX_LOCAL_CHANGES 50
char *apLocalChange[MAX_LOCAL_CHANGES][3];
int anLocalChange[MAX_GLOBAL_CHANGES];
int iLocalChange = 0;

int processCmdPatch(char *psz) { return 0; }
int processCmdInfo (char *psz) { return 0; }

// 0 == error, >0 == normal msg, >= 5 do not tell if in QuickSum mode
void log(int nLevel, const char *pFormat, ... )
{
   va_list argList;
   va_start(argList, pFormat);
   if (nLevel == 0) {
      vfprintf(stderr, pFormat, argList);
      fflush(stderr);
   } else {
      if (!(bGlblQuickSum && nLevel >= 5)) {
         vprintf(pFormat, argList);
         fflush(stdout);
      }
   }
   va_end(argList);
}

int detabLine(char *pBuf, char *pTmp, int nBufSize, int nTabSize)
{
   strcpy(pTmp, pBuf);
   int iout=0, nInsert=0;
   for (int icol=0; pTmp[icol] && iout < (nBufSize-nTabSize-2); icol++) {
      char c1 = pTmp[icol];
      if (c1 == '\t') {
         nInsert = nTabSize - (iout % nTabSize);
         for (int i2=0; i2<nInsert; i2++)
            pBuf[iout++] = ' ';
      } else {
         pBuf[iout++] = c1;
      }
   }
   pBuf[iout] = '\0';
   if (iout >= (nBufSize-nTabSize-2)) {
      log(0, "error  : detab: line buffer overflow. max line len supported is %d\n",(nBufSize-nTabSize-2));
      return 9;
   }
   return 0;
}

int processCmdRoot (char *pszIn) {
   strcpy(szCmdBuf, pszIn);
   char *psz = 0;
   if ((psz = strchr(szCmdBuf, '\r')) != 0) *psz = 0;
   if ((psz = strchr(szCmdBuf, '\n')) != 0) *psz = 0;
   pszRoot = strdup(szCmdBuf);
   if (!bGlblIgnoreRoot && strcmp(pszRoot, pszGlblRelWorkDir)) {
      log(0, "error  : you are not in the %s directory.\n",pszRoot);
      return 1+4;
   }
   return 0;
}

int processCmdFile(char *psz);
extern int printx(const char *pszFormat, ...);

// ensure allocation of buffers when entering patchMain,
// and freeing of buffers when leaving:
class PatchMemCover {
public:
   PatchMemCover  ( ) {
      bdead = 0;
      apOut = new char*[MAX_OUT_LINES+10];
      if (!apOut) bdead = 1;
   }
   ~PatchMemCover ( ) {
      if (apOut) { delete [] apOut; apOut = 0; }
   }
   int bdead;
};

int patchMain(int argc, char *argv[], int offs)
{
   int processPatchFile(char *pszPatchFileName);

   PatchMemCover mem;
   if (mem.bdead) {
      log(0, "error: out of memory in patchMain\n");
      return 9;
   }

   if (!strcmp(argv[1+offs], "-example"))
   {
      printx(
         "#patchfile example, containing all supported patchfile commands:\n\n"
         ":patch \"enable FooBar testing\"\n"
         ":info makes some stuff public, for direct access by test funcs\n"
         "\n"
         ":root foosrc\n"
         "\n"
         ":file include\\Foobar.hpp\n"
         ":from \n"
         "private:\n"
         "    bool                  isAvailable               (int nResource);\n"
         "    void                  openBar                   (int nMode);\n"
         ":to\n"
         "public: // [patch-id]\n"
         "    bool                  isAvailable               (int nResource);\n"
         "    void                  openBar                   (int nMode);\n"
         ":from \n"
         "    // returns the application type, 0x00 == not set.\n"
         "    UInt16                getAppType                ( );\n"
         ":to\n"
         "    // returns the application type, 0x00 == not set.\n"
         "    UInt16                getAppType                ( );\n"
         "    UInt32                getAppTypeInternal        ( );\n"
         ":done\n"
         "\n"
         ":## this is a remark, allowed only outside :file blocks.\n"
         ":## the above syntax is sufficient for most cases; but now follow some\n"
         ":## more commands for global replace, file and dir creation, etc.\n"
         ":## select-replace has 3 parms, and applies changes (parms 2+3) only\n"
         ":## in lines containing the search term (parm 1).\n"
         "\n"
         ":file include\\Another.cpp\n"
         ":select-replace /MY_TRACE(/\\n\"/\"/\n"
         ":select-replace _printf(\"spam: _printf(_while(0) printf(_\n"
         ":set only-lf-output\n"
         ":from \n"
         "    bool                  existsFile                (char *psz);\n"
         ":to\n"
         "    // [patch-id]\n"
         "    int                   existsFile                (char *psz);\n"
         ":done\n"
         "\n"
         ":mkdir sources\n"
         ":create sources\\MyOwnFix.hpp\n"
         "// this file is generated by sfk patch.\n"
         "##define OTHER_SYMBOL MY_OWN_SYMBOL\n"
         ":done\n"
         "\n"
         ":skip-begin\n"
         "this is outcommented stuff. the skip-end is optional.\n"
         ":skip-end\n"
         );
      return -1;
   }

   if (!strcmp(argv[1+offs], "-template") || !strcmp(argv[1+offs], "-tpl"))
   {
      printf(
         ":patch \"thepatch\"\n"
         "\n"
         ":root theproject\n"
         "\n"
         ":file include\\file1.hpp\n"
         ":from \n"
         ":to\n"
         "    // [patch-id]\n"
         ":from \n"
         ":to\n"
         ":done\n"
         "\n"
         ":file sources\\file1.cpp\n"
         ":from \n"
         ":to\n"
         "    // [patch-id]\n"
         ":done\n"
         "\n"
         );
      return -1;
   }

   #ifdef _WIN32
   _getcwd(szCmdBuf,sizeof(szCmdBuf)-10);
   #else
   getcwd(szCmdBuf,sizeof(szCmdBuf)-10);
   #endif
   char *psz = strrchr(szCmdBuf, glblPathChar);
   if (!psz) {
      log(0, "error: cannot identify working dir. make sure you are in the correct directory.\n");
      return 9;
   }
   psz++;
   pszGlblRelWorkDir = strdup(psz);
   
   char *pszPatchFileName = 0;
   char *pszPatchFileBackup = 0;
   int bWantRevoke = 0;
   int bWantRedo   = 0;
   int bJustSim    = 0;

   for (int iarg=1; iarg<argc; iarg++) {
      if (!strcmp(argv[iarg+offs], "-revoke")) {
         bWantRevoke = true;
      }
      else
      if (!strcmp(argv[iarg+offs], "-redo")) {
         bWantRevoke = true;
         bWantRedo   = true;
      }
      else
      if (!strcmp(argv[iarg+offs], "-keep-dates")) {
         bGlblTouchOnRevoke = 0;
      }
      else
      if (!strcmp(argv[iarg+offs], "-exact-match")) {
         bGlblIgnoreWhiteSpace = 0;
      }
      else
      if (!strcmp(argv[iarg+offs], "-sim")) {
         bJustSim = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-verify")) {
         bJustSim    = 1;
         bGlblVerify = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-qs")) {
         bGlblQuickSum = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-verbose")) {
         bGlblVerbose = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-stat")) {
         bGlblStats = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-nopid")) {
         bGlblNoPID = 1;
      }
      else
      if (!strcmp(argv[iarg+offs], "-anyroot")) {
         bGlblIgnoreRoot = 1;
      }
      else
      if (!strncmp(argv[iarg+offs], "-", 1)) {
         printf("unknown option: %s\nuse with no parameters to get help.\n",argv[iarg+offs]);
         return 9;
      }
      else {
         pszPatchFileName = argv[iarg+offs];
         char *psz = strrchr(pszPatchFileName, glblPathChar);
         if (!psz) psz = strrchr(pszPatchFileName,':');
         if (!psz) { psz = pszPatchFileName; } else psz++;
         sprintf(szCmdBuf,"save_patch%c%s", glblPathChar, psz);
         pszPatchFileBackup = strdup(szCmdBuf);
      }
   }

   // pass -1: check logic
   if (bJustSim && bWantRevoke) {
      log(0, "error  : cannot simulate and revoke together.\n");
      return 9;
   }

   // pass 0: init stats
   for (int i1=0; i1<MAX_GLOBAL_CHANGES; i1++)
      anGlobalChange[i1] = 0;
   for (int i2=0; i2<MAX_LOCAL_CHANGES; i2++)
      anLocalChange[i2]  = 0;

   // pass 1: revoke: always, unconditional.
   int iRC = 0;
   if (bWantRevoke) {
      bGlblRevoke = 1;
      if (fileExists(pszPatchFileBackup)) {
         log(5, "* revoking changes from: %s\n", pszPatchFileBackup);
         iRC = processPatchFile(pszPatchFileBackup);
      } else {
         log(5, "* revoking changes from: %s\n", pszPatchFileName);
         iRC = processPatchFile(pszPatchFileName);
      }
      bGlblRevoke = 0;
      if (!iRC && !bWantRedo) {
         if (bGlblQuickSum) {
            printf("* patch revoked: %s - %d target files\n",pszPatchFileName,nGlblRevokedFiles);
         } else {
            if (bGlblTouchOnRevoke)
               printf("* all changes revoked. the target files got the current time stamp,\n"
                      "* to ease recompile. you may use -keepdates to change this behaviour.\n");
            else
               printf("* all changes revoked, including original timestamps.\n");
         }
      }
      if (!iRC) {
         // remove patchfile backup
         if (fileExists(pszPatchFileBackup))
            if (remove(pszPatchFileBackup)) {
               log(0, "error  : cannot remove stale backup: %s\n",pszPatchFileBackup);
               return 1;
            }
      }
      if (!iRC && !bWantRedo)
         return 0;
   }

   // pass 2: pre-scan if all patches may be applied
   bGlblSimulate = 1;
   if (!iRC) {
      log(5, "* checking target file compliancies\n");
      iRC = processPatchFile(pszPatchFileName);
   }
   if (bJustSim) {
      if (!iRC) {
         if (bGlblQuickSum)
          printf("* patch %s: %s\n", bGlblVerify ? "intact" : "valid", pszPatchFileName);
         else
          printf("* all checked. the patch is %s.\n", bGlblVerify ? "still intact" : "valid and may be applied.");
         return 0;
      } else {
         if (bGlblQuickSum)
            log(0, "patch  : %s\n",pszPatchFileName);
         printf("* there were errors. the patch cannot be applied.\n");
         printf("* however, if an older patch is active, you may still -revoke it.\n");
         return 1;
      }
   }
   bGlblSimulate = 0;

   // pass 3: create backups
   if (!iRC && !bGlblNoPID) {
      log(5, "* creating backups\n");
      bGlblBackup = 1;
      iRC = processPatchFile(pszPatchFileName);
      bGlblBackup = 0;
   }

   // pass 3: apply patches
   if (!iRC) {
      log(5, "* applying patches%s\n", bGlblNoPID?" permanently":"");
      bGlblCheckSelRep = 1;
      iRC = processPatchFile(pszPatchFileName);
      if (!iRC) {
         if (bWantRedo) {
            if (bGlblQuickSum)
               printf("* all changes re-applied: %s - %d target files\n",pszPatchFileName,nGlblPatchedFiles);
            else
               printf("* all changes re-applied.\n");
         } else {
            if (bGlblQuickSum) {
               printf("* patch applied: %s - %d target files\n",pszPatchFileName,nGlblPatchedFiles);
            } else {
               printf("* all done.\n");
            }
         }
         // patch applied: save the patchfile for future revoke
         if (!bGlblNoPID)
         {
         #ifdef _WIN32
         _mkdir("save_patch");
         #else
         mkdir("save_patch", S_IREAD | S_IWRITE | S_IEXEC);
         #endif
         FILE *fout = fopen(pszPatchFileBackup,"w");
         if (fout) { fprintf(fout,"dummy"); fclose(fout); }
         #ifdef _WIN32
         sprintf(szCmdBuf, "xcopy /Q /K /Y %s %s >nul",pszPatchFileName,pszPatchFileBackup);
         #else
         sprintf(szCmdBuf, "cp -p %s %s",pszPatchFileName,pszPatchFileBackup);
         #endif
         iRC = system(szCmdBuf);
         if (iRC)
            log(0, "error  : cannot backup patchfile to: %s\n",pszPatchFileBackup);
         }
      } else {
         if (iRC & 4)
            log(0, "info   : make sure you are above the \"%s\" directory.\n",pszRoot);
      }
   } else {
      if (bGlblQuickSum)
         log(0, "patch  : %s\n",pszPatchFileName);
      if (iRC & 4) {
         log(0, "info   : make sure you are in the \"%s\" directory.\n",pszRoot);
         return iRC;
      }
      printf("* NOTHING CHANGED: i will either apply ALL changes, or NONE.\n"
             "* you may also try sfk patch -revoke or -redo.\n"
            );
      fflush(stdout);
   }

   return iRC;
}

char *skipspace(char *psz) {
   while (*psz && *psz == ' ')
      psz++;
   return psz;
}

int processPatchFile(char *pszPatchFileName)
{
   int processCreateFile(char *pszIn);
   int processCreateDir(char *pszIn);

   int iRC = 0;

   fpatch = fopen(pszPatchFileName, "r");
   if (!fpatch) { log(0, "error  : cannot open patchfile: %s\n",pszPatchFileName); return 2+4; }
   nGlblLine = 0;

   // parse patch file, exec commands
   int bWithinSkip = 0;
   char szBuf[MAX_LINE_LEN];
   while (fgets(szBuf,sizeof(szBuf)-10,fpatch) != NULL) 
   {
      nGlblLine++;

      // determine line endings of the command file
      if (nCmdFileLineEndings == -1)
      {
         if (strstr(szBuf,"\r\n") != 0)
            nCmdFileLineEndings = 2;   // CR/LF
         else
            nCmdFileLineEndings = 1;   // LF only
      }

      if (!strncmp(szBuf,":# ",3))
         continue;

      if (!strncmp(szBuf,":skip-end",strlen(":skip-end"))) {
         if (!bWithinSkip) {
            log(0, "error  : skip-end without skip-begin in line %d\n",nGlblLine); 
            return 2;
         }
         bWithinSkip = 0;
         continue;
      }

      if (!strncmp(szBuf,":skip-begin",strlen(":skip-begin"))) {
         if (bWithinSkip) {
            log(0, "error  : skip-begin twice in line %d\n",nGlblLine);
            return 2;
         }
         bWithinSkip = 1;
         continue;
      }

      if (bWithinSkip)
         continue;

      if (!strncmp(szBuf,":patch ",7)) {
         iRC |= processCmdPatch(skipspace(szBuf+7));
         continue;
      }
      if (!strncmp(szBuf,":info ",6)) {
         iRC |= processCmdInfo(skipspace(szBuf+6));
         continue;
      }
      if (!strncmp(szBuf,":root ",6)) {
         if (processCmdRoot(skipspace(szBuf+6)))
            return 1+4;
         continue;
      }
      if (!strncmp(szBuf,":select-replace ",strlen(":select-replace "))) 
      {
         bGlblAnySelRep = 1;
         if (iGlobalChange < MAX_GLOBAL_CHANGES-2) {               
            char *psz = skipspace(szBuf+strlen(":select-replace "));
            char  nCC = *psz++;
            char *pszMaskBegin = psz;
            while (*psz && (*psz != nCC)) psz++;
            char *pszMaskEnd   = psz;
            if (*psz) psz++;
            char *pszFromBegin = psz;
            while (*psz && (*psz != nCC)) psz++;
            char *pszFromEnd   = psz;
            if (*psz) psz++;
            char *pszToBegin   = psz;
            while (*psz && (*psz != nCC)) psz++;
            char *pszToEnd     = psz;
            *pszMaskEnd = 0;
            *pszFromEnd = 0;
            *pszToEnd   = 0;
            if (strlen(pszFromBegin) > 0) {
               char *pszMaskDup = strdup(pszMaskBegin);
               char *pszFromDup = strdup(pszFromBegin);
               char *pszToDup   = strdup(pszToBegin);
               apGlobalChange[iGlobalChange][0] = pszMaskDup;
               apGlobalChange[iGlobalChange][1] = pszFromDup;
               apGlobalChange[iGlobalChange][2] = pszToDup;
               iGlobalChange++;
            }
         } else {
            log(0, "error  : too many global select-replace, only up to %d supported.\n",MAX_GLOBAL_CHANGES);
            return 2;
         }
         continue;
      }
      if (!strncmp(szBuf,":create ",8)) {
         iRC |= processCreateFile(skipspace(szBuf+8));
         continue;
      }
      if (!strncmp(szBuf,":mkdir ",7)) {
         iRC |= processCreateDir(skipspace(szBuf+7));
         continue;
      }
      if (!strncmp(szBuf,":file ",6)) {
         iRC |= processCmdFile(skipspace(szBuf+6));
         continue;
      }
      if (!strncmp(szBuf,":set only-lf-output",strlen(":set only-lf-output"))) {
         bGlblUnixOutput = 1;
         continue;
      }
      if (!strncmp(szBuf,":set detab=",strlen(":set detab="))) {
         nGlblDetabOutput = atol(szBuf+strlen(":set detab="));
         log(5, "setting detab to %d\n",nGlblDetabOutput);
         continue;
      }
      if (!strncmp(szBuf,":",1)) {
         log(0, "error  : unknown command in line %d: %s\n",nGlblLine,szBuf);
         return 1;
      }

      // everything else handled by processCmdFile
   }

   fclose(fpatch);

   // now that all was processed, cleanup table of global changes
   for (int i=0;i<iGlobalChange;i++) {
      if (bGlblCheckSelRep) {
         if (!anGlobalChange[i])
            log(5, "info   : global select-replace never applied: %s, %s, %s\n",apGlobalChange[i][0],apGlobalChange[i][1],apGlobalChange[i][2]);
         else
         if (bGlblStats || bGlblVerbose)
            log(5, "global select-replace applied %d times: %s, %s, %s\n",anGlobalChange[i],apGlobalChange[i][0],apGlobalChange[i][1],apGlobalChange[i][2]);
      }
      free(apGlobalChange[i][0]);
      free(apGlobalChange[i][1]);
      free(apGlobalChange[i][2]);
   }
   iGlobalChange = 0;

   return iRC;
}

int processCmdFile(char *pszIn)
{
   int processFileUntilDone(char *pszName);

   if (strlen(pszIn) < 1) { log(0, "error  : missing filename after :file\n"); return 2; }

   strcpy(szCmdBuf, pszIn);
   char *psz = 0;
   if ((psz = strchr(szCmdBuf, '\r')) != 0) *psz = 0;
   if ((psz = strchr(szCmdBuf, '\n')) != 0) *psz = 0;
   char *pszFile = strdup(szCmdBuf);
 
   strcpy(szCmdBuf, pszFile);

   // check if file exists
   if (!bGlblRevoke)
      if (!fileExists(szCmdBuf)) {
         log(0, "error  : unable to open file: %s\n",szCmdBuf);
         return 2+4; 
      }

   iLocalChange = 0;
   int iRC = processFileUntilDone(szCmdBuf);

   // always cleanup this table, which is feeded by processFileUntilDone
   for (int i=0;i<iLocalChange;i++) {
      if (bGlblCheckSelRep) {
         if (!anLocalChange[i])
            log(5, "info   : local select-replace never applied: %s, %s, %s\n",apLocalChange[i][0],apLocalChange[i][1],apLocalChange[i][2]);
         else
         if (bGlblStats || bGlblVerbose)
            log(5, "local select-replace applied %d times: %s, %s, %s\n",anLocalChange[i],apLocalChange[i][0],apLocalChange[i][1],apLocalChange[i][2]);
      }
      free(apLocalChange[i][0]);
      free(apLocalChange[i][1]);
      free(apLocalChange[i][2]);
   }
   iLocalChange = 0;

   return iRC;
}

int processCreateFile(char *pszIn)
{
   if (strlen(pszIn) < 1) { log(0, "error  : missing filename after :create\n"); return 2; }

   strcpy(szCmdBuf, pszIn);
   char *psz = 0;
   if ((psz = strchr(szCmdBuf, '\r')) != 0) *psz = 0;
   if ((psz = strchr(szCmdBuf, '\n')) != 0) *psz = 0;
   char *pszFile = strdup(szCmdBuf);
 
   strcpy(szCmdBuf, pszFile);

   // any existing file?
   if (!bGlblRevoke && !bGlblBackup) {
      if (fileExists(szCmdBuf)) {
         if (!bGlblVerify)
            log(0, "warning: file already exists: %s\n", szCmdBuf);
      } else {
         if (bGlblVerify) {
            log(0, "error  : file no longer exists: %s\n", szCmdBuf);
            return 1;
         }
      }
   }

   FILE *fout = 0;

   if (!bGlblSimulate && bGlblRevoke) {
      // do exact opposite: delete old file
      if (remove(szCmdBuf))
         log(0, "warning: cannot remove: %s\n", szCmdBuf);
      else {
         nGlblRevokedFiles++;
         log(5, "removed: %s\n", szCmdBuf);
      }
   }
   else 
   {
      // create new output file
      if (!bGlblSimulate && !bGlblRevoke && !bGlblBackup) {
         fout = fopen(szCmdBuf, "w");
         if (!fout) {
            log(0, "error  : cannot create file: %s\n", szCmdBuf); // return 2+4; 
            // fall-through, continue to allow creation of other files.
         }
      }
   }

   // copy-through contents from patchfile until :done
   char szBuf[MAX_LINE_LEN];
   int iout = 0;
   int bGotDone = 0;
   int nStartLine = nGlblLine;
   while (fgets(szBuf,sizeof(szBuf)-10,fpatch) != NULL) 
   {
      nGlblLine++;
      if (!strncmp(szBuf, ":done", 5)) {
         bGotDone = 1;
         break;
      }
      if (!bGlblSimulate && !bGlblRevoke && !bGlblBackup && fout) {
         fputs(szBuf, fout);
         iout++;
      }
   }

   // close output file
   if (!bGlblSimulate && !bGlblRevoke && !bGlblBackup && fout) {
      fflush(fout);
      fclose(fout);
      nGlblPatchedFiles++;
      log(5, "written: %s, %d lines.\n",szCmdBuf,iout);
   }

   if (!bGotDone) {
      if (!fout) { log(0, "error  : missing :done after :create of line %d\n", nStartLine); return 2; }
   }

   return 0;
}

int processCreateDir(char *pszIn)
{
   if (strlen(pszIn) < 1) { log(0, "error  : missing filename after :mkdir\n"); return 2; }

   strcpy(szCmdBuf, pszIn);
   char *psz = 0;
   if ((psz = strchr(szCmdBuf, '\r')) != 0) *psz = 0;
   if ((psz = strchr(szCmdBuf, '\n')) != 0) *psz = 0;
   char *pszFile = strdup(szCmdBuf);
 
   strcpy(szCmdBuf, pszFile);

   if (!bGlblSimulate && bGlblRevoke) {
      // not yet supported: remove dir on revoke (sequence problem)
      // _rmdir(szCmdBuf);
      return 0;
   }

   // create dir. have to use Win-specific API.
   if (!bGlblSimulate && !bGlblRevoke && !bGlblBackup) {
      #ifdef _WIN32
      int iRC = _mkdir(szCmdBuf);
      #else
      int iRC = mkdir(szCmdBuf, S_IREAD | S_IWRITE | S_IEXEC);
      #endif
      if (!iRC) log(5, "created: %s\n",szCmdBuf);
   }

   return 0;
}

// ====================================================================

char *aPatch[MAX_CMD_LINES];
char *aBuf[MAX_CACHE_LINES];
int   aifrom[MAX_CMD];
int   aifromlen[MAX_CMD];
int   aito[MAX_CMD];
int   aitolen[MAX_CMD];

char szLine1[MAX_LINE_LEN];
char szLine2[MAX_LINE_LEN];

void shrinkLine(char *psz, char *pszOut)
{
   int bWithinString = 0;
   int nWhiteCnt = 0;
   for (;*psz; psz++)
   {
      if (*psz == '\r' || *psz == '\n')
         break;   // strip also CR, LF from shrinked strings

      if (*psz == '\"' || *psz == '\'')
         bWithinString = 1-bWithinString;

      if (bWithinString)
         *pszOut++ = *psz;
      else
      if (*psz != ' ' && *psz != '\t') {      
         *pszOut++ = *psz;
         nWhiteCnt = 0;
      }
      else {
         // always apply first whitespace
         nWhiteCnt++;
         if (nWhiteCnt == 1)
            *pszOut++ = ' ';
      }
   }
   *pszOut = 0;
}

int compareLines(char *psz1, char *psz2)
{
   if (bGlblIgnoreWhiteSpace)
   {
      shrinkLine(psz1, szLine1);
      shrinkLine(psz2, szLine2);
      int iRC = strcmp(szLine1,szLine2);
      if (bGlblVerbose)
      {
         if (iRC)
            printf("src-> %s\npat-> %s\n",szLine1,szLine2);
         else
            printf("src*> %s\npat*> %s\n",szLine1,szLine2);
      }
      return iRC;
   }
   return strcmp(psz1, psz2);
}

int processFileUntilDone(char *pszTargFileName)
{
   // INPUT implicite: fpatch stream

   // 1. read next block from patchfile until ":done"
   char szBuf[MAX_LINE_LEN];
   char szBuf2[MAX_LINE_LEN];
   int ipatch = 0;
   while (fgets(szBuf,sizeof(szBuf)-10,fpatch) != NULL) 
   {
      nGlblLine++;
      aPatch[ipatch++] = strdup(szBuf);
      aPatch[ipatch] = 0;
      if (ipatch > MAX_CMD_LINES) { log(0, "command block too large, max %d lines supported.\n",(int)MAX_CMD_LINES); return 2; }
      if (!strncmp(szBuf, ":done", 5))
         break;
   }

   // 2. find commands
   int icmd=0; int iline=0; int bNextCmd=0; int bDone=0;
   int bWithinToBlock=0, bHavePatchIDForThisFile=0;
   int bFromPassed=0, nLocalDetabOutput=0;
   while (!bDone)
   {
      aifrom   [icmd]=-1;
      aifromlen[icmd]=-1;
      aito     [icmd]=-1;
      aitolen  [icmd]=-1;
      for (;iline<ipatch;iline++) 
      {
         if (!strncmp(aPatch[iline],":select-replace ",strlen(":select-replace "))) 
         {
            bGlblAnySelRep = 1;
            if (iLocalChange < MAX_LOCAL_CHANGES-2) {               
               char *psz = aPatch[iline]+strlen(":select-replace ");
               char  nCC = *psz++;
               char *pszMaskBegin = psz;
               while (*psz && (*psz != nCC)) psz++;
               char *pszMaskEnd   = psz;
               if (*psz) psz++;
               char *pszFromBegin = psz;
               while (*psz && (*psz != nCC)) psz++;
               char *pszFromEnd   = psz;
               if (*psz) psz++;
               char *pszToBegin   = psz;
               while (*psz && (*psz != nCC)) psz++;
               char *pszToEnd     = psz;
               *pszMaskEnd = 0;
               *pszFromEnd = 0;
               *pszToEnd   = 0;
               if (strlen(pszFromBegin) > 0) {
                  char *pszMaskDup = strdup(pszMaskBegin);
                  char *pszFromDup = strdup(pszFromBegin);
                  char *pszToDup   = strdup(pszToBegin);
                  apLocalChange[iLocalChange][0] = pszMaskDup;
                  apLocalChange[iLocalChange][1] = pszFromDup;
                  apLocalChange[iLocalChange][2] = pszToDup;
                  iLocalChange++;
               }
            } else {
               log(0, "error  : too many select-replace in one :file, only up to %d supported.\n",MAX_LOCAL_CHANGES);
               return 2;
            }
            continue;
         }

         if (!strncmp(aPatch[iline],":set detab=",strlen(":set detab="))) {
            nLocalDetabOutput = atol(aPatch[iline]+strlen(":set detab="));
            // log(5, "setting local detab to %d\n",nLocalDetabOutput);
            continue;
         }

         if (!strncmp(aPatch[iline],":from",strlen(":from"))) 
         {
            bFromPassed = 1;
            bWithinToBlock = 0;
            if (aifrom[icmd] == -1) {
               // starts new command (only at file start)
               aifrom[icmd]    = iline+1;
            } else {
               // ends current command
               aitolen[icmd]   = iline-aito[icmd];
               bNextCmd = 1;
            }
         }

         if (!bFromPassed && !strncmp(aPatch[iline], ":", 1)) {
            log(0, "unexpected command: %s\n", aPatch[iline]);
            return 9;
         }

         if (!strncmp(aPatch[iline],":to",strlen(":to"))) {
            aito[icmd]      = iline+1;
            aifromlen[icmd] = aito[icmd]-aifrom[icmd]-1;
            bWithinToBlock  = 1;
         }

         if (!strncmp(aPatch[iline],":done",strlen(":done"))) {
            // only at EOF
            bWithinToBlock = 0;
            aitolen[icmd]   = iline-aito[icmd];
            bNextCmd = 1;
            bDone = 1;
         }

         if (strstr(aPatch[iline], "[patch-id]") != 0) {
            if (bWithinToBlock)
               bHavePatchIDForThisFile = 1;
            else {
               log(0, "error  : line %d: [patch-id] not allowed within :from block. expected in :to block.\n",nGlblLine);
               return 2;
            }
         }

         if (bNextCmd)
         {
            bNextCmd=0;
            if (aifrom[icmd]   ==-1) { log(0, "error  : line %d: :from block missing\n",nGlblLine); return 2; }
            if (aito[icmd]     ==-1) { log(0, "error  : line %d: :to block missing\n",nGlblLine); return 2; }
            if (aifromlen[icmd]<= 0) { log(0, "error  : line %d: :from block empty\n",nGlblLine); return 2; }
            if (aitolen[icmd]  <= 0) { log(0, "error  : line %d: :to block empty\n",nGlblLine);   return 2; }
            icmd++;
            if (icmd >= MAX_CMD-2)   { log(0, "error  : too many commands, only %d supported\n",MAX_CMD); return 2; }
            aifrom[icmd]    = iline+1;
            break;
         }

      }  // endfor inner loop

   }  // endfor outer loop (bDone)

   if (!bHavePatchIDForThisFile && !bGlblNoPID) {
      log(0, "error  : line %d: [patch-id] missing!\n",nGlblLine);
      log(0, "info   : you must supply at least one [patch-id] within a :to block per :file,\n"
             "info   : otherwise i cannot identify already-patched files.\n");
      return 2;
   }

   // =========== pre-scan the target file, find out if it's patched =====================

   int bTargetIsPatched = 0;

   FILE *ftarg2 = fopen(pszTargFileName, "r");

   if (!ftarg2) { log(0, "error  : cannot read target file: %s\n", pszTargFileName); return 2+4; }

   while (fgets(szBuf,sizeof(szBuf)-10,ftarg2) != NULL) 
      if (strstr(szBuf,"[patch-id]") != 0)
         bTargetIsPatched = 1;

   fclose(ftarg2);

   // ====================================================================================

   char szProbeCmd[MAX_LINE_LEN];

 if (!bGlblSimulate)
 { // begin backup block

   // make a backup of the target file
   char szRelFileName[1024];
   char szBackupDir[MAX_LINE_LEN];
   strcpy(szBackupDir, pszTargFileName);
   char *pszLastDir = strrchr(szBackupDir, glblPathChar);
   // if (!pszLastDir) { log(0, "error  : no '%c' path character in %s, cannot create backup dir.\n", glblPathChar, szBackupDir); return 2; }
   if (pszLastDir) {
      pszLastDir++;
      strcpy(szRelFileName,pszLastDir);
      *pszLastDir = 0;
      strcat(szBackupDir, "save_patch");
   } else {
      // file name without any path:
      strcpy(szRelFileName, pszTargFileName);
      strcpy(szBackupDir, "save_patch");
   }

   // IS a backup file already existing?
   sprintf(szProbeCmd,"%s%c%s",szBackupDir,glblPathChar,szRelFileName);

   // are we by any chance in REVOKE mode?
   if (bGlblRevoke && bTargetIsPatched)
   {
      // YES: copy backup file back.
      char szCopyCmd[MAX_LINE_LEN];
      // 0. ensure backup exists
      if (!fileExists(szProbeCmd)) { log(0, "error  : cannot revoke, no backup file: %s\n",szProbeCmd); return 1+4; }
      // is there an old target?
      if (fileExists(pszTargFileName))
      {
         // 1. ensure target is writeable
         #ifdef _WIN32
         sprintf(szCopyCmd, "attrib -R %s",pszTargFileName);
         #else
         sprintf(szCopyCmd, "chmod +w %s", pszTargFileName);
         #endif
         system(szCopyCmd);
         #ifdef _WIN32
         // 2. delete target to ensure copy will work
         sprintf(szCopyCmd, "del %s",pszTargFileName);
         system(szCopyCmd);
         #endif
      }
      // 3. copy backup over target
      if (bGlblTouchOnRevoke) {
         // make sure target has current timestamp
         FILE *fsrc = fopen(szProbeCmd, "rb");
         if (!fsrc) { log(0, "error  : cannot open %s\n",szProbeCmd); return 2+4; }
         FILE *fdst = fopen(pszTargFileName,"wb");
         if (!fdst) { log(0, "error  : cannot open %s\n",pszTargFileName); return 2+4; }
         // quick binary block copy
         // size_t fread( void *buffer, size_t size, size_t count, FILE *stream );
         // size_t fwrite( const void *buffer, size_t size, size_t count, FILE *stream );
         size_t ntotal = 0;
         while (1) {
            size_t nread = fread(szCopyCmd, 1, sizeof(szCopyCmd), fsrc);
            if (nread <= 0)
               break;
            fwrite(szCopyCmd, 1, nread, fdst);
            ntotal += nread;
         }
         fclose(fdst);
         fclose(fsrc);
         // write-protect target
         #ifdef _WIN32
         sprintf(szCopyCmd, "attrib +R %s",pszTargFileName);
         #else
         sprintf(szCopyCmd, "chmod -w %s", pszTargFileName);
         #endif
         system(szCopyCmd);
         log(5, "revoked: %s, %u bytes\n",pszTargFileName,(unsigned int)ntotal);
         nGlblRevokedFiles++;
      } else {
         // xcopy requires us to create a dummy, otherwise we get a prompting
         FILE *fdst = fopen(pszTargFileName,"w");
         if (!fdst) { log(0, "error  : cannot open %s\n",pszTargFileName); return 2+4; }
         fprintf(fdst, "tmp\n\n"); fflush(fdst);
         fclose(fdst);
         // no overwrite this with /K, keeping potential +R attributes
         #ifdef _WIN32
         sprintf(szCopyCmd, "xcopy /Q /K /Y %s %s >nul",szProbeCmd,pszTargFileName);
         #else
         sprintf(szCopyCmd, "cp -p %s %s",szProbeCmd,pszTargFileName);
         #endif
         int iRC = system(szCopyCmd);
         if (!iRC) {
            log(5, "revoked: %s\n",pszTargFileName);
            nGlblRevokedFiles++;
         } else {
            log(0, "revoke failed: %s\n",pszTargFileName);
            return 1;
         }
      }
      // 4. make backup writeable, and remove
      #ifdef _WIN32
      sprintf(szCopyCmd, "attrib -R %s",szProbeCmd);
      #else
      sprintf(szCopyCmd, "chmod +w %s", szProbeCmd);
      #endif
      system(szCopyCmd);
      if (remove(szProbeCmd)) {
         log(0, "error  : cannot delete stale backup: %s\n",szProbeCmd);
         return 1;
      }
      return 0;
   }

   if (bGlblRevoke && !bTargetIsPatched)
   {
      if (!bTargetIsPatched) {
         log(0, "warning: isn't patched, will not revoke: %s\n",pszTargFileName);
      }
   }

   // otherwise continue creating a backup
   if (bGlblBackup) 
   {
      // prepare: if stale backup, delete first
      if (fileExists(szProbeCmd)) 
      {
         // remove old backup, most probably stale
         char szCopyCmd[MAX_LINE_LEN];
         log(5, "del.bup: %s\n",szProbeCmd);
         #ifdef _WIN32
         sprintf(szCopyCmd, "attrib -R %s",szProbeCmd);
         #else
         sprintf(szCopyCmd, "chmod +w %s", szProbeCmd);
         #endif
         system(szCopyCmd);
         if (remove(szProbeCmd)) {
            log(0, "error  : cannot delete stale backup: %s\n",szProbeCmd);
            return 1;
         }
      }

      // create backup dir and file
      char szCopyCmd[MAX_LINE_LEN];
      int iRC = 0;
      // sprintf(szCopyCmd,"mkdir %s",szBackupDir);
      // int iRC = system(szCopyCmd); // create backup dir, if not done yet
      #ifdef _WIN32
      if (_mkdir(szBackupDir))
      #else
      if (mkdir(szBackupDir, S_IREAD | S_IWRITE | S_IEXEC))
      #endif
      {
         // log(0, "warning: cannot create backup dir: %s\n",szBackupDir);
         // return 1;
      }
      #ifdef _WIN32
      sprintf(szCopyCmd,"xcopy /Q /K %s %s >nul",pszTargFileName,szBackupDir);
      #else
      sprintf(szCopyCmd,"cp -p %s %s",pszTargFileName,szBackupDir);
      #endif
      iRC = system(szCopyCmd); // create backup file
      if (iRC) {log(0, "error  : creation of backup file failed: RC %d\n",iRC); return 2+4; }
      if (fileExists(szProbeCmd)) {
         log(5, "backupd: %s\n",szProbeCmd);
      } else {
         log(0, "error  : verify of backup file failed: %s\n",szProbeCmd); return 2+4;
      }
      return 0;
   }

 } // end backup block

   if (bGlblRevoke)
      return 0;

   // 3. read and patch the target file
   FILE *ftarg = fopen(pszTargFileName, "r");
   if (!ftarg) { log(0, "error  : cannot read target file: %s\n", pszTargFileName); return 2+4; }

   // we cache the output in memory
   int iout = 0;

   int isrcbuf=0, ipatmatch=0;
   aBuf[0] = 0;
   int icmd2 = 0, nLine = 0, nTargLineEndings = -1;
   while (fgets(szBuf,sizeof(szBuf)-10,ftarg) != NULL) 
   {
      nLine++;

      // make sure target file has SAME line endings as cmd file.
      if (nTargLineEndings == -1)
      {
         if (strstr(szBuf,"\r\n") != 0)
            nTargLineEndings = 2;   // CR/LF
         else
            nTargLineEndings = 1;   // LF only
         if (nCmdFileLineEndings != nTargLineEndings)
         {
            log(0, "error  : different line endings!\n");
            log(0, "error  :   the patch file uses %s line endings.\n",(nCmdFileLineEndings==2)?"CR/LF":"LF");
            log(0, "error  :   this target file uses %s line endings: %s\n",(nTargLineEndings==2)?"CR/LF":"LF",pszTargFileName);
            return 2;
         }
      }

      if (strstr(szBuf,"[patch-id]") != 0) {
         if (bGlblVerify) {
            if (!bGlblQuickSum)
               log(5, "checked: %s is still patched\n",pszTargFileName);
            return 0;
         }
         if (!bGlblNoPID) {
            log(0, "error  : %s already patched\n",pszTargFileName);
            return 1;
         }
      }

      // does current in-line match the CURRENT pattern's current line?
      if ((icmd2<icmd) && !compareLines(szBuf,aPatch[aifrom[icmd2]+ipatmatch]))
      {
         ipatmatch++;
         aBuf[isrcbuf++] = strdup(szBuf);
         if (isrcbuf > MAX_CACHE_LINES-10) { log(0, "pattern cache overflow, %d lines exceeded\n",(int)MAX_CACHE_LINES); return 2; }
         if (ipatmatch == aifromlen[icmd2]) {
            // full pattern match:
            // write replacement pattern
            for (int i3=0;i3<aitolen[icmd2];i3++) {
               apOut[iout++] = strdup(aPatch[aito[icmd2]+i3]);
               if (iout > MAX_OUT_LINES-10) { log(0, "output cache overflow, %d lines exceeded\n",(int)MAX_OUT_LINES); return 2; } 
            }
            // drop the cache
            for (int i4=0;i4<isrcbuf;i4++)
               free(aBuf[i4]);
            // reset stats
            isrcbuf=0;
            ipatmatch=0;
            // switch to next command
            icmd2++;
         }
      } else {
         // flush and clear the cache, if any
         for (int i2=0;i2<isrcbuf;i2++) {
            apOut[iout++] = strdup(aBuf[i2]);
            if (iout > MAX_OUT_LINES-10) { log(0, "output cache overflow, %d lines exceeded\n",(int)MAX_OUT_LINES); return 2; } 
            free(aBuf[i2]);
         }
         // flush also current line, which wasn't cached
         apOut[iout++] = strdup(szBuf);
         if (iout > MAX_OUT_LINES-10) { log(0, "output cache overflow, %d lines exceeded\n",(int)MAX_OUT_LINES); return 2; } 
         // reset stats
         isrcbuf=0;
         ipatmatch=0;
      }
   }

   if (icmd2 != icmd) {
      log(0, "error  : from-pattern %d of %d mismatch, in file %s\n",icmd2+1,icmd,pszTargFileName);
      log(0, "info   : check pattern content and SEQUENCE (must match target sequence).\n");
      return 2;
   }

   fclose(ftarg);

 if (!bGlblSimulate)
 {
   // now, we hold the patched target file in apOut.
   // overwrite the target.
   int bSwitchedAttrib = 0;
   cchar *pszFileMode = "w";
   if (bGlblUnixOutput)
         pszFileMode = "wb";
   ftarg = fopen(pszTargFileName, pszFileMode);
   if (!ftarg) {
      // open for write failed? try switching attributes
      #ifdef _WIN32
      sprintf(szProbeCmd, "attrib -R %s",pszTargFileName);
      #else
      sprintf(szProbeCmd, "chmod +w %s", pszTargFileName);
      #endif
      system(szProbeCmd);
      bSwitchedAttrib = 1;
      ftarg = fopen(pszTargFileName, pszFileMode);
   }
   if (!ftarg) {
      log(0, "error  : cannot overwrite target file: %s\n", pszTargFileName); return 2+4;
   }

   // write and free all memory lines
   for (int n=0; n<iout; n++) 
   {
      strcpy(szBuf, apOut[n]);

      // apply local changes, which have priority over globals
      int ipat=0;
      for (ipat=0; ipat<iLocalChange; ipat++) {
         if (   (strstr(szBuf, apLocalChange[ipat][0]) != 0)
             && (strstr(szBuf, apLocalChange[ipat][1]) != 0)
            )
         {
            // replace pattern within line
            if (bGlblVerbose) { printf("lc1>%s",szBuf);fflush(stdout); }

            char *psz = strstr(szBuf, apLocalChange[ipat][1]);
            *psz = 0;
            strcpy(szBuf2,szBuf);
            strcat(szBuf2,apLocalChange[ipat][2]);
            psz += strlen(apLocalChange[ipat][1]);
            strcat(szBuf2,psz);
            strcpy(szBuf,szBuf2);

            if (bGlblVerbose) { printf("lc2>%s",szBuf);fflush(stdout); }

            anLocalChange[ipat]++;
         }
      }

      // apply global changes, if anything left to change
      for (ipat=0; ipat<iGlobalChange; ipat++) {
         if (   (strstr(szBuf, apGlobalChange[ipat][0]) != 0)
             && (strstr(szBuf, apGlobalChange[ipat][1]) != 0)
            )
         {
            // replace pattern within line
            if (bGlblVerbose) { printf("gc1>%s",szBuf);fflush(stdout); }

            char *psz = strstr(szBuf, apGlobalChange[ipat][1]);
            *psz = 0;
            strcpy(szBuf2,szBuf);
            strcat(szBuf2,apGlobalChange[ipat][2]);
            psz += strlen(apGlobalChange[ipat][1]);
            strcat(szBuf2,psz);
            strcpy(szBuf,szBuf2);

            if (bGlblVerbose) { printf("gc2>%s",szBuf);fflush(stdout); }

            anGlobalChange[ipat]++;
         }
      }

      // apply detabbing, if selected
      if (nLocalDetabOutput) {
         if (detabLine(szBuf, szBuf2, MAX_LINE_LEN, nLocalDetabOutput))
            return 9;
      }
      if (nGlblDetabOutput) {
         if (detabLine(szBuf, szBuf2, MAX_LINE_LEN, nGlblDetabOutput))
            return 9;
      }

      // apply unix conversion or not, and finally write
      if (bGlblUnixOutput) {
         char *psz = strrchr(szBuf,'\n'); if (psz) *psz = 0;
               psz = strrchr(szBuf,'\r'); if (psz) *psz = 0;
         fprintf(ftarg, "%s\n", szBuf);
      } else {
         fputs(szBuf,ftarg);
      }

      free(apOut[n]);
   }

   fclose(ftarg);

   // have to re-enable write protection?
   if (bSwitchedAttrib) {
      #ifdef _WIN32
      sprintf(szProbeCmd, "attrib +R %s",pszTargFileName);
      #else
      sprintf(szProbeCmd, "chmod -w %s", pszTargFileName);
      #endif
      system(szProbeCmd);
   }

   if (bSwitchedAttrib) {
      log(5, "Patched: %s, %d lines.\n",pszTargFileName,iout);
   } else {
      log(5, "patched: %s, %d lines.\n",pszTargFileName,iout);
   }
   nGlblPatchedFiles++;
 }
 else {
   log(5, "checked: %s, %d lines.\n",pszTargFileName,iout);
 }

   if (bGlblVerify) {
      log(0, "error  : %s no longer patched - probably overwritten\n",pszTargFileName);
      return 1;
   }

   return 0;
}
