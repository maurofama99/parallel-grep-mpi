# Grep parallelization using MPI
Keyword search algorithm that works like grep function, distributing the work of the grep function over multiple processes. From original grep, outputs differ only in the trailing spaces at the end of each line.

## Speedup Analysis
The execution time of the algorithm is measured first executing with only one processor, in order to set a base line. Is then measured the execution time with different number of processors, calculating the speedup for each configuration.

### Theoretic speedup modeling

To evaluate the performance of the algorithm, was measured the time taken to execute the loop that searches for the given string in the text file.
After *scattering* the input text equally across the processes, this loop is executed **in parallel** by each process. \
The input text file is divided equally among the processes, so as the number of processes increases, the portions sent to each one will be smaller, decreasing the number of iterations to be made per search. \

```
for (int i = 0; i < input_strings.size(); i++) {
            if (input_strings[i].find(search_string) != std::string::npos) {
                std::string line(input_strings[i]);
                line.append(LINELENGTH - line.size(), ' ');

                std::pair<unsigned, std::string> found(starting_line + i + 1, line);
                lines.push_back(found);
                local_lines_number++;
            }
}
```
This is the for loop which searches for the given word in the portion of file received by each process. The complexity depends on the number of lines received by the process: \
Given *n* lines in the input text file to analyze and *p* process used for the execution, the complexity is O(n/p). \
* Time to solve problem of input size n on one processor, using sequential algorithm: O(n)
* Time to solve on p processors: O(n/p)
* Speedup on p processors: p (linear speedup)

### Measured Speedup

The effectively measured speedup is not the theoretical one, for the obvious overhead introduced by communications. However, a speedup that increases as the number of processes increases has actually been measured. \

* Speedup on 2 processes: 1.47x
* Speedup on 3 processes: 2.32x
* Speedup on 5 processes: 2.94x
