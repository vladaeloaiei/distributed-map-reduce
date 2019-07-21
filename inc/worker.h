#ifndef WORKER_H_
#define WORKER_H_

/*******************************************
 *          FUNCTION DECLARATION
 ******************************************/

/**
 * @brief   Function called by a worker to do the tasks assigned by master
 * @param[in] worker_rank - The curently process rank
 * @param[in] output_dir_path - The directory path in which the result will be stored
 * @return void
 **/
void do_worker(const int worker_rank, const char *output_dir_path);

#endif /* WORKER_H_ */