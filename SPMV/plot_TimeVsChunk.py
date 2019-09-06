import os
import sys
import subprocess
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker
import pandas as pd

chunkSize = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000,
             20000, 50000, 100000, 200000, 500000, 1000000]
dirName = sys.argv[1]

destDir = "{}/TimeVsChunk".format(dirName)
if (not os.path.exists(destDir)):
    os.mkdir(destDir)

for folder in os.listdir(dirName):
    if not folder[:7] == 'Threads':
        continue
    print(folder)

    matrix_name = folder.split('_')[-1]

    destPath = "{}/{}".format(destDir, matrix_name)
    if (not os.path.exists(destPath)):
        os.mkdir(destPath)

    pathName = dirName + "/" + folder
    for file in os.listdir(pathName):
        if (not file.split('.')[-1] == "txt"):
            continue
        print(file, end='\t\t')

        DF = pd.read_csv("{}/{}".format(pathName, file), sep='\t')
        static = np.array(DF['Static'])
        dynamic = np.array(DF['Dynamic'])
        guided = np.array(DF['Guided'])

        fig, ax = plt.subplots(figsize=(32, 16))

        plt.title("{} with Threads {}".format(matrix_name, file.split('.')[0]), fontsize='30', fontweight='bold')
        plt.xlabel('ChunkSize', fontsize='25', fontweight='bold')
        plt.ylabel('Time in Seconds', fontsize='25', fontweight='bold')
        plt.xscale("log")
        plt.yscale("log")

        plt.plot(chunkSize, static,  'bo-', label='Static' )
        plt.plot(chunkSize, dynamic, 'ro-', label='Dynamic')
        plt.plot(chunkSize, guided,  'go-', label='Guided' )

        plt.xticks(chunkSize, [str(x) for x in chunkSize], fontsize='20', rotation='30')

        maxTime = np.max([np.max(static), np.max(dynamic), np.max(guided)])
        minTime = np.min([np.min(static), np.min(dynamic), np.min(guided)])
        print(minTime, maxTime, end='\t')

        count = 0
        minT = minTime
        while (minT < 1):
            minT = minT * 10
            count += 1
        expoUp = pow(10, count)
        expoDown = pow(10, -count)

        maxCeil = int(maxTime*expoUp) + 1
        minCeil = int(minTime*expoUp)
        print(count, minCeil, maxCeil)

        y_ticks = np.arange(minCeil, maxCeil+1, 1)
        y_ticks = [expoDown*y for y in y_ticks]
        y_ticks = [(int(10000*y))/10000 for y in y_ticks]
        # ax.set_yticks(y_ticks)
        # ax.set_yticklabels([str(y) for y in y_ticks])
        plt.yticks(y_ticks, [str(y) for y in y_ticks], fontsize='20')

        ax.grid(which='major', color='#CCCCCC', linestyle='-')
        # ax.grid(which='minor', color='#CCCCCC', linestyle=':')

        plt.grid(True, which='both')
        plt.legend(fontsize='25')
        plt.savefig("{}/{}.jpg".format(destPath, file.split('.')[0]))
        # plt.show()
        plt.close()
