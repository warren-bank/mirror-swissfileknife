
The Swiss File Knife is a text file processor intended for developers,
allowing some high performance working techniques, based on working
with the command line.

NOTE: all examples use windows syntax. the sfk linux syntax differs
      at least in the use of two characters ( ! => :  and  $ => # ).


Chapter 1: cluster file editing by examples

   HOW TO EDIT ALL FILES OF testfiles/FooBank/ IN REALTIME
   =======================================================

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

7. CREATING A VIEW CONFIG FILE:

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

8. CREATING A READ-ONLY CLUSTER ("SNAPFILE")

if you have a very large source base, let's say 10,000 files
with more than 2 mio lines of code, you cannot use "syncto"
to collect them - i haven't tried it, but most probably
the memory will break down.

but you can create a read-only "snap file" using:

   sfk snapto=all-src.cpp -dir . -file .hpp .cpp

which simply collects all contents into one large file.
in my everyday work, i'm regularly using snapfiles of approx.
100 mbytes - a good editor (e.g. ultraedit) can load this quickly,
allowing realtime searches (<= 5 sec per lookup) even within
the largest codebases.

if you start using snapfiles, i also recommend that you
-  duplicate the editor window, creating two views onto the same file
   (without loading it twice, of course)
-  tile vertical, e.g. have both windows using half of the screen
so if you find something on the left side, you can run
a second search on the right side without keeping track
of the first place.





Chapter 2: dynamic source file patching by examples

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

so that's the place where the spam output is produced.
unfortunately,

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

okay, so what to do? of course you change the source locally,
from hand. for now, you can work. but the next time you
get the latest sources from cvs... your fix is gone,
and again you get

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

imagine dr. stresser's manager walks in some day and needs
a demo run of your code, instantly! just before he screams
"the superimportant output is gone!?!!?", do a

   sfk patch c:\patches\remove-spam.cpp -revoke

with a short recompile, and your fix is gone.

Features/advantages of sfk dynamic patching

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


More documentation / examples can be found within "working.txt".
