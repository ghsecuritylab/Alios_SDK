fn=/tmp/log.txt
if [ -n "$1" ];then
fn=$1
fi

grep Backtrace: $fn|sed 's/Backtrace: //'|sed 's/:0x[0-9a-z]*\s/ /g'|sed 's/:.*$//'|xargs xtensa-esp32-elf-addr2line -f -e out/helloworld@esp32devkitc/binary/helloworld@esp32devkitc.elf

