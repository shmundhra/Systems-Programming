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

destDir = "{}/SpeedupVsChunk".format(dirName)
if (not os.path.exists(destDir)):
    os.mkdir(destDir)

for folder in os.listdir(dirName):
    if not folder[:5] == 'Chunk':
        continue
    print(folder)

    matrix_name = folder.split('_')[-1]
    staticSpeedup, dynamicSpeedup, guidedSpeedup  = [0]*len(chunkSize), [0]*len(chunkSize), [0]*len(chunkSize)

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
        staticSpeedup[index] = np.max(static)/np.min(static)
        dynamicSpeedup[index] = np.max(dynamic)/np.min(dynamic)
        guidedSpeedup[index] = np.max(guided)/np.min(guided)

    fig, ax = plt.subplots(figsize=(32, 16))

    plt.title("{} Speedups".format(matrix_name), fontsize='30', fontweight='bold')
    plt.xlabel('ChunkSize', fontsize='25', fontweight='bold')
    plt.ylabel('Speedup Factor', fontsize='25', fontweight='bold')
    plt.xscale('log')

    plt.plot(chunkSize, staticSpeedup,  'bo-', label='Static')
    plt.plot(chunkSize, dynamicSpeedup, 'ro-', label='Dynamic')
    plt.plot(chunkSize, guidedSpeedup,  'go-', label='Guided')

    plt.xticks(chunkSize, [str(x) for x in chunkSize], fontsize='20', rotation='30')
    plt.yticks(fontsize='20')
    ax.yaxis.set_minor_locator(tick.AutoMinorLocator(10))

    ax.grid(which='major', color='#CCCCCC', linestyle='-')
    ax.grid(which='minor', color='#CCCCCC', linestyle=':')

    plt.grid(True, which='both')
    plt.legend(fontsize='25')
    plt.savefig("{}/{}.jpg".format(destDir, matrix_name))
    # plt.show()
    plt.close()