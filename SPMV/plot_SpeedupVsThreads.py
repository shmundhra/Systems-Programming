import os
import sys
import subprocess
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as tick
import pandas as pd

threads = [t+1 for t in range(24)]
dirName = sys.argv[1]

destDir = "{}/SpeedupVsThreads".format(dirName)
if (not os.path.exists(destDir)):
    os.mkdir(destDir)

for folder in os.listdir(dirName):
    if not folder[:6] == 'Thread':
        continue
    print(folder)
    pathName = dirName + "/" + folder
    staticSpeedup, dynamicSpeedup, guidedSpeedup  = [0]*len(threads), [0]*len(threads), [0]*len(threads)

    matrix_name = folder.split('_')[1]
    
    for file in os.listdir(pathName):
        if (not file.split('.')[-1] == "txt"):
            continue
        print(file, end='\n')

        DF = pd.read_csv("{}/{}".format(pathName, file), sep='\t')
        static = np.array(DF['Static'])
        dynamic = np.array(DF['Dynamic'])
        guided = np.array(DF['Guided'])

        index = threads.index(int(file.split('.')[0]))
        staticSpeedup[index] = np.max(static)/np.min(static)
        dynamicSpeedup[index] = np.max(dynamic)/np.min(dynamic)
        guidedSpeedup[index] = np.max(guided)/np.min(guided)

    fig, ax = plt.subplots(figsize=(24, 12))

    plt.title("{} Speedups".format(matrix_name), fontsize='30', fontweight='bold')
    plt.xlabel('Number of Threads', fontsize='25', fontweight='bold')
    plt.ylabel('Speedup Factor', fontsize='25', fontweight='bold')

    plt.plot(threads, staticSpeedup,  'bo-', label='Static')
    plt.plot(threads, dynamicSpeedup, 'ro-', label='Dynamic')
    plt.plot(threads, guidedSpeedup,  'go-', label='Guided')

    plt.xticks(threads, [str(t) for t in threads], fontsize='20')
    plt.yticks(fontsize='20')
    ax.yaxis.set_minor_locator(tick.AutoMinorLocator(10))

    ax.grid(which='major', color='#CCCCCC', linestyle='-')
    ax.grid(which='minor', color='#CCCCCC', linestyle=':')

    plt.grid(True, which='both')
    plt.legend(fontsize='25')
    plt.savefig("{}/{}.jpg".format(destDir, matrix_name))
    # plt.show()
    plt.close()