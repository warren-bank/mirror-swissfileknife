#!/bin/bash
#
# XMakeTarg - instant command line FTP Server for targets.
# Receive binary file updates, instantly, without
# complex user configuration or certificate setup.
# requires:
#   sfk on the target, 32 bit ARM download by
#           wget http://stahlworks.com/sfkarm
#     or    curl -o sfk http://stahlworks.com/sfkarm
#     then  mv sfkarm sfk; chmod +x sfk
# Store and edit this using UNIX LINE ENDINGS (LF only).

# === server parameters ===
export SFK_FTP_PW=mybinpw456

# === 1. run build server for one user ===
sfk sftserv -rw -run -port=2201
