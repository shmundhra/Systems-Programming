import os
import sys
import pandas as pd
import numpy as np

threads = [t+1 for t in range(24)]
chunkSize = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000,
             20000, 50000, 100000, 200000, 500000, 1000000]

dirName = sys.argv[1]
destPath = "Threads"

for folder in os.listdir(dirName):
    if not folder[:5] == 'Chunk':
        continue

    matrix_name = folder.split('_')[-1]
    destDir = "{}/{}_{}".format(dirName, destPath, matrix_name)
    if not os.path.exists(destDir):
        os.mkdir(destDir)
    print(destDir)

    DF = [0]*len(chunkSize)

    pathName = "{}/{}".format(dirName, folder)
    for file in os.listdir(pathName):
        if (not file.split('.')[-1] == "txt"):
            continue
        print(pathName, end='\n')

        index = chunkSize.index(int(file.split('.')[0]))
        DF[index] = pd.read_csv("{}/{}".format(pathName, file), sep='\t')

    for i in range(len(threads)):
        filePath = "{}/{}.txt".format(destDir, str(threads[i]))
        if os.path.exists(filePath):
            continue

        with open(filePath, 'w') as f:
            f.write("{}\t{}\t{}\n".format("Static", "Dynamic", "Guided"))

            for index in range(len(chunkSize)):
                f.write("{}\t".format(DF[index]['Static'][i]))
                f.write("{}\t".format(DF[index]['Dynamic'][i]))
                f.write("{}\n".format(DF[index]['Guided'][i]))
