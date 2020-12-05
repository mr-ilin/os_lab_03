#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <sys/time.h>

// 11.	Наложить K раз фильтры эрозии и наращивания на матрицу, состоящую из вещественных чисел. На выходе получается 2 результирующие матрицы
// Ввод - размеры матрицы, сама матрица
// Окно фильтров 3х3

void print_usage(char* cmd) {
    printf("Usage: %s [-threads num]\n", cmd);
}

bool read_matrix(float* matrix, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            if (scanf("%f", &matrix[i*cols + j]) != 1) {
                perror("Error while reading matrix");
                return false;
            }
            //scanf("%f", &matrix[i*cols + j]);
            //printf("=== matrix[%ld] = %f\n", i*cols + j, matrix[i*cols + j]);
        }
    }
    return true;;
}

bool print_matrix(float* matrix, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            printf("%.20g ", matrix[i*cols + j]);
        }
        printf("\n");
    }
    return false;
}

void copy_matrix(float* from, float* to, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            to[i*cols + j] = from[i*cols + j];
        }
    }
}

typedef enum {
    EROISON,
    DILATION
} filter_type;

typedef struct {
    int thread_num;
    int th_count;

    int rows;
    int cols;
    int w_dim;

    float* matrix;
    float* result;

    filter_type filter;
} thread_arg;

// Функция, которая будет выполняться в потоках
// Происзводит попоточную обработку строк матрицы
void* edit_line(void* argument) {
    thread_arg* args = (thread_arg*)argument;
    const int thread_num = args->thread_num;
    const int th_count = args->th_count;
    const int rows = args->rows;
    const int cols = args->cols;
    int offset = args->w_dim / 2;
    const float* matrix = args->matrix;
    const filter_type filter = args->filter;
    float* result = args->result;
    // Либо 2 одновременно либо 2 ф-ии

    //printf("\n=== IN THREAD %d - %d ===\n", thread_num, filter);
    //printf("offset = %d\n", offset);

    for (int th_row = thread_num; th_row < rows; th_row += th_count) {
        //printf("th_row = %d\n", th_row);
        for (int th_col = 0; th_col < cols; ++th_col) {
            //printf(" th_col = %d\n", th_col);
            float to_put = matrix[th_row*cols + th_col];
            
            for (int i = th_row - offset; i < th_row + offset + 1; ++i) {
                for (int j = th_col - offset; j < th_col + offset + 1; ++j) {
                    float curr;
                    if ((i < 0) || (i >= rows) || (j < 0) || (j >= cols)) {
                        curr = 0;
                    } else {
                        curr = matrix[i*cols + j];
                    }
                    //printf("[%d][%d] = %f ", i, j, curr);
                    if ((filter == EROISON) && (curr < to_put)) {
                        to_put = curr;
                    } 
                    if ((filter == DILATION) && (curr > to_put)) {
                        to_put = curr;
                    }
                }
                //printf("\n");
            }
            //printf("to_put = %f\n", to_put);
            result[th_row*cols + th_col] = to_put;
        }
        //printf("\n");
    }
    pthread_exit(NULL); // Заканчиваем поток
}

void put_filters(float* matrix, size_t rows, size_t cols, size_t w_dim, float* res1, float* res2, int filter_cnt, int th_count) {
    float* tmp1 = (float*)malloc(rows * cols * sizeof(float));
    if (!tmp1) {
        perror("Error while allocating matrix\n");
        exit(1);
    }
    float* tmp2 = (float*)malloc(rows * cols * sizeof(float));
    if (!tmp2) {
        perror("Error while allocating matrix\n");
        exit(1);
    }
    copy_matrix(matrix, tmp1, rows, cols);
    copy_matrix(matrix, tmp2, rows, cols);

    pthread_t eroison_ids[th_count];
    pthread_t dilation_ids[th_count];
    thread_arg eroison_args[th_count];
    thread_arg dilation_args[th_count];

    for (int k = 0; k < filter_cnt; ++k) {
        for (int i = 0; i < th_count; ++i) {
            eroison_args[i].thread_num = i;
            eroison_args[i].th_count = th_count;
            eroison_args[i].rows = rows;
            eroison_args[i].cols = cols;
            eroison_args[i].w_dim = w_dim;
            eroison_args[i].matrix = tmp1;
            eroison_args[i].result = res1;
            eroison_args[i].filter = EROISON;
            if (pthread_create(&eroison_ids[i], NULL, edit_line, &eroison_args[i]) != 0) {
                perror("Can't create a thread.\n");
            }

            dilation_args[i].thread_num = i;
            dilation_args[i].th_count = th_count;
            dilation_args[i].rows = rows;
            dilation_args[i].cols = cols;
            dilation_args[i].w_dim = w_dim;
            dilation_args[i].matrix = tmp2;
            dilation_args[i].result = res2;
            dilation_args[i].filter = DILATION;
            if (pthread_create(&dilation_ids[i], NULL, edit_line, &dilation_args[i]) != 0) {
                perror("Can't create a thread.\n");
            }
        }

        for(int i = 0; i < th_count; ++i) {
            if ((pthread_join(eroison_ids[i], NULL) != 0) || (pthread_join(dilation_ids[i], NULL) != 0)) {
                perror("Can't wait for thread\n");
            }
        }
        
        if (filter_cnt != 1) {
            copy_matrix(res1, tmp1, rows, cols);
            copy_matrix(res2, tmp2, rows, cols);
        }
    }

    free(tmp1);
    free(tmp2);
}

int main(int argc, char* argv[]) {
    int threads = 1;
    if (argc == 3) {
        threads = atoi(argv[2]);
    } else if (argc != 1) {
        print_usage(argv[0]);
        return 0;
    }
    printf("Total thread = %d\n", threads);
    
    int rows;
    int cols;
    printf("Enter matrix dimensions:\n");
    scanf("%d", &cols);
    scanf("%d", &rows);
    float* matrix = (float*)malloc(rows * cols * sizeof(float));
    float* res1 = (float*)malloc(rows * cols * sizeof(float));
    float* res2 = (float*)malloc(rows * cols * sizeof(float));
    if (!matrix || !res1 || !res2) {
        perror("Error while allocating matrix\n");
        return 1;
    }
    read_matrix(matrix, rows, cols);

    int w_dim;
    printf("Enter window dimension:\n");
    scanf("%d", &w_dim);
    if (w_dim % 2 == 0) {
        perror("Window dimension must be an odd number\n");
        return 1;
    }

    printf("Enter K:\n");
    int k;
    scanf("%d", &k);

    struct timeval start, end;
    gettimeofday(&start, NULL);
    put_filters(matrix, rows, cols, w_dim, res1, res2, k, threads);
    gettimeofday(&end, NULL);

    long sec = end.tv_sec - start.tv_sec;
    long microsec = end.tv_usec - start.tv_usec;
    if (microsec < 0) {
        --sec;
        microsec += 1000000;
    }
    long elapsed = sec*1000000 + microsec;
    

    printf("EROISON:\n");
    print_matrix(res1, rows, cols);
    printf("DILATION:\n");
    print_matrix(res2, rows, cols);
    printf("TOTAL TIME: %ld ms\n", elapsed);

    free(res1);
    free(res2);
    free(matrix);
    return 0;
}
