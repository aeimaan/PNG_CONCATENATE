#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>
#include <unistd.h> 
#include <pthread.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "lib/shm_stack.h"
#include "lib/catpng.h"
#include <sys/time.h>

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

typedef struct recv_buf2 {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
                     /* <0 indicates an invalid seq number */
} RECV_BUF;

#define SHM_SIZE 256
#define SEM_PROC 1
#define NUM_SEMS 5
sem_t *sems;
int *counters;
struct int_stack *pstack2;
char *inflated_buf;
#define IMG_URL "http://ece252-1.uwaterloo.ca:2530/image?img="
#define IMG_URL2 "http://ece252-2.uwaterloo.ca:2530/image?img="
#define IMG_URL3 "http://ece252-3.uwaterloo.ca:2530/image?img="
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */
#define BUF_LEN2 (1024*1024)
U8 gp_buf_inf[BUF_LEN2];
U8 gp_buf_def[BUF_LEN2]; 

// Function Declarations
int main(int argc, char ** argv);
void producer(int i, int n);
void consumer(int i);
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);
int X;


int main(int argc, char ** argv){
    
    double times[2];
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    int B = atoi(argv[1]);  // BUffer Size 
    int P = atoi(argv[2]);  // Num of Producers
    int C = atoi(argv[3]);  // Num of Consumers
    X = atoi(argv[4]);  // Num of mS consumeer sleeps before processing image
    int N = atoi(argv[5]);  // Image Number that we want 
    const int NUM_CHILD = P+C;
    int i=0;
    pid_t pid=0;
    pid_t cpids[NUM_CHILD];
    int state;
    int shm_size = sizeof_shm_stack(B);
    int inflated_size = 50*9606;
    int STACK_SIZE = B;

    // Setting up Shared Memory
    int shmid_sems = shmget(IPC_PRIVATE, sizeof(sem_t) * NUM_SEMS, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int shmid_counter = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int shmid_stack = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);    
    int shmid_inflated_buf = shmget(IPC_PRIVATE, inflated_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);    
    
    if (shmid_sems == -1 || shmid_counter == -1 || shmid_stack == -1 || 
        shmid_inflated_buf == -1) {
        perror("shmget");
        abort();
    }

    // Initializing Semaphores
    sems = shmat(shmid_sems, NULL, 0);
    counters = shmat(shmid_counter, NULL, 0);
    pstack2 = shmat(shmid_stack, NULL, 0);
    inflated_buf = shmat(shmid_inflated_buf, NULL, 0);
    if ( sems == (void *) -1 ||  
        pstack2 == (void *) -1 ||
        counters == (void *) -1) {
        perror("shmat");
        abort();
    }
    
    // INITIALIZING SHARED MEMORY 
    memset(counters, 0, SHM_SIZE);
    init_shm_stack(pstack2, STACK_SIZE);
    // sems = [ Mutex1 , Mutex2 , Mutex3 , Items, Spaces ]
    if ( sem_init(&sems[0], SEM_PROC, 1) != 0 ) {
        perror("sem_init(sem[0])");
        abort();
    }
    if ( sem_init(&sems[1], SEM_PROC, 1) != 0 ) {
        perror("sem_init(sem[1])");
        abort();
    }
    if ( sem_init(&sems[2], SEM_PROC, 1) != 0 ) {
        perror("sem_init(sem[2])");
        abort();
    }
    if ( sem_init(&sems[3], SEM_PROC, 0) != 0 ) {
        perror("sem_init(sem[3])");
        abort();
    }
    if ( sem_init(&sems[4], SEM_PROC, B) != 0 ) {
        perror("sem_init(sem[4])");
        abort();
    }


    // Creating Processes
    for ( i = 0; i < NUM_CHILD; i++) {
        
        pid = fork();

        if ( pid > 0 ) {        /* parent proc */
            cpids[i] = pid;
            
        } else if ( pid == 0 ) { /* child proc */
            if (i < P) {
                producer((i%3), N);      // Creates Producers/Consumers

                if ( shmdt(sems) != 0 ) {
                    printf("DAMNNNN3");
                    perror("shmdt");
                    abort();
                }
                if ( shmdt(counters) != 0 ) {
                    printf("DAMNNNN4");
                    perror("shmdt");
                    abort();
                }
                detach_stack(pstack2);
                if ( shmdt(pstack2) != 0 ) {
                    perror("shmdt");
                    abort();
                }
                if ( shmdt(inflated_buf) != 0 ) {
                    printf("DAMNNNN3");
                    perror("shmdt");
                    abort();
                }
            
            }
            else {
                consumer(i-P);
                if ( shmdt(sems) != 0 ) {
                    printf("DAMNNNN3");
                    perror("shmdt");
                    abort();
                }
                if ( shmdt(counters) != 0 ) {
                    printf("DAMNNNN4");
                    perror("shmdt");
                    abort();
                }
                detach_stack(pstack2);
                if ( shmdt(pstack2) != 0 ) {
                    perror("shmdt");
                    abort();
                }
                if ( shmdt(inflated_buf) != 0 ) {
                    printf("DAMNNNN3");
                    perror("shmdt");
                    abort();
                }
            }
            break;
        } else {
            perror("fork");
            abort();
        }
    }

    // Joining Processes
    if ( pid > 0 ) {            /* parent process */
        for ( i = 0; i < NUM_CHILD; i++ ) {
            waitpid(cpids[i], &state, 0);
        }
        // EVERYTHING DONE
        int len_def;
        int ret = mem_def(gp_buf_def, &len_def, inflated_buf, inflated_size, Z_DEFAULT_COMPRESSION);
        if (ret == 0) { /* success */
            printf("len_def = %lu\n", len_def);
        } else { /* failure */
            fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
        }

        int size_of_new_png = (8+25+12+12+len_def);
        U8 *new_png = malloc(sizeof(U8) * size_of_new_png);        

        create_HEADER(new_png);
        create_IHDR(new_png, 300, 400);
        create_IDAT(new_png, len_def, gp_buf_def);
        create_IEND(new_png, len_def);

        FILE *f2 = fopen("all.png", "w");
        fwrite(new_png, 1, size_of_new_png, f2);
        fclose(f2);

        if (gettimeofday(&tv, NULL) != 0) {
            perror("gettimeofday");
            abort();
        }
        times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
        printf("paster2 execution time: %.6lf seconds\n",  times[1] - times[0]);

        free(new_png);
        new_png = NULL;
    

        if ( shmdt(sems) != 0 ) {
            printf("DAMNNNN1");
            perror("shmdt");
            abort();
                }
        if ( shmdt(counters) != 0 ) {
            printf("AYOoo\n");
            perror("shmdt");
            abort();
            }
        if ( shmdt(inflated_buf) != 0 ) {
            printf("DAMNNNN3");
            perror("shmdt");
            abort();
        }
        destroy_stack(pstack2);
        if ( shmdt(pstack2) != 0 ) {
            perror("shmdt");
            abort();
        }
        
        if ( shmctl(shmid_sems, IPC_RMID, NULL) == -1 ) {
            perror("shmctl");
            abort();
            }
        if ( shmctl(shmid_counter, IPC_RMID, NULL) == -1 ) {
            perror("shmctl");
            abort();
        }
        if ( shmctl(shmid_stack, IPC_RMID, NULL) == -1 ) {
            perror("shmctl");
            abort();
        }
        if ( shmctl(shmid_inflated_buf, IPC_RMID, NULL) == -1 ) {
            perror("shmctl");
            abort();
        }
    }
    return 0;
}

