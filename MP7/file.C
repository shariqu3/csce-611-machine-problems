/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("Opening file.\n");
    current = 0;
    fs = _fs;
    inode = fs->LookupFile(_id);
    if(inode == NULL)
    {
        Console::puts("File Not Found!");
        assert(false);
    }
    // Reading the index block to cache
    index_block_cache = new int[SimpleDisk::BLOCK_SIZE/sizeof(int)];
    fs->disk->read(inode->index_block_no, (unsigned char *)index_block_cache);

    // Reading all file blocks to cache
    file_block_caches = new unsigned char*[SimpleDisk::BLOCK_SIZE / sizeof(int)];
    n_loaded_blocks = SimpleDisk::BLOCK_SIZE / sizeof(int);
    for(int i = 0; i < n_loaded_blocks; ++i){
        file_block_caches[i] = new unsigned char[SimpleDisk::BLOCK_SIZE];
        if(index_block_cache[i] != -1) {
            fs->disk->read(index_block_cache[i], file_block_caches[i]);
        }
    }

    current_pos = 0;
    current_block_idx = 0;
    current_offset = 0;
}

File::~File() {
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */
    for(int i = 0; i < n_loaded_blocks; ++i) {
        if(index_block_cache[i] != -1) {
            fs->disk->write(index_block_cache[i], file_block_caches[i]);
        }
        delete[] file_block_caches[i];
    }
    delete[] file_block_caches;

    // Write back index block cache
    fs->disk->write(inode->index_block_no, (unsigned char*)index_block_cache);
    delete[] index_block_cache;

    fs->sync_metadata();
}
/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");
    int n_read = 0;
    while(!EoF() && current<SimpleDisk::BLOCK_SIZE)
    {
        _buf[n_read++] = block_cache[current++];
    }
    return n_read;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");
    int n_written = 0;
    while(n_written <_n && current<SimpleDisk::BLOCK_SIZE)
    {
        block_cache[current++] = _buf[n_written++];
    }
    if(current !=SimpleDisk::BLOCK_SIZE)
    {
        block_cache[current++] = -1;
    }
    return n_written;
}

void File::Reset() {
    Console::puts("resetting file\n");
    current = 0;
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    return (int)block_cache[current] == 255;
}
