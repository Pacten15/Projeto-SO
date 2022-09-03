#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

#define SIZE 256
/*definir uma estrutura que contem os argumentos necessarios para usar a funcao tfs_write*/
struct arg_struct {
        int arg1;
        char arg2[SIZE];
        size_t arg3;
    };
/*funcao que recebe estrutura definida anteriormente e realiza o tfs_write com os elementos dessa estrutura*/
void* write_args( void *arguments){
    struct arg_struct *args = (struct arg_struct*)arguments;
    tfs_write(args->arg1, args->arg2, args->arg3);
    return NULL;
    }
int main(){

    /*inicializar os 4 conjunto de argumentos que vamos usar */
    struct arg_struct args[4];

    /*inicializar as tarefas para cada tfs_write executado*/
    pthread_t thread_id[4];

    /*colocar os paths do tfs nos respetivos elementos da estrutura definida*/
    char *path = "/f1";
    char *path2 = "/f2";
    char *path3 = "/f3";
    char *path4 = "/f4";

    /*Escrever no buffer(um dos elementos da estrutura) size vezes um caracter(segundo argumento) que vai ser guardados no block atingir um unico bloco pois
    o size e 1024 tratando-se de um multiplo de 256*/
     
    memset(args[0].arg2, 'A', SIZE);
    memset(args[1].arg2, 'B', SIZE);
    memset(args[2].arg2, 'C', SIZE);
    memset(args[3].arg2, 'D', SIZE);

    char output[SIZE];

    assert(tfs_init() != -1);

    /*criar os ficheiros com o respetivo nome(path) e colocamo-lo como o respetivo fhandle em cada  */
    args[0].arg1 = tfs_open(path, TFS_O_CREAT);
    assert(args[0].arg1 != -1);

    args[1].arg1 = tfs_open(path2, TFS_O_CREAT);
    assert(args[1].arg1 != -1);

    args[2].arg1 = tfs_open(path3, TFS_O_CREAT);
    assert(args[2].arg1 != -1);

    args[3].arg1 = tfs_open(path4, TFS_O_CREAT);
    assert(args[3].arg1 != -1);

    /*colocar o tamanho que vai ser lido o tfs_write em cada estrutura definida sendo este o terceiro argumento*/
    for (int i = 0; i < 4; i++){
        args[i].arg3 = SIZE;
    }

    for (int i=0; i < 4; i++){
        assert(pthread_create (&thread_id[i], NULL , (void*)&write_args, (void*)&args[i]) == 0);
    }

    for (int i=0; i < 4; i++){
        pthread_join (thread_id[i], NULL);
    }

    for (int i=0; i < 4; i++){
        assert(tfs_close(args[i].arg1) != -1);
    }

    args[0].arg1 = tfs_open(path, TFS_O_CREAT);
    assert(args[0].arg1 != -1);

    args[1].arg1 = tfs_open(path2, TFS_O_CREAT);
    assert(args[1].arg1 != -1);

    args[2].arg1 = tfs_open(path3, TFS_O_CREAT);
    assert(args[2].arg1 != -1);

    args[3].arg1 = tfs_open(path4, TFS_O_CREAT);
    assert(args[3].arg1 != -1);

    for (int i = 0; i < 4; i++) {
        assert(tfs_read(args[i].arg1, output, SIZE) == SIZE);
        assert (memcmp(args[i].arg2, output, SIZE) == 0);
    }

    for (int i=0; i < 4; i++){
        assert(tfs_close(args[i].arg1) != -1);
    }

    printf("Sucessful test\n");

    return 0;

}