#include "lib.h"
#include<pthread.h>

//all pthreads done but not tested

#define MAX_OBJS 1e6
#define IMAP_BLOCKS 31
#define DMAP_BLOCKS 256
#define inode_offset 287
#define OBJ_SIZE 88
#define data_offset (int)(22528+287)
#define CACHE_VALUE 32768


struct object{
   long id;
   long size;
   int cache_index;
   int dirty;
   char key[32];
   int d_ptr[4];
   int i_ptr[4];
   int faltu1,faltu2;
};

int global_counter = 0;

pthread_mutex_t ilock;
pthread_mutex_t dlock;
pthread_mutex_t cache_lock;

struct object *objs;
struct object *obj_backup;
unsigned int* i_bitmap;
unsigned int* d_bitmap;
int* darray;
int* cache_array;
int* cache_dirty;

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
  // //dprintf("-------------find_object_id\n");
  struct object *obj = objs;
  for(int j=0; j < IMAP_BLOCKS*BLOCK_SIZE/4 ; j++){
    for(int k=31; k >=0; k--){
      if(((i_bitmap[j]>>k)&1)==1){
        if(obj->id && !strcmp(obj->key, key)){
          // //dprintf("obj->id = %d\n",obj->id);
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
  //dprintf("in create object\n");
  struct object *obj = objs;
  struct object *free = NULL; 
  int ctr=0,flag=0;
  //lock
  pthread_mutex_lock(&ilock);
  for(int j=0; j < IMAP_BLOCKS*BLOCK_SIZE/4; j++){
    for(int k=31; k >=0; k--){
      if(((i_bitmap[j]>>k)&1)==0 && !free){
        free=obj;
        free->id=ctr+2;
        i_bitmap[j] = (i_bitmap[j] | (1<<k));
        
        darray[ctr/46]=1;
        strcpy(free->key, key);
        flag=1;
        pthread_mutex_unlock(&ilock);
        // //dprintf("it is ok with id = %d\n",free->id);
        return free->id;
      }
      // else if(((i_bitmap[j]>>k)&1)==1 && !strcmp(obj->key, key)){
      //   if(free) free_4k(free,sizeof(struct object*));
      //   return -1;
      // }
      obj++;
      ctr++;
      // //dprintf("oooooo\n");
    }
    if(flag==1) break;
  }

  if(!free){
     //dprintf("%s: objstore full\n", __func__);
     //unlock
     pthread_mutex_unlock(&ilock);
     return -1;
  }
  
  // //dprintf("free->id = %d\n",free->id);
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


/*
  Renames a new object with obj.key=key. Object ID must be >=2.
  Must check for duplicates.  
  Return value: Success --> object ID of the newly created object
                Failure --> -1
*/

long rename_object(const char *key, const char *newname, struct objfs_state *objfs)
{
  // //dprintf("=======================\n");
  struct object *obj = objs;
  int ctr=0;
  //check for duplicates
  for(int j=0; j < IMAP_BLOCKS*BLOCK_SIZE/4 ; j++){
    for(int k=31; k >=0; k--){
      if(((i_bitmap[j]>>k)&1)==1){
        if(!strcmp(obj->key, newname)){
          return -1;
        }
      }
      obj++;
      ctr++;
    }
  }
  ctr=0;
  obj = objs;
  for(int j=0; j < IMAP_BLOCKS*BLOCK_SIZE/4 ; j++){
    for(int k=31; k >=0; k--){
      if(((i_bitmap[j]>>k)&1)==1){
        if(!strcmp(obj->key, key)){
          strcpy(obj->key,newname);
          darray[ctr/46]=1;
          return obj->id;
        }
      }
      obj++;
      ctr++;
    }
  }
  return -1;
}


/*
  Writes the content of the buffer into the object with objid = objid.
  Return value: Success --> #of bytes written
                Failure --> -1
*/

/*
  Reads the content of the object onto the buffer with objid = objid.
  Return value: Success --> #of bytes written
                Failure --> -1
*/

int free_addr(){
  int ctr=0;
  for(int j=0; j < DMAP_BLOCKS*BLOCK_SIZE/4 ; j++){
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

#ifdef CACHE

void new_write(struct objfs_state *objfs,int pos,char *buf2){
  pthread_mutex_lock(&cache_lock);
  int block = pos/CACHE_VALUE;
  int block_offset = pos%CACHE_VALUE;
  char *cache_ptr = objfs->cache;
  char* pointer = cache_ptr + (block_offset<<12);

  if(cache_dirty[block_offset]==1&&cache_array[block_offset]!=block && cache_array[block_offset]!=-1){
    //write previous block to disc and set dirty to 1 (dont change)
    int prev_pos = cache_array[block_offset]*CACHE_VALUE + block_offset;
    write_block(objfs,data_offset+prev_pos,pointer);
  }
  for(int i=0; i < BLOCK_SIZE; i++) *(pointer+i) = *(buf2+i);
  cache_dirty[block_offset]=1;
  cache_array[block_offset] = block;
  pthread_mutex_unlock(&cache_lock);
  // //dprintf("in write cache, pos = %d\n",pos);
}

void new_read(struct objfs_state *objfs,int pos,char *buf2){
  // //dprintf("in read cache, pos = %d\n",pos);
  pthread_mutex_lock(&cache_lock);
  int block = pos/CACHE_VALUE;
  int block_offset = pos%CACHE_VALUE;
  char *cache_ptr = objfs->cache;
  char* pointer = cache_ptr + (block_offset<<12);
  // //dprintf("here------\n");
  if(cache_array[block_offset]!=block){
    if(cache_dirty[block_offset]==1){
      //write previous block to disc and set dirty to 1 (dont change)
      int prev_pos = cache_array[block_offset]*CACHE_VALUE + block_offset;
      write_block(objfs,data_offset+prev_pos,pointer);
    }
    // //dprintf("coming here\n");
    read_block(objfs,data_offset+pos,pointer);
    cache_dirty[block_offset]=0;
    cache_array[block_offset] = block;
  }
  for(int i=0; i < BLOCK_SIZE; i++) *(buf2+i) = *(pointer+i);
  pthread_mutex_unlock(&cache_lock);
}

long objstore_write(int objid, const char *buf, int size, struct objfs_state *objfs, off_t offset)
{
  // global_counter++;
  // //dprintf("global counter = %d\n",global_counter);
  char * buf2;
  malloc_4k(buf2,BLOCK_SIZE);
  for(int i=0; i < BLOCK_SIZE; i++){
    *(buf2+i) = 0;
  }
  for(int i=0; i < size; i++){
    *(buf2+i) = *(buf+i);
  }
  struct object *obj = objs + objid - 2;
  if(obj->id != objid)
    return -1;
  if(size > BLOCK_SIZE) 
    return -1;
  //dprintf("Doing write size = %d\n", size);
  // pthread_mutex_lock(&dlock);
  int value = obj->id-2;
  darray[value/46]=1;
  //lock
  pthread_mutex_lock(&dlock);
  int pos = free_addr();
  pthread_mutex_unlock(&dlock);
  if(pos==-1){
    free_4k(buf2,BLOCK_SIZE);
    return -1;
  }

  //cache
  // pthread_mutex_lock(&cache_lock);
  new_write(objfs,pos,buf2); //---------------new_write
  // pthread_mutex_unlock(&cache_lock);
  // write_block(objfs,data_offset+pos,(char*)buf2);
  
  // write_block(objfs,data_offset+pos,(char*)buf2);
  free_4k(buf2,BLOCK_SIZE);
  int num = offset/BLOCK_SIZE;
  if(num<4){
    //lock -- to check
    pthread_mutex_lock(&dlock);
    if(!obj->d_ptr[num]){
      obj->size+=size;
    }
    else{
      //change data bitmap
      int data_pointer = obj->d_ptr[num] - data_offset;
      int q = data_pointer/32;
      int r = data_pointer%32;
      d_bitmap[q] = (d_bitmap[q]^(1<<(31-r)));
    }
    obj->d_ptr[num] = data_offset+pos;
    //unlock
    pthread_mutex_unlock(&dlock);
    return size;
  }
  else{
    int i,place;
    if(num-4<1024){
      place = num-4;
      i=0;
    }
    else if(num-4<2048){
      place = num-1028;
      i=1;
    }
    else if(num-4<3072){
      place = num-4-2048;
      i=2;
    }
    else if(num-4<4096){
      place = num-4-3072;
      i=3;
    }
    else return -1;
    //lock
    pthread_mutex_lock(&dlock);
    if(!obj->i_ptr[i]){
      // pthread_mutex_lock(&dlock);
      int pos2 = free_addr();
      // pthread_mutex_unlock(&dlock);
      if(pos2==-1) return -1;
      obj->i_ptr[i] = data_offset + pos2;
      int* temp;
      malloc_4k(temp,BLOCK_SIZE);
      for(int j=0; j < 1024; j++) temp[j]=0;
      temp[place] = data_offset + pos;
      // write_block(objfs,data_offset+pos2,(char*)temp);
      // pthread_mutex_lock(&cache_lock);
      new_write(objfs,pos2,(char*)temp);
      // pthread_mutex_unlock(&cache_lock);
      free_4k(temp,BLOCK_SIZE);
      obj->size+=size;
      pthread_mutex_unlock(&dlock);
      return size;
    }
    else{
      int* temp;
      malloc_4k(temp,BLOCK_SIZE);
      // read_block(objfs,obj->i_ptr[i],(char*)temp);
      // pthread_mutex_lock(&cache_lock);
      new_read(objfs,obj->i_ptr[i]-data_offset,(char*)temp);
      // pthread_mutex_unlock(&cache_lock);
      if(!temp[place]){
        obj->size+=size;
      }
      else{
        int data_pointer = temp[place] - data_offset;
        int q = data_pointer/32;
        int r = data_pointer%32;
        d_bitmap[q] = (d_bitmap[q]^(1<<(31-r)));
      }
      //add various things
      //change data bitmap for idirect case
      temp[place] = data_offset+pos;
      // write_block(objfs,obj->i_ptr[i],(char*)temp);
      // pthread_mutex_lock(&cache_lock);
      new_write(objfs,obj->i_ptr[i]-data_offset,(char*)temp);
      // pthread_mutex_unlock(&cache_lock);
      free_4k(temp,BLOCK_SIZE);
      pthread_mutex_unlock(&dlock);
      return size;
    }
    //unlock

  }
  
  return -1;
}


long objstore_read(int objid, char *buf, int size, struct objfs_state *objfs, off_t offset)
{
  struct object *obj = objs + objid - 2;
  // //dprintf("obj->id = %d objid = %d \n", obj->id,objid);
   if(objid < 2)
       return -1;
   if(obj->id != objid)
       return -1;
  int n = (size+BLOCK_SIZE-1)/BLOCK_SIZE;
  int n_save = n;
  char * temp;
  malloc_4k(temp,n*BLOCK_SIZE);
  //dprintf("Doing read size = %d\n", size);
  int skip = offset/BLOCK_SIZE;
  //dprintf("n = %d\n",n);
  // int rem = size%BLOCK_SIZE;
  int count = 0;
  for(int i=0; i < 4; i++){
   if(n>0&&skip<=0){
    // read_block(objfs,obj->d_ptr[i],(char*)(temp+count*BLOCK_SIZE));
    // pthread_mutex_lock(&cache_lock);
    new_read(objfs,obj->d_ptr[i]-data_offset,(char*)(temp+count*BLOCK_SIZE));
    // pthread_mutex_unlock(&cache_lock);
    n--;
    count++;
   }
   else if(skip>0) skip--;
   else break;
  }
  for(int i=0; i < 4; i++){
   if(n>0){
    int* i_value;
    malloc_4k(i_value,BLOCK_SIZE);
    // read_block(objfs,obj->i_ptr[i],(char*)(i_value));
    // pthread_mutex_lock(&cache_lock);
    new_read(objfs,obj->i_ptr[i]-data_offset,(char*)(i_value));
    // pthread_mutex_lock(&cache_lock);
    //dprintf("hh\n");
    for(int j=0; j < 1024; j++){
      if(n>0&&skip<=0){
        //dprintf("hh\n");
        int off = (int)i_value[j];
        //dprintf("off = %d\n",off);
        // read_block(objfs,off,(char*)(temp+count*BLOCK_SIZE));
        // pthread_mutex_lock(&cache_lock);
        new_read(objfs,off- data_offset ,(char*)(temp+count*BLOCK_SIZE));
        // pthread_mutex_lock(&cache_lock);
        n--;
        count++;
      }
      else if(skip>0) skip--;
      else break;
    }
    free_4k(i_value,BLOCK_SIZE);
   }
   else break;
  }
  if(skip>0){
    //dprintf("here\n");
    return -1;
  }
  if(n>0){
    //dprintf("or here\n");
    return -1;
  }
  // if(find_read_cached(objfs, obj, buf, size) < 0)
  //     return -1;
  for(int i=0; i < size; i++){
    *(buf+i)=(*(temp+i));
    // //dprintf("%u",(unsigned int)(*(buf+i)));
  }
  //dprintf("size = %d\n",size);
  free_4k(temp,n_save*BLOCK_SIZE);
  
  return size;
}

void find_new(struct objfs_state *objfs,int pos){
  int block = pos/CACHE_VALUE;
  int block_offset = pos%CACHE_VALUE;
  char *cache_ptr = objfs->cache;
  char* pointer = cache_ptr + (block_offset<<12);
  if(cache_array[block_offset] == block){
    cache_dirty[block_offset]=0;
    cache_array[block_offset] = -1;
  }
}

void find_in_cache(struct objfs_state *objfs,int pos){
  pthread_mutex_lock(&cache_lock);
  find_new(objfs,pos);
  pthread_mutex_unlock(&cache_lock);
}

long destroy_object(const char *key, struct objfs_state *objfs)
{
  //dprintf("inside destroy object with key = %s\n",key);
  struct object *obj = objs;
  int ctr=0;
  for(int j=0; j < IMAP_BLOCKS*BLOCK_SIZE/4 ; j++){
    for(int k=31; k >=0; k--){
      if(((i_bitmap[j]>>k)&1)==1){
        if(!strcmp(obj->key, key)){
          // dmap[j] = (dmap[j]|(1<<k));
          darray[ctr/46]=1;
          for(int i=0; i < 4; i++){
            if(obj->d_ptr[i]){
              int value = obj->d_ptr[i]-data_offset;
              int q = value/32;
              int r = value%32;
              d_bitmap[q] = (d_bitmap[q]^(1<<(31-r)));
              obj->d_ptr[i]=0;
              // pthread_mutex_lock(&cache_lock);
              find_in_cache(objfs,value);
              // pthread_mutex_unlock(&cache_lock);
            }
            // else break;
          }
          for(int i=0; i < 4; i++){
            if(obj->i_ptr[i]){
              //do these in the end
              int* temp;
              malloc_4k(temp,BLOCK_SIZE);
              // read_block(objfs,obj->i_ptr[i],(char*)temp);
              new_read(objfs,obj->i_ptr[i]-data_offset,(char*)temp);
              for(int l=0; l < 1024; l++){
                if(temp[l]){
                  int value = temp[l]-data_offset;
                  int q = value/32;
                  int r = value%32;
                  d_bitmap[q] = (d_bitmap[q]^(1<<(31-r)));
                  // pthread_mutex_lock(&cache_lock);
                  find_in_cache(objfs,value);
                  // pthread_mutex_unlock(&cache_lock);
                }
              }
              obj->i_ptr[i]=0;
              free_4k(temp,BLOCK_SIZE);
              int value = obj->i_ptr[i]-data_offset;
              int q = value/32;
              int r = value%32;
              d_bitmap[q] = (d_bitmap[q]^(1<<(31-r)));
              // pthread_mutex_lock(&cache_lock);
              find_in_cache(objfs,value);
              // pthread_mutex_unlock(&cache_lock);
            }
            // else break;
          }
          // //dprintf("okay\n");
          i_bitmap[j] = ((i_bitmap[j])^(1<<k));
          obj->size=0;
          return 0;
        }
      }
      obj++;
      ctr++;
    }
  }
  return -1;
}

#else
long destroy_object(const char *key, struct objfs_state *objfs)
{
  //dprintf("inside destroy object with key = %s\n",key);
  struct object *obj = objs;
  int ctr=0;
  for(int j=0; j < IMAP_BLOCKS*BLOCK_SIZE/4 ; j++){
    for(int k=31; k >=0; k--){
      if(((i_bitmap[j]>>k)&1)==1){
        if(!strcmp(obj->key, key)){
          // dmap[j] = (dmap[j]|(1<<k));
          darray[ctr/46]=1;
          for(int i=0; i < 4; i++){
            if(obj->d_ptr[i]){
              int value = obj->d_ptr[i]-data_offset;
              int q = value/32;
              int r = value%32;
              d_bitmap[q] = (d_bitmap[q]^(1<<(31-r)));
              obj->d_ptr[i]=0;
            }
            // else break;
          }
          for(int i=0; i < 4; i++){
            if(obj->i_ptr[i]){
              //do these in the end
              int* temp;
              malloc_4k(temp,BLOCK_SIZE);
              read_block(objfs,obj->i_ptr[i],(char*)temp);
              for(int l=0; l < 1024; l++){
                if(temp[l]){
                  int value = temp[l]-data_offset;
                  int q = value/32;
                  int r = value%32;
                  d_bitmap[q] = (d_bitmap[q]^(1<<(31-r)));
                }
              }
              obj->i_ptr[i]=0;
              free_4k(temp,BLOCK_SIZE);
              int value = obj->i_ptr[i]-data_offset;
              int q = value/32;
              int r = value%32;
              d_bitmap[q] = (d_bitmap[q]^(1<<(31-r)));
            }
            // else break;
          }
          // //dprintf("okay\n");
          i_bitmap[j] = ((i_bitmap[j])^(1<<k));
          obj->size=0;
          return 0;
        }
      }
      obj++;
      ctr++;
    }
  }
  return -1;
}


long objstore_write(int objid, const char *buf, int size, struct objfs_state *objfs, off_t offset)
{
  // //dprintf("%s\n",buf);
  // //dprintf("<-------->\n");
  char * buf2;
  malloc_4k(buf2,BLOCK_SIZE);
  for(int i=0; i < BLOCK_SIZE; i++){
    *(buf2+i) = 0;
  }
  for(int i=0; i < size; i++){
    *(buf2+i) = *(buf+i);
    // //dprintf("%c",(*(buf+i)));
  }
  // //dprintf("objstore_write size = %u\n",size);
  // //dprintf("--------------\n");
  struct object *obj = objs + objid - 2;
  if(obj->id != objid)
    return -1;
  if(size > BLOCK_SIZE) 
    return -1;
  //dprintf("Doing write size = %d\n", size);
  // pthread_mutex_lock(&dlock);
  int value = obj->id-2;
  darray[value/46]=1;
  //lock
  pthread_mutex_lock(&dlock);
  int pos = free_addr();
  pthread_mutex_unlock(&dlock);
  //unlock
  if(pos==-1){
    free_4k(buf2,BLOCK_SIZE);
    return -1;
  }
  // obj->size+=size;
  write_block(objfs,data_offset+pos,(char*)buf2);
  free_4k(buf2,BLOCK_SIZE);
  int num = offset/BLOCK_SIZE;
  if(num<4){
    //lock -- to check
    pthread_mutex_lock(&dlock);
    if(!obj->d_ptr[num]){
      obj->size+=size;
    }
    else{
      //change data bitmap
      int data_pointer = obj->d_ptr[num] - data_offset;
      int q = data_pointer/32;
      int r = data_pointer%32;
      d_bitmap[q] = (d_bitmap[q]^(1<<(31-r)));
    }
    obj->d_ptr[num] = data_offset+pos;
    //unlock
    pthread_mutex_unlock(&dlock);
    return size;
  }
  else{
    int i,place;
    if(num-4<1024){
      place = num-4;
      i=0;
    }
    else if(num-4<2048){
      place = num-1028;
      i=1;
    }
    else if(num-4<3072){
      place = num-4-2048;
      i=2;
    }
    else if(num-4<4096){
      place = num-4-3072;
      i=3;
    }
    else return -1;
    //lock
    pthread_mutex_lock(&dlock);
    if(!obj->i_ptr[i]){
      // pthread_mutex_lock(&dlock);
      int pos2 = free_addr();
      // pthread_mutex_unlock(&dlock);
      if(pos2==-1) return -1;
      obj->i_ptr[i] = data_offset + pos2;
      int* temp;
      malloc_4k(temp,BLOCK_SIZE);
      for(int j=0; j < 1024; j++) temp[j]=0;
      temp[place] = data_offset + pos;
      write_block(objfs,data_offset+pos2,(char*)temp);
      free_4k(temp,BLOCK_SIZE);
      obj->size+=size;
      pthread_mutex_unlock(&dlock);
      return size;
    }
    else{
      int* temp;
      malloc_4k(temp,BLOCK_SIZE);
      read_block(objfs,obj->i_ptr[i],(char*)temp);
      if(!temp[place]){
        obj->size+=size;
      }
      else{
        int data_pointer = temp[place] - data_offset;
        int q = data_pointer/32;
        int r = data_pointer%32;
        d_bitmap[q] = (d_bitmap[q]^(1<<(31-r)));
      }
      //add various things
      //change data bitmap for idirect case
      temp[place] = data_offset+pos;
      write_block(objfs,obj->i_ptr[i],(char*)temp);
      free_4k(temp,BLOCK_SIZE);
      pthread_mutex_unlock(&dlock);
      return size;
    }
    //unlock

  }
  
  return -1;
}

long objstore_read(int objid, char *buf, int size, struct objfs_state *objfs, off_t offset)
{
  
  struct object *obj = objs + objid - 2;
  // //dprintf("obj->id = %d objid = %d \n", obj->id,objid);
   if(objid < 2)
       return -1;
   if(obj->id != objid)
       return -1;
  int n = (size+BLOCK_SIZE-1)/BLOCK_SIZE;
  int n_save = n;
  char * temp;
  malloc_4k(temp,n*BLOCK_SIZE);
  //dprintf("Doing read size = %d\n", size);
  int skip = offset/BLOCK_SIZE;
  //dprintf("n = %d\n",n);
  // int rem = size%BLOCK_SIZE;
  int count = 0;
  for(int i=0; i < 4; i++){
   if(n>0&&skip<=0){
    read_block(objfs,obj->d_ptr[i],(char*)(temp+count*BLOCK_SIZE));
    n--;
    count++;
   }
   else if(skip>0) skip--;
   else break;
  }
  for(int i=0; i < 4; i++){
   if(n>0){
    int* i_value;
    malloc_4k(i_value,BLOCK_SIZE);
    read_block(objfs,obj->i_ptr[i],(char*)(i_value));
    for(int j=0; j < 1024; j++){
      if(n>0&&skip<=0){
        int off = (int)i_value[j];
        read_block(objfs,off,(char*)(temp+count*BLOCK_SIZE));
        n--;
        count++;
      }
      else if(skip>0) skip--;
      else break;
    }
    free_4k(i_value,BLOCK_SIZE);
   }
   else break;
  }
  if(skip>0) return -1;
  if(n>0) return -1;
  // if(find_read_cached(objfs, obj, buf, size) < 0)
  //     return -1;
  for(int i=0; i < size; i++){
    *(buf+i)=(*(temp+i));
    // //dprintf("%u",(unsigned int)(*(buf+i)));
  }
  //dprintf("size = %d\n",size);
  free_4k(temp,n_save*BLOCK_SIZE);
  
  return size;
}
#endif


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

void initialize(struct objfs_state *objfs){
  malloc_4k(objs,22528*BLOCK_SIZE);
  malloc_4k(obj_backup,OBJ_SIZE);
  malloc_4k(i_bitmap,IMAP_BLOCKS*BLOCK_SIZE);
  malloc_4k(d_bitmap,DMAP_BLOCKS*BLOCK_SIZE);
  malloc_4k(darray,22528*4);
  malloc_4k(cache_dirty,CACHE_VALUE*4);
  malloc_4k(cache_array,CACHE_VALUE*4);
}

void objfs_copying(struct objfs_state *objfs){
  char* temp;
  malloc_4k(temp,BLOCK_SIZE);
  for(int i=0;i<22528;i++){
    read_block(objfs,i+inode_offset,(char*)temp);
    for(int j=0; j < 4048; j++){
      *((char*)objs+i*4048+j) = *(temp+j);
    }
  }
  free_4k(temp,BLOCK_SIZE);
}


int objstore_init(struct objfs_state *objfs)
{
  //dprintf("Entered objstore init\n");
  malloc_4k(objs,22528*BLOCK_SIZE);
  malloc_4k(obj_backup,OBJ_SIZE);
  malloc_4k(i_bitmap,IMAP_BLOCKS*BLOCK_SIZE);
  malloc_4k(d_bitmap,DMAP_BLOCKS*BLOCK_SIZE);
  malloc_4k(darray,22528*4);
  malloc_4k(cache_dirty,CACHE_VALUE*4);
  malloc_4k(cache_array,CACHE_VALUE*4);

  for(int i=0; i < CACHE_VALUE; i++){
    cache_array[i] = -1;
    cache_dirty[i] = 0;
  }

  char* temp;
  malloc_4k(temp,BLOCK_SIZE);
  for(int i=0;i<22528;i++){
    read_block(objfs,i+inode_offset,(char*)temp);
    for(int j=0; j < 4048; j++){
      *((char*)objs+i*4048+j) = *(temp+j);
    }
  }
  free_4k(temp,BLOCK_SIZE);
  
  // for(int i=0; i < 22528; i++){
  //   read_block(objfs,287+i,(char*)((char*)objs+i*BLOCK_SIZE));
  // }
  
  for(int i=0; i < 22528; i++){
    darray[i]=0;
  }
  for(int i=0; i < IMAP_BLOCKS; i++){
    read_block(objfs,i,(char*)((char*)i_bitmap+i*BLOCK_SIZE));
  }
  for(int i=0; i < DMAP_BLOCKS; i++){
    read_block(objfs,i+IMAP_BLOCKS,((char*)d_bitmap+i*BLOCK_SIZE));
  }
  pthread_mutex_init(&ilock,NULL);
  pthread_mutex_init(&dlock,NULL);
  pthread_mutex_init(&cache_lock,NULL);
  // for(int i=0; i < 22528; i++){
  //   read_block(objfs,inode_offset+i,((char*)objs+i*4048));
  // }
  //dprintf("Done objstore init\n");
  return 0;
}

/*
   Cleanup private data. FS is being unmounted
*/

void free_func(struct objfs_state *objfs){
  free_4k(objs,22528*BLOCK_SIZE);
  free_4k(obj_backup,OBJ_SIZE);
  free_4k(i_bitmap,IMAP_BLOCKS*BLOCK_SIZE);
  free_4k(d_bitmap,DMAP_BLOCKS*BLOCK_SIZE);
  free_4k(darray,22528*4);
  free_4k(cache_dirty,CACHE_VALUE*4);
  free_4k(cache_array,CACHE_VALUE*4);
}

void objfs_copying_back(struct objfs_state *objfs){
  char* temp;
  malloc_4k(temp,BLOCK_SIZE);
  for(int i=0; i < 22528; i++){
    if(darray[i]==1){
      // //dprintf("dirty is 1 for i = %d\n",i);
      for(int j=0; j < 4048; j++){
        *(temp+j)=*((char*)objs+i*4048+j);
        // //dprintf("%c",*(temp+j));
      }
      write_block(objfs,inode_offset+i,(char*)temp);
      
      // write_block(objfs,287+i,(char*)((char*)objs+i*BLOCK_SIZE));
      
    }
  }
  free_4k(temp,BLOCK_SIZE);
}


int objstore_destroy(struct objfs_state *objfs)
{
  for(int i=0; i < IMAP_BLOCKS; i++){
    write_block(objfs,i,(char*)((char*)i_bitmap+i*BLOCK_SIZE));
  }
  for(int i=0; i < DMAP_BLOCKS; i++){
    write_block(objfs,i+IMAP_BLOCKS,((char*)d_bitmap+i*BLOCK_SIZE));
  }
  char* temp;
  malloc_4k(temp,BLOCK_SIZE);
  for(int i=0; i < 22528; i++){
    if(darray[i]==1){
      // //dprintf("dirty is 1 for i = %d\n",i);
      for(int j=0; j < 4048; j++){
        *(temp+j)=*((char*)objs+i*4048+j);
        // //dprintf("%c",*(temp+j));
      }
      write_block(objfs,inode_offset+i,(char*)temp);
      
      // write_block(objfs,287+i,(char*)((char*)objs+i*BLOCK_SIZE));
      
    }
  }
  free_4k(temp,BLOCK_SIZE);

  for(int i=0; i < CACHE_VALUE; i++){
    if(cache_dirty[i]==1&&cache_array[i]>=0){
      int pos = cache_array[i]*CACHE_VALUE + i;
      char *cache_ptr = objfs->cache;
      char* pointer = cache_ptr + (i<<12);
      write_block(objfs,data_offset+pos,pointer);
    }
  }

  free_4k(objs,22528*BLOCK_SIZE);
  free_4k(obj_backup,OBJ_SIZE);
  free_4k(i_bitmap,IMAP_BLOCKS*BLOCK_SIZE);
  free_4k(d_bitmap,DMAP_BLOCKS*BLOCK_SIZE);
  free_4k(darray,22528*4);
  free_4k(cache_dirty,CACHE_VALUE*4);
  free_4k(cache_array,CACHE_VALUE*4);
  //dprintf("\nDone objstore destroy\n");
  return 0;
}
