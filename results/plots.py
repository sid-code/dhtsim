import matplotlib.pylab as plt
import re
from typing import List, Tuple

BandwidthRecord = Tuple[int, int]
SuccessRecord = int
FailureRecord = int

def read_transcript(fname: str) -> Tuple[List[BandwidthRecord],
                                         List[SuccessRecord],
                                         List[FailureRecord]]:
    bwline = r'\[E\] T (\d+) (\d+)'
    sucline = r'\[E\] S \d+ \d+ (\d+)'
    failine = r'\[E\] F \d+ \d+ (\d+)'

    bws: List[BandwidthRecord] = []
    succs: List[SuccessRecord] = []
    fails: List[FailureRecord] = []
    with open(fname, 'r') as f:
        for line in f:
            m = re.match(bwline, line)
            if m is not  None:
                bws.append( (int(m.group(1)), int(m.group(2))) )
            m = re.match(sucline, line)
            if m is not None:
                succs.append(int(m.group(1)))
            m = re.match(failine, line)
            if m is not None:
                fails.append(int(m.group(1)))

    return (bws,succs, fails)

def mean(xs: List[float]) -> float:
    return sum(xs)/len(xs)
    
if __name__ == '__main__':
    import sys
    k=sys.argv[1]
    print("k=", k)
    (bw,succs,fails) = read_transcript('/home/sid/tmp/log_k'+str(k)+'.txt')
    #plt.plot([ p[0] for p in bw ], [ p[1] for p in bw ])
    #plt.show()
    print("Average latency", mean(succs))
    print("Average bw ", mean([p[1] for p in bw]))
    print("Failure rate", len(fails)/(len(succs)+len(fails))*100)
