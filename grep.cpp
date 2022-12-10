#include <fstream>

#include <mpi.h>
#include <algorithm>
#include "grep.h"

//    grep::get_lines(input_lines, argv[2]);
//    grep::search_string(input_lines, argv[1], local_filtered_lines, local_lines_number);
//    grep::print_result(local_filtered_lines, local_lines_number);

namespace grep{

    // Function get_lines must allow process with rank 0 to read the input file, and then split the input
    // data into chunks that are evenly spread across available processes. You can safely assume that all
    // lines read from the input file are shorter than LINELENGTH, that is 80 characters.
    void get_lines(std::vector<std::string> &input_string,
                   const std::string &file_name){

        char* data = nullptr;
        size_t num_rows = 0, rows_per_process;
        std::vector<std::string> buffer;

        int rk, num_procs;
        MPI_Comm_rank(MPI_COMM_WORLD, &rk);
        MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

        // process 0 reads the input file
        if (rk == 0) {

            std::ifstream f_stream(file_name);
            // read input file line by line
            for (std::string inputline; std::getline(f_stream, inputline);) {
                //increment local number of lines
                ++num_rows;
                if (inputline.size() < LINELENGTH){
                    for (int j = 0; j < LINELENGTH - inputline.size(); j++){
                        inputline.append(" ");
                    }
                }
                // insert line into buffer
                buffer.push_back(inputline);
            }
            f_stream.close();

            data = (char*) malloc(num_rows*LINELENGTH*sizeof(char));
            char *dataptr = data;

            for (int i = 0; i < num_rows; i++){
                sprintf(dataptr, "%.80s", buffer[i].c_str());
                dataptr += LINELENGTH;
            }

        } // input file is now stored in data

        // Broadcast the total number of rows so each process will retrieve how many rows will receive
        MPI_Bcast(&num_rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
        rows_per_process = num_rows/num_procs;

        /**
         * INPUT TEXT SCATTER
         */
         if (num_rows % 2 == 0){
             char * chunk = (char*) malloc (rows_per_process*LINELENGTH);
             MPI_Scatter(data, rows_per_process*(LINELENGTH), MPI_CHAR, chunk, rows_per_process*(LINELENGTH), MPI_CHAR, 0, MPI_COMM_WORLD);

             char *chunkptr = chunk;
             for (int i = 0; i < rows_per_process; i++){
                 input_string.emplace_back(chunkptr);
                 chunkptr += LINELENGTH;
             }
             free(chunk);
         } else {
             if (rk == num_procs - 1) rows_per_process++;
             char *chunk = (char *) malloc((rows_per_process) * LINELENGTH);

             if (rk == 0) {
                 int sendcounts[num_procs];
                 for (int i = 0; i < num_procs - 1; i++) {
                     sendcounts[i] = rows_per_process * (LINELENGTH);
                 }
                 if (num_procs == 1) sendcounts[num_procs - 1] = (rows_per_process) * (LINELENGTH);
                 else sendcounts[num_procs - 1] = (rows_per_process + 1) * (LINELENGTH);

                 int displs[num_procs];
                 displs[0] = 0;
                 for (int i = 1; i < num_procs; i++) {
                     displs[i] = displs[i - 1] + sendcounts[i - 1];
                 }
                 MPI_Scatterv(data, sendcounts, displs, MPI_CHAR, chunk, (rows_per_process) * LINELENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);
             } else
                 MPI_Scatterv(data, nullptr, nullptr, MPI_CHAR, chunk, (rows_per_process) * LINELENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);

             char *chunkptr = chunk;
             for (int i = 0; i < rows_per_process; i++){
                 input_string.emplace_back(chunkptr);
                 chunkptr += LINELENGTH;
             }
             free(chunk);
         }

        free(data);
    }

    void search_string(const std::vector<std::string> & input_strings,
                       const std::string & search_string, // string we need to search
                       lines_found &lines, // lines we found
                       unsigned &local_lines_number) {
        int rk, starting_line, offset;
        local_lines_number = 0;
        MPI_Comm_rank(MPI_COMM_WORLD, &rk);

        if (rk == 0) offset = input_strings.size();
        MPI_Bcast(&offset, 1, MPI_INT, 0, MPI_COMM_WORLD);
        starting_line = rk * offset;

        for (int i = 0; i < input_strings.size(); i++) {
            if (input_strings[i].find(search_string) != std::string::npos) {
                std::string line(input_strings[i]);
                line.append(LINELENGTH - line.size(), ' ');

                std::pair<unsigned, std::string> found(starting_line + i + 1, line);
                lines.push_back(found);
                local_lines_number++;
            }
        }
    }

    void print_result(const lines_found & lines, unsigned local_lines_number){
        int rk, num_proc;
        MPI_Comm_rank(MPI_COMM_WORLD, &rk);
        MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
        int count[num_proc];
        int displ[num_proc];
        char * recvbuf;
        unsigned total_chars_found = 0;

        unsigned local_chars_found = local_lines_number * LINELENGTH;
        MPI_Gather(&local_chars_found, 1, MPI_UNSIGNED, count, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

        if (rk==0) {
            total_chars_found = 0;
            for(auto& num : count)
                total_chars_found += num;

            displ[0] = 0;
            for (int i = 1; i < num_proc; i++){
                displ[i] = displ[i-1] + count[i-1];
            }
            recvbuf = (char*) malloc (total_chars_found * sizeof(char));
        }

        /**
         * LINES GATHER
         */

        char * sendbuf = (char*) malloc (local_chars_found * sizeof(char));

        char * sendbufptr = sendbuf;
        for (const auto & line : lines){
            sprintf(sendbufptr, "%s", line.second.c_str());
            sendbufptr += LINELENGTH;
        }

        MPI_Gatherv(sendbuf, local_chars_found, MPI_CHAR, recvbuf, count, displ, MPI_CHAR, 0, MPI_COMM_WORLD);

        /**
         * LINES NUMBER GATHER
         */
        unsigned recvbufnum[total_chars_found / LINELENGTH];
        unsigned sendbufnum[lines.size()];
        for (int i = 0; i < lines.size(); i ++){
            sendbufnum[i] = lines[i].first;
        }

        if (rk == 0){
            int displnum[num_proc];

            for (int i = 0; i < num_proc; i++){
                count[i] = count[i] / LINELENGTH;
            }

            displnum[0] = 0;
            for (int i = 1; i < num_proc; i++){
                displnum[i] = displnum[i-1] + count[i-1];
            }

            MPI_Gatherv(sendbufnum, lines.size(), MPI_UNSIGNED, recvbufnum, count, displnum, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
        } else MPI_Gatherv(sendbufnum, lines.size(), MPI_UNSIGNED, nullptr, nullptr, nullptr, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

        // root process print results into file
        if (rk == 0) {
            FILE *fp = fopen("output.txt", "w+");
            int j = 0;
            fprintf(fp, "%d: %c", recvbufnum[j],recvbuf[j]);
            j++;
            for (int i = 1; i < total_chars_found; i++) {
                if (i % 81 == 0) {
                    fprintf(fp, "\n%d:", recvbufnum[j]);
                    j++;
                }
                fprintf(fp, "%c", recvbuf[i]);
            }
        }
    }
}
