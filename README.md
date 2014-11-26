Android I2C dev tools
=======================

I statet with I2C bus scan utility for Arduino based upon i2c_scanner from:
    http://playground.arduino.cc/Main/I2cScanner

This utility is meant to replicate the basic functionality of the
Linux i2c tools (familiar to Raspberry Pi users, etc.)

I took some ideas from https://github.com/MartyMacGyver/Arduino_I2C_Scanner

In the momend it can
 -detect devices
 -dump
 -get regester
 -set register
It only work's with 8-bit data and addresses
