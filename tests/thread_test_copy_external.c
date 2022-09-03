#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


#define SIZE 256

/*definir uma estrutura que contem os argumentos sendo primeiro um path do tfs 
e o segundo um path para o sistema de ficheiros do pc*/
struct arg_struct {
        char* arg1;
        char* arg2;
    };
/*recebe a estrutura do argumento e usa a função de copia de conteudo para um ficheiro exterior usando
os elementos da estrutura*/
void* copy_args( void *arguments){
    struct arg_struct *args = (struct arg_struct*)arguments;
    tfs_copy_to_external_fs(args->arg1, args->arg2);
    return NULL;
    }
int main(){

    
    /*inicializar os 4 conjunto de argumentos que vamos usar */
    struct arg_struct args[4];

    /*inicializar as tarefas para cada tfs_copy_to_external executado*/
    pthread_t thread_id[4];

    /*colocar os paths do tfs nos respetivos elementos da estrutura definida*/
    args[0].arg1 = "/f1";
    args[1].arg1 = "/f2";
    args[2].arg1 = "/f3";
    args[3].arg1 = "/f4";
     /*colocar os paths do sistema de ficheiros exterior(nosso pc) nos 
     respetivos elementos da estrutura definida*/
    args[0].arg2 = "external_file.txt";
    args[1].arg2 = "external_file2.txt";
    args[2].arg2 = "external_file3.txt";
    args[3].arg2 = "external_file4.txt";
    /*inicializar o conteudo que vai ser copiado do tfs para o exterior
    sendo que testamos com 4 strings de tamanho 5 uma para cada argumento*/
    char to_read[4][5];

    /*As strings que vamos usar*/
    char input[4][5]={"AAAA!", "BBBB!", "CCCC!", "DDDD!"};

    /*inicializacao da estrutura onde vao ficar os file_handles*/
    int file[4];

    assert(tfs_init() != -1);
    
    /*criar quatro ficheiros no tfs*/
    for (int i=0; i < 4; i++){
        file[i] = tfs_open(args[i].arg1, TFS_O_CREAT);
        assert(file[i] != -1);
    }
    /*escrever nesse quatro ficheiros*/
    for (int i = 0; i < 4; i++){
        assert(tfs_write(file[i], input[i], SIZE) != -1);
    }

    /*Fechar os ficheiros ja criados e escritos*/
    for (int i=0; i < 4; i++){
        assert(tfs_close(file[i]) != -1);
    }
    /*criar a tarefa para cada um destes ficheiros para realizacao sincrona do
    tfs_copy_to_external_fs*/
    for (int i=0; i < 4; i++){
        assert(pthread_create (&thread_id[i], NULL , (void*)&copy_args, (void*)&args[i]) == 0);
    }

    /*sincronizacao das tarefas*/
    for (int i=0; i < 4; i++){
        pthread_join (thread_id[i], NULL);
    }
    
    for (int i=0; i < 4; i++){
        /*abrir respetivamente para cada ficheiro do sistema de ficheiros do pc com o conteúdo ja copiado*/
        FILE *fp = fopen(args[i].arg2, "r");

        assert(fp != NULL);
        /*ler o ficheiro e verificar se o tamanho do conteudo lido e igual ao do input*/
        assert(fread(to_read[i], sizeof(char), strlen(input[i]), fp) == strlen(input[i]));

        /*comparar o conteudo do input com o que foi lido*/
        assert(strcmp(input[i], to_read[i]) == 0);

        assert(fclose(fp) != -1);

    }
    
    /*eliminar os ficheiros criados no sistema de ficheiros do pc*/
    for(int i=0;i<4;i++)
    {
        unlink(args[i].arg2);
    }

    printf("Successful test.\n");

    return 0;
}