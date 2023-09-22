//Author:David Oti-George

#include "calcpi.h"
#include <pthread.h>
#include <unistd.h>


struct input{
  int start;
  int end;
  int id;
  uint64_t localc;
  double r;

};

//Same set up as before...
void * threadfunction(void * args){
  struct input * in = ((struct input *) args);
  int start = in -> start; 
  int end = in -> end;
  double r = in -> r;
  
  uint64_t localc = 0;
  
  double rsq = double(r) * r;

  //Instead of doing any fancy tricks here, we'll just normally iterate from start to end and just rely on the 
  //parent to have giving us good start and end values
  for( double x = start ; x <= end ; x ++)
    for( double y = 0 ; y <= r ; y ++)
      if( x*x + y*y <= rsq) localc ++;
  
  in->localc = localc;

  //Exit...
  pthread_exit(NULL);
  return NULL;
}


uint64_t count_pixels(int r, int n_threads)
{
  pthread_t threadpool[n_threads];
  input inputs[n_threads];


  int div = r / n_threads;
  int mod = r % n_threads;
  int lastend = 0;

  for(int i = 0; i<n_threads; i++){
    inputs[i].id = i;
    inputs[i].localc = 0;
    inputs[i].r = r;
    

    //Okay so each thread will start where the last thread ended, evenly distributing them ina 
    //Since we wrote our thread to be non-inclusive of the end this is okay
    inputs[i].start = lastend + 1;

    //IE we evenly distribute all of our elements
    if(i < mod){//suppose there are n elements that dont evenly divide into the number of threads, then one of those n elements will be added each to the first n threads
        inputs[i].end = inputs[i].start + div + 1;
    }
    else{
        inputs[i].end = inputs[i].start + div;// adds the predetermined increment, once the remainders that dont divide evenly have been taken care of.
    }
    
    //Make sure to update where the last element is...
    lastend = inputs[i].end;
  }

  for(int i = 0; i<n_threads; i++){
      pthread_create(&threadpool[i], NULL, threadfunction, (void *) &inputs[i]);// create and run threads
  }
  for(int i = 0; i<n_threads; i++){
      pthread_join(threadpool[i], NULL);//join threads together
  }

  uint64_t count = 0;

  for(int i = 0; i<n_threads; i++){
    count = count + inputs[i].localc;
  }

  //single-threaded solution below
  // double rsq = double(r) * r;
  // uint64_t count = 0;
  // for( double x = 1 ; x <= r ; x ++)
  //   for( double y = 0 ; y <= r ; y ++)
  //     if( x*x + y*y <= rsq) count ++;
  
  return count * 4 + 1;
}
