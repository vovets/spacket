#!/bin/sh -e
[ $# -lt 1 ] && { echo "Expected build dir path"; exit 1; }

wait_enter() {
    local line
    echo $1
    read -p "Press Enter when ready..." line
}
    
cd $1
wait_enter "Please connect serial interface to host and short TX and RX"
ctest --output-on-failure --verbose -R host -E host_target
echo
wait_enter "Please connect jlink and short target's TX and RX"
ctest --output-on-failure --verbose -R target -E host_target
echo
wait_enter "Please connect target's TX and RX to serial interface"
ctest --output-on-failure --verbose -R host_target
