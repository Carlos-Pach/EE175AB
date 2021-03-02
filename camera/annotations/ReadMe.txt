'''''''
(for single pictures, use LabelImg)
..
(for video annotations use cvat)
CONVERT CVAT's SINGLE XML ANNOTATION FOR VIDEO, to SEPERATE FRAMES + XML FILES
'''''''

Included in main directory folder:
	-split_vid_into_frames.py
	-convert_annot_to_xml.py (abbrv. catx.py)
	-xml_copy.xml
	-all the .mp4 videos!

	-resulting 'class name you input as argument to catx.py' folder storing xmls+jpgs
	-resulting 'video' folder storing all videos

STEPS:
	(main directory)
	1. Place all videos in some main folder
	2. Run split_vid_into_frames.py
		(makes new folder, places frames, xml_copies, catx.py_copy, .mp4 video inside it!)
	3. Take your cvat annotations.xml file and paste in that new folder
	(videoname directory)
	4. Run convert_annot_to_xml.py --input newfoldername
		(Set argument as a class name for ease of use ex. squirrel)
		(places info from annotations.xml into copied individual xmls, moves those xml + jpgs into main's 
		new class name folder, move .mp4 video into main's video folder, delete current uneeded videoname directory)
	5. Repeat for any amount of videos 

		