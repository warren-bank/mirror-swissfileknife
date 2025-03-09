#### Common Configuration

* `bin/.path.bat` and `bin/.path.sh` are optional files
  - they both serve the same purpose, but target different terminals
    * `.bat` is for the Windows .cmd shell
    * `.sh` is for the Bash shell
  - if the one for your terminal is present, then it is called by each recipe
    * its purpose is to add `sfk.exe` to `PATH`
      - note that this naming convention may require renaming the binary executable,<br>depending on your platform and the release assets for your current version
      - if already on `PATH`, then no configuration is needed

#### Recipes

* `bin/ftpd.cmd`
* `bin/ftpd.sh`
  - starts an FTP server on port 21
  - the virtual root directory can be specified in any of the following ways:
    1. passing a directory path on the command-line as the first (and only) parameter
       * special case for Windows:
         - dragging and dropping a directory in Windows Explorer<br>
           onto either `ftpd.cmd`, or any shortcut that points to it
    2. using the current working directory
       * by default
