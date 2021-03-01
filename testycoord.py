######## Webcam Object Detection Using Tensorflow-trained Classifier #########
# Author: Evan Juras
# Date: 10/27/19
# Description: 
# This program uses a TensorFlow Lite model to perform object detection on a live webcam
# feed. It draws boxes and scores around the objects of interest in each frame from the
# webcam. To improve FPS, the webcam object runs in a separate thread from the main program.
# This script will work with either a Picamera or regular USB webcam.
#
# This code is based off the TensorFlow Lite image classification example at:
# https://github.com/tensorflow/tensorflow/blob/master/tensorflow/lite/examples/python/label_image.py
#
# I added my own method of drawing boxes and labels using OpenCV.

#Kelly: I added conversion of coordinates (of objects), and name to send by i2c to Arduino/Teensy
#IMPORTANT: 
# in (tflite1-env) pi@raspberrypi:~/tflite1 $ 
# python3 TFLite_detection_webcam.py --modeldir=TFLite_model --resolution=640x480


# Import packages
import os
import argparse
import cv2
import numpy as np
import sys
import time
from threading import Thread
import importlib.util
from smbus2 import SMBus, i2c_msg

block=[]            # orig data read by TF; added by .append()
addr = 0x8          #for i2c bus address
bus = SMBus(1)      #for i2c using ic2-1 rather than ic2-0 port
c=0                 #for mapping coordinate of obj
data_bet= [0,0,0]   #init final data [] to send over
flag=0              #see if there is a detection or not




def send_to_pi(data):
    #data=[c,"potted plant"] #EXAMPLE INPUT DATA
    #take 3 pieces of data, send to pi via i2c
    #1st bit: if data is available
    #2-4th bit: name of detected obj
    #5-12th bit: coord of obj in degrees for sprayer    

    data_arr = ['0000','0000','0000'] #init byte array, size 3
    if (data[0] )==0:
        return [0,0,0]      #tells Teensy not to read empty data
    else:
        data_arr[0] =(format(0x8,'04b')) #puts a 1 on left-most bit to show info is coming 
                                         #(optional: bin to see array in print funct)

        #map obj names to a string of 4bit binary for ea of 8 obj classes ex.(0000 or 0001)
        obj= (data[1][:2]) 
        if obj=="ze":
            obj=(format(0x00,'04b'))
        elif obj=="on":
            obj=(format(0x01,'04b'))
        elif obj=="tw":
            obj=(format(0x02,'04b'))
        elif obj=="th":
            obj=(format(0x03,'04b'))
        elif obj=="sq":
            obj=(format(0x04,'04b'))
        elif obj=="ra":
            obj=(format(0x05,'04b'))
        elif obj=="pe":
            obj=(format(0x06,'04b'))
        elif obj=="po":
            obj=(format(0x07,'04b'))
        else:
            print("Madam, something's wrong, this isn't an avaliable object.")

        #first 4 bits: fstring (faster than format) to complete binary string 
        #combine bits: to create (1st for availablility, last 3 for object name)
        data_arr[0] = f"{(int(data_arr[0],2) | int(obj,2)):b}" #aka f"{insert int(string bit | object bit):b}" aka format(c, "b")
        
        #for obj coord. create bin string size 8bits, this goes last in final bits
        coord=data[0]
        bin_coord=(format(coord,'08b'))  
        
        #concatenate entire binary string of info
        it=1                #iteration number, happens 2x for the 1 byte coordinates takes up
        c=0                 #counts for 4 bits per set of bits (2 sets for 1 byte)
        temp_byt=''
        for v in bin_coord: #iterable string ! :D
                            #for the first 4 take it, then next 4 
                            #save and add to bin string until reached 3rd set then export as a byte to datat_array
            temp_byt=temp_byt+v
            c=c+1
            if c==4:
                c=0
                data_arr[it]= temp_byt
                it=it+1
                temp_byt=''                

        #convert to decimal form to send to pi
        for byttte in data_arr: 
            data_bet[c]= (int(byttte,2))  #optionally: for hex string, wrap hex outside of int
            c=c+1
    return data_bet







# Define VideoStream class to handle streaming of video from webcam in separate processing thread
# Source - Adrian Rosebrock, PyImageSearch: https://www.pyimagesearch.com/2015/12/28/increasing-raspberry-pi-fps-with-python-and-opencv/
class VideoStream:
    """Camera object that controls video streaming from the Picamera"""
    def __init__(self,resolution=(640,480),framerate=30):
    #is the lowest supported resolution already!

        # Initialize the PiCamera and the camera image stream
        self.stream = cv2.VideoCapture(0)
        ret = self.stream.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
        ret = self.stream.set(3,resolution[0])
        ret = self.stream.set(4,resolution[1])
            
        # Read first frame from the stream
        (self.grabbed, self.frame) = self.stream.read()

    # Variable to control when the camera is stopped
        self.stopped = False

    def start(self):
    # Start the thread that reads frames from the video stream
        Thread(target=self.update,args=()).start()
        return self

    def update(self):
        # Keep looping indefinitely until the thread is stopped
        while True:
            # If the camera is stopped, stop the thread
            if self.stopped:
                # Close camera resources
                self.stream.release()
                return

            # Otherwise, grab the next frame from the stream
            (self.grabbed, self.frame) = self.stream.read()

    def read(self):
    # Return the most recent frame
        return self.frame

    def stop(self):
    # Indicate that the camera and thread should be stopped
        self.stopped = True
        
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
        if perWidth == 0:
            return 0
        else:
            return (knownWidth * focalLength) / perWidth

