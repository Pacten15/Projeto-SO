#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

#define SIZE 256
struct arg_struct {
        int arg1;
        char arg2[SIZE];
        size_t arg3;
    };
void* read_args( void *arguments){
    struct arg_struct *args = (struct arg_struct*)arguments;
    tfs_read(args->arg1, args->arg2, args->arg3);
    return NULL;
    }
int main(){

    

    struct arg_struct args[4];

    pthread_t thread_id[4];

    char *path = "/f1";
    char *path2 = "/f2";
    char *path3 = "/f3";
    char *path4 = "/f4";

    char input[4][SIZE];

    /* Writing this buffer multiple times to a file stored on 1KB blocks will 
       always hit a single block (since 1KB is a multiple of SIZE=256) */
    memset(&input[0], 'A', SIZE); 
    memset(&input[1], 'B', SIZE); 
    memset(&input[2], 'C', SIZE);
    memset(&input[3], 'D', SIZE);

    assert(tfs_init() != -1);

    args[0].arg1 = tfs_open(path, TFS_O_CREAT);
    assert(args[0].arg1 != -1);

    args[1].arg1 = tfs_open(path2, TFS_O_CREAT);
    assert(args[1].arg1 != -1);

    args[2].arg1 = tfs_open(path3, TFS_O_CREAT);
    assert(args[2].arg1 != -1);

    args[3].arg1 = tfs_open(path4, TFS_O_CREAT);
    assert(args[3].arg1 != -1);

    for (int i = 0; i < 4; i++){
        args[i].arg3 = SIZE;
    }

    for (int i = 0; i < 4; i++){
        assert(tfs_write(args[i].arg1, input[i], SIZE) != -1);
    }

    for (int i=0; i < 4; i++){
        assert(tfs_close(args[i].arg1) != -1);
    }
    args[0].arg1 = tfs_open(path, 0);
    assert(args[0].arg1 != -1);

    args[1].arg1 = tfs_open(path2, 0);
    assert(args[1].arg1 != -1);

    args[2].arg1 = tfs_open(path3, 0);
    assert(args[2].arg1 != -1);

    args[3].arg1 = tfs_open(path4, 0);
    assert(args[3].arg1 != -1);

    for (int i=0; i < 4; i++){
        assert(pthread_create (&thread_id[i], NULL , (void*)&read_args, (void*)&args[i]) == 0);
    }

    for (int i=0; i < 4; i++){
        pthread_join (thread_id[i], NULL);
    }

    for (int i=0; i < 4; i++){
        assert(tfs_close(args[i].arg1) != -1);
    }

    for (int i = 0; i < 4; i++) {
        assert (memcmp(input[i], args[i].arg2, SIZE) == 0);
    }


    printf("Sucessful test\n");

    return 0;
}