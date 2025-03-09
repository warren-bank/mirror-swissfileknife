#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

data_dir="${DIR}/data/"
github_repo="${DIR}/repo/"

if [ ! -d "$data_dir" ];then
  echo 'ERROR: data directory for file downloads does not exist'
  exit 1
fi

[ -d "$github_repo" ] || mkdir "$github_repo"
cd "$github_repo"

[ -d '.git' ] || git init >/dev/nul 2>&1

unzip_file() {
  fname="$1"
  zipfile="${data_dir}${fname}"

  if [ ! -s "$zipfile" ];then
    return 1
  fi

  tld=${fname:0:6}
  unzip -qq "$zipfile" -d .
  mv ./"$tld"/* ./
  rm -d ./"$tld"
  return 0
}

while IFS= read -r line; do
  line_arr=($line)

  date=${line_arr[0]}
  version=${line_arr[1]}
  fname=${line_arr[2]}

  echo "date:    '${date}'"
  echo "version: '${version}'"
  echo "fname:   '${fname}'"
  echo ''

  # cleanup repo working tree
  git rm -rf . >/dev/nul 2>&1

  # unzip release into repo working tree
  unzip_file "$fname"
  if [ $? -ne 0 ];then
    echo "${version} zip file not found. Skipping.."
    echo ''
    continue
  fi

  echo "${version} unzipped into working tree."
  echo ''

  # pause to allow manual editing of repo working tree before commit
  echo 'Perform any manual editing of repo working tree before commit.'
  echo ''
  echo 'List of deleted executables:'
  find . -type f -name '*.exe' -delete -print -o -type f -name '*.dmg' -delete -print
  echo ''

  # Additional automatic deletions
  [ -d 'macbin' ] && rm -rf 'macbin'
  [ -d 'uxbin'  ] && rm -rf 'uxbin'
  find . -type f -name '*.dvbm' -delete

  read -p 'Press any key to continue.. ' -n1 -s 0</dev/tty
  echo ''
  echo ''

  git add --all .
  git commit -m "[${date}] release for version ${version}"
  git tag "$version"
  echo "${version} committed from working tree to index"
  echo "${version} tagged"
  echo ''
  
done < "${DIR}/data.txt"

echo 'Done!'
echo 'Please remember to add remotes to the new git repo, and push.'
echo ''
