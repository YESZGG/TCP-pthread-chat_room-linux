#!/bin/bash
clear
gcc server.c -o server -I ../include/ -lpthread
./server