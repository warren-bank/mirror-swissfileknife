#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -f "${DIR}/.path.sh" ];then
  source "${DIR}/.path.sh"
fi

if [ -z "$1" ];then
  ftp_root_dir=$(pwd)
else
  ftp_root_dir="$1"
fi
ftp_root_dir=$(realpath "$ftp_root_dir")

sfk.exe ftpserv -port=21 -timeout=3600 -rw -noclose -usedir "$ftp_root_dir"
