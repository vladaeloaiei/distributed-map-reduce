/*******************************************
 *                INCLUDES
 ******************************************/
#include <stdio.h>  /* stdout/stderr   */
#include <dirent.h> /* DIR             */
#include <errno.h>  /* errno           */
#include <string.h> /* strerror        */
#include <stdlib.h> /* dynamic memory  */
#include "mpi.h"
#include "master.h"
#include "utils.h"

/*******************************************
 *                DEFINES
 ******************************************/
#define ENGLISH_ALPHABET_SIZE ('z' - 'a' + 1)
#define RESULT_FILE_NAME "result.txt"

/*******************************************
 *       STATIC FUNCTION DECLARATION
 ******************************************/

/**
 * @brief Function called by master to assign files the workers durring map phase
 * @param[in] input_dir_path    - Input directory's path
 * @param[in] number_of_workers - Number of workers
 * @return void
 **/
static void master_map_phase(const char *input_dir_path, const int number_of_workers);

/**
 * @brief Function called by master to signal workers to start the reduce phase
 * @param[in] number_of_workers - Number of workers
 * @return void
 **/
static void master_reduce_phase(const int number_of_workers);

/**
 * @brief Function called by master to signal workers to write the result
 * @param[in] output_file_name  - The output file name in which the result will be stored
 * @param[in] number_of_workers - Number of workers
 * @return void
 **/
static void master_store_result_phase(const char *output_file_name, const int number_of_workers);

/*******************************************
 *       STATIC FUNCTION DEFINITION
 ******************************************/

/**
 * @brief Function called by master to assign files the workers durring map phase
 * @param[in] input_dir_path    - Input directory's path
 * @param[in] number_of_workers - Number of workers
 * @return void
 **/
static void master_map_phase(const char *input_dir_path, const int number_of_workers)
{
    DIR *input_directory = opendir(input_dir_path);
    File *input_file = NULL;
    char input_file_path[MAX_PATH] = {'\0'};
    int files_count = 0; /* Number of files sent to be parsed */

    if (NULL == input_directory)
    {
        log_message(stderr, "Master: %s(): Failed to open dir: %s. Errno: %s.\n", __FUNCTION__, input_dir_path, strerror(errno));
    }
    else
    {
        /* assign the first number_of_workers files to the workers */
        for (int i = 0; i < number_of_workers; ++i)
        {
            errno = 0;
            input_file = get_next_file_from_dir(input_directory);

            /* In case of end of stream or error */
            if (NULL == input_file)
            {
                /* If errno changed means that a problem occured while trying to read file from directory.
                 * Print error message and continue as usual */
                if (0 != errno)
                {
                    log_message(stderr, "Master: %s(): Failed to read file from directory. Errno %s", __FUNCTION__, strerror(errno));
                }

                log_message(stdout, "Master: %s(): There is no more work to do. Send the stop signal to the worker %d.\n", __FUNCTION__, i + 1);
                /* Send to worker that he has nothing to do in the phase */
                MPI_Send(INVALID_FILE, strlen(INVALID_FILE), MPI_CHAR, i + 1, TAG_SLEEP, MPI_COMM_WORLD);
            }
            else
            {
                /* Send the file to worker */
                if ('/' != input_dir_path[strlen(input_dir_path) - 1])
                {
                    snprintf(input_file_path, MAX_PATH, "%s/%s", input_dir_path, input_file->d_name);
                }
                else
                {
                    snprintf(input_file_path, MAX_PATH, "%s%s", input_dir_path, input_file->d_name);
                }

                log_message(stdout, "Master: %s(): File '%s' is sent to worker %d.\n", __FUNCTION__, input_file_path, i + 1);
                MPI_Send(input_file_path, strlen(input_file_path), MPI_CHAR, i + 1, TAG_WORK, MPI_COMM_WORLD);
                /* One file sent to be parsed, increment the counter */
                ++files_count;
                /* clear the buffer to be reused */
                memset(input_file_path, '\0', MAX_PATH);
            }
        }

        /* Now wait untill workers finishes their job and send another file untill all are parsed. */

        do
        {
            char parsed_file_name[MAX_PATH] = {'\0'};
            MPI_Status worker_status = {0};

            MPI_Recv(parsed_file_name, MAX_PATH, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &worker_status);
            /* One file was parsed, decrement the counter */
            --files_count;
            log_message(stdout, "Master: %s(): The worker nr. %d finished to parse file: '%s'.\n", __FUNCTION__, worker_status.MPI_SOURCE, parsed_file_name);

            errno = 0;
            input_file = get_next_file_from_dir(input_directory);

            if (NULL == input_file)
            {
                /* In case of end of stream or error.
                 * If errno changed means that a problem occured while trying to read the file from directory.
                 * Print error message and continue as usual */
                if (0 != errno)
                {
                    log_message(stderr, "Master: %s(): Failed to read file from directory. Errno %s", __FUNCTION__, strerror(errno));
                }

                log_message(stdout, "Master: %s(): There is no more work to do. Send the stop signal to the worker %d.\n", __FUNCTION__, worker_status.MPI_SOURCE);
                /* Send to worker that he has nothing to do in the phase */
                MPI_Send(INVALID_FILE, strlen(INVALID_FILE), MPI_CHAR, worker_status.MPI_SOURCE, TAG_SLEEP, MPI_COMM_WORLD);
            }
            else if (DT_REG == input_file->d_type)
            {
                /* Found a file, send it to worker */
                if ('/' != input_dir_path[strlen(input_dir_path) - 1])
                {
                    snprintf(input_file_path, MAX_PATH, "%s/%s", input_dir_path, input_file->d_name);
                }
                else
                {
                    snprintf(input_file_path, MAX_PATH, "%s%s", input_dir_path, input_file->d_name);
                }

                log_message(stdout, "Master: %s(): File '%s' is sent to worker %d.\n", __FUNCTION__, input_file_path, worker_status.MPI_SOURCE);
                MPI_Send(input_file_path, strlen(input_file_path), MPI_CHAR, worker_status.MPI_SOURCE, TAG_WORK, MPI_COMM_WORLD);
                /* One file sent to be parsed, increment the counter */
                ++files_count;
                /* clear the buffer to be reused */
                memset(input_file_path, '\0', MAX_PATH);
            }
        } while (0 < files_count);

        log_message(stdout, "Master: %s(): The workers parsed all the files from directory: '%s'. Map phase done!\n", __FUNCTION__, input_dir_path);

        closedir(input_directory);
    }
}

