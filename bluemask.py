#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  bluemask.py
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


def find_marker(image):
	# convert the image to grayscale, blur it, and detect edges
	color = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
	lowblue = np.array([94, 80, 2])
	highblue = np.array([126, 255, 255])
	bluemask = cv2.inRange(color, lowblue, highblue)
	#color = cv2.GaussianBlur(redmask, (5, 5), 0)
	#edged = cv2.Canny(redmask, 100, 200)
	# find the contours in the edged image and keep the largest one;
	# we'll assume that this is our piece of paper in the image
	cnts = cv2.findContours(bluemask, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
	cnts = imutils.grab_contours(cnts)
	c = max(cnts, key = cv2.contourArea)
	# compute the bounding box of the of the paper region and return it
	return cv2.minAreaRect(c)
