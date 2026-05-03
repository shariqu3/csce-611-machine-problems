/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
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
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    // Console::puts("In file system constructor.\n");
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
    disk = NULL;
    inodes = new Inode[MAX_INODES];
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */
    Console::puts("FUNCTION NOT IMPLEMENTED\n");
    assert(false);
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/


bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system from disk\n");
    /* Here you read the inode list and the free list into memory */
    /*
    1. Check if disk is NULL, if not return false
    2. Initialize  disk, n_blocks & size
    3. Read to memory inodes and free_blocks
    End: return true
    */

    // 1. Check if disk is NULL, if not return false
    if(disk != NULL)
        return false;

    // 2. Initialize  disk, n_blocks & size
    disk = _disk;
    size = MAX_INODES * SimpleDisk::BLOCK_SIZE;
    n_blocks = MAX_INODES;

    // 3. Read to memory inodes and free_blocks
    free_blocks = new unsigned char[n_blocks];
    disk->read(1, free_blocks); 

    unsigned char* buf = new unsigned char[SimpleDisk::BLOCK_SIZE/sizeof(unsigned char)];
    disk->read(0, buf); 
    Console::puti(int(free_blocks[0]));

    delete[](buf);
    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("formatting disk\n");

    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
    /*
    1. Overwrite inodes block
    2. Overwrite free_block
    */

    // 1. Overwrite inodes block
    unsigned char* buf = new unsigned char[SimpleDisk::BLOCK_SIZE/sizeof(unsigned char)];
    _disk->write(0, buf);

    // 2. Overwrite free_block
    buf[0] = 1;
    buf[1] = 1;
    _disk->write(1, buf);
    delete[] buf;
    return true;
}

Inode * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you go through the inode list to find the file. */
    Console::puts("FUNCTION NOT IMPLEMENTED\n");
    assert(false);
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
    Console::puts("FUNCTION NOT IMPLEMENTED\n");
    assert(false);
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* First, check if the file exists. If not, throw an error. 
       Then free all blocks that belong to the file and delete/invalidate 
       (depending on your implementation of the inode list) the inode. */
    Console::puts("FUNCTION NOT IMPLEMENTED\n");
    assert(false);
}
