#!/bin/bash

sudo pqos -I -e "llc:1=0x003;llc:0=0x7fc;"
sudo pqos -I -a "llc:1=0;llc:0=1-79;"