# Define and parse input arguments
parser = argparse.ArgumentParser()
parser.add_argument('--modeldir', help='Folder the .tflite file is located in',
                    required=True)
parser.add_argument('--graph', help='Name of the .tflite file, if different than detect.tflite',
                    default='detect.tflite')
parser.add_argument('--labels', help='Name of the labelmap file, if different than labelmap.txt',
                    default='labelmap.txt')
parser.add_argument('--threshold', help='Minimum confidence threshold for displaying detected objects',
                    default=0.5)
parser.add_argument('--resolution', help='Desired webcam resolution in WxH. If the webcam does not support the resolution entered, errors may occur.',
                    default='1280x720')

args = parser.parse_args()

MODEL_NAME = args.modeldir
GRAPH_NAME = args.graph
LABELMAP_NAME = args.labels
min_conf_threshold = float(args.threshold)
resW, resH = args.resolution.split('x')
imW, imH = int(resW), int(resH)

# Import TensorFlow libraries
# If tflite_runtime is installed, import interpreter from tflite_runtime, else import from regular tensorflow
pkg = importlib.util.find_spec('tflite_runtime')
if pkg:
    from tflite_runtime.interpreter import Interpreter
else:
    from tensorflow.lite.python.interpreter import Interpreter

# Get path to current working directory
CWD_PATH = os.getcwd()

# Path to .tflite file, which contains the model that is used for object detection
PATH_TO_CKPT = os.path.join(CWD_PATH,MODEL_NAME,GRAPH_NAME)

# Path to label map file
PATH_TO_LABELS = os.path.join(CWD_PATH,MODEL_NAME,LABELMAP_NAME)

# Load the label map
with open(PATH_TO_LABELS, 'r') as f:
    labels = [line.strip() for line in f.readlines()]

# Have to do a weird fix for label map if using the COCO "starter model" from
# https://www.tensorflow.org/lite/models/object_detection/overview
# First label is '???', which has to be removed.
if labels[0] == '???':
    del(labels[0])

# Load the Tensorflow Lite model.
interpreter = Interpreter(model_path=PATH_TO_CKPT)
interpreter.allocate_tensors()

# Get model details
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()
height = input_details[0]['shape'][1]
width = input_details[0]['shape'][2]

floating_model = (input_details[0]['dtype'] == np.float32)

input_mean = 127.5
input_std = 127.5

# Initialize frame rate calculation
frame_rate_calc = 1
frame_rate_calc2 = 1
freq = cv2.getTickFrequency()

# Initialize video stream
videostream = VideoStream(resolution=(imW,imH),framerate=30).start()
time.sleep(1)
xmid=0 #init line in middle of detected object
# initialize the known distance from the camera to the object, which
# in this case is 24 inches
#KNOWN_DISTANCE = 24.0
# initialize the known object width, which in this case, the piece of
# paper is 12 inches wide
KNOWN_WIDTH = 2.547
# load the furst image that contains an object that is KNOWN TO BE 2 feet
# from our camera, then find the paper marker in the image, and initialize
# the focal length
#ret, image = cap.read()
#marker = find_marker(image)
focalLength = 1170.6
#(marker[1][0] * KNOWN_DISTANCE) / KNOWN_WIDTH
newim = 0
pastim = 0

