#include <mpi.h>
#include <iostream>

#include "grep.h"

int main (int argc, char * argv[])
{
    if(argc != 3) {
        std::cout << "Expected 2 inputs, got " << argc - 1 << std::endl;
        return 0;
    }

    MPI_Init(&argc, &argv);

    grep::lines_found local_filtered_lines;
    unsigned local_lines_number;
    std::vector<std::string> input_lines;

    grep::get_lines(input_lines, argv[2]);
    grep::search_string(input_lines, argv[1], local_filtered_lines, local_lines_number);
    grep::print_result(local_filtered_lines, local_lines_number);

    MPI_Finalize();

    return 0;
}