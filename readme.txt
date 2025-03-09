
This is the Swiss File Knife, a support kit for daily development tasks.
SFK was born from the need to work efficiently in a million code line project
with disorganized development, inefficient tracing, and a working force
repeating endlessly "we hope to improve it. since years!".

To get an instant syntax overview, run sfk without parameters.

The main function groups are:

1) to speed up full text search.

   1.1) within files (sfk grep).

        there are, of course, many grep tools around, but most of them
        requiring initial effort (give me DLL xyz, run installer first),
        with stupid defaults (who needs case-sensitive search?), cryptic
        parameters (how to walk the tree?), and unable to grep through binaries.
        let's say you're searching two words, "lemon" and "curry", then type

            sfk grep lemon curry        

        and sfk will:
        -   search through ALL files within the current directory tree,
            and ALL subtrees (recursive tree walk is DEFAULT with sfk).
        -   search case-IN-sensitive (so it finds Lemon, lemon, and LEMON).
        -   dump the results in a nice colored way (colors active by default
            on windows; linux users say "sfk help colors" for details).
        -   search also through BINARY files by default. text is extracted,
            scanned for patterns, and listed.

   1.2) by filtering text streams (sfk filter).

        e.g. you created a logfile of your software, and want to list
        all lines beginning with "BarSys:", and all errors, then say

           sfk filter log.txt -+barsys: -+error:

        again, case-IN-sensitive filtering is DEFAULT, so don't bother
        to type the exact "BarSys:".

2) to fix some garbage, like TAB characters in source-code

   sfk scantab . .hpp .cpp

      scans the whole directory tree and lists all .hpp and .cpp files
      containing TABS.

   sfk detab=3 . .hpp .cpp

      goes through the whole dir tree and replaces TABs by blanks.

3) to change text files in an easy, flexible way through "patch scripts".

   this way you can modify source code permanently without ever checking in.
   the patch scripts are totally human-readable, in the form

      :file BarDriver.cpp
      :from
         *p = 'a';
      :to
         if (*p) *p = 'a';
      :done

   which anyone understands, even without using sfk.

4) to ease the often infinitely complex story of "how to get a file
   from system a to system b" (firewall blocks, network drive fails,
   where to download / how to install ftp server ...) through an instant,
   zero installation ftp server (using also a simplified ftp protocol,
   if connected with sfk ftp client). for details, say

      sfk ftpserv -help

5) and to provide some highly specialized functions, like replacing or reviving
   a dead tracing system by the free micro tracing kernel coming with sfk.

   simply copy the mtk/ dir into your project - it's unlimited open source -
   and insert mtklog calls like

      mtklog("%s [%s]", pszRawBuf, pszModule)

   at the places where your dead tracing system(s) collects messages.
   then you may trace to logfile by setting an env variable:

      set MTK_TRACE=logfile:log.txt

   for more details, read on in mtktrace.hpp.

   furthermore, you may instrument your c++ code to track method
   entries and exits:

   sfk inst mtk/mtktrace.hpp _mtkb_ -dir testfiles !save_ -file .cpp

      -  goes through all .cpp files

      -  identifies C++ method blocks, e.g.

            bool LemonBar::open(char *pszFile)
            {
               clFile = fopen(pszFile, "rb");
               return (clFile != 0);
            }

         and inserts micro tracing block macro calls:

            bool LemonBar::open(char *pszFile)
            {_mtkb_("LemonBar::open");
               clFile = fopen(pszFile, "rb");
               return (clFile != 0);
            }

   this will result in a structured output of your logfile, with indentation
   based on nesting level (per thread). of course, always make sure that you
   DO NOT CHECK IN PATCHED OR INSTRUMENTED SOURCECODE, unless you have
   official approvement from management.

   of course, if your company already uses a well-functioning tracing system,
   congratulations - but i've seen far too many developers forced into printf.


END OF INTRODUCTION - LET'S HAVE EXAMPLES.
==========================================

