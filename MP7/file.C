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
    fs->disk->read(inode->block_no, block_cache);
}

File::~File() {
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */
    fs->disk->write(inode->block_no, block_cache);
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
