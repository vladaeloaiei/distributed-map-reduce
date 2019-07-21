/*******************************************
 *                INCLUDES
 ******************************************/
#include <stdio.h>  /* stdout/stderr   */
#include <stdlib.h> /* dynamic memory  */
#include "master.h" /* master          */
#include "worker.h" /* worker          */
#include "utils.h"  /* log             */
#include "mpi.h"

/*******************************************
 *                 MAIN
 ******************************************/
int main(int argc, char **argv)
{
    int my_rank = -1;
    int workers_count = -1;

    if (3 != argc)
    {
        log_message(stderr, "%s():Invalid number of input parameters! Expected %d, received %d.\n", __FUNCTION__, 3, argc);
    }
    else
    {
        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &workers_count);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        --workers_count; /* the master assign tasks to workers */

        if (0 == my_rank)
        {
            do_master(argv[1], workers_count);
        }
        else
        {
            do_worker(my_rank, argv[2]);
        }

        MPI_Finalize();
    }

    return 0;
}