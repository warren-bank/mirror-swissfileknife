#ifndef _MTKTRACE_DEFINED_
#define _MTKTRACE_DEFINED_

/*
   Micro Tracing Kernel 0.5.5 by stahlworks technologies.
   Unlimited Open Source License, free for use in any project.

   NOTE: changing this header probably forces recompile of MANY files!

   export MTK_TRACE=ring:twe,term:te,filename:log.txt

   20060420 Win32 adaptions
   20060411 added example remark
*/

extern void mtkTraceMethodEntry  (void *p, const char *psz, int n, const char *pszfn);
extern void mtkTraceMethodExit   (void *p, const char *psz, int n, const char *pszfn);

class MTKBlock {
public:
   MTKBlock(const char *pszfile, int nline, const char *pszfn) {
      mfile = pszfile;
      mline = nline;
      mfunc = pszfn;
      mtkTraceMethodEntry(this, mfile, mline, mfunc);
   }
   ~MTKBlock() {
      mtkTraceMethodExit(this, mfile, mline, mfunc);
   }
   void dummy() { }
private:
   const char *mfile;
   int mline;
   const char *mfunc;
};

#define _mtkb_(mtkbfn) MTKBlock tmp983451(__FILE__,__LINE__,mtkbfn);tmp983451.dummy();

extern void mtkTraceRaw(const char *pszFile, int nLine, char cPrefix, char *pszRaw);
extern void mtkTraceForm(const char *pszFile, int nLine, char cPrefix, const char *pszFormat, ...);
extern int  mtkTracePre(const char *pszFile, int nLine, char cPrefix);
extern void mtkTracePost(const char *pszFormat, ...);
extern void mtkDumpStackTrace(int bOnlyOfCurrentThread);
extern void mtkDumpLastSteps(int bOnlyOfCurrentThread);
extern void mtkSetRingTrace(char *pszMask);
extern void mtkSetTermTrace(char *pszMask);
extern void mtkHexDump(const char *pszLinePrefix, const char *pDataIn, long lSize, const char *pszFile, int nLine, char cPrefix);

#ifdef _WIN32
 #define mtklog mtkTracePre(__FILE__,__LINE__,'D'),mtkTracePost
#else
 #define mtklog(form, args...) mtkTraceForm(__FILE__,__LINE__,'D',form, ##args)
#endif
#define mtkdump(pLinePrefix, pData, nSize) mtkHexDump(pLinePrefix, pData, nSize, __FILE__,__LINE__,'T')

/*
// example for redirecting a foreign printf-like trace macro to mtk:
#undef BADTRACE
extern int  mtkTracePre (const char *pszFile, int nLine, char cPrefix);
extern void mtkTracePost(const char *pszFormat, ...);
#define BADTRACE(xwrap) do { if (mtkTracePre(__FILE__,__LINE__,'E')) mtkTracePost xwrap; } while(0)
*/

#endif
