#created so I could reliably count the number of appearances of each class label per train/test data

import csv
zero=0
one=0
two=0
thr=0
squir=0
hum=0
plant=0
rac=0


file = csv.reader(open('test_labels.csv'), delimiter=',')
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
    elif line[3] =="human":
        hum=hum+1
    elif line[3] =="potted plant":
        plant=plant+1
    else :#if line[3] =="raccoon":
        rac=rac+1


print(zero, " = zero")
print(one, " = one")
print(two, " = two")
print(thr, " = three")
print(squir, " = squirrel")
print(plant, " = potted plant")
print(rac, " = raccoon")


# with open('train_labels.csv') as file:
#     for line in file:
#         print(line)