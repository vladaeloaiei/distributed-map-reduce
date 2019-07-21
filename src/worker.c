/*******************************************
 *                INCLUDES
 ******************************************/
#include <stdio.h>  /* stdout/stderr   */
#include <string.h> /* strcmp */
#include "mpi.h"
#include "worker.h"
#include "utils.h"

/*******************************************
 *                DEFINES
 ******************************************/
#define MIN_WORD_SIZE 3
#define MAX_WORD_SIZE 128
#define MAX_LINE_SIZE (MAX_WORD_SIZE + 10)
#define WORD_DELIMITER "�!?.,_-*&()[]{}|/:;~\" \t\n1234567890"

/*******************************************
 *      STATIC FUNCTION DECLARATION
 ******************************************/

/**
 * @brief Function called by a worker to do the work durring map phase
 * @param[in] worker_rank     - The process rank
 * @param[in] output_dir_path - Path of the directory in which the result of the map phase will be stored
 * @return void
 **/
static void worker_map_phase(const int worker_rank, const char *output_dir_path);

/**
 * @brief Function called by worker to parse a file durring in map phase
 * @param[in] worker_rank      - The process rank
 * @param[in] input_file_path  - Path of the file that will be parsed
 * @param[in] output_file_path - Path of the file in which the result will be stored
 * @return void
 **/
static void worker_parse_file(const int worker_rank, const char *input_file_path, const char *output_file_path);

/**
 * @brief Function called by a worker to do the work durring reduce phase
 * @param[in] worker_rank    - The process rank
 * @param[in] input_dir_path - Path of the directory that will be parsed
 * @param[out] result        - Dictionary in which the result is stored
 * @return void
 **/
static void worker_reduce_phase(const int worker_rank, const char *input_dir_path, Dictionary *result);

/**
 * @brief Function called by worker to write the result
 * @param[in] worker_rank     - The process rank
 * @param[in] output_dir_path - Path of the directory in which the result is stored
 * @param[in] result          - Dictionary that contains the result that will be stored
 * @return void
 **/
static void worker_store_result_phase(const int worker_rank, const char *output_dir_path, Dictionary *result);

/*******************************************
 *      STATIC FUNCTION DEFINITION
 ******************************************/

/**
 * @brief Function called by a worker to do the work durring map phase
 * @param[in] worker_rank     - The process rank
 * @param[in] output_dir_path - Path of the directory in which the result of the map phase will be stored
 * @return void
 **/
static void worker_map_phase(const int worker_rank, const char *output_dir_path)
{
    char file_to_parse[MAX_PATH] = {'\0'};
    char output_file_path[MAX_PATH] = {'\0'};
    MPI_Status master_status = {0};

    if ('/' != output_dir_path[strlen(output_dir_path) - 1])
    {
        snprintf(output_file_path, MAX_PATH, "%s/map%d.txt", output_dir_path, worker_rank);
    }
    else
    {
        snprintf(output_file_path, MAX_PATH, "%smap%d.txt", output_dir_path, worker_rank);
    }

    MPI_Recv(file_to_parse, MAX_PATH, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &master_status);

    while (TAG_WORK == master_status.MPI_TAG)
    {
        log_message(stdout, "Worker: %s(): The worker nr. %d received file '%s' to parse.\n", __FUNCTION__, worker_rank, file_to_parse);
        worker_parse_file(worker_rank, file_to_parse, output_file_path);
        log_message(stdout, "Worker: %s(): The worker nr. %d finished to parse file '%s'.\n", __FUNCTION__, worker_rank, file_to_parse);

        /* Notify that the worker finished. */
        MPI_Send(file_to_parse, strlen(file_to_parse), MPI_CHAR, master_status.MPI_SOURCE, TAG_WORK, MPI_COMM_WORLD);
        /* clear the buffer to be reused */
        memset(file_to_parse, '\0', MAX_PATH);
        /* Get the master's feedback. */
        MPI_Recv(file_to_parse, MAX_PATH, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &master_status);
    }
}

/**
 * @brief Function called by worker to parse a file durring in map phase
 * @param[in] worker_rank      - The process rank
 * @param[in] input_file_path  - Path of the file that will be parsed
 * @param[in] output_file_path - Path of the file in which the result will be stored
 * @return void
 **/