void producer(int i, int n){
    // printf("Producer\n");

    char url[256];
    char tmp_url[256];

    if(i == 0){
        sprintf(tmp_url, "%s%d%s", IMG_URL ,n, "&part=");
    }else if (i  == 1)
    {
        sprintf(tmp_url, "%s%d%s", IMG_URL2 ,n, "&part=");
    }else{
        sprintf(tmp_url, "%s%d%s", IMG_URL3 ,n, "&part=");
    }
    while(1){
        sem_wait(&sems[0]);
        int tmp_count = counters[0]++;
        sem_post(&sems[0]);
        // printf("C %d : %d \n",i, tmp_count );
        if(tmp_count >= 50) {
            printf("COUNT %d\n", tmp_count);
            break;
        }
        // printf("P %d : %d \n",i, tmp_count );

        // CURL REQUEST HERE___________________________________
        CURL *curl_handle;
        CURLcode res;
        sprintf(url, "%s%d", tmp_url ,tmp_count);
        char fname[256];
        RECV_BUF recv_buf;

        recv_buf_init(&recv_buf, BUF_SIZE);
    
        // printf("URL is %s\n", url);

        curl_global_init(CURL_GLOBAL_DEFAULT);

        /* init a curl session */
        curl_handle = curl_easy_init();

        if (curl_handle == NULL) {
            fprintf(stderr, "curl_easy_init: returned NULL\n");
            return 1;
        }

        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv_buf);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
        curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        res = curl_easy_perform(curl_handle);
        if( res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
        printf("%lu bytes received in memory %p, seq=%d.\n", \
                recv_buf .size, recv_buf.buf, recv_buf.seq);
        }

        // sprintf(fname, "./output_%d.png", recv_buf.seq);
        // write_file(fname, recv_buf.buf, recv_buf.size);

         

        // _____________________________________________________
        sem_wait(&sems[4]);
        sem_wait(&sems[1]);
        // Store INTO buffer
        int ret = push(pstack2, &recv_buf);
        // printf("PUSHED ONTO STAck %d\n", recv_buf.seq);
        sem_post(&sems[1]);
        sem_post(&sems[3]);

         /* cleaning up */
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
        // recv_buf_cleanup(&recv_buf);       TRY And Deallocate later 
    }



    // return;
}

