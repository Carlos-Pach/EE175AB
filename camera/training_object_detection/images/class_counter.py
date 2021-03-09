#created so I could reliably count the number of appearances of each class label per train/test data

import csv
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--input", required=True, type=str, help="specify train or test folder or others")
args = parser.parse_args()



zero=0
one=0
two=0
thr=0
squir=0
hum=0
plant=0
rac=0
misc=0

#'test_labels.csv'
csvfolder = args.input + '_labels.csv'
file = csv.reader(open(csvfolder), delimiter=',')
for line in file:
    #print(line[3]) #gives the class labels, or can do title,sizex,sizey,classlabel = line
    if line[3]=="zero":
        zero=zero+1
    elif line[3] =="one":
        one=one+1
    elif line[3] =="two":
        two=two+1
    elif line[3] =="three":
        thr=thr+1
    elif line[3] =="squirrel":
        squir=squir+1
    elif line[3] =="person":
        hum=hum+1
    elif line[3] =="potted plant":
        plant=plant+1
    elif line[3] =="raccoon":
        rac=rac+1
    else:
        misc=misc+1

print(args.input, " folder:")
print(zero, " = zero")
print(one, " = one")
print(two, " = two")
print(thr, " = three")

print(hum, " = persons")

print(squir, " = squirrel")
print(rac, " = raccoon")
print(plant, " = potted plant")

print(misc, " =other unsure")


# with open('train_labels.csv') as file:
#     for line in file:
#         print(line)