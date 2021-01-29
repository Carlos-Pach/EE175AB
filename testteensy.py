#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  testteensy.py
#  
#  Copyright 2021  <pi@raspberrypi>
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
#  


from smbus import SMBus

addr = 0x8
bus = SMBus(1)

numb = 1

print("Enter 1 for ON or 0 for OFF")
while numb == 1:
    
    ledstate = input(">>>>  ")
    
    if ledstate == "1":
        bus.write_byte(addr, 0x20)
    elif ledstate == "0":
        bus.write_byte(addr, 0x0)
    else:
        numb = 0