void consumer(int i){
    // printf("Consumer\n");

    while(1){
        sem_wait(&sems[2]);
        int tmp_count = counters[1]++;
        sem_post(&sems[2]);
        // printf("C %d : %d \n",i, tmp_count );
        if(tmp_count >= 50 ) {
            // printf("C DONE\n");
            break;
        }
        // printf("C %d : %d \n",i, tmp_count );

        sem_wait(&sems[3]);
        sem_wait(&sems[1]);
        // READ FROM THE BUFFER
        struct recv_buf2 item;
        int ret = pop(pstack2, &item);
        usleep(X*1000);         // Sleep for X mS
        int len_inf;
        // printf("POPPED OF STACK:%d\n", item.seq);
        U32 data_len = 0;
        memcpy(&data_len, item.buf+33, 4);
        data_len = ntohl(data_len);
        char chunk_data[data_len];
        memcpy(chunk_data, item.buf+41, data_len);

        int ret_val = mem_inf(gp_buf_inf, &len_inf, chunk_data, data_len);
        printf("MEMORY INF %d  INFLATED PICTURE: %d\n", len_inf, item.seq);

        memcpy(inflated_buf+(item.seq * 9606), gp_buf_inf, len_inf);

        sem_post(&sems[1]);
        sem_post(&sems[4]);
    }
    
}



// PROBLEMS______________________________________________

// Added the stack tester file to this so that we can 
// call stack here

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;
    
    if (realsize > strlen(ECE252_HEADER) &&
	strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
	p->seq = atoi(p_recv + strlen(ECE252_HEADER));
    

    }
    return realsize;
}

size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        /* received data is not 0 terminated, add one byte for terminating 0 */
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);   
        char *q = realloc(p->buf, new_size);
        if (q == NULL) {
            perror("realloc"); /* out of memory */
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}

int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void *p = NULL;
    
    if (ptr == NULL) {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL) {
	return 2;
    }
    
    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1;              /* valid seq should be non-negative */
    return 0;
}

int recv_buf_cleanup(RECV_BUF *ptr)
{
    if (ptr == NULL) {
	return 1;
    }
    
    free(ptr->buf);
    ptr->size = 0;
    ptr->max_size = 0;
    return 0;
}

int write_file(const char *path, const void *in, size_t len)


{
    FILE *fp = NULL;

    if (path == NULL) {
        fprintf(stderr, "write_file: file name is null!\n");
        return -1;
    }

    if (in == NULL) {
        fprintf(stderr, "write_file: input data is null!\n");
        return -1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("fopen");
        return -2;
    }

    if (fwrite(in, 1, len, fp) != len) {
        fprintf(stderr, "write_file: imcomplete write!\n");
        return -3; 
    }
    return fclose(fp);
}