NOTE: all examples use windows syntax. the sfk linux syntax differs
      at least in the use of two characters ( ! => :  and  $ => # ).


Chapter 1: dynamic source file patching by examples

          HOW TO KEEP LOCAL SOURCE CHANGES FOREVER
      =================================================

the following examples are based on the testfiles/ directory
delivered with the sfk package.

imagine "FooBank" is a big application. you compile and run it.
suddenly, on the console appears:

         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput

a million times, making every normal work impossible.
okay, let's search the reason. you may say

   sfk snapto=all-src.cpp -dir testfiles -file .hpp .cpp

then look through all-src.cpp with your editor, search for
"super important traceoutput" and you will instantly find:

   void BarDriver::runDrawThread( )
   {
      for (long i=0; ;) {
         // dr. stresser: inserted super-important trace output here.
         // do not remove, i need this.
         printf("this is super important traceoutput\n");
      }
   }

so that's the place where the spam output is produced. unfortunately,

-  you have no cvs editing rights for "BarDriver".
   so simply cannot change the code permanently. 

-  you call dr. stresser and ask him to remove the spam.
   he tells you this "spam" is no spam, very important,
   and he's now going on vacation.

-  you call dr. stresser's manager and try to complain.
   you get an answer "that's exactly how it should be.
   this console output is VERY IMPORTANT to us all.
   it will NEVER be changed."

-  you think you're in a bad dream, but it's reality.

okay, so what to do? of course you change the source locally, from hand.
for now, you can work. but the next time you get the latest sources
from cvs... your fix is gone, and again you get:

         this is super important traceoutput
         this is super important traceoutput
         this is super important traceoutput
         ...

so you need to automatize the local changes.

1. APPLYING A PATCH

create a file "c:\patches\remove-spam.cpp" with this content:

------------------- remove-spam.cpp begin ---------------------------

:patch "remove spam from console output of the FooBank project"

:root FooBank

:file BarDriver\source\BarDriver.cpp
:from
void BarDriver::runDrawThread( )
{
   for (long i=0; ;) {
      // dr. stresser: inserted super-important trace output here.
      // do not remove, i need this.
      printf("this is super important traceoutput\n");
   }
}
:to
void BarDriver::runDrawThread( )
{
   /* [patch-id]
   for (long i=0; ;) {
      // dr. stresser: inserted super-important trace output here.
      // do not remove, i need this.
      printf("this is super important traceoutput\n");
   }
   */
}
:done

------------------ remove-spam.cpp end ---------------------------

then say
   cd testfiles\FooBank
   sfk patch c:\patches\remove-spam.cpp

and sfk will
   -  load BarDriver\source\BarDriver.cpp
   -  search for the :from block
   -  replace it with the :to block

now recompile your project, and be productive.

2. RE-APPLYING A PATCH

a few days later, you check out / synchronize to the latest FooBank sources,
and the patch might be overwritten. no problem - just say

   sfk patch c:\patches\remove-spam.cpp -redo

and it's active again. technically
   -  sfk identifies the files still patched,
      and replace them by their backups
   -  those files that have been overwritten are not changed,
      as they are no longer patched
   -  then sfk checks the target files again
      if the patch is still matching the new source files
   -  only if everything is OK, the patch is re-applied.

Note that sfk patchfiles are applied atomic: either ALL :file statements
are executed, or none.

3. REVOKING A PATCH

imagine you may have to EDIT parts of the system that you PATCHED recently.
then strictly follow these steps:

   a) revoke the patch.

      sfk patch c:\patches\comfort.cpp -revoke

   b) checkout/open for edit the target files with cvs,
      enter your changes, test, and check in.

   c) re-apply the patch.

      sfk patch c:\patches\comfort.cpp

Features/advantages of sfk dynamic patching:

-  even if the source code changes daily,
   in most cases the patches can still be applied.
   in the above example, as long as the code of BarDriver::runDrawThread
   itself isn't changed, the patch will work.

-  patches can be undone anytime, sfk creates backups.

