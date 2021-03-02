'''
Kelly:
In .mp4 Specific Directory:
Find and replace data from [main annotation (cvat) xml file] to [individual frame's xml]
In main directory: place result xml+jpgs in input folder & .mp4 in video folder
----
Run:
----
Keep script in same dir as specific folder frames/xml files, and single .mp4 video
--input [put in a class name]
'''
import xml.etree.ElementTree as ET
import os
import cv2
import shutil
import copy
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--input", required=True, type=str, help="Directory to store results")
args = parser.parse_args()


#NEED TO GET FPS FIRST:
my_dir = os.getcwd()
#shows files n folders in current directory
for vids in os.listdir(my_dir):
        vidname=vids[:-4]
        vid_dir=os.path.join(my_dir, vidname)
        #check videofile, and if it doesn't have an existing folder: (for vid frames & .xml later)
        if vids.endswith(".mp4" ):
            capt = cv2.VideoCapture(vids)
            vidname=vids[:-4]
            full_vidname=vids
            fps = capt.get(cv2.CAP_PROP_FPS)
            break
capt.release()
cv2.destroyAllWindows()

print("FILENAME: ", vidname)
print("FULL FILENAME: ", full_vidname)
print ("Frames per second : {0}".format(fps) )



tree = ET.parse('annotations.xml')
root = tree.getroot()
get_frame=fps #fps/2 for 2fps, but i switched back to 1fps
add_frame=0 
# CVAT XML FILE
for child in root[2:]:  #start at [2:] bc first 2 == None
    if add_frame%get_frame==0 :

        if add_frame ==0:
            frame_num = child.attrib.get('name')[11:] 
        else:
            frame_num=child.attrib.get('name')[6:].lstrip("0")
        # print( "actual frame num attrib=" , frame_num )    
        
        #match frame # to frame #s from .jpg
        if int(frame_num)>0:
            frame_num= int(frame_num)/get_frame
        frame_name = vidname+"_{:d}.jpg".format(int(frame_num)) 
        xml_dest=os.path.join(my_dir, frame_name[:-4] + ".xml")
        xmlname= frame_name[:-4] + '.xml'

        wid = child.attrib.get('width')
        hei= child.attrib.get('height')
        full_vidname_fromxml = root[1][2].text  #name of video

        #edit copied xml file @ frame #, !!! 'with' statement closes opened file after it ends
        with open(xmlname, encoding='latin-1') as fr:
            tree2 = ET.parse(fr)
            root2 = tree2.getroot()

            root2[1].text = str(frame_name)
            root2[4][0].text= str(wid)
            root2[4][1].text= str(hei)
            for obj1 in tree2.findall(".//object"):
                obj2 = copy.deepcopy(obj1)                
            root2.remove(root2[6])

            j=0
            for image in child:
            #Print obj class attribute (if it exists!) (usually once, but can do multiple)
                class_label =  image.attrib.get('label')
                minX= image.attrib.get('xtl',"")    #top left= min
                minY= image.attrib.get('ytl',"")
                maxX= image.attrib.get('xbr',"")    #bottom right= max
                maxY= image.attrib.get('ybr',"") 

                if j==0 :
                    root2.append(obj2)
                else:                               #helps update object! 
                    for obj1 in tree2.findall(".//object"):
                        obj2 = copy.deepcopy(obj1)
                        root2.append(obj2)

                n_name = root2.findall('.//name')
                n_name[j].text= str(class_label)

                n_xmin = root2.findall('.//xmin')
                n_xmin[j].text= str(minX)
                n_ymin = root2.findall('.//ymin')
                n_ymin[j].text= str(minY)
                n_xmax = root2.findall('.//xmax')
                n_xmax[j].text= str(maxX)
                n_ymax = root2.findall('.//ymax')
                n_ymax[j].text= str(maxY)
                # root2[it][0].text = str(class_label)
                # root2[it][4][0].text= str(minX)
                j+=1
        tree2.write(xmlname)
        print(" ")
    else:
        pass
    add_frame+=1


main_dir = os.path.dirname(my_dir)
dest = os.path.join(main_dir, args.input) # (parent_dir,newdir)
if not os.path.exists(dest):
   os.mkdir(dest)
vid_dest = os.path.join(main_dir, "videos") # (parent_dir,newdir)
if not os.path.exists(vid_dest):
   os.mkdir(vid_dest)

source = my_dir
files = os.listdir(my_dir)

for f in files:
    if ( f.endswith(".jpg") or (f.endswith(".xml") and not f.endswith("annotations.xml"))   ):
        shutil.move(f, dest)
    elif ( f.endswith(".mp4")):
        shutil.move(f, vid_dest)

os.chdir('..')  #go back to main directory       


## Try to remove tree; if failed show an error using "try...except" on screen
try:
    shutil.rmtree(my_dir)
except OSError as e:
    print ("Error: %s - %s." % (e.filename, e.strerror))
