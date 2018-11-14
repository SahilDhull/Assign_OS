#include "lib.h"

#define MAX_OBJS 1e6
#define MAX_INODE_BLOCKS 31
#define MAX_DATA_BLOCKS 256
#define inode_offset 287
#define OBJ_SIZE 88
#define data_offset (int)(22528+287)


struct object{
   long id;
   long size;
   int cache_index;
   int dirty;
   char key[32];
   int d_ptr[4];
   int i_ptr[4];
};

struct object *objs;
struct object *obj_backup;
unsigned int* i_bitmap;
unsigned int* d_bitmap;

#define malloc_4k(x,y) do{\
                         (x) = mmap(NULL, y, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);\
                         if((x) == MAP_FAILED)\
                              (x)=NULL;\
                     }while(0); 
#define free_4k(x,y) munmap((x), y)


/*
Returns the object ID.  -1 (invalid), 0, 1 - reserved
*/
long find_object_id(const char *key, struct objfs_state *objfs)
{
  // dprintf("-------------find_object_id\n");
  struct object *obj = objs;
  for(int j=0; j < MAX_INODE_BLOCKS*BLOCK_SIZE/4 ; j++){
    for(int k=31; k >=0; k--){
      if(((i_bitmap[j]>>k)&1)==1){
        if(obj->id && !strcmp(obj->key, key)){
          // dprintf("obj->id = %d\n",obj->id);
          obj_backup = obj;
          return obj->id;
        }
      }
      obj++;
    }
  }
  // for(ctr=0; ctr < MAX_OBJS; ++ctr){
  //       if(obj->id && !strcmp(obj->key, key))
  //           return obj->id;
  //       obj++;
  // }      
  return -1; 
}

/*
  Creates a new object with obj.key=key. Object ID must be >=2.
  Must check for duplicates.

  Return value: Success --> object ID of the newly created object
                Failure --> -1
*/
long create_object(const char *key, struct objfs_state *objfs)
{
  // dprintf("in create object\n");
  struct object *obj = objs;
  struct object *free = NULL; 
  int ctr=0,flag=0;
  for(int j=0; j < MAX_INODE_BLOCKS*BLOCK_SIZE/4; j++){
    for(int k=31; k >=0; k--){
      if(((i_bitmap[j]>>k)&1)==0 && !free){
        free=obj;
        free->id=ctr+2;
        i_bitmap[j] = (i_bitmap[j] | (1<<k));
        flag=1;
        break;
      }
      // else if(((i_bitmap[j]>>k)&1)==1 && !strcmp(obj->key, key)){
      //   if(free) free_4k(free,sizeof(struct object*));
      //   return -1;
      // }
      obj++;
      ctr++;
      // dprintf("oooooo\n");
    }
    if(flag==1) break;
  }

  


  if(!free){
     dprintf("%s: objstore full\n", __func__);
     return -1;
  }
  strcpy(free->key, key);
  // dprintf("free->id = %d\n",free->id);
    // init_object_cached(obj);
  return free->id;
}
/*
  One of the users of the object has dropped a reference
  Can be useful to implement caching.
  Return value: Success --> 0
                Failure --> -1
*/
long release_object(int objid, struct objfs_state *objfs)
{
    return 0;
}

/*
  Destroys an object with obj.key=key. Object ID is ensured to be >=2.

  Return value: Success --> 0
                Failure --> -1
*/
long destroy_object(const char *key, struct objfs_state *objfs)
{
    return -1;
}

/*
  Renames a new object with obj.key=key. Object ID must be >=2.
  Must check for duplicates.  
  Return value: Success --> object ID of the newly created object
                Failure --> -1
*/

long rename_object(const char *key, const char *newname, struct objfs_state *objfs)
{
   
   return -1;
}

/*
  Writes the content of the buffer into the object with objid = objid.
  Return value: Success --> #of bytes written
                Failure --> -1
*/

int free_addr(){
  int ctr=0;
  for(int j=0; j < MAX_DATA_BLOCKS*BLOCK_SIZE/4 ; j++){
    for(int k=31; k >=0; k--){
      if(((d_bitmap[j]>>k)&1)==0){
        d_bitmap[j] = (d_bitmap[j] | (1<<k));
        return ctr;
      }
      ctr++;
    }
  }
  return -1;
}

