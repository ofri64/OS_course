#!/bin/bash
rm -rf ${HW1DIR}
mkdir ${HW1DIR}
chmod 755 ${HW1DIR}
cd ${HW1DIR}
cp ${1} ${PWD}/${HW1TF}
chmod 744 ${HW1TF}
cd ../
./hw1_subs file cat
