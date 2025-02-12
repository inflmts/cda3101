#!/bin/sh

set -ue
ssh_command='set -e;
rm -rf ~/workdir
mkdir ~/workdir
cd ~/workdir
printf '\''\033[1m==> Extracting...\033[0m\n'\''
tar -xvf-
sh test.sh --emulator'

if [ "${1-}" != --emulator ]; then
  tar -c pa1.txt test.sh | ssh -p 3101 root@localhost "$ssh_command"
  exit
fi

printf '\033[1m==> Compiling...\033[0m\n'
gcc -o pa1 -x assembler pa1.txt

test_case() {
  local input="$1" result="$2"
  echo "** test: '$input'"
  echo "$input" | stdbuf -oL ./pa1 | tee output
  if cmp -s - output <<EOF
Input a string: String length: ${#input}
String is a palindrome (T/F): $result
EOF
  then
    printf '\033[1;32m** pass\033[0m\n'
  else
    printf '\033[1;31m** fail\033[0m\n'
  fi
}

printf '\033[1m==> Testing...\033[0m\n'
test_case 'racecar' T
test_case 'cannon' F
test_case 'Racecar' F
test_case 'r' T
test_case '' T
test_case 'catac' T
test_case 'cattac' T
test_case 'cataclysm' F
