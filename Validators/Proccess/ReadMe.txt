To compile this program you can run the command
gcc -pthread -o output mssv.c

Then you can run the program using 
./output sudoku maxwait
where maxwait is an integer and sudoku is the name of your sudoku solution.
The program will not run if you do not provide both arguments.