import re
import matplotlib.pyplot as plt

DATA = r"""
=====
n             : 512
nteams        : 1
nthreads      : 1
volume        : 0.523606822
error         : 8.046296e-06
elapsed       :   0.047
n^3 / nsec    :   2.885
=====
n             : 512
nteams        : 1
nthreads      : 2
volume        : 0.523606822
error         : 8.046296e-06
elapsed       :   0.047
n^3 / nsec    :   2.876
=====
n             : 512
nteams        : 1
nthreads      : 4
volume        : 0.523606822
error         : 8.046296e-06
elapsed       :   0.047
n^3 / nsec    :   2.882
=====
n             : 512
nteams        : 1
nthreads      : 6
volume        : 0.523606822
error         : 8.046296e-06
elapsed       :   0.047
n^3 / nsec    :   2.871
=====
n             : 512
nteams        : 1
nthreads      : 8
volume        : 0.523606822
error         : 8.046296e-06
elapsed       :   0.047
n^3 / nsec    :   2.882
=====
n             : 512
nteams        : 1
nthreads      : 9
volume        : 0.523606822
error         : 8.046296e-06
elapsed       :   0.047
n^3 / nsec    :   2.877
=====
n             : 512
nteams        : 1
nthreads      : 12
volume        : 0.523606822
error         : 8.046296e-06
elapsed       :   0.047
n^3 / nsec    :   2.883
=====
n             : 512
nteams        : 1
nthreads      : 18
volume        : 0.523606822
error         : 8.046296e-06
elapsed       :   0.046
n^3 / nsec    :   2.888
"""

def put_rec(R, D):
    num_teams = int(D["nteams"])
    num_threads = int(D["nthreads"])
    perf = float(D["n^3 / nsec"])
    R.append((num_teams * num_threads, perf))

def speedup(data):
    data = data.strip().split("\n")
    R = []                      # (nteams * nthreads, n^3/nsec)
    D = None
    pat = re.compile(r"(?P<k>[^:]+?) *: +(?P<v>\d+(\.\d+)?)")
    for line in data:
        if line.strip() == "=====":
            if D is not None:
                put_rec(R, D)
            D = {}
        else:
            m = pat.match(line)
            key = m.group("k")
            val = m.group("v")
            D[key] = val
    if D is not None:
        put_rec(R, D)
    plt.ylabel("n^3/nsec")
    plt.xlabel("num_teams * num_threads")
    R.sort()
    X = [x    for x, perf in R]
    Y = [perf for x, perf in R]
    L = [Y[0]/X[0] * x for x in X]
    plt.plot(X, Y)
    plt.plot(X, L)
    plt.show()

speedup(DATA)
