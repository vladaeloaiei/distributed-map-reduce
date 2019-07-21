# distributed-map-reduce
Distributed implementation of Map-Reduce using MPI

* How to run: `mpirun -np [number_of_processes] bin/dmr.out [input_directory_path] [output_directory_path]`
* The result of map phase is stored into `[output_directory_path]/map[index].txt`
* The result of reduce phase is stored into `[output_directory_path]/result.txt`