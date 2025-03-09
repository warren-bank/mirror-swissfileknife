
This is the Swiss File Knife (SFK), a command line multi function tool
created by StahlWorks Technologies, http://stahlworks.com/

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

   Download the executables for Windows, Mac OS/X, or desktop Linux

   By browser:

   -  http://stahlworks.com/sfk/

      then click on one of the top links to either download
      binaries instantly, or look further on sourceforge:

   -  http://sourceforge.net/projects/swissfileknife/

   OR

   Apple Macintosh:

      The Mac 64-bit intel binary is available from:

      -  curl -o sfkmac http://stahlworks.com/sfkmac

      Alternatively, a self compile under Mac is done by
      getting the sources as described below, then type

         g++ sfk.cpp sfkext.cpp sfkpack.cpp -o sfk

   OR

   Desktop Linux:

      The Linux i686 64-bit binary is available from:

      -  wget http://stahlworks.com/sfkux

   OR
   
   Any system:

      Get the whole distribution package in a .zip:

      -  wget http://stahlworks.com/sfk.zip

      Then extract this, and compile by:

         g++ sfk.cpp sfkext.cpp sfkpack.cpp -o sfk

   OR

   -  working on a machine without internet access, network folders,
      non-functioning USB ports, missing admin rights, and a thousand
      other reasons why you cannot copy a single file?

      if the machine has ANY connection to a local network, try this:

      SFK Instant HTTP Server for easy file exchange

         -  on another machine where you have SFK already, type
   
            sfk webserv -port=9090
   
         -  then, on the target machine, try to open a web browser
            and to access
   
               http://othermachine:9090/
   
            OR type
   
               wget http://othermachine:9090/sfk.zip
   
            provided that you have the SFK distribution zip file 
            located in the directory where you typed "sfk webserv"

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
               get sfk.zip

            provided that you have the SFK distribution zip file 
            located in the directory where you typed "sfk ftpserv".

         -  if ftp connections fail to work, check if the "ftp"
            client on the target accepts the command

               passive

            then try to "get" again (ftp creates a new connection
            per file download, which is often blocked by firewalls.
            the passive command changes the way in which those
            connections are created.)


How to prepare the SFK binary under Linux/Mac:

   -  after download, you have to rename it like

         mv sfkux sfk

      and you must enable execution by

         chmod +x sfk

      Then simply type

         ./sfk

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

   Recommendation for Linux/Mac:

      -  type "cd" then "pwd" to find out what your account's
         home directory is.

      -  within your home directory (e.g. /home/users/youruserid/)
         create a directory "tools"

            mkdir tools

         then rename sfkux to sfk, and copy that
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

   -  the source code provided in the sfk.zip download packages
      is provided under BSD license, and therefore free for unlimited
      use also in commercial projects, without any warranty.

   Read the file bsd-license.txt for details.


See also:
=========

   -  Swiss File Knife Extended Edition (XE) is the commercial version
      of SFK, allowing direct reading of (nested) .zip, .tar.gz and
      .tar.bz2 file contents, and high performance replace of patterns
      in text and binary files. Read more under:

         http://stahlworks.com/sfkxe.html

   -  The whole SFK source code and documentation was created with
      Depeche View, the world's fastest text file browser and editor. 
      A free demo is available under:

         http://stahlworks.com/depeche-view.html

   -  The whole SFK documentation is available as an e-book,
      optimized for reading on mobile devices.
      For details see www.stahlworks.com

   -  The whole SFK documentation is available as the paperback book
      "100 Command Line Tools" on Amazon

The SFKTray GUI Status Display
==============================

   is a small Windows tool installed by sfktray-set-up.exe.
   the Freeware edition displays four independent status lights
   in the system tray which can be controlled by the sfk status 
   command like:

      sfk status local "v1 slot=1 color=green text='service ok'"

   type "sfk status" for the full syntax.
   a status can be sent from the local or a remote machine by UDP.
   the full edition with 9 status lights is available from:
   
      http://stahlworks.com/sfktray

