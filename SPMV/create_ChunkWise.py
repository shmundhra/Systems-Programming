import os
import sys
import subprocess
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import (AutoMinorLocator, MultipleLocator)

InputFiles = ["mat1.bin", "mat2.bin", "mat3.bin", "mat4.bin"]
ChunkSize = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000,
             20000, 50000, 100000, 200000, 500000, 1000000]
TotalThreads = [i+1 for i in range(24)]

ObjectFile = "spmv_norm"
Compile = subprocess.Popen(['g++', '-std=c++11', '-O3', sys.argv[1], '-fopenmp', '-o', ObjectFile])
Compile.wait()

for file in InputFiles:
    srcPath = "matrices/" + file
    dirName = file.split('.')[0]
    os.mkdir("Chunk_{}".format(dirName))

    for chunk in ChunkSize:
        fileName = dirName + "/" + str(chunk)

        with open("{}.txt".format(fileName), "w") as f:
            f.write("{}\t{}\t{}\n".format("Static", "Dynamic", "Guided"))

            for numThreads in TotalThreads:
                execTime = []

                for schedMode in range(3):
                    StdOut = subprocess.Popen(['./' + ObjectFile, srcPath, str(chunk), 
                                               str(numThreads), str(schedMode), str(10000)],
                                              stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE)
                    Output = StdOut.communicate()[0].decode()
                    execTime.append(list(map(float, Output.split()))[-1])

                f.write("{}\t{}\t{}\n".format(execTime[0], execTime[1], execTime[2]))