static void worker_parse_file(const int worker_rank, const char *input_file_path, const char *output_file_path)
{
    FILE *input_file = NULL;
    FILE *output_file = NULL;
    char *word = NULL;
    char buffer[MAX_WORD_SIZE] = {'\0'};
    Dictionary file_words = {0};

    input_file = fopen(input_file_path, "r");
    output_file = fopen(output_file_path, "a");

    if (NULL == input_file)
    {
        log_message(stderr, "Worker: %s(): The worker nr. %d failed to open file '%s'.\n", __FUNCTION__, worker_rank, input_file_path);
    }
    else if (NULL == output_file)
    {
        log_message(stderr, "Worker: %s(): The worker nr. %d failed to open file '%s'.\n", __FUNCTION__, worker_rank, output_file_path);
    }
    else
    {
        while (EOF != fscanf(input_file, "%s", buffer))
        {
            word = strtok(buffer, WORD_DELIMITER);

            do
            {
                if ((NULL != word) && (MIN_WORD_SIZE <= strlen(word)))
                {
                    insert_word_into_dictionary(&file_words, input_file_path, utils_strlwr(word));
                }

                word = strtok(NULL, WORD_DELIMITER);
            } while (NULL != word);

            /* clear the buffer to be reused */
            memset(buffer, '\0', MAX_WORD_SIZE);
        }

        /* Now store the words and counts in worker's output file */

        for (int i = 0; i < file_words.elements_length; ++i)
        {
            /* First of all, write the file path */
            fprintf(output_file, "%s\n", input_file_path);

            /* Now, write all the words:counts contained in the file */
            for (int j = 0; j < file_words.elements[i].values_length; ++j)
            {
                fprintf(output_file, "%s:%d\n", file_words.elements[i].values[j], file_words.elements[i].counts[j]);
            }

            /* Finally, write an end of line representing the end of the word list */
            fprintf(output_file, "\n");
        }

        /* Now free the memory */
        free_dictionary(&file_words);
    }

    if ((NULL != input_file) && (0 != fclose(input_file)))
    {
        log_message(stderr, "Worker: %s(): The worker nr. %d failed to close file '%s'.\n", __FUNCTION__, worker_rank, input_file_path);
    }

    if ((NULL != output_file) && (0 != fclose(output_file)))
    {
        log_message(stderr, "Worker: %s(): The worker nr. %d failed to close file '%s'.\n", __FUNCTION__, worker_rank, output_file_path);
    }
}

/**
 * @brief Function called by a worker to do the work durring reduce phase
 * @param[in] worker_rank    - The process rank
 * @param[in] input_dir_path - Path of the directory that will be parsed
 * @param[out] result        - Dictionary in which the result is stored
 * @return void
 **/
static void worker_reduce_phase(const int worker_rank, const char *input_dir_path, Dictionary *result)
{
    DIR *input_directory = opendir(input_dir_path);
    File *file_from_dir = NULL;

    char input_file_path[MAX_PATH] = {'\0'};
    char file_name[MAX_PATH] = {'\0'};
    char word[MAX_WORD_SIZE] = {'\0'};
    char line[MAX_LINE_SIZE] = {'\0'};
    char bounds_for_reduce[] = {'A' - 1, 'A' - 1};
    int count = 0;

    MPI_Status master_status = {0};
    FILE *input_file = NULL;

    MPI_Recv(bounds_for_reduce, sizeof(bounds_for_reduce), MPI_CHAR, MPI_ANY_SOURCE, TAG_WORK, MPI_COMM_WORLD, &master_status);
    log_message(stdout, "Worker: %s(): The worker nr. %d received the bounds: [%c, %c] for reduce phase.\n",
                __FUNCTION__, worker_rank, bounds_for_reduce[0], bounds_for_reduce[1]);

    if (NULL == input_directory)
    {
        log_message(stderr, "Worker: %s(): The worker nr. %d failed to open dir: %s.\n", __FUNCTION__, worker_rank, input_dir_path);
    }
    else
    {
        /* parse all the files from the directory */
        file_from_dir = get_next_file_from_dir(input_directory);

        while (NULL != file_from_dir)
        {
            if ('/' != input_dir_path[strlen(input_dir_path) - 1])
            {
                snprintf(input_file_path, MAX_PATH, "%s/%s", input_dir_path, file_from_dir->d_name);
            }
            else
            {
                snprintf(input_file_path, MAX_PATH, "%s%s", input_dir_path, file_from_dir->d_name);
            }

            /* open the input file and reduce for the received bounds */
            input_file = fopen(input_file_path, "r");

            if (NULL == input_file)
            {
                log_message(stderr, "Worker: %s(): The worker nr. %d failed to open file: %s.\n", __FUNCTION__, worker_rank, input_file_path);
            }
            else
            {
                /* Get the first line (representing the input file) */
                while (NULL != fgets(file_name, MAX_PATH, input_file))
                {
                    file_name[strlen(file_name) - 1] = '\0';
                    /* Now read all the words from that file */
                    while ((NULL != fgets(line, MAX_LINE_SIZE, input_file)) && (0 != strcmp(line, "\n")))
                    {
                        line[strlen(line) - 1] = '\0';
                        sscanf(line, "%[^:]:%d", word, &count);

                        /* process only the words from the given limit */
                        if ((bounds_for_reduce[0] <= word[0]) && (word[0] <= bounds_for_reduce[1]))
                        {
                            if (0 != insert_file_into_dictionary(result, word, file_name, count))
                            {
                                log_message(stderr, "Worker: %s(): The worker nr. %d failed to insert into dictionary.\n",
                                            __FUNCTION__, worker_rank);
                            }
                        }

                        /* clear the buffer to be reused */
                        memset(line, '\0', MAX_LINE_SIZE);
                        memset(word, '\0', MAX_WORD_SIZE);
                        count = 0;
                    }

                    /* clear the buffer to be reused */
                    memset(file_name, '\0', MAX_PATH);
                }

                if (0 != fclose(input_file))
                {
                    log_message(stderr, "Worker: %s(): The worker nr. %d failed to close file '%s'.\n", __FUNCTION__, worker_rank, input_file_path);
                }
            }

            /* get another file */
            file_from_dir = get_next_file_from_dir(input_directory);
        }
    }

    /* Notify that the worker finished */
    log_message(stdout, "Worker: %s(): The worker nr. %d finished the reduce for the bounds: [%c, %c].\n",
                __FUNCTION__, worker_rank, bounds_for_reduce[0], bounds_for_reduce[1]);
    MPI_Send(bounds_for_reduce, sizeof(bounds_for_reduce), MPI_CHAR, master_status.MPI_SOURCE, TAG_SLEEP, MPI_COMM_WORLD);
}

