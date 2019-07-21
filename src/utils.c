/*******************************************
 *              INCLUDES
 ******************************************/
#include <stdio.h>  /* stdout/stderr   */
#include <stdarg.h> /* varargs         */
#include <stdlib.h> /* NULL            */
#include <ctype.h>  /* tolower         */
#include <string.h> /* strerror        */
#include "utils.h"

/*******************************************
 *                DEFINES
 ******************************************/
#define LOG_FILE "log.txt"

/*******************************************
 *          FUNCTION DEFINITION
 ******************************************/

/**
 * @brief   Function called to log a message
 *          to the provided stream (stdio, stderr)
 *          and into log.txt file
 * @param[in] stream - The stream in which the message will be printed
 * @param[in] format - The format of the message
 * @param[in] ...    - Variable length parameters list
 * @return  void
 **/
void log_message(FILE *stream, const char *format, ...)
{
    FILE *log_file = fopen(LOG_FILE, "a");
    va_list vargs_stream;
    va_list vargs_file;

    va_start(vargs_stream, format);
    va_start(vargs_file, format);

    vfprintf(stream, format, vargs_stream);

    if (NULL != log_file)
    {
        vfprintf(log_file, format, vargs_file);
        fclose(log_file);
    }

    va_end(vargs_stream);
    va_end(vargs_file);
}

/**
 * @brief   Function called to retrieve the next 'File' found in a directory given as argument
 * @param[in] input_dir - The directory structure from which the file will be retrieved
 * @return  Pointer to 'File' or NULL in case of EOS (end of stream).
 *          In case of error, the function return NULL and set errno to EBADF
 * @note    This structure may be statically allocated; do not attempt to free(3) it.
 **/
File *get_next_file_from_dir(DIR *input_dir)
{
    File *file_found = readdir(input_dir);

    while (NULL != file_found)
    {
        if (DT_REG == file_found->d_type)
        {
            break;
        }
        else
        {
            file_found = readdir(input_dir);
        }
    }

    return file_found;
}

/**
 * @brief   The implementation for built-in strlwr funtion
 * @param[in] str - The string that will be processed
 * @return  The input pointer.
 * @note    The input string is modified and the retrieved pointer to string is returned.
 *          Do not try to free it.
 **/
char *utils_strlwr(char *str)
{
    unsigned char *p = (unsigned char *)str;

    while (*p)
    {
        *p = tolower((unsigned char)*p);
        p++;
    }

    return str;
}

/**
 * @brief   Function used to insert a new pair file_name, word into a Dictionary.
 *          This function consider the dictionary as being of type: < docIDx, {termk : countk} [] >
 * @param[in] dic       - Dictionary in which the pair will be stored
 * @param[in] file_name - Name of the file
 * @param[in] word      - Word
 * @return  0 for success or -1 in case or error
 * @note    Doesn't matter if the dictionary is empty.
 *          If it is initialized with 0, the memory will be allocated with malloc.
 **/
