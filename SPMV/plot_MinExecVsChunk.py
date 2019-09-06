import os
import sys
import subprocess
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as tick
import pandas as pd

chunkSize = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000,
             20000, 50000, 100000, 200000, 500000, 1000000]
dirName = sys.argv[1]

destDir = "{}/MinExecVsChunk".format(dirName)
if (not os.path.exists(destDir)):
    os.mkdir(destDir)

for folder in os.listdir(dirName):
    if not folder[:5] == 'Chunk':
        continue
    print(folder)

    matrix_name = folder.split('_')[-1]
    staticMinExec, dynamicMinExec, guidedMinExec  = [0]*len(chunkSize), [0]*len(chunkSize), [0]*len(chunkSize)

    pathName = dirName + "/" + folder
    for file in os.listdir(pathName):
        if (not file.split('.')[-1] == "txt"):
            continue
        print(file, end='\n')

        DF = pd.read_csv("{}/{}".format(pathName, file), sep='\t')
        static = np.array(DF['Static'])
        dynamic = np.array(DF['Dynamic'])
        guided = np.array(DF['Guided'])

        index = chunkSize.index(int(file.split('.')[0]))
        staticMinExec[index] = np.min(static)
        dynamicMinExec[index] = np.min(dynamic)
        guidedMinExec[index] = np.min(guided)

    fig, ax = plt.subplots(figsize=(32, 16))

    plt.title("{} Minimum Execution Time".format(matrix_name), fontsize='35', fontweight='bold')
    plt.xlabel('ChunkSize', fontsize='28', fontweight='bold')
    plt.ylabel('Min ExecTime', fontsize='28', fontweight='bold')
    plt.xscale('log')
    plt.yscale('log')

    plt.plot(chunkSize, staticMinExec,  'bo-', label='Static')
    plt.plot(chunkSize, dynamicMinExec, 'ro-', label='Dynamic')
    plt.plot(chunkSize, guidedMinExec,  'go-', label='Guided')

    plt.xticks(chunkSize, [str(x) for x in chunkSize], fontsize='20', rotation='30')
    
    maxTime = np.max([np.max(staticMinExec), np.max(dynamicMinExec), np.max(guidedMinExec)])
    minTime = np.min([np.min(staticMinExec), np.min(dynamicMinExec), np.min(guidedMinExec)])
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
    plt.savefig("{}/{}.jpg".format(destDir, matrix_name))
    # plt.show()
    plt.close()