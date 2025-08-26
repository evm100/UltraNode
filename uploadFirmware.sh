#!/bin/bash

. $HOME/esp-idf/export.sh

rm -r build

idf.py build
sudo mv /srv/firmware/UltraNode2/old.bin
sudo cp build/UltraLightsNodeV2.bin /srv/firmware/UltraNode2/latest.bin