long objstore_write(int objid, const char *buf, int size, struct objfs_state *objfs)
{
  // dprintf("%s\n",buf);
  // dprintf("<-------->\n");
  char * buf2;
  malloc_4k(buf2,size);
  for(int i=0; i < size; i++){
    *(buf2+i) = *(buf+i);
    // dprintf("%c",(*(buf+i)));
  }
  // dprintf("objstore_write size = %u\n",size);
  struct object *obj = objs + objid - 2;
  if(obj->id != objid)
    return -1;
  if(size > BLOCK_SIZE) 
    return -1;
  dprintf("Doing write size = %d\n", size);
  int pos = free_addr();
  if(pos==-1){
    return -1;
  }
  obj->size+=size;
  write_block(objfs,data_offset+pos,(char*)buf2);
  for(int i=0; i < 4; i++){
    if(!(obj->d_ptr[i])){
      obj->d_ptr[i] = data_offset+pos;
      return size;
    }
  }
  //indirect pointer
  for(int i=0; i < 4; i++){
    if(!(obj->i_ptr[i])){
      // dprintf("should be here---\n");
      int pos2 = free_addr();
      if(pos2==-1){
        //do something
        return -1;
      }
      obj->i_ptr[i] = data_offset + pos2;
      dprintf("pos = %u\n",pos);
      int* temp;
      malloc_4k(temp,BLOCK_SIZE);
      for(int j=0; j < 1024; j++) temp[j]=0;
      temp[0] = data_offset + pos;
      write_block(objfs,data_offset+pos2,(char*)temp);
      return size;
    }
    else{
      int* temp;
      malloc_4k(temp,BLOCK_SIZE);
      read_block(objfs,obj->i_ptr[i],(char*)temp);
      for(int j=0; j < 1024; j++){
        if(temp[j]==0){
          temp[j] = data_offset+pos;
          dprintf("pos = %u\n",pos);
          dprintf("indirect ptr = %u\n",obj->i_ptr[i]-data_offset);
          write_block(objfs,obj->i_ptr[i],(char*)temp);
          return size;
        }
      }
    }
  }
   // if(find_write_cached(objfs, obj, buf, size) < 0)
   //     return -1; 
  
  return size;
}


/*
  Reads the content of the object onto the buffer with objid = objid.
  Return value: Success --> #of bytes written
                Failure --> -1
*/

long objstore_read(int objid, char *buf, int size, struct objfs_state *objfs)
{
  char * temp;
  malloc_4k(temp,size);
  struct object *obj = objs + objid - 2;
  // dprintf("obj->id = %d objid = %d \n", obj->id,objid);
   if(objid < 2)
       return -1;
   if(obj->id != objid)
       return -1;
   dprintf("Doing read size = %d\n", size);
   int n = (size+BLOCK_SIZE-1)/BLOCK_SIZE;
   dprintf("n = %d\n",n);
   // int rem = size%BLOCK_SIZE;
   for(int i=0; i < 4; i++){
     if(n>0){
      read_block(objfs,obj->d_ptr[i],(char*)(temp+i*BLOCK_SIZE));
      n--;
     }
     else break;
   }
   int rem = n;
   for(int i=0; i < 4; i++){
     if(n>0){
      int* i_value;
      malloc_4k(i_value,BLOCK_SIZE);
      read_block(objfs,obj->i_ptr[i],(char*)(i_value));
      for(int j=0; j < 1024; j++){
        int off = (int)i_value[j];
        if(n>0){
          read_block(objfs,off,(char*)(temp+(4+i*1024+j)*BLOCK_SIZE));
          n--;
        }
        else break;
      }
     }
   }
   // if(find_read_cached(objfs, obj, buf, size) < 0)
   //     return -1;
   for(int i=0; i < size; i++){
      *(buf+i)=(char*)(*(temp+i));
      // dprintf("%u",(unsigned int)(*(buf+i)));
    }
    dprintf("size = %d\n",size);
   return size;
}



/*
  Reads the object metadata for obj->id = buf->st_ino
  Fillup buf->st_size and buf->st_blocks correctly
  See man 2 stat 
*/
int fillup_size_details(struct stat *buf)
{
  struct object *obj =obj_backup;
   if(buf->st_ino < 2 || obj->id != buf->st_ino)
       return -1;
   buf->st_size = obj->size;
   buf->st_blocks = obj->size >> 9;
   if(((obj->size >> 9) << 9) != obj->size)
       buf->st_blocks++;
   return 0;
}

/*
   Set your private pointeri, anyway you like.
*/
int objstore_init(struct objfs_state *objfs)
{
  dprintf("Entered objstore init\n");
  malloc_4k(objs,22528*BLOCK_SIZE);
  malloc_4k(obj_backup,OBJ_SIZE);
  for(int i=0;i<MAX_INODE_BLOCKS;i++){
    read_block(objfs,i,(char*)((char*)objs+i*BLOCK_SIZE));
  }
  malloc_4k(i_bitmap,MAX_INODE_BLOCKS*BLOCK_SIZE);
  malloc_4k(d_bitmap,MAX_DATA_BLOCKS*BLOCK_SIZE);
  dprintf("Done objstore init\n");
  return 0;
}

/*
   Cleanup private data. FS is being unmounted
*/
int objstore_destroy(struct objfs_state *objfs)
{
   dprintf("Done objstore destroy\n");
   return 0;
}
