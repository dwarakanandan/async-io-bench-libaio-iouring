#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/aio_abi.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

inline int io_setup(unsigned nr, aio_context_t *ctxp) { return syscall(__NR_io_setup, nr, ctxp); }
inline int io_destroy(aio_context_t ctx) { return syscall(__NR_io_destroy, ctx); }
inline int io_submit(aio_context_t ctx, long nr,  struct iocb **iocbpp) { return syscall(__NR_io_submit, ctx, nr, iocbpp); }
inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr, struct io_event *events, struct timespec *timeout) { return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout); }

#define PAGE_SIZE (16*1024)
#define PAR 256

using namespace std;

static inline double gettime(void) {
  struct timeval now_tv;
  gettimeofday (&now_tv, NULL);
  return ((double)now_tv.tv_sec) + ((double)now_tv.tv_usec)/1000000.0;
}


void* malloc_huge(size_t size) {
   void* p = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
   madvise(p, size, MADV_HUGEPAGE);
   return p;
}

int main(int argc,char** argv) {
   char* filename=argv[1];
//task_scheduler_init init(atoi(argv[3]));

   struct iocb cb[PAR];
   struct iocb *cbs[PAR];
   for (int i=0; i<PAR; i++)
      cbs[i]=&cb[i];

   char* data = new char[PAR*PAGE_SIZE]; //(char*)malloc(PAR*PAGE_SIZE);
   memset(data,'a',(PAR*PAGE_SIZE));
   
   cout << "First entry of data array " << (data)[0]   << "\n";
   for (int i = 0; i < 100; ++i) {
      cout << data[i]  << std::endl;
   }

   
   io_event events[PAR];
   int ret;
   int fd;

   fd = open(filename, O_RDWR | O_CREAT | O_DIRECT, S_IRUSR | S_IWUSR);
   if (fd < 0) {
      perror("open error");
      return -1;
   }

   struct stat sb;
   fstat(fd, &sb);
   uint64_t nblocks = (200ull*1024*1024*1024) / PAGE_SIZE;

   aio_context_t ctx=0;
   ret = io_setup(PAR, &ctx);
   if (ret < 0) {
      perror("io_setup error");
      return -1;
   }

   size_t counter = 0;
   memset(cb,0,sizeof(cb));
   for (unsigned i=0; i<PAR; i++) {
      cbs[i]=&cb[i];
      cb[i].aio_fildes = fd;
      cb[i].aio_lio_opcode = IOCB_CMD_PREAD;
      cb[i].aio_buf = (uint64_t)(data+(PAGE_SIZE*i));
      cb[i].aio_nbytes = PAGE_SIZE;
      cb[i].aio_offset = counter++ * PAGE_SIZE; //(random()%nblocks)*PAGE_SIZE;
   }

   // setup I/O control block
   uint64_t sum=0,count=0;

   for (unsigned i=0; i<PAR; i++)
      cb[i].aio_offset = (random()%nblocks)/*(next++)*/*PAGE_SIZE;
   ret = io_submit(ctx, PAR, cbs);
   if (ret != PAR) {
      if (ret < 0)
         perror("io_submit error");
      else
         fprintf(stderr, "could not submit IOs %d\n", ret);
      exit(1);
   }

   while (true) {
      uint64_t ops = 0;
      double start=gettime();
      
      while (gettime()-start<1) {
         while (!(ret = io_getevents(ctx, 64, PAR, events, NULL))){
            if (ret<0) {
               fprintf(stderr, "could not get events %d\n", ret);
               exit(1);
            }
         }
         count+=ret;
         for (int i=0; i<ret; i++) {
            auto c=(struct iocb*)events[i].obj;
            sum+=((uint64_t*)c->aio_buf)[0];
            if(events[i].res < 0){
               std::cout << "Error occurred in event " << i << " with error code  " << events[i].res  << "\n";
               exit(1);
            }else{
               cout << "Wrote bytes " << events[i].res  << "\n";
            }
            
            (*c).aio_offset = counter++ * PAGE_SIZE; //(random()%nblocks)/*(next++)*/*PAGE_SIZE;
            cbs[i]=c;
            ops++;
         }
         ret = io_submit(ctx, ret, cbs);
         if (ret < 0) {
            perror("io_submit error");
            exit(1);
         }
      }
      cout << ((ops*PAGE_SIZE)/(1024.0*1024*1024)) << " GB/s" << endl;
   }

   ret = io_destroy(ctx);
   if (ret < 0) {
      perror("io_destroy error");
      return -1;
   }

   return 0;
}