/**
 * @brief Function called by worker to write the result
 * @param[in] worker_rank     - The process rank
 * @param[in] output_dir_path - Path of the directory in which the result is stored
 * @param[in] result          - Dictionary that contains the result that will be stored
 * @return void
 **/
static void worker_store_result_phase(const int worker_rank, const char *output_dir_path, Dictionary *result)
{
    FILE *output_file = NULL;
    char output_file_name[MAX_PATH] = {'\0'};
    char output_file_path[MAX_PATH] = {'\0'};
    MPI_Status master_status = {0};

    MPI_Recv(output_file_name, MAX_PATH, MPI_CHAR, MPI_ANY_SOURCE, TAG_WORK, MPI_COMM_WORLD, &master_status);

    log_message(stdout, "Worker: %s(): The worker nr. %d received signal to store the result into file: '%s'.\n",
                __FUNCTION__, worker_rank, output_file_name);

    if ('/' != output_dir_path[strlen(output_dir_path) - 1])
    {
        snprintf(output_file_path, MAX_PATH, "%s/%s", output_dir_path, output_file_name);
    }
    else
    {
        snprintf(output_file_path, MAX_PATH, "%s%s", output_dir_path, output_file_name);
    }

    /* open the file and write the result */
    output_file = fopen(output_file_path, "a");

    if (NULL == output_file)
    {
        log_message(stdout, "Worker: %s(): The worker nr. %d failed to open file: '%s'.\n",
                    __FUNCTION__, worker_rank, output_file_path);
    }
    else
    {
        /* parse the dictionary and store the result */
        /* free the dynamicaly allocated memory */
        for (int i = 0; i < result[0].elements_length; ++i)
        {
            fprintf(output_file, "%s: ", result[0].elements[i].key);

            for (int j = 0; j < result[0].elements[i].values_length; ++j)
            {
                fprintf(output_file, "<%s: %d>", result[0].elements[i].values[j], result[0].elements[i].counts[j]);
            }

            fprintf(output_file, "\n");
        }

        log_message(stdout, "Worker: %s(): The worker nr. %d finished to write the result into file: '%s'.\n",
                    __FUNCTION__, worker_rank, output_file_path);

        if (0 != fclose(output_file))
        {
            log_message(stderr, "Worker: %s(): The worker nr. %d failed to close file '%s'.\n", __FUNCTION__, worker_rank, output_file_path);
        }
    }

    /* Notify that the worker finished */
    MPI_Send(output_file_path, MAX_PATH, MPI_CHAR, master_status.MPI_SOURCE, TAG_WORK, MPI_COMM_WORLD);

    /* Receive the master's feedback */
    MPI_Recv(output_file_path, MAX_PATH, MPI_CHAR, MPI_ANY_SOURCE, TAG_SLEEP, MPI_COMM_WORLD, &master_status);
    /* My job is done here */
}

/*******************************************
 *          FUNCTION DEFINITION
 ******************************************/

/**
 * @brief   Function called by a worker to do the tasks assigned by master
 * @param[in] worker_rank - The curently process rank
 * @param[in] output_dir_path - The directory path in which the result will be stored
 * @return void
 **/
void do_worker(const int worker_rank, const char *output_dir_path)
{
    Dictionary reduce_phase_result = {0};

    log_message(stdout, "Worker: %s(): The worker nr. %d: Hello guys!\n", __FUNCTION__, worker_rank);
    worker_map_phase(worker_rank, output_dir_path);
    worker_reduce_phase(worker_rank, output_dir_path, &reduce_phase_result);
    worker_store_result_phase(worker_rank, output_dir_path, &reduce_phase_result);
    log_message(stdout, "Worker: %s(): The worker nr. %d: Good bye guys! See you tomorrow!\n", __FUNCTION__, worker_rank);

    /* free the dynamicaly allocated memory */
    free_dictionary(&reduce_phase_result);
}
