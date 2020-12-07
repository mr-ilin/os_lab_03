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

typedef struct {
    int thread_num;
    int th_count;

    int rows;
    int cols;
    int w_dim;

    float* matrix1;
    float* result1;
    float* matrix2;
    float* result2;
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

    const float* matrix1 = args->matrix1;
    const float* matrix2 = args->matrix2;
    float* result1 = args->result1;
    float* result2 = args->result2;
    // Либо 2 одновременно либо 2 ф-ии

    //printf("\n=== IN THREAD %d ===\n", thread_num);
    // printf("offset = %d\n", offset);

    for (int th_row = thread_num; th_row < rows; th_row += th_count) {
        //printf("THREAD %d ROW  %d\n", thread_num, th_row);
        for (int th_col = 0; th_col < cols; ++th_col) {
            //printf(" th_col = %d\n", th_col);
            float max = matrix1[th_row*cols + th_col];
            float min = matrix2[th_row*cols + th_col];
            
            for (int i = th_row - offset; i < th_row + offset + 1; ++i) {
                for (int j = th_col - offset; j < th_col + offset + 1; ++j) {
                    float curr1, curr2;
                    if ((i < 0) || (i >= rows) || (j < 0) || (j >= cols)) {
                        curr1 = 0;
                        curr2 = 0;
                    } else {
                        curr1 = matrix1[i*cols + j];
                        curr2 = matrix2[i*cols + j];
                    }
                    //printf("[%d][%d] = %f ", i, j, curr1);
                    if (curr1 > max) {
                        max = curr1;
                    }
                    if (curr2 < min) {
                        min = curr2;
                    }
                }
                //printf("\n");
            }
            result1[th_row*cols + th_col] = max;
            result2[th_row*cols + th_col] = min;
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

    pthread_t ids[th_count];
    thread_arg args[th_count];

    for (int k = 0; k < filter_cnt; ++k) {
        for (int i = 0; i < th_count; ++i) {
            args[i].thread_num = i;
            args[i].th_count = th_count;
            args[i].rows = rows;
            args[i].cols = cols;
            args[i].w_dim = w_dim;
            args[i].matrix1 = tmp1;
            args[i].result1 = res1;
            args[i].matrix2 = tmp2;
            args[i].result2 = res2;

            if (pthread_create(&ids[i], NULL, edit_line, &args[i]) != 0) {
                perror("Can't create a thread.\n");
            }
        }

        for(int i = 0; i < th_count; ++i) {
            if (pthread_join(ids[i], NULL) != 0) {
                perror("Can't wait for thread\n");
            }
        }
        
        if (filter_cnt > 1) {
            float* swap = res1;
            res1 = tmp1;
            tmp1 = swap;

            swap = res2;
            res2 = tmp2;
            tmp2 = swap;
            // copy_matrix(res1, tmp1, rows, cols);
            // copy_matrix(res2, tmp2, rows, cols);
        }
    }

    free(tmp1);
    free(tmp2);
}

int main(int argc, char* argv[]) {
    int pid = getpid();
    printf("PID <%d>\n", pid);
    
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
    

    printf("DILATION:\n");
    print_matrix(res1, rows, cols);
    printf("EROISON:\n");
    print_matrix(res2, rows, cols);
    printf("TOTAL TIME: %ld ms\n", elapsed);

    free(res1);
    free(res2);
    free(matrix);
    return 0;
}
