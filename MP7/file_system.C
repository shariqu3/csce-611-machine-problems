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
    disk = NULL;
    n_blocks = MAX_INODES;
    size = n_blocks* SimpleDisk::BLOCK_SIZE;
    free_blocks = new unsigned char[n_blocks];
    inodes = new Inode[n_blocks];
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */
    sync_metadata();
    delete[] free_blocks;
    delete[] inodes;
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

    // 3. Read to memory inodes and free_blocks
    disk->read(0, (unsigned char* )inodes); 

    unsigned char* buf = new unsigned char[SimpleDisk::BLOCK_SIZE/sizeof(unsigned char)];
    disk->read(1, buf); 
    for(unsigned int i=0;i<n_blocks;i++)
        {free_blocks[i] = buf[i];}

    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("formatting disk\n");

    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */
    /*
    1. Overwrite inodes block
    2. Overwrite free_block
    */

    // 1. Overwrite inodes block
    struct Inode* buf = new Inode[MAX_INODES];
    _disk->write(0, (unsigned char*)buf);
    delete[] buf;

    // 2. Overwrite free_block
    unsigned char* buf2 = new unsigned char[MAX_INODES];
    buf2[0] = 1;
    buf2[1] = 1;
    _disk->write(1, buf2);
    delete[] buf2;
    return true;
}

void FileSystem::sync_metadata()
{
    disk->write(0, (unsigned char *) inodes);
    disk->write(1, free_blocks);
}

Inode * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you go through the inode list to find the file. */
    for(unsigned int i=0;i<n_blocks;i++)
    {
        Inode * inode = &inodes[i];
        if(inode->id == _file_id)
        {
            return inode;
        }
    }
    return nullptr;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
    /*
    1. Lookup if file exists
    2. Find a free block
    3. Find a free inode
    4. Set file metadata on inode
    5. Allocate index_block for inode
    6. Allocate first file block
    7. Mark the blocks as reserved
    8. Rewrite block with first char as end of file(-1);
    9. Update inode list and freelist on disk
    */

    // 1. Lookup if file exists
    Inode* ret = LookupFile(_file_id);

    if(ret)
    {
        Console::puts("File already exists! File ID:");
        Console::puti(_file_id);
        return false;
    }

    // 2. Find free blocks & 
    // 5. Allocate index_block for inode & 
    // 6. Allocate first file block & 
    // 7. Mark the blocks as reserved
    int index_block_no = GetFreeBlock();
    if(index_block_no == -1)
    {
        Console::puts("Disk Space Full!");
        assert(false);
    }
    free_blocks[index_block_no] = 1;

    int file_block_no = GetFreeBlock();
    if(file_block_no== -1)
    {
    free_blocks[index_block_no] = 0;
        Console::puts("Disk Space Full!");
        assert(false);
    }
    free_blocks[file_block_no] = 1;

    // 3. Find a free inode 
    short inode_idx= GetFreeInode();

    if(inode_idx== -1)
    {
    free_blocks[index_block_no] = 0;
    free_blocks[file_block_no] = 0;
        Console::puts("Disk Space Full!");
        assert(false);
    }

    // 4. Set file metadata on inode
    struct Inode * inode = &inodes[inode_idx];
    inode->index_block_no = index_block_no;
    inode->id = _file_id;
    inode->size = 0;


    // 6. Rewrite block with first char as end of file(-1);
    unsigned char * buf = new unsigned char[SimpleDisk::BLOCK_SIZE];
    buf[0] = -1;
    disk->write(file_block_no, buf);
    delete[] buf;

    int *index_block_buf = new int[SimpleDisk::BLOCK_SIZE/sizeof(int)];
    for(unsigned int i=0;i<SimpleDisk::BLOCK_SIZE/sizeof(int);i++)
    {
        index_block_buf[i] = -1;
    }
    disk->write(index_block_no, (unsigned char *)index_block_buf);
    delete[] index_block_buf;

    //7. Update inode list and freelist on disk
    sync_metadata();
    return true;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* First, check if the file exists. If not, throw an error. 
       Then free all blocks that belong to the file and delete/invalidate 
       (depending on your implementation of the inode list) the inode. */
    /*
    1. Find inode 
    2. Read index block
    3. Mark all file blocks as free
    4. Mark index block as free
    5. Make inode empty
    */

    // 1. Find inode 
    Inode *inode= LookupFile(_file_id); 
    if(inode==nullptr)
    {
        Console::puts("File does not exist!");
        assert(false);
    }

    // 2. Read index block
    int index_block_no = inode->index_block_no;
    unsigned char * buf = new unsigned char[SimpleDisk::BLOCK_SIZE];
    disk->read(index_block_no, buf);

    // 3. Mark all file blocks as free
    int *block_lists = (int *)buf;
    for(unsigned int i=0;i<SimpleDisk::BLOCK_SIZE/sizeof(int);i++)
    {
        if(block_lists[i] != -1)
        {
            free_blocks[block_lists[i]] = 0;
        }
    }

    // 4. Mark index block as free
    free_blocks[index_block_no] = 0;

    // 5. Make inode empty
    inode->index_block_no = -1;
    inode->id = -1;
    inode->size = 0;
    return true;
}


Inode::Inode(){
    id = -1;
    index_block_no=-1;
}

short FileSystem::GetFreeInode(){
    for(short int i=0;i<(short)n_blocks;i++)
    {
        if(inodes[i].id == -1)
        {
            return i;
        }
    }
    return -1;
}

int FileSystem::GetFreeBlock(){
    for(int i=0;i<n_blocks;i++)
    {
        if(free_blocks[i] != 1)
        {
            return i;
        }
    }
    return -1;
}
