#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "inode.h"
#include "diskimg.h"

// remove the placeholder implementation and replace with your own
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {

    int numOfinode = DISKIMG_SECTOR_SIZE/ sizeof(struct inode);//how many inode in a sector
    //inumber starts from etc. 1-512 but iNode as an array starts from etc.0-511
    --inumber;


    int atSector = inumber/numOfinode;// at which sector
    int atInode = inumber%numOfinode;//at this sector in which inode

    struct inode buf[numOfinode];//buf is a inode array to store inode read by diskimg
    if(diskimg_readsector(fs->dfd,atSector + INODE_START_SECTOR,buf) != -1) {
        //copy inode content to inp
       *inp = buf[atInode];
        return 0;
    }
    return -1;



}

// remove the placeholder implementation and replace with your own
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {

  if(!inp->i_mode % IALLOC) return -1;//this block is not allocated

  int numofblk = DISKIMG_SECTOR_SIZE/ sizeof(uint16_t);

    //if this file is a small, regular file
  if((inp->i_mode & ILARG) == 0) {
    return inp->i_addr[blockNum];
  }
    // large regular
 else if((inp->i_mode & ILARG) != 0) {

      if(blockNum < 7*numofblk) {
          // single indirect
          int atAddr = blockNum/numofblk;// there are 8 in total
          int atBlock = blockNum%numofblk;// there are numofblk each
          uint16_t buf[numofblk];
          if(diskimg_readsector(fs->dfd,inp->i_addr[atAddr],buf) == -1) {
              return -1;

          }
          else return buf[atBlock];
      }
      else {
          //double indirect
          uint16_t firstIndir[numofblk];//to store first layer content
          if(diskimg_readsector(fs->dfd,inp->i_addr[7],firstIndir) == - 1) {
              return - 1;
          }
          //The first layer block
          int firstIndirStart = blockNum  - 7*numofblk;// all number of blocks in the very first layer
          int atAddr = firstIndirStart/numofblk;
          int atBlock = firstIndirStart%numofblk;

          //second layer block
          uint16_t secondIndir[numofblk];
          if(diskimg_readsector(fs->dfd,firstIndir[atAddr],secondIndir) == -1) {
              return -1;
          }
          return secondIndir[atBlock];

      }
  }
    return -1;
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