/**
 * @brief Function called by master to signal workers to start the reduce phase
 * @param[in] number_of_workers - Number of workers
 * @return void
 **/
static void master_reduce_phase(const int number_of_workers)
{
    /* Number of english alphabetic characters every worker should process.
     * e.g For 4 workers, the first worker should process every word that start with a caracter
     * from the interval: [a, g] (7 characters) */
    int bounds_step = ENGLISH_ALPHABET_SIZE / number_of_workers;
    char bounds_for_reduce[] = {'a' - 1, 'a' - 1};

    /* Increment bounds step in case that the ENGLISH_ALPHABET_SIZE > number_of_workers * bounds_step */
    if (0 != ENGLISH_ALPHABET_SIZE % number_of_workers)
    {
        ++bounds_step;
    }

    for (int i = 0; i < number_of_workers; ++i)
    {
        /* Calculate the bounds for the i + 1 worker
         * The left limit is the previous right limit + 1
         * e.g. If the previous iteration the bounds were [a, d] the current one the bounds are [e, h]
         * The right limit is the current left limit + bounds_step */
        bounds_for_reduce[0] = bounds_for_reduce[1] + 1;
        bounds_for_reduce[1] = bounds_for_reduce[0] + bounds_step - 1;

        /* If the bounds are not in the range a-z, the worker will process only the words
         * in range [a, z] & [bound0, bound1] */

        log_message(stdout, "Master: %s(): Send start reduce phase to worker %d with bounds: [%c, %c].\n",
                    __FUNCTION__, i + 1, bounds_for_reduce[0], bounds_for_reduce[1]);
        MPI_Send(bounds_for_reduce, sizeof(bounds_for_reduce), MPI_CHAR, i + 1, TAG_WORK, MPI_COMM_WORLD);
    }

    /* Now wait untill workers finishes their job and signal when to write the result . */
    log_message(stdout, "Master: %s(): The workers are in the reduce phase. Wait untill they finish their job!\n", __FUNCTION__);

    for (int i = 0; i < number_of_workers; ++i)
    {
        MPI_Status worker_status = {0};

        MPI_Recv(bounds_for_reduce, sizeof(bounds_for_reduce), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &worker_status);
        log_message(stdout, "Master: %s(): The worker nr. %d finished the reduce phase for bounds: [%c, %c].\n",
                    __FUNCTION__, worker_status.MPI_SOURCE, bounds_for_reduce[0], bounds_for_reduce[1]);
    }

    log_message(stdout, "Master: %s(): The workers finished. The reduce phase is done!\n", __FUNCTION__);
}

/**
 * @brief Function called by master to signal workers to write the result
 * @param[in] output_file_name  - The output file name in which the result will be stored
 * @param[in] number_of_workers - Number of workers
 * @return void
 **/
static void master_store_result_phase(const char *output_file_name, const int number_of_workers)
{
    MPI_Status worker_status = {0};
    char temp_buffer[MAX_PATH] = {'\0'};

    for (int i = 0; i < number_of_workers; ++i)
    {
        MPI_Send(output_file_name, strlen(output_file_name), MPI_CHAR, i + 1, TAG_WORK, MPI_COMM_WORLD);
        log_message(stdout, "Master: %s(): Sent the signal to worker nr. %d to write the result into file: %s.\n",
                    __FUNCTION__, i + 1, output_file_name);

        MPI_Recv(temp_buffer, sizeof(temp_buffer), MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &worker_status);
        log_message(stdout, "Master: %s(): The worker nr. %d wrote the result into file: %s.\n",
                    __FUNCTION__, i + 1, temp_buffer);

        MPI_Send("You did well. It's time to go home.", strlen("You did well. It's time to go home."), MPI_CHAR, i + 1, TAG_SLEEP, MPI_COMM_WORLD);
        log_message(stdout, "Master: %s(): Sent the signal to worker nr. %d to go to his family.\n",
                    __FUNCTION__, i + 1);
    }
}

/*******************************************
 *          FUNCTION DEFINITION
 ******************************************/

/**
 * @brief Function called by master to schedule the workers
 * @param[in] input_dir_path - Input directory path
 * @param[in] number_of_workers - Number of workers
 * @return void
 **/
void do_master(const char *input_dir_path, const int number_of_workers)
{
    log_message(stdout, "Master: %s(): The master: Hello world!\n", __FUNCTION__);
    master_map_phase(input_dir_path, number_of_workers);
    master_reduce_phase(number_of_workers);
    master_store_result_phase(RESULT_FILE_NAME, number_of_workers);
    log_message(stdout, "Master: %s(): The master: Good bye cruel world!\n", __FUNCTION__);
}