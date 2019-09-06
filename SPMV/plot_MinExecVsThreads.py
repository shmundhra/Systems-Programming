import os
import sys
import subprocess
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as tick
import pandas as pd

threads = [t+1 for t in range(24)]
dirName = sys.argv[1]

destDir = "{}/MinExecVsThreads".format(dirName)
if (not os.path.exists(destDir)):
    os.mkdir(destDir)

for folder in os.listdir(dirName):
    if not folder[:6] == 'Thread':
        continue
    print(folder)

    matrix_name = folder.split('_')[1]
    staticMinExec, dynamicMinExec, guidedMinExec  = [0]*len(threads), [0]*len(threads), [0]*len(threads)

    pathName = dirName + "/" + folder
    for file in os.listdir(pathName):
        if (not file.split('.')[-1] == "txt"):
            continue
        print(file, end='\n')

        DF = pd.read_csv("{}/{}".format(pathName, file), sep='\t')
        static = np.array(DF['Static'])
        dynamic = np.array(DF['Dynamic'])
        guided = np.array(DF['Guided'])

        index = threads.index(int(file.split('.')[0]))
        staticMinExec[index] = np.min(static)
        dynamicMinExec[index] = np.min(dynamic)
        guidedMinExec[index] = np.min(guided)

    fig, ax = plt.subplots(figsize=(24, 12))

    plt.title("{} Minimum Execution Time".format(matrix_name), fontsize='30', fontweight='bold')
    plt.xlabel('Number of Threads', fontsize='25', fontweight='bold')
    plt.ylabel('Min ExecTime', fontsize='25', fontweight='bold')
    plt.yscale('log')

    plt.plot(threads, staticMinExec,  'bo-', label='Static')
    plt.plot(threads, dynamicMinExec, 'ro-', label='Dynamic')
    plt.plot(threads, guidedMinExec,  'go-', label='Guided')

    plt.xticks(threads, [str(x) for x in threads], fontsize='20')
    
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