int insert_word_into_dictionary(Dictionary *dic, const char *file_name, const char *word)
{
    int error_code = -1;
    int key_index = -1;
    void *temp_pointer = NULL; /* In case that realloc fails. Exit the program without memory leak */

    //log_message(stderr, "UTILS: %s(): TEST: size %d .\n", __FUNCTION__, dic[0].size);

    /* Check if the file_name already exists. If not, allocate memory for it */
    for (int i = 0; i < dic[0].elements_length; ++i)
    {
        if (0 == strcmp(file_name, dic[0].elements[i].key))
        {
            key_index = i;
        }
    }

    if (-1 == key_index)
    {
        //log_message(stderr, "UTILS: %s(): TEST: -1 == index .\n", __FUNCTION__);
        /* The file_name does not exist in the dictionary
         * Create a new Pair and insert it into dictionary */
        temp_pointer = (Pair *)realloc(dic[0].elements, (dic[0].elements_length + 1) * sizeof(Pair));

        if (NULL != temp_pointer)
        {
            //log_message(stderr, "UTILS: %s(): TEST: NULL != temp_pointer .\n", __FUNCTION__);
            dic[0].elements = temp_pointer;
            key_index = dic[0].elements_length++;

            /* initialize the new allocated memory */
            dic[0].elements[key_index].key = (char *)calloc(strlen(file_name) + 1, sizeof(char));
            dic[0].elements[key_index].values = (char **)calloc(1, sizeof(char *));
            dic[0].elements[key_index].counts = (int *)calloc(1, sizeof(int));
            dic[0].elements[key_index].values_length = 0;

            if ((NULL != dic[0].elements[key_index].key) &&
                (NULL != dic[0].elements[key_index].values) &&
                (NULL != dic[0].elements[key_index].counts))
            {
                strcpy(dic[0].elements[key_index].key, file_name);
                dic[0].elements[key_index].values_length = 1;
                dic[0].elements[key_index].values[0] = (char *)calloc(strlen(word) + 1, sizeof(char));

                if (NULL != dic[0].elements[key_index].values[0])
                {
                    dic[0].elements[key_index].counts[0] = 1;
                    strcpy(dic[0].elements[key_index].values[0], word);
                    error_code = 0;
                }
                else
                {
                    log_message(stderr, "UTILS: %s(): Out of memory! .\n", __FUNCTION__);
                }
            }
            else
            {
                log_message(stderr, "UTILS: %s(): Out of memory! .\n", __FUNCTION__);
            }
        }
    }
    else
    {
        /* The file_name exists in the dictionary
         * Check if the word already exists in the word's list 
         * If not, write it*/
        int word_found = 0;

        for (int i = 0; i < dic[0].elements[key_index].values_length; ++i)
        {
            if (0 == strcmp(dic[0].elements[key_index].values[i], word))
            {
                word_found = 1;
                ++dic[0].elements[key_index].counts[i];
                error_code = 0;
            }
        }

        /* word not found, allocate memory for it */
        if (0 == word_found)
        {
            temp_pointer = (char **)realloc(dic[0].elements[key_index].values, (dic[0].elements[key_index].values_length + 1) * sizeof(char *));

            if (NULL != temp_pointer)
            {
                dic[0].elements[key_index].values = temp_pointer;
                temp_pointer = (char **)realloc(dic[0].elements[key_index].counts, (dic[0].elements[key_index].values_length + 1) * sizeof(int));

                if (NULL != temp_pointer)
                {
                    int value_index = 0;

                    dic[0].elements[key_index].counts = temp_pointer;
                    value_index = dic[0].elements[key_index].values_length++;

                    dic[0].elements[key_index].values[value_index] = (char *)calloc(strlen(word) + 1, sizeof(char));

                    if (NULL != dic[0].elements[key_index].values[value_index])
                    {
                        strcpy(dic[0].elements[key_index].values[value_index], word);
                        dic[0].elements[key_index].counts[value_index] = 1;
                    }
                    else
                    {
                        log_message(stderr, "UTILS: %s(): Out of memory! .\n", __FUNCTION__);
                    }
                }
                else
                {
                    log_message(stderr, "UTILS: %s(): Out of memory! .\n", __FUNCTION__);
                }
            }
            else
            {
                log_message(stderr, "UTILS: %s(): Out of memory! .\n", __FUNCTION__);
            }
        }
    }

    return error_code;
}

/**
 * @brief   Function used to insert a new pair word, file_name into a Dictionary.
 *          This function consider the dictionary as being of type: < termk,{docIDx : countk} >
 * @param[in] dic       - Dictionary in which the pair will be stored
 * @param[in] word      - Word
 * @param[in] file_name - Name of the file
 * @param[in] count     - Word count
 * @return  0 for success or -1 in case or error
 * @note    Doesn't matter if the dictionary is empty.
 *          If it is initialized with 0, the memory will be allocated with malloc.
 **/
