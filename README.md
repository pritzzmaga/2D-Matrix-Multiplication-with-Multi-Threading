First run the MatrixGen file and input the dimensions of the two matricies you want to multiply, and the 
program will generate two random matrices of your specified size (ixj and jxk). If you dont want a random 
matrix, just paste the matrix you want in the file 'in1.txt' and 'in2.txt'
Then run transpose.py to preprocess the inputs

Compile the sched.c file and run the output file by passing the following arguments via command line:
	'i'
	'j'
	'k'
	'in1.txt'
	'in2.txt'
	'out.txt'

The resultant matrix should be available in in 'out.txt'

The actuall multiplication takes place in p2.c, and can be optimized. Majority of the time comes from
creating and joining the threads. We can further minimize the number of thread creates and thread joins
by calculate what all multiplications each thread will have to do beforehand and placing the 'for loops'
inside the thread runner function. This optimization could provide a speedup of around 40
