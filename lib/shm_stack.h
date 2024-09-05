
/* The code is 
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
 *
 * This software may be freely redistributed under the terms of the X11 License.
 */
/**
 * @brief  stack to push/pop integer(s), API header  
 * @author yqhuang@uwaterloo.ca
 */

// struct int_stack;
// typedef struct curl_res{
//     char *img_data;
//     int index;
// };
// typedef struct recv_buf2 {
//     char *buf;       /* memory to hold a copy of received data */
//     size_t size;     /* size of valid data in buf in bytes*/
//     size_t max_size; /* max capacity of buf in bytes*/
//     int seq;         /* >=0 sequence number extracted from http header */
//                      /* <0 indicates an invalid seq number */
// } ;

typedef struct int_stack
{
    int size;               /* the max capacity of the stack */
    int pos;                /* position of last item pushed onto the stack */
    struct recv_buf2 *items;             /* stack of stored integers */
    char* all_buf;
    int shmid;
} ISTACK;

int sizeof_shm_stack(int size);
int init_shm_stack(struct int_stack *p, int stack_size);
struct int_stack *create_stack(int size);
void destroy_stack(struct int_stack *p);
void detach_stack(struct int_stack *p);
int is_full(struct int_stack *p);
int is_empty(struct int_stack *p);
int push(struct int_stack *p, struct recv_buf2 *item);
int pop(struct int_stack *p, struct recv_buf2 *p_item);
