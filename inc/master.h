#ifndef MASTER_H_
#define MASTER_H_

/*******************************************
 *          FUNCTION DECLARATION
 ******************************************/

/**
 * @brief Function called by master to schedule the workers
 * @param[in] input_dir_path - Input directory path
 * @param[in] number_of_workers - Number of workers
 * @return void
 **/
void do_master(const char *input_dir_path, const int number_of_workers);

#endif /* MASTER_H_ */