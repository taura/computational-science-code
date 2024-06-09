import matplotlib.pyplot as plt

DATA = r"""
1 xxxx GFLOPS
2 xxxx GFLOPS
3 xxxx GFLOPS
4 xxxx GFLOPS
6 xxxx GFLOPS
8 xxxx GFLOPS
12 xxxx GFLOPS
16 xxxx GFLOPS
20 xxxx GFLOPS
24 xxxx GFLOPS
28 xxxx GFLOPS
32 xxxx GFLOPS
"""

def main():
    data = DATA.strip().split("\n")
    X = []
    Y = []
    for line in data:
        fields = line.strip().split()
        if len(fields) != 3:
            continue
        x, y = fields[:2]
        X.append(float(x))
        Y.append(3.0)
    plt.ylabel("GFLOPS")
    plt.xlabel("num_threads")
    plt.plot(X, Y)
    plt.show()

main()
