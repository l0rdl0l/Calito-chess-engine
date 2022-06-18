import subprocess
import re


def printData(arr):
    
    string = ""
    for i in arr:
        string += str(round(i, 1)).rjust(11)

    print(string)


file = open("benchmark-positions", "r")

nodes = []
times = []

maxDepth = 7

positionNum = 0
for x in file:

    nodes.append([])
    times.append([])

    fen = x[0:len(x)-1]

    p = subprocess.Popen("../src/CalitoEngine", text=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    p.stdin.write("uci\n")
    p.stdin.write("isready\n")
    p.stdin.write("position fen " + fen+"\n")
    p.stdin.write("go depth " + str(maxDepth) + "\n")
    p.stdin.flush()


    while(True) :
        line = p.stdout.readline()
        if(line[:10] == "info depth"):

            regex = re.compile("nodes ([0-9]+)")
            nodes[positionNum].append(int(re.search(regex, line)[1]))
            regex = re.compile("time ([0-9]+)")
            times[positionNum].append(int(re.search(regex, line)[1]))

        if(line[:8] == "bestmove"):
            break

    p.kill()
    print("position " + str(positionNum+1)+":")
    print("nodes explored: ", end="")
    printData(nodes[positionNum])
    print("time used in ms:", end="")
    printData(times[positionNum])

    positionNum += 1
    
nodeSum = []
timeSum = []

for i in range(maxDepth):
    nodeSum.append(0)
    timeSum.append(0)
    for j in range(positionNum):
        nodeSum[i] += nodes[j][i]
        timeSum[i] += times[j][i]
    
    nodeSum[i] /= positionNum
    timeSum[i] /= positionNum

print("average nodes explored:    ", end= "")
printData(nodeSum)
print("average time used (in ms): ", end = "")
printData(timeSum)