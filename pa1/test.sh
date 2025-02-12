#!/bin/sh

set -ue
space=' '
ssh_command='set -e;
rm -rf ~/workdir
mkdir ~/workdir
cd ~/workdir
tar -xvf-
sh test.sh --emulator'

if [ "${1-}" != --emulator ]; then
  tar -c pa1.txt test.sh | ssh -p 3101 root@localhost "$ssh_command"
  exit
fi

echo "==> Compiling..."
gcc -o pa1 -x assembler pa1.txt

test_case() {
  local input="$1" result="$2"
  echo "** test: '$input'"
  echo "$input" | ./pa1 | tee output
  if cmp -s - output <<EOF
Input a string:$space
String length: ${#input}
String is a palindrome (T/F): $result
EOF
  then
    echo "** pass"
  else
    echo "** fail"
  fi
}

echo "==> Testing..."
test_case racecar T
test_case cannon F
test_case Racecar F
test_case r T
test_case '' T
