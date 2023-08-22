#!/bin/bash
###
 # @Description: 
 # @version: 1.80.1
 # @Author: ZGG
 # @Date: 2023-08-22 17:40:35
 # @LastEditors: ZGG
 # @LastEditTime: 2023-08-22 17:40:44
### 
clear
gcc client.c userinfo.c  -o client -I ../include/ -lpthread
./client