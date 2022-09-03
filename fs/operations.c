#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

pthread_mutex_t inode_lock;
pthread_mutex_t ext_file_lock;
int tfs_init() {
    
    state_init();
    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}


int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid*/
    if (!valid_pathname(name)) {
        return -1;
    }

    pthread_mutex_lock(&inode_lock);
    inum = tfs_lookup(name);
    if (inum >= 0) {
        /* The file already exists */

        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            pthread_mutex_unlock(&inode_lock);
            return -1;
        }

        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                if (free_all_blocks(inum) == -1) {
                    pthread_mutex_unlock(&inode_lock);
                    return -1;
                }
                inode->i_size = 0;
                
            }
        }
        
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (flags & TFS_O_CREAT) {

        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            pthread_mutex_unlock(&inode_lock);
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            pthread_mutex_unlock(&inode_lock);
            return -1;
        }
        offset = 0;
    } else {
        pthread_mutex_unlock(&inode_lock);
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    pthread_mutex_unlock(&inode_lock);
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}


int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {


    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    pthread_mutex_lock(&inode_lock);
    inode_t *inode = inode_get(file->of_inumber);
    
    if (inode == NULL) {
        pthread_mutex_unlock(&inode_lock);
        return -1;
    }

    /* Determine how many bytes to write */

    if (to_write + file->of_offset > (MAX_NUM_BLOCKS + BLOCK_SIZE / sizeof(int)) * BLOCK_SIZE ) {
        to_write = ((MAX_NUM_BLOCKS + BLOCK_SIZE / sizeof(int)) * BLOCK_SIZE) - file->of_offset;
        
    }
    size_t total_write = to_write;

    if (to_write > 0) {

        if (inode->i_size == 0) {
            /* If empty file, allocate new block */
            inode->i_data_block[0] = data_block_alloc();
        }

        /* First block to write in */
        size_t write_block = file->of_offset / BLOCK_SIZE;

        size_t block_offset = file->of_offset % BLOCK_SIZE;

        /* Perform the actual write */
        for(size_t i=write_block; i < MAX_NUM_BLOCKS; i++){

            void *block = data_block_get(inode->i_data_block[i]);
            if (block == NULL) {
                inode->i_data_block[i] = data_block_alloc();
                block = data_block_get(inode->i_data_block[i]);
            }

            size_t block_write = to_write;

            if (to_write > BLOCK_SIZE){
                block_write = BLOCK_SIZE - block_offset;
            }

            memcpy(block + block_offset,buffer,block_write);
            to_write -= block_write;
            buffer += block_write;
            block_offset = 0;
            if (to_write <= 0){
                to_write = 0;
                break;
            }
        }
        size_t allbits = to_write + file->of_offset;

        if (to_write > 0 && allbits>11){
            int *block_index = data_block_get(inode->i_data_block[MAX_NUM_BLOCKS]);

            if (block_index == NULL){
                inode->i_data_block[MAX_NUM_BLOCKS] = data_block_alloc();
                block_index = data_block_get(inode->i_data_block[MAX_NUM_BLOCKS]);

            }

            if (write_block < 10){
                write_block = 0;
            }
            else{
                write_block-=10;
            }

            for (size_t i=write_block; i < BLOCK_SIZE/sizeof(int); i++){

                if (i*sizeof(int) >= inode->index_block_offset){
                    block_index[i] = data_block_alloc();
                    inode->index_block_offset = i*sizeof(int) + sizeof(int);

                }

                size_t block_write = to_write;

                if (to_write > BLOCK_SIZE)
                {
                    block_write = BLOCK_SIZE - block_offset;

                }

                void *block = data_block_get(block_index[i]);
                memcpy(block + block_offset, buffer, block_write);
                to_write -= block_write;
                buffer += block_write;
                block_offset = 0;
                if (to_write <= 0){
                    to_write = 0;
                    break;
                }
            }
        }
        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset += total_write;

        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }
    pthread_mutex_unlock(&inode_lock);

    return (ssize_t)total_write;
}


ssize_t tfs_read(int fhandle, void *buffer, size_t len) {

    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    pthread_mutex_lock(&inode_lock);
    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);

    if (inode == NULL) {
        pthread_mutex_unlock(&inode_lock);
        return -1;
    }

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;

    if (to_read > len) {
        to_read = len;
    }

    size_t total_read = to_read;

    if (to_read > 0) {

        size_t read_block = file->of_offset / BLOCK_SIZE;

        size_t block_offset = file->of_offset % BLOCK_SIZE;

        /* Perform the actual read */
        for(size_t i=read_block; i < MAX_NUM_BLOCKS; i++){

            void *block = data_block_get(inode->i_data_block[i]);
            if (block == NULL) {
                pthread_mutex_unlock(&inode_lock);
                return -1;
            }
            size_t block_read = to_read;

            if (to_read > BLOCK_SIZE)
            {
                block_read = BLOCK_SIZE - block_offset;
            }

            memcpy(buffer, block + block_offset, block_read);
            to_read -= block_read;
            buffer += block_read;
            block_offset = 0;

            if (to_read <= 0){
                to_read = 0;
                break;
            }
        }

        size_t total_blocks_write = inode->i_size / BLOCK_SIZE;

        if (to_read > 0 && total_blocks_write >= 11){

            int*block_index = data_block_get(inode->i_data_block[MAX_NUM_BLOCKS]);
            if (block_index == NULL){
                pthread_mutex_unlock(&inode_lock);
                return -1;
            }
            if (read_block < 10){
                read_block = 0;
            }
            else{
                read_block-=10;
            }
            for (size_t i=read_block; i < BLOCK_SIZE/sizeof(int); i++){
                size_t block_read = to_read;
                if (to_read > BLOCK_SIZE){
                    block_read = BLOCK_SIZE - block_offset;
                }
                void *block = data_block_get(block_index[i]);
                memcpy(buffer, block + block_offset, block_read);
                to_read -= block_read;
                buffer += block_read;
                block_offset = 0;
                if (to_read <= 0){
                    to_read = 0;
                    break;
                }
            }
        }

        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset += total_read;
    }
    pthread_mutex_unlock(&inode_lock);
    

    return (ssize_t)total_read;
}

int tfs_copy_to_external_fs(char const *source_path, char const *dest_path)
{
    void *buffer;
    FILE *fp;
    pthread_mutex_lock(&ext_file_lock);
    fp = fopen(dest_path,"w");

    int open = tfs_open(source_path, 1);
    if (open < 0)
    {
        pthread_mutex_unlock(&ext_file_lock);
        return -1;
    }
    size_t len = inode_get(get_open_file_entry(open)->of_inumber)->i_size;
    
    if(len > 0)
    {
        buffer = malloc(len*BLOCK_SIZE);
    }
    else
    {
        pthread_mutex_unlock(&ext_file_lock);
        return -1;
    }
    ssize_t bytes_read = tfs_read(open, buffer, len);

    if (bytes_read < 0)
    {
        pthread_mutex_unlock(&ext_file_lock);
        return -1;
    }
    
    size_t bytes_written = fwrite(buffer,sizeof(char),(size_t)bytes_read,fp);
    
    if ((int)bytes_written < 0)
        return -1;
    free(buffer);
    tfs_close(open);
    fclose(fp);
    pthread_mutex_unlock(&ext_file_lock);
    return 0;
} 