-  sfk automatically detects already-patched files.
   per :file, at least one ":to" block MUST contain
   the string [patch-id] in form of a remark,

   e.g. java, c, c++:
        // [patch-id]
        /* [patch-id] */

   e.g. in makefiles you may use:
        # [patch-id]

-  you can play around with a source code base,
   TRY some changes, and if it's not working, UNDO everything.

-  sfk patch files are human readable, self-explaining,
   and can also be used as documentation of a change.

   promoting changes to other developers is very easy,
   even with slightly varying source bases.

   small patches can even be applied from hand,
   without the sfk tool itself.

WARNING:

   before you actually check-in changes into cvs,
   VERIFY that you do NOT check in PATCHED files.
   this needs DISCIPLINE from your side.

   -  remember exactly which files you patched,
      or at least in which directory.

   -  be aware if they mix up with files you have to edit
      to implement new features.

   -  BEFORE CHECKING IN FILES, ALWAYS VERIFY THAT NONE OF THEM
      CONTAIN THE WORD [patch-id] !!!


Chapter 2. using snapfiles

if you have a very large source base, let's say 10,000 files
with more than 2 mio lines of code, it is hopeless to open them
file by file to get an overview. instead, create a "snap file" using:

   sfk snapto=all-src.cpp -dir . -file .hpp .cpp

which simply collects all contents into one large file.
then load THIS into your favourite editor.

in my everyday work, i'm regularly using snapfiles of approx. 100 mbytes.



Chapter 3: cluster file editing by examples

Once you collect many files into one for viewing, the idea of
editing them is obvious, with changes being written back
to the input files automatically. This is what cluster files do.

Note: i have to admit this function is complex. if you're
feeling efficiently enough with your IDE on full text changes
accross many files, skip this chapter.


   HOW TO EDIT ALL FILES OF testfiles/FooBank/ IN REALTIME
   -------------------------------------------------------

cd testfiles\FooBank

sfk syncto=snap-all.cpp -dir testfiles\FooBank -file .hpp .cpp
; -> sfk goes into sync mode, waiting for changes.

1. CLUSTER FILE EDITING:

edit snap-all.cpp using your favourite text editor.
change something, somewhere.
select SAVE in your text editor.
-> sfk detects changes within snap-all.cpp
   and writes them back into the original files.

and why are we using ".cpp" as file extension for snap-all?
simply because a good text editor will then activate
syntax highlighting, and you can continue writing source code
at full comfort within the cluster file.

2. SINGLE FILE EDITING, CLUSTER RELOADING:

edit BarDriver\include\BarDriver.hpp
change something.
select SAVE in your text editor.
-> sfk detects changes in BarDriver.hpp,
   and re-integrates BarDriver.hpp into snap-all.cpp.

   you see a blinking popup information that you
   have to reload the latest snap-all.cpp in your editor.

   every good text editor should auto-detect that snap-all.cpp
   was changed. you may first have to remove the focus
   from your editor window: click on some other application
   (or other window symbol down in the task bar), then again
   on the editor window. now the editor should ask you
   if the file should be reloaded. just confirm this,
   and keep on working.

in general, it doesn't matter if you change snap-all.cpp
or the single files directly. changes are synchronized in both ways.

3. CREATING A WORKING SET SNAPSHOT:

imagine you added a feature to the sources, and it's working,
and you would like to set a "save point" to possibly
jump back later to this code version.
just enter the shell and do a

   copy snap-all.cpp snap-version-1.cpp

4. JUMPING BACK TO A SNAPSHOT:

image you added further features, and they turn out to crash everything. 
you want to revoke the changes, jump back to the good, working source version.

   -  stop sfk syncing by pressing ESCAPE

   -  copy snap-version-1.cpp snap-all.cpp

   -  sfk syncto=snap-all.cpp

notice that now, you do NOT specify "-dir" and "-file".
by doing so, you tell sfk that you want to use exactly
this code version. sfk will automatically load snap-all.cpp,
write all contents back to the target files (DOWN SYNC),
and then go into sync mode.

5. EXTENDING A CLUSTER WITH MORE FILES:

loading snap-all.cpp with your text editor, you see there is
an index at the beginning, listing all files contained:

   :# ----- 6 target files -----
   :edit testfiles\FooBank\BarDriver\include\BarDriver.hpp
   :edit testfiles\FooBank\BarDriver\source\BarDriver.cpp
   :edit testfiles\FooBank\DB\include\DBController.hpp
   :edit testfiles\FooBank\DB\source\DBController.cpp
   :edit testfiles\FooBank\GUI\include\FooGUI.hpp
   :edit testfiles\FooBank\GUI\source\FooGUI.cpp
   :# ----- target index end -----

if you want to add further files, just add another ":edit" line
after the last :edit line (FooGUI.cpp):

   :# ----- 6 target files -----
   :edit testfiles\FooBank\BarDriver\include\BarDriver.hpp
   :edit testfiles\FooBank\BarDriver\source\BarDriver.cpp
   :edit testfiles\FooBank\DB\include\DBController.hpp
   :edit testfiles\FooBank\DB\source\DBController.cpp
   :edit testfiles\FooBank\GUI\include\FooGUI.hpp
   :edit testfiles\FooBank\GUI\source\FooGUI.cpp
   :edit testfiles\BaseLib\Trace\include\Trace.hpp
   :# ----- target index end -----

then select SAVE in your editor, reload the cluster,
and the file is included at the end. note that

   -  you can only add fully specified filenames

   -  every filename needs a single :edit statement

   -  if "reload cluster" doesn't appear,
      you probably misspelled the filename.
      have a look at the console where sfk is running.

6. REDUCING A CLUSTER:

delete lines from the target index, save, reload.

7. CREATING A CLUSTER CONFIG FILE:

imagine you want to create a slightly more "complicated" cluster,
containing these files:

   FooBank\BarDriver\include\BarDriver.hpp
   FooBank\DB\include\DBController.hpp
   BaseLib\Trace\include\Trace.hpp

then the full command can be come rather long:

   cd testfiles
   sfk syncto=snap-heads.hpp -dir FooBank\BarDriver FooBank\DB BaseLib\Trace -file .hpp

so you better out-source this into a file like

   -------------- example: c:\config\snap-heads.sfk ----------------
   -dir
      FooBank\BarDriver
      FooBank\DB
      BaseLib\Trace
      !save_patch
   -file
      .hpp
   -------------- end of c:\config\snap-heads.sfk ----------------

now just say

   cd testfiles
   sfk syncto=snap-heads.hpp -from=c:\config\snap-heads.sfk

note that i added an example how to EXCLUDE directories: if you
say !save_patch like above, potential directories of this name
will not be included. (save_patch are backup dir's created
by sfk patch, more about this following below.)


Cluster files, another example: adding features to project FooBank
------------------------------------------------------------------

imagine FooBank is checked into some cvs. you're working locally
within some dir "work100", and just checked out the project into there.
so you have the files:

   work100\BaseLib\Trace\include\Trace.hpp
   work100\FooBank\BarDriver\include\BarBottle.hpp
   work100\FooBank\BarDriver\include\BarDriver.hpp
   work100\FooBank\BarDriver\include\BarGlass.hpp
   work100\FooBank\BarDriver\include\BarMug.hpp
   work100\FooBank\BarDriver\source\BarBottle.cpp
   work100\FooBank\BarDriver\source\BarDriver.cpp
   work100\FooBank\BarDriver\source\BarGlass.cpp
   work100\FooBank\BarDriver\source\BarMug.cpp
   work100\FooBank\DB\include\DBController.hpp
   work100\FooBank\DB\source\DBController.cpp
   work100\FooBank\GUI\include\FooGUI.hpp
   work100\FooBank\GUI\source\FooGUI.cpp
   
now you want to rework all files of BarDriver, with a massive change
concerning all files. as an example, you want to derive all classes
from bar driver from some new "BarObject". of course you also create
a new file "BarObject.". step by step:

01 check out / open for edit the whole BarDriver tree from cvs.
02 cd work100
03 sfk syncto=100-bar-from-depot.cpp -dir FooBank\BarDriver -file .hpp .cpp

   what you now hold in 100-bar-from-depot is a COPY FROM CVS.
   save this point so that you may jump back later. to do so,

04 exit sfk by pressing ESCAPE
05 copy 100-bar-from-depot.cpp 110-bar-add-barobject.cpp

   and why do we prefix all filenames by a number? to speed up working
   through command line autocomplete. let's say your editor is "edit",
   then you type this
   
      edit 110{PRESS TABKEY}
   
   and this is instantly expanded to 
   
      edit 110-bar-add-barobject.cpp
   
06 by the way, have you fully configured your Windows Command Line environment?
   depending on the system you're using (Win2K, WinXP) autocomplete may not
   be active. to activate it manually, use
   
      regedit
         HKEY_CURRENT_USER\Software\Microsoft\Command Processor
            => set CompletionChar to value 9
   
   furthermore, set the layout of your command prompt like this:

      properties
         Edit Options
            QuickEdit Mode ACTIVE
            Insert Mode ACTIVE
   
      properties
         layout
            screen buffer size
               width: 160
               height: 3000
            window size
               width: 160
               height: 31
   
   now you have a power shell with 3000 lines of scrollback - essential
   for effective working. i also recommend a 7 x 12 raster font.
   
   anyway, we did copy a source snapshot, and will now start editing.
   in the following, replace "edit" by your favourite editor command.

   first restart sfk syncing, but now on the new file:

07 sfk syncto=110-bar-add-barobject.cpp

08 open another command shell, place the sfk shell to the top of screen.
   so you always have two shells:
   -  in the upper you see sfk status infos
   -  in the lower, you're editing and compiling.

09 edit 110-bar-add-barobject.cpp

   you now see the cluster. it contains this index:

   :# ----- 8 target files -----
   :edit FooBank\BarDriver\include\BarBottle.hpp
   :edit FooBank\BarDriver\include\BarDriver.hpp
   :edit FooBank\BarDriver\include\BarGlass.hpp
   :edit FooBank\BarDriver\include\BarMug.hpp
   :edit FooBank\BarDriver\source\BarBottle.cpp
   :edit FooBank\BarDriver\source\BarDriver.cpp
   :edit FooBank\BarDriver\source\BarGlass.cpp
   :edit FooBank\BarDriver\source\BarMug.cpp
   :# ----- target index end -----

10 you may now go through the cluster source and apply global changes
   of this kind

      from: class BarBottle
      to  : class BarBottle : public BarObject

   as soon as you select SAVE within your editor, sfk auto-syncs
   your changes into all the above listed files.

   now, the class BarObject itself is still missing.
   create 2 new files

      FooBank\BarDriver\include\BarObject.hpp
      FooBank\BarDriver\source\BarObject.cpp

   with some code in it. to add these to the cluster,

11 expand the above index section in the editor this way:

   :# ----- 8 target files -----
   :edit FooBank\BarDriver\include\BarBottle.hpp
   :edit FooBank\BarDriver\include\BarDriver.hpp
   :edit FooBank\BarDriver\include\BarGlass.hpp
   :edit FooBank\BarDriver\include\BarMug.hpp
   :edit FooBank\BarDriver\source\BarBottle.cpp
   :edit FooBank\BarDriver\source\BarDriver.cpp
   :edit FooBank\BarDriver\source\BarGlass.cpp
   :edit FooBank\BarDriver\source\BarMug.cpp
   :edit FooBank\BarDriver\include\BarObject.hpp
   :edit FooBank\BarDriver\source\BarObject.cpp 
   :# ----- target index end -----

   as you see, new files are always added at the end.
   select SAVE in your editor - sfk will integrate
   the new files into the cluster, and ask you to reload it:

12 to reload the cluster, you may have to click into any
   window not being the editor (or the desktop, or the editor's
   iconified window in the task bar), and then again
   on the editor window. at least with ultraedit, you're now 
   asked automatically if the editor may reload the cluster.

13 now go the end of the cluster - you see contents were added.
 
   exceptions: if you do NOT a request to reload the cluster,
   something went wrong, e.g. a typo in the index section.
   then have a look into the sfk shell area. files it cannot load
   are simply ignored until the index section is fixed by you.

14 compile your code. you may get an error like

      BarGlass.cpp line 15: wrong syntax

   now, if you want to jump to that line directly, you cannot
   do this within the cluster - line numbers are different there.

   so what to do? you may edit barglass.cpp directly

15 edit FooBank\BarDriver\source\BarGlass.cpp

   and apply some changes, then select SAVE in your editor.
   -  sfk will autodetect that BarGlass.cpp was changed.
   -  the file is reintegrated into the cluster.
   -  you get a request to reload the cluster.

   as long as you're changing just and only BarGlass.cpp,
   simply ignore the cluster reload requests (maybe drop
   the cluster temporarily from your editor).

   as soon as BarGlass.cpp fully compiles, reload the cluster
   and continue working there.

   in general,
   -  editing the cluster is best for massive global changes.
   -  but when it comes to compiling, you may be better off
      by editing the target files directly.

15.2 automatic line number mapping

   since sfk 1.0.2, you may also have your compiler output
   parsed directly and error line numbers mapped to the cluster.
   to do so, write a batch, e.g. do-compile.bat:

      make FooBank.make 1>err.txt 2>&1
      sfk mapto=110-bar-add-barobject.cpp <err.txt

   this will
      -  write the compiler stdout and stderr to err.txt
         (depending on your compiler, just stderr "2>err.txt"
         might be sufficient)
      -  then, sfk reads from err.txt and
         -  searches for filenames and nearby numbers
         -  searches for matching filenames in the cluster
         -  re-calculates the line numbers
         -  dumps the input (err.txt) mixed with the mapping results
            to the console. you may also tell sfk to dump only
            the mapping results by specifying -nomix.

15.3 automatically jump into the editor, to the first error

   since sfk 1.0.3, you may add another option to "mapto",
   telling sfk to execute a command with the first identified error.

      make FooBank.make 1>err.txt 2>&1
      sfk mapto=110-bar-add-barobject.cpp -cmd=c:\tools\jumpline.ahk <err.txt

   this will run the script "jumpline.ahk" with two parameters,
   the cluster name 110-bar-add-barobject.cpp and the line number of the 
   first error identified in the compiler output.

   now, if you download and install the AutoHotkey GUI automation tool,
   and provide a script "c:\tools\jumpline.ahk" with this content:

      nErrFile = %1%
      nErrLine = %2%
      IfWinExist,YourEditor
      {
         WinActivate,YourEditor,,2
         WinWaitActive,YourEditor,,2
         WinMenuSelectItem,YourEditor,,Search,Goto Line
         WinWaitActive,Goto,,2
         Send,%nErrLine%{ENTER}
         WinWaitActive,YourEditor,,2
         Send,{HOME}{UP}+{DOWN}+{DOWN}+{DOWN}
      }

   then the following will happen automatically:
   on the first compile error encountered,

      -  the window of your editor is activated.
      -  the menu command "Search / Goto Line" is selected.
      -  the line number is entered.
      -  three lines of context are marked.

   of course you have to adapt the script to the name
   and menu structure of your favourite text editor.

   now, back to the working process. feature completed, what next?
   if you don't want to check in yet, create a savepoint:

16 copy 110-bar-add-barobject.cpp 120-bar-next.cpp

   now continue adding the next code.
   if this code turns out to work fatally wrong, jump back
   to the 110 cluster this way:

17 jumping back to 110-bar-add-barobject.cpp

   -  exit sfk by pressing ESCAPE

   -  you may not want to work on 110 directly, as you don't want
      to modify this savepoint. so make a copy from it, e.g.

      copy 110-bar-add-barobject.cpp 130-bar-retry.cpp

   -  then re-sync to this through

         sfk syncto=130-bar-retry.cpp

      with this syntax, sfk automatically
         -  loads 130-bar-retry.cpp
         -  writes all contents to the targets without
            considering any file dates (forced DOWN SYNC)
         -  of course all target files must still be writeable
            to allow this.


Chapter 4. a working process for handling huge parallel codebases

imagine a project BigBank is checked into cvs, with more than 10,000
source files. you have five versions of it on your local PC under:

   d:\work150\BigBank\...
   d:\work151\BigBank\...
   d:\work152\BigBank\...
   d:\work160\BigBank\...
   d:\work170\BigBank\...

and you want to be able to jump between these versions anytime.
in the following examples, you keep all your batch files in O:\batch
and of course have O:\batch in your PATH.


   setting up base batches

10 create a file O:\batch\setcurbank.bat

      @echo off
      echo >O:\batch\setcur.bat set VCURRENT=work%1\BigBank
      call O:\tools\bin\setcur.bat

   create a file O:\batch\cdcur.bat

      @echo off
      IF "%VCURRENT%"=="" goto xend
      d:
      cd \%VCURRENT%
      :xend

   now, to select a specific codebase, e.g. 152, you say

      setcurbank 152
      cdcur

   and afterwards, e.g. after opening new shells, just

      setcur
      cdcur

   in both cases you're landing within d:\work152\BigBank

  

20 setting up source collector batches

   create a file O:\batch\jamsrc2.bat

      @ECHO off
      IF "%VCURRENT%"=="" goto xend
      sfk snapto=D:\%VCURRENT%\all-src.cpp -dir D:\%VCURRENT% !\save_patch\ -file .cpp .c .hpp .h .xml
      dir D:\%VCURRENT% /S /B >D:\%VCURRENT%\lslr
      :xend

   create a file O:\batch\jamsrc.bat

      @ECHO off
      IF "%VCURRENT%"=="" goto xend
      sfk snapto=all-src.cpp -dir . !save_patch\ -file .cpp .c .hpp .h .xml
      :xend

   create a file O:\batch\eall.bat

      @echo off
      IF EXIST D:\%VCURRENT%\all-src.cpp (
         edit D:\%VCURRENT%\all-src.cpp
      ) ELSE (
         echo "you forgot to jamsrc2"
      )

21 (re)creating the global snapfile

   now, no matter where you are within the file work152 file tree,
   just say

      jamsrc2

   and a GLOBAL snapfile containing ALL 10,000 source files of the project
   will be created. on a pentium IV system, this may take between 30 sec
   and a few minutes. when this is done,

22 loading the global snapfile
   just type

      eall

   and your editor loads the global snapfile of your current codebase
   (takes about 5 sec for a 100 mbyte file using ultraedit).

23 creating a local snapfile

   often, you only want an overview of sources within a local path.
   e.g. you're within BigBank\CoreDriver\HAL and want to read
   through all files from this point on.
   then just say

      jamsrc

   and a local snapfile under HAL\ is created. then

      edit all-src.cpp

   and there you have it.


24 search for class declarations in the global snapfile

   create a file O:\batch\getclass.bat

      @IF "%VCURRENT%"=="" goto xend
      sfk filter -lnum -c <D:\%VCURRENT%\all-src.cpp "+ls+class " ++%1 "-!;"
      :xend

   then, whenever you need to lookup a class you haven't heard of before,
   type for example

      getclass CVirtualCurrencyLayer

   and you will get a listing of line-numbers and class names.
   an experienced developer will quickly identify the relevant line.
   now,
   -  in the shell, mark the relevant line number through double-click;
      (requires Edit Mode active, see use case 01, point 05)
   -  enter your editor
   -  select GO TO LINE NUMBER
   -  press SHIFT+INSERT

   this way you quickly jump to any class declaration, even in codebases
   with millions of lines.

