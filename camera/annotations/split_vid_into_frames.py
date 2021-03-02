#OLD:
# Take 2 fps out of a . mp4 video, and add a copy of a corresp. xml file to its folder name
'''
Kelly:
NEW:
Take 1 fps from ea. video, 
put that + .mp4 video + catxml.py script needed + copy of corresp .xml files
into new created folder (named [.mp4 video name])
----
Run:
----
Keep script in same directory (folder) as all .mp4 videos, aka the main folder
'''
# this is super helpful too 
# to understand os https://automatetheboringstuff.com/chapter8/
import cv2
import numpy as np
import os
from shutil import copyfile
import shutil



my_dir = os.getcwd()
xml_file=vid_dir=os.path.join(my_dir, "xml_copy.xml")
count=0

#throw error if unsucessful
try:
    #shows files + folders in current directory
    for vids in os.listdir(my_dir):
        vidname=vids[:-4]
        vid_dir=os.path.join(my_dir, vidname)

        #check videofile, and if it doesn't have an existing folder: (for vid frames & .xml later)
        if vids.endswith(".mp4") and not (os.path.isdir(vid_dir)  ):
            capt = cv2.VideoCapture(vids)
            print("FILENAME: ", vidname)
            fps = capt.get(cv2.CAP_PROP_FPS)
            print ("Frames per second : {0}".format(fps) )
            get_frame=fps/2
            add_frame=0  
                
            count+=1
            #make new folderw/ vidname
            os.mkdir(vid_dir)
            os.chdir(vid_dir)
            cur_frame=0
            while(True):
                capt.set(cv2.CAP_PROP_POS_MSEC,(cur_frame*1000)) #[*1000 for 1fps, 500 for 2 fps]
                # Capture frame-by-frame
                tru, frame = capt.read()

                # Saves image of the current frame in jpg file
                if not tru:
                    break
                #indiv frame_name = vidname_#.jpg
                frame_name = vidname+"_{:d}.jpg".format(cur_frame)
                #save frame as JPG file
                cv2.imwrite(frame_name, frame)    
                xml_dest=os.path.join(vid_dir, frame_name[:-4] + ".xml")
                #copy xml file path to target filename
                copyfile(xml_file, xml_dest)   
                # To stop duplicate images
                cur_frame += 1

            
            # When everything done, release the capture
            capt.release()
            cv2.destroyAllWindows()

            #copy convert_annot_to_xml.py to new video folder
            xml_convert_dir= os.path.join(my_dir, "convert_annot_to_xml.py")
            xml_convert_dest= os.path.join(vid_dir, "convert_annot_to_xml.py")
            copyfile(xml_convert_dir, xml_convert_dest) 
            #move .mp4 video to same folder
            mp4_dir = os.path.join(my_dir, vidname + ".mp4") 
            mp4_dest=os.path.join(vid_dir, vidname + ".mp4")
            shutil.move(mp4_dir, mp4_dest)
            #go back to main folder
            os.chdir(my_dir)

    print(count, " num of new, non-moved .mp4 files")
except OSError:
    print ("Error: Creating directory of data")
else: 
    print("Success! Made new directories for each .mp4 vid")

