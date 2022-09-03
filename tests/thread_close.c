#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

#define SIZE 256

int main(){

    int args[4];

    pthread_t thread_id[4];

    char *path = "/f1";
    char *path2 = "/f2";
    char *path3 = "/f3";
    char *path4 = "/f4";

    assert(tfs_init() != -1);

    args[0] = tfs_open(path, TFS_O_CREAT);
    assert(args[0] != -1);

    args[1] = tfs_open(path2, TFS_O_CREAT);
    assert(args[1] != -1);

    args[2] = tfs_open(path3, TFS_O_CREAT);
    assert(args[2] != -1);

    args[3] = tfs_open(path4, TFS_O_CREAT);
    assert(args[3] != -1);

    for (int i=0; i < 4; i++){
        assert(pthread_create (&thread_id[i], NULL , (void*)&tfs_close, (void*)&args[i]) == 0);
    }
    for (int i=0; i < 4; i++){
        pthread_join (thread_id[i], NULL);
    }

    printf("Sucessful test\n");

    return 0;
}