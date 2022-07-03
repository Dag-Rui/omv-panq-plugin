#!/bin/bash
(cd panq-fan-service && make clean && make panq)
cp panq-fan-service/panq omv-panq-plugin/usr/bin/panq
rm -r ~/omv-panq-plugin
cp -r omv-panq-plugin ~
cd ~
chmod -R 775 omv-panq-plugin
dpkg --build omv-panq-plugin
dpkg -r openmediavault-panq-control
dpkg -i omv-panq-plugin.deb