#for frame1 in camera.capture_continuous(rawCapture, format="bgr",use_video_port=True):
while True:

    # Start timer (for calculating frame rate)
    t1 = cv2.getTickCount()

    # Grab frame from video stream
    frame1 = videostream.read()

    # Acquire frame and resize to expected shape [1xHxWx3]
    frame = frame1.copy()
    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    frame_resized = cv2.resize(frame_rgb, (width, height))
    input_data = np.expand_dims(frame_resized, axis=0)

    # Normalize pixel values if using a floating model (i.e. if model is non-quantized)
    if floating_model:
        input_data = (np.float32(input_data) - input_mean) / input_std

    # Perform the actual detection by running the model with the image as input
    interpreter.set_tensor(input_details[0]['index'],input_data)
    interpreter.invoke()

    # Retrieve detection results
    boxes = interpreter.get_tensor(output_details[0]['index'])[0] # Bounding box coordinates of detected objects
    classes = interpreter.get_tensor(output_details[1]['index'])[0] # Class index of detected objects
    scores = interpreter.get_tensor(output_details[2]['index'])[0] # Confidence of detected objects
    #num = interpreter.get_tensor(output_details[3]['index'])[0]  # Total number of detected objects (inaccurate and not needed)
    # Loop over all detections and draw detection box if confidence is above minimum threshold
    flag=0   #init flag for sending to i2c
    for i in range(len(scores)):
        cv2.line(frame, (xmid, 0),(xmid,900), (255,255,0),5 ) #this makes sure the line stays after image has disappeared
        
        if ((scores[i] > min_conf_threshold) and (scores[i] <= 1.0)):
            flag=1 #there is obj detected
            # Get bounding box coordinates and draw box
            # Interpreter can return coordinates that are outside of image dimensions, need to force them to be within image using max() and min()
            ymin = int(max(1,(boxes[i][0] * imH)))
            xmin = int(max(1,(boxes[i][1] * imW)))
            ymax = int(min(imH,(boxes[i][2] * imH)))
            xmax = int(min(imW,(boxes[i][3] * imW)))
            xmid = int((xmin+xmax) /2)
            # MAP Xmid Coordinate TO DEGREES [for water sprayer to spray (by using interpolation)]
            c=int(np.interp(xmid, (0, 640), (0, 180)) )
            print(xmid, "Xmiddle")
            print(c, "convert to degrees")
            block.clear()
            block.append(c)
            cv2.line(frame, (xmid, 0),(xmid,900), (255,255,0),5 ) #draw the line down center

            cv2.rectangle(frame, (xmin,ymin), (xmax,ymax), (10, 255, 0), 2)

            # Draw label
            object_name = labels[int(classes[i])] # Look up object name from "labels" array using class index
            label = '%s: %d%%' % (object_name, int(scores[i]*100)) # Example: 'person: 72%'
            labelSize, baseLine = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.7, 2) # Get font size
            label_ymin = max(ymin, labelSize[1] + 10) # Make sure not to draw label too close to top of window
            cv2.rectangle(frame, (xmin, label_ymin-labelSize[1]-10), (xmin+labelSize[0], label_ymin+baseLine-10), (255, 255, 255), cv2.FILLED) # Draw white box to put label text in
            cv2.putText(frame, label, (xmin, label_ymin-7), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 0), 2) # Draw label text
            print(label) #or if only want name print object_name
            block.append(object_name)


    # Draw framerate in corner of frame
    cv2.putText(frame,'FPS: {0:.2f}'.format(frame_rate_calc),(30,50),cv2.FONT_HERSHEY_SIMPLEX,1,(255,255,0),2,cv2.LINE_AA)
    cv2.putText(frame,'FPS for detection: {0:.2f}'.format(frame_rate_calc2),(30,75),cv2.FONT_HERSHEY_SIMPLEX,1,(255,255,0),2,cv2.LINE_AA)

    #convert, then send data over w i2c
    if (flag==0):
        block=[0,"placeholder"] #set flag for leftmost bits to 0, don't do more math
    dec_list = send_to_pi(block)
    
    marker = find_marker(frame)
    if marker == 0:
        inches = 0
        cv2.putText(frame, "%.2fft" % (0 / 12),(frame.shape[1] - 200, frame.shape[0] - 20), cv2.FONT_HERSHEY_SIMPLEX, 2.0, (0, 255, 0), 3)
    else:
        inches = distance_to_camera(KNOWN_WIDTH, focalLength, marker[1][0])
        # draw a bounding box around the image and display it
        newim = int(inches * 2.54)
        if pastim == 0:
            pastim = newim
        else:
            if abs(pastim - newim) < 5:
                pastim = newim
            else:
                pastim = pastim
        box = cv2.cv.BoxPoints(marker) if imutils.is_cv2() else cv2.boxPoints(marker)
        box = np.int0(box)
        cv2.drawContours(frame, [box], -1, (0, 255, 0), 2)
        cv2.putText(frame, "%.2fcm" % (pastim),(frame.shape[1] - 300, frame.shape[0] - 20), cv2.FONT_HERSHEY_SIMPLEX, 2.0, (0, 255, 0), 3)
    dec_list.append(pastim)
    
    msg = i2c_msg.write(addr,dec_list)
    bus.i2c_rdwr(msg)
    
    # All the results have been drawn on the frame, so it's time to display it.
    cv2.imshow('Object detector', frame)

    # Calculate framerate
    t2 = cv2.getTickCount()
    time1 = (t2-t1)/freq
    frame_rate_calc= 1/time1

    # Press 'q' to quit
    if cv2.waitKey(1) == ord('q'):
        print('FPS: {0:.2f}'.format(frame_rate_calc))
        break

#data_to_t = bus.write_block_data(addr,(block))
print(block, " BLOCK LIST ")
# Clean up
cv2.destroyAllWindows()
videostream.stop()
