#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  distance_to_camera.py
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

# import the necessary packages
import numpy as np
import imutils
import cv2
        
def find_marker(image):
        # convert the image to grayscale, blur it, and detect edges
        color = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
        lowred = np.array([161, 155, 84])
        highred = np.array([179, 255, 255])
        redmask = cv2.inRange(color, lowred, highred)
        #color = cv2.GaussianBlur(redmask, (5, 5), 0)
        #edged = cv2.Canny(redmask, 100, 200)
        # find the contours in the edged image and keep the largest one;
        # we'll assume that this is our piece of paper in the image
        cnts = cv2.findContours(redmask, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
        cnts = imutils.grab_contours(cnts)
        if len(cnts) > 0:
                c = max(cnts, key = cv2.contourArea)
                return cv2.minAreaRect(c)
        else:
        # compute the bounding box of the of the paper region and return it
                return 0
    
def distance_to_camera(knownWidth, focalLength, perWidth):
        # compute and return the distance from the maker to the camera
        return (knownWidth * focalLength) / perWidth


cap = cv2.VideoCapture(0)
# initialize the known distance from the camera to the object, which
# in this case is 24 inches
#KNOWN_DISTANCE = 24.0
# initialize the known object width, which in this case, the piece of
# paper is 12 inches wide
KNOWN_WIDTH = 11.0
# load the furst image that contains an object that is KNOWN TO BE 2 feet
# from our camera, then find the paper marker in the image, and initialize
# the focal length
#ret, image = cap.read()
#marker = find_marker(image)
focalLength = 543.45
#(marker[1][0] * KNOWN_DISTANCE) / KNOWN_WIDTH


# loop over the images
while(True):
        # load the image, find the marker in the image, then compute the
        # distance to the marker from the camera
    ret, image = cap.read()
    marker = find_marker(image)
    if marker == 0:
        cv2.putText(image,'Target Lost',(50, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 255), 2, cv2.LINE_4)
    else:
        inches = distance_to_camera(KNOWN_WIDTH, focalLength, marker[1][0])
        # draw a bounding box around the image and display it
        box = cv2.cv.BoxPoints(marker) if imutils.is_cv2() else cv2.boxPoints(marker)
        box = np.int0(box)
        cv2.drawContours(image, [box], -1, (0, 255, 0), 2)
        cv2.putText(image, "%.2fft" % (inches / 12),(image.shape[1] - 200, image.shape[0] - 20), cv2.FONT_HERSHEY_SIMPLEX, 2.0, (0, 255, 0), 3)
    cv2.imshow("image", image)
    if cv2.waitKey(1) & 0xFF == ord('q'):
                break
cap.release()
cv2.destroyAllWindows()