int insert_file_into_dictionary(Dictionary *dic, const char *word, const char *file_name, int count)
{
    int error_code = -1;
    int key_index = -1;
    void *temp_pointer = NULL; /* In case that realloc fails. Exit the program without memory leak */

    //log_message(stderr, "UTILS: %s(): TEST: size %d .\n", __FUNCTION__, dic[0].elements_length);

    /* Check if the word already exists. If not, allocate memory for it */
    for (int i = 0; i < dic[0].elements_length; ++i)
    {
        if (0 == strcmp(word, dic[0].elements[i].key))
        {
            key_index = i;
        }
    }

    if (-1 == key_index)
    {
        //log_message(stderr, "UTILS: %s(): TEST: -1 == index .\n", __FUNCTION__);
        /* The word does not exist in the dictionary
         * Create a new Pair and insert it into dictionary */
        temp_pointer = (Pair *)realloc(dic[0].elements, (dic[0].elements_length + 1) * sizeof(Pair));

        if (NULL != temp_pointer)
        {
            //log_message(stderr, "UTILS: %s(): TEST: NULL != temp_pointer .\n", __FUNCTION__);
            dic[0].elements = temp_pointer;
            key_index = dic[0].elements_length++;

            /* initialize the new allocated memory */
            dic[0].elements[key_index].key = (char *)calloc(strlen(word) + 1, sizeof(char));
            dic[0].elements[key_index].values = (char **)calloc(1, sizeof(char *));
            dic[0].elements[key_index].counts = (int *)calloc(1, sizeof(int));
            dic[0].elements[key_index].values_length = 0;

            if ((NULL != dic[0].elements[key_index].key) &&
                (NULL != dic[0].elements[key_index].values) &&
                (NULL != dic[0].elements[key_index].counts))
            {
                dic[0].elements[key_index].values_length = 1;
                strcpy(dic[0].elements[key_index].key, word);
                dic[0].elements[key_index].values[0] = (char *)calloc(strlen(file_name) + 1, sizeof(char));

                if (NULL != dic[0].elements[key_index].values[0])
                {
                    dic[0].elements[key_index].counts[0] = count;
                    strcpy(dic[0].elements[key_index].values[0], file_name);
                    error_code = 0;
                }
                else
                {
                    log_message(stderr, "UTILS: %s(): Out of memory! .\n", __FUNCTION__);
                }
            }
            else
            {
                log_message(stderr, "UTILS: %s(): Out of memory! .\n", __FUNCTION__);
            }
        }
    }
    else
    {
        /* The word exists in the dictionary
         * Check if the file_name already exists in the file_name's list 
         * If not, write it*/
        int file_name_found = 0;

        for (int i = 0; i < dic[0].elements[key_index].values_length; ++i)
        {
            if (0 == strcmp(dic[0].elements[key_index].values[i], file_name))
            {
                file_name_found = 1;
                error_code = 0;
            }
        }

        /* if file_name not found, allocate memory for it */
        if (0 == file_name_found)
        {
            temp_pointer = (char **)realloc(dic[0].elements[key_index].values, (dic[0].elements[key_index].values_length + 1) * sizeof(char *));

            if (NULL != temp_pointer)
            {
                dic[0].elements[key_index].values = temp_pointer;
                temp_pointer = (char **)realloc(dic[0].elements[key_index].counts, (dic[0].elements[key_index].values_length + 1) * sizeof(int));

                if (NULL != temp_pointer)
                {
                    int value_index = 0;

                    dic[0].elements[key_index].counts = temp_pointer;
                    value_index = dic[0].elements[key_index].values_length++;
                    dic[0].elements[key_index].values[value_index] = (char *)calloc(strlen(file_name) + 1, sizeof(char));

                    if (NULL != dic[0].elements[key_index].values[value_index])
                    {
                        strcpy(dic[0].elements[key_index].values[value_index], file_name);
                        dic[0].elements[key_index].counts[value_index] = count;
                        error_code = 0;
                    }
                    else
                    {
                        log_message(stderr, "UTILS: %s(): Out of memory! .\n", __FUNCTION__);
                    }
                }
                else
                {
                    log_message(stderr, "UTILS: %s(): Out of memory! .\n", __FUNCTION__);
                }
            }
            else
            {
                log_message(stderr, "UTILS: %s(): Out of memory! .\n", __FUNCTION__);
            }
        }
    }

    return error_code;
}

/**
 * @brief   Function used to free the dynamically allocated memory from a Dictionary.
 * @param[in] dic - Dictionary which will be deleted
 * @return  void
 **/
void free_dictionary(Dictionary *dic)
{
    for (int i = 0; i < dic[0].elements_length; ++i)
    {
        for (int j = 0; j < dic[0].elements[i].values_length; ++j)
        {
            free(dic[0].elements[i].values[j]);
            dic[0].elements[i].values[j] = NULL;
            dic[0].elements[i].counts[j] = 0;
        }

        free(dic[0].elements[i].key);
        free(dic[0].elements[i].values);
        free(dic[0].elements[i].counts);

        dic[0].elements[i].key = NULL;
        dic[0].elements[i].values = NULL;
        dic[0].elements[i].counts = NULL;
    }

    free(dic[0].elements);
    dic[0].elements = NULL;
}