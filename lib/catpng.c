#include "catpng.h"

#define BUF_LEN  (1024*1024)
#define BUF_LEN2 (1024*1024)

U8 gp_buf_def[BUF_LEN2]; /* output buffer for mem_def() */
U8 gp_buf_inf[BUF_LEN2]; /* output buffer for mem_inf() */
U8 IDAT_CONCAT_def[BUF_LEN2];

int PNG_HEADER_LENGTH = 8;
U32 png_dimensions[2];
U32 global_height =0 ;

typedef struct recv_buf2 {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
                     /* <0 indicates an invalid seq number */
} RECV_BUF;

int test1(int argc, char **argv, void *arg) {
    printf("HELLO\n");
    struct recv_buf2 *all_pngs = arg;
    printf("bye\n");
    unsigned char **all_chunk = malloc(50*sizeof(unsigned char*)); //a char array ptr for char array
    all_chunk[0] = NULL;
    U64 IDAT_length;
    U64 len_inf = 0; 
    U32 len_def = 0;
    U64 IDAT_len_def=0; // lenght of the inflated data in the file


    // Inflating/Concating the IDAT_def of all files 
    for(int i =0; i <50; i++){
        // printf("%d\n", all_pngs[i].seq);
        struct recv_buf2 png = all_pngs[i];
        int k = 8;      // To iterate through the buffers
        for(int j=0; j < 3; j++){       // To iterare through each block
            U32 data_len= 0;
            if(j != 1){
                memcpy(&data_len, all_pngs[i].buf+k, 4);
                data_len = ntohl(data_len);
                // printf("length: %d\n", data_len);
                k += data_len + 12;
                // printf("iterator: %d\n", k);
            }
            else       // IDAT
            {      
                memcpy(&data_len, all_pngs[i].buf+k, 4);
                k+=8;       // Skip the len and type field to data
                data_len = ntohl(data_len);
                IDAT_length = data_len;
                unsigned char *chunk_data = (char*)malloc(data_len);
                memcpy(chunk_data, all_pngs[i].buf+k, data_len);
                k += data_len + 4;
                all_chunk[i] = chunk_data;
                chunk_data = NULL;   
            }
        }

        int ret = mem_inf(gp_buf_inf, &len_inf, all_chunk[i], IDAT_length);
        printf("MEMORY INF %d\n", len_inf);

        if (ret == 0) { /* success */
            printf("original len = %d, len_inf = %lu\n", BUF_LEN, len_inf);

                memcpy(IDAT_CONCAT_def+ IDAT_len_def, gp_buf_inf, len_inf);
                IDAT_len_def += len_inf;

        } else { /* failure */
            zerr(ret);
        }
    }

    // Extract/deflate the data from the file
    int ret = mem_def(gp_buf_def, &len_def, IDAT_CONCAT_def, IDAT_len_def, Z_DEFAULT_COMPRESSION);
    if (ret == 0) { /* success */
        printf("original len = %d, len_def = %lu\n", BUF_LEN, len_def);
    } else { /* failure */
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
    }
    // free(IDAT_CONCAT_def);
    // IDAT_CONCAT_def = NULL;

    // Create a New PNG
    int size_of_new_png = (8+25+12+12+len_def);
    U8 *new_png = malloc(sizeof(U8) * size_of_new_png);

    create_HEADER(new_png);
    create_IHDR(new_png, global_height, ntohl(png_dimensions[0]));
    create_IDAT(new_png, len_def, gp_buf_def);
    create_IEND(new_png, len_def);

    FILE *f2 = fopen("all.png", "w");
    fwrite(new_png, 1, size_of_new_png, f2);
    fclose(f2);
    

    // Deallocating Arrays and data

    for (int i =0; i < 50; i++){
        free(all_chunk[i]);
        all_chunk[i] = NULL;
    }

    free(all_chunk);
    all_chunk = NULL;

    free(new_png);
    new_png = NULL;
    return 0;
}

void create_HEADER(U8 *arr){
    arr[0] = 0x89;
    arr[1] = 0x50;
    arr[2] = 0x4E;
    arr[3] = 0x47;
    arr[4] = 0x0D;
    arr[5] = 0x0A;
    arr[6] = 0x1A;
    arr[7] = 0x0A;
    return;
}


void create_IHDR(U8 *arr, U32 height, U32 width){
    height = ntohl(300);
    width = ntohl(400);

    U32 hdr_length = 13;
    hdr_length = ntohl(hdr_length);


    U8 *new_height = (U8*)&height;
    U8 *new_width = (U8*)&width;
    U8 *new_length = (U8*)&hdr_length;
    
    memcpy(arr+8, new_length, 4);   // stores the length [8 .. 11]
    memcpy(arr+16, new_width, 4);   // stores the width [16 .. 19]
    memcpy(arr+20, new_height, 4);  // stores the height [20 .. 23]

    arr[12] = 'I';
    arr[13] = 'H';
    arr[14] = 'D';
    arr[15] = 'R';

    arr[24] = 8;
    arr[25] = 6;
    arr[26] = 0;
    arr[27] = 0;
    arr[28] = 0;
    
    // Now need to create the crc and send to the png_arr

    U32 crc_val = crc(&arr[12], 13+4);
    crc_val = ntohl(crc_val);
    U8 *new_crc = (U8*)&crc_val;

    memcpy(arr+29, new_crc, 4);
    
    return;
}

void create_IDAT(U8 *arr, U32 length_data, U8 *idat_data){
    length_data = ntohl(length_data);

    U8 *new_length = (U8*)&length_data;

    memcpy(arr + 33, new_length, 4);


    arr[37] = 'I';
    arr[38] = 'D';
    arr[39] = 'A';
    arr[40] = 'T';

    memcpy(arr + 41, idat_data, ntohl(length_data));

    U32 crc_val = crc(&arr[37], ntohl(length_data)+4);
    crc_val = ntohl(crc_val);
    U8 *new_crc = (U8*)&crc_val;

    memcpy(arr+37+4+ntohl(length_data), new_crc, 4);
}

void create_IEND(U8 *arr, U32 IDAT_length_data){
    U32 to_start_printing = IDAT_length_data + 45;  // start index of the IDAT block

    U8 null_arr[8] = {0,0,0,0, 'I', 'E', 'N', 'D'};

    memcpy(arr+ to_start_printing, &null_arr, 8 );

    U32 crc_val = crc(&arr[to_start_printing+4], 4);
    crc_val = ntohl(crc_val);
    U8 *new_crc = (U8*)&crc_val;

    memcpy(arr+to_start_printing+8, new_crc, 4);

    
}