!!!Make sure you do this in your venv!!! 
-----------------------------------------------------------------------------
--> (CURRENT CODE with debugging!) combine_CK.py and .ino for current script with i2c 

		(Code w/o Showing camera window to viewer: faster fps!):
		--> no_gui.py (cv2.imshow() commented out, etc.)


==============================================================================
==============================================================================

Versions:
Python 3.7.3
When it asks to install TF (1c.) install TF 1.13.1 first (if doesn't work, try 1.14)
opencv 4.5.1.48 (pip3 install --upgrade opencv-python==4.5.1.48)


Follow this link for help on installation:
https://github.com/EdjeElectronics/TensorFlow-Lite-Object-Detection-on-Android-and-Raspberry-Pi/blob/master/Raspberry_Pi_Guide.md
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


In step 1d. Option 2: Using our own trained model, copy everything needed in this raspi_camera, paste inside tflite1, and follow the instructions.

-----------------------------------------------------------
-----------------------------------------------------------
In TFLite1 Folder,"TFLite_detection_webcam.py", and a sub-folder "TFLite_model" containing our trained model, etc.

(enter venv by)
source /home/pi/tflite1/tflite1-env/bin/activate

(tflite1-env) pi@raspberrypi:~/tflite1 $ 
(type in)
python3 TFLite_detection_webcam.py --modeldir=TFLite_model --resolution=640x480
-----
-----
use above to test, then use below to check with more code + connect to i2c or will get error.
replace TFLite_detection_webcam.py with ...
		-->no_gui.py   (faster, but doesn't show you camera view)
		--)combine_CK.py (best for debugging)

===========================================================
===========================================================

Make sure you have smbus2, imutils, etc. packages installed!


................................................................................................................................

ERRORS:

VIDEOIO ERROR: V4L: can't open camera by index 0 Traceback (most recent call last): 
File "tflite/TFLite_detection_webcam.py", 
line 171, in frame = frame1.copy() 
AttributeError: 'NoneType' object has no attribute 'copy'

	^ This is probably caused by a camera error, check it's plugged in correctly, or worst case, may have to replace.
	  To check, use a seperate sript to check to see if Pi recieves camera input correctly.


Can't open opencv, etc., mismatch for permissions or versions ? 
	^ Please make sure you did everything in venv, if still wrong, worst case, reformat the entire sd card.

Remote I/O error
	^ Happens once in a while, not sure why. Maybe sudden outlier values may throw errors. Looking into.
	^Make sure Arduino has fully updated script, or else could get this error too if you run too early.
	^Either wire issues with resistors/interferences/clock speed, soln: add quick timer.sleep after i2c, only send i2c when needed., use try except to restart i2c.