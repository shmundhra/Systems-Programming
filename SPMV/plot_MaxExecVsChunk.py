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

destDir = "{}/MaxExecVsChunk".format(dirName)
if (not os.path.exists(destDir)):
    os.mkdir(destDir)

for folder in os.listdir(dirName):
    if not folder[:5] == 'Chunk':
        continue
    print(folder)

    matrix_name = folder.split('_')[-1]
    staticMaxExec, dynamicMaxExec, guidedMaxExec  = [0]*len(chunkSize), [0]*len(chunkSize), [0]*len(chunkSize)

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
        staticMaxExec[index] = np.max(static)
        dynamicMaxExec[index] = np.max(dynamic)
        guidedMaxExec[index] = np.max(guided)

    fig, ax = plt.subplots(figsize=(32, 16))

    plt.title("{} Maximum Execution Time".format(matrix_name), fontsize='35', fontweight='bold')
    plt.xlabel('ChunkSize', fontsize='28', fontweight='bold')
    plt.ylabel('Max ExecTime', fontsize='28', fontweight='bold')
    plt.xscale('log')
    plt.yscale('log')

    plt.plot(chunkSize, staticMaxExec,  'bo-', label='Static')
    plt.plot(chunkSize, dynamicMaxExec, 'ro-', label='Dynamic')
    plt.plot(chunkSize, guidedMaxExec,  'go-', label='Guided')

    plt.xticks(chunkSize, [str(x) for x in chunkSize], fontsize='20', rotation='30')
    
    maxTime = np.max([np.max(staticMaxExec), np.max(dynamicMaxExec), np.max(guidedMaxExec)])
    minTime = np.min([np.min(staticMaxExec), np.min(dynamicMaxExec), np.min(guidedMaxExec)])
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