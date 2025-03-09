
This is the Swiss File Knife (SFK), a command line multi function tool,
made by Vincent Stahl, StahlWorks Art & Technology, http://stahlworks.com/

SFK was made to make things easy.

File exchange between machines, find duplicates, find and replace text,
list directory tree sizes, and tons of other functions for daily tasks -
it's all contained within a single, truely portable executable.

Forget about installing dozens of tools on every new machine,
battling with missing or wrong versioned DLL's, missing admin rights,
missing package dependencies, user account configurations,
spammed registries and endless time wasting in general.

On every new Windows/Linux PC, Notebook, Workstation or Server,
just download or copy the SFK binary and use it - INSTANTLY.

How to get SFK up and running anywhere
--------------------------------------

   Download the executables for Windows, Linux/lib6 or Linux/lib5

   By browser:

   -  http://stahlworks.com/dev/swiss-file-knife.html

      then click on one of the top links to either download
      binaries instantly, or look further on sourceforge:

   -  http://sourceforge.net/projects/swissfileknife/

   OR

   On a Linux text console, use one of these:

      Instant binaries:

      -  wget http://stahlworks.com/dev/sfk/sfk       (Linux lib6)
      -  wget http://stahlworks.com/dev/sfk/sfk-lib5  (Linux lib5)

      Whole distribution package in a .zip (insert version for nnn):

      -  wget http://stahlworks.com/dev/sfk/sfknnn.zip

      Make sure your system has the real Info-ZIP unzip command,
      in version 5.50 or higher (just type "unzip" to find out).
      If nothing is available, get one of these:

      -  wget http://stahlworks.com/dev/unzip-linux-lib6.exe
      -  wget http://stahlworks.com/dev/unzip-linux-lib5.exe

   OR

   Apple Macintosh and 64-bit Linux in general:

   -  there are no binary distributions so far, but you may compile
      the source code easily, even if you're no software developer.

      first, type "g++" to find out if a compiler exists on your system.
      if so, download the source code, contained in the sfknnn.zip, from

      http://sourceforge.net/projects/swissfileknife/

      then extract that by "unzip sfknnn.zip",
      and take a look into do-compile-unix.bat or type:

         Mac compile:

            g++ -DMAC_OS_X sfk.cpp patch.cpp inst.cpp -o sfk

         Linux 64-bit compile:

            g++ sfk.cpp patch.cpp inst.cpp -o sfk

   OR

   -  working on a machine without internet access, network folders,
      non-functioning USB ports, missing admin rights, and a thousand
      other reasons why you cannot copy a single file?

      if the machine has ANY connection to a local network, try this:

      SFK Instant HTTP Server for easy file exchange

         -  on another machine where you have SFK already, type
   
            sfk httpserv -port=9090
   
         -  then, on the target machine, try to open a web browser
            and to access
   
               http://othermachine:9090/
   
            OR type
   
               wget http://othermachine:9090/sfknnn.zip
   
            provided that you have the SFK distribution zip file 
            located in the directory where you typed "sfk httpserv"

      If that fails (no browser, no gui, no wget command),
      check if there exists an "ftp" command on the target.
      If so, try:

      SFK Instant FTP Server for easy file exchange

         -  on another machine where you have SFK already, type
   
            sfk ftpserv
   
         -  then, on the target machine, type
   
               ftp othermachine

            and if the login succeeds, try

               dir
               bin
               get sfknnn.zip

            provided that you have the SFK distribution zip file 
            located in the directory where you typed "sfk ftpserv".
            Of course you may also try

               get sfk-linux.exe
               get sfk-linux-lib5.exe

            if your target machine is a linux system.

         -  if ftp connections fail to work, check if the "ftp"
            client on the target accepts the command

               passive

            then try to "get" again (ftp creates a new connection
            per file download, which is often blocked by firewalls.
            the passive command changes the way in which those
            connections are created.)


How to prepare the SFK binary under Linux:

   -  after download, you have to type

         chmod +x sfk-linux.exe

      to enable execution (the 'x' flag) of sfk.
      Then simply type

         ./sfk-linux.exe

      to get it running (the "./" is often needed as
      the PATH may not contain the current directory ".").               

Where to place the SFK executable:

   Recommendation for Windows:

      -  create a directory structure

            c:\app\bin

         then rename sfknnn.exe to sfk.exe, and copy that to c:\app\bin .

         If you use the "sfk alias" command later, the created alias
         batch files will also be placed into "c:\app\bin".

      -  extend the Windows Shell Path

            set PATH=%PATH%;c:\app\bin

      -  make sure your Windows Shell supports command autocompletion
         and Copy/Paste of text (the QuickEdit and Insert setting),
         otherwise it is very hard to use! read more by typing

            sfk help shell

      -  further tools can be installed parallel to "bin" into c:\app

      This way you avoid the long, blank-character-contaminated,
      inefficient default paths like "C:\Program Files".

   Recommendation for Linux:

      -  type "cd" then "pwd" to find out what your account's
         home directory is.

      -  within your home directory (e.g. /home/users/youruserid/)
         create a directory "tools"

            mkdir tools

         then rename sfk-linux.exe to sfk, and copy that
         into the tools dir.

      -  extend the PATH like

            export PATH=$PATH:/home/users/youruserid/tools

      -  then you should be able to run sfk by typing "sfk".

      By default, there are no colors, as it is not possible
      to autodetect the background color under Linux.

      If you like colorful output, type "sfk help color"
      and read on how to set the "SFK_COLORS" variable
      (read it careful! if you set a wrong "def:" value,
       you may end up with white text on white background)


License:
========

   -  Swiss File Knife Base is provided completely for free,
      and can be used unlimited, without any warranty.

   -  the source code provided in the sfknnn.zip download packages
      is provided under BSD license, and therefore free for unlimited
      use also in commercial projects, without any warranty.

   Read the file bsd-license.txt for details.


See also:
=========

   -  Swiss File Knife Extended Edition (XE) is the commercial version
      of SFK, allowing direct reading of (nested) .zip, .tar.gz and
      .tar.bz2 file contents.

         http://stahlworks.com/dev/sfkxe.html

   -  Depeche View for Windows is a realtime text browser
      that allows instant repeated searches across thousands
      of text files, especially source code.

      The Base edition (provided also within the sfknnn.zip file)
      is free for private, non-commercial use.

      Depeche View Extended Edition (XE) also provides an integrated
      text editor, allowing search and edit on the fly. Furthermore, 
      it can view .zip, .tar.gz and .tar.bz2 contents directly, 
      without the need for extraction - allowing to research downloaded 
      open source packages instantly with the least possible effort.

         http://stahlworks.com/dev/dvxe.html

   -  Vincent Stahl: Computer Art of the 21st century is provided as
      posters, mousepads, suitcase stickers, coffee mugs, CGI movies,
      desktop backgrounds and ring tones under:

         http://stahlworks.com/
