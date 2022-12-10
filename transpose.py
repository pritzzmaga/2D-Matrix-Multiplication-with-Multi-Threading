import numpy as np

data = np.loadtxt('in2.txt').T
np.savetxt('in2.txt', data, fmt='%i')
print("transposing done")
