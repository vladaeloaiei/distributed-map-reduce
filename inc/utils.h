#ifndef UTILS_H_
#define UTILS_H_

/*******************************************
 *                INCLUDES
 ******************************************/
#include <dirent.h> /* DIR */

/*******************************************
 *                DEFINES
 ******************************************/
#define TAG_WORK 0
#define TAG_SLEEP 1

#define INVALID_FILE "${NOTAFILE}"
#define MAX_PATH 257

/*******************************************
 *                TYPES
 ******************************************/

/* typedef for struct dirent used as a File */
typedef struct dirent File;

/* struct used to store pair of <key, {value:count}[]> */
typedef struct Pair_
{
    char *key;
    char **values;
    int *counts;
    int values_length;
} Pair;

/* struct used to store an array of pairs */
typedef struct Dictionary_
{
    Pair *elements;
    int elements_length;
} Dictionary;

/*******************************************
 *          FUNCTION DECLARATION
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
void log_message(FILE *stream, const char *format, ...);

/**
 * @brief   Function called to retrieve the next 'File' found in a directory given as argument
 * @param[in] input_dir - The directory structure from which the file will be retrieved
 * @return  Pointer to 'File' or NULL in case of EOS (end of stream).
 *          In case of error, the function return NULL and set errno to EBADF
 * @note    This structure may be statically allocated; do not attempt to free(3) it.
 **/
File *get_next_file_from_dir(DIR *input_dir);

/**
 * @brief   The implementation for built-in strlwr funtion
 * @param[in] str - The string that will be processed
 * @return  The input pointer.
 * @note    The input string is modified and the retrieved pointer to string is returned.
 *          Do not try to free it.
 **/
char *utils_strlwr(char *str);

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
int insert_word_into_dictionary(Dictionary *dic, const char *file_name, const char *word);

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
int insert_file_into_dictionary(Dictionary *dic, const char *word, const char *file_name, int count);

/**
 * @brief   Function used to free the dynamically allocated memory from a Dictionary.
 * @param[in] dic - Dictionary which will be deleted
 * @return  void
 **/
void free_dictionary(Dictionary *dic);

#endif /* UTILS_H_ */