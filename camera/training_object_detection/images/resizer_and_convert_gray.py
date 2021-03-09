## Bulk image resizer

# This script simply resizes all the images in a folder to one-eigth their
# original size. It's useful for shrinking large cell phone pictures down
# to a size that's more manageable for model training.

# Usage: place this script in a folder of images you want to shrink,
# and then run it.

import numpy as np
import cv2
import os

dir_path = os.getcwd()
#'C:/tensorflow1/models/research/object_detection/testSample'


for filename in os.listdir(dir_path):
    # if filename.endswith(".jpg"):
    #     orig_im = cv2.imread(filename)
        # image = cv2.cvtColor(orig_im, cv2.COLOR_BGR2GRAY)

    #     (height, width) = image.shape[:2]
    #     if (height <= 700) or (width <= 700):
	#         resized_to7 = cv2.resize(image, (700,933), interpolation = cv2.INTER_AREA)
	#         cv2.imwrite(filename,resized_to7)
    
    # if filename.endswith(".png"):
    #     orig_im = cv2.imread(filename)
    #     image = cv2.cvtColor(orig_im, cv2.COLOR_BGR2GRAY)

    #     (height, width) = image.shape[:2]
    #     if (height <= 700) or (width <= 700):
	#         resized_to7 = cv2.resize(image, (700,933), interpolation = cv2.INTER_AREA)
	#         cv2.imwrite(filename,resized_to7)
    
    if filename.endswith(".jpg"):
        image = cv2.imread(filename)
        (height, width) = image.shape[:2]
        if height<550 or height >950 or width<550 or width<950:
        # if (height!=700) and (width!=700) or ((height!=700) and (width!=933)):
	        resized_to7 = cv2.resize(image, (700,933), interpolation = cv2.INTER_AREA)
	        cv2.imwrite(filename,resized_to7)

    if filename.endswith(".png"):
        image = cv2.imread(filename)
        (height, width) = image.shape[:2]
        if height<550 or height >950 or width<550 or width<950:
        # if (height!=700) and (width!=700) or ((height!=700) and (width!=933)):
	        resized_to7 = cv2.resize(image, (700,933), interpolation = cv2.INTER_AREA)
	        cv2.imwrite(filename,resized_to7)




    # If the images are not .JPG images, change the line below to match the image type.
    #if filename.endswith(".jpg"):
        #image = cv2.imread(filename)
        #gray_img = cv2.cvtColor(image,cv2.COLOR_BGR2GRAY)
        #img_inv = cv2.bitwise_not(image)
        #(height, width) = image.shape[:2]

        #resized = cv2.resize(img_inv,None,fx=18, fy=18, interpolation=cv2.INTER_AREA)
        #cv2.imwrite(filename,resized)


    # if filename.endswith(".png"):
    #     image = cv2.imread(filename)
    #     #img_inv = cv2.bitwise_not(image)

    #     resized = cv2.resize(image,None,fx=10, fy=10, interpolation=cv2.INTER_AREA)
    #     cv2.imwrite(filename,resized)


#K- I edited to resize to a bigger picture (at least 300 pixels) and to invert color
