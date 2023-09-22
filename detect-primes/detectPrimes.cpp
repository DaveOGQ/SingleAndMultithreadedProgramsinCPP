//Author:David Oti-George

#include "detectPrimes.h"
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <pthread.h>

#include<iostream>
#include<utility>
#include<vector>
#include <algorithm>

std::vector<int64_t> possibleFactors;
std::vector<int64_t> globalPrimes;

std::vector<int64_t> allnums;
int64_t prospectPrime;

int currentindex = 0;

std::atomic<bool> cancel;
//Thread Cancellation
std::atomic<bool> done;
//Thread Exiting

pthread_barrier_t barrier;

int n_threads;


struct data{
  int id;
};


void * thread_function(void * args){
  struct data *in_args = ((struct data *) args);
  int id = in_args->id;
  //one thread pulls out the prospect prime and check for trivial cases
  //all threads will wait for the above code to run before continuing

  // continues to loop until the threads exit
  while(1){
    //if the done atomic boolean is true then the threads exit
    if(done.load(std::memory_order_relaxed)){
      pthread_exit(NULL);
      return(NULL);
    }

    int flag = pthread_barrier_wait(&barrier);
    if(flag == PTHREAD_BARRIER_SERIAL_THREAD){//one thread will execute the following while the rest wait 
      //set initial index to 0
      done.store(false,std::memory_order_relaxed);
      cancel.store(false, std::memory_order_relaxed);

      prospectPrime = allnums[currentindex];

      //get first prime number
      if(prospectPrime < 2){
        cancel.store(true, std::memory_order_relaxed);
        //tell threads to cancel as the number is found to not be prime
      } else if (prospectPrime <=3){
        cancel.store(true, std::memory_order_relaxed);
        globalPrimes.push_back(prospectPrime);
        //tell threads to cancel as the number is found to be prime
      } else if  (prospectPrime % 2 == 0 ){
        cancel.store(true, std::memory_order_relaxed);
        //tell threads to cancel as the number is found to not be prime 
        //even numbers and numbers divisible by 3
      }else if  ( prospectPrime % 3 == 0){
        cancel.store(true, std::memory_order_relaxed);
        //tell threads to cancel as the number is found to not be prime 
        //even numbers and numbers divisible by 3
      } 
    }
    pthread_barrier_wait(&barrier);//all threads will wait for the above code to run before continuing
    
    //if the prospect prime has not been found to be prime or to have any factors  
    //threads will execute parrallel
    if(!cancel.load(std::memory_order_relaxed)){
      int64_t i = 5 + (id * 6);
      int64_t max = sqrt(prospectPrime);
      // possibleFactors.clear();

      // for(int64_t k = i; k<max; k= k + 6){
      //   possibleFactors.push_back(k);
      // }
    
      // makes a list of all the possible factors from 5 to sqrt(prospectprime)

      //spreads the possible factors amongst the threads to each check 
      // for(int64_t j = i; j < max ; j = j + n_threads * 6){
      for(int64_t j = i; j <= max ; j = j + n_threads * 6){
        if(cancel.load(std::memory_order_relaxed)) break;//each thread check if they should cancel, if so, they exit the loop
      
        if(prospectPrime % j == 0 || prospectPrime % (j + 2) == 0 ){
          // the number is not prime (break)
          cancel.store(true, std::memory_order_relaxed);

          break;
        }
      }         
    }

    //for each number from i to max see if that number is prime give the number to each thread
    
    flag = pthread_barrier_wait(&barrier);
    if(flag == PTHREAD_BARRIER_SERIAL_THREAD){//one thread will execute the following while the rest wait 
      if(cancel.load(std::memory_order_relaxed)){
        //reset cancel boolean if it was used
        cancel.store(false, std::memory_order_relaxed);
      }else{
        //if cancel was not used it means the prospect prime was not a trivial prime number 
        //and there were no factors found for it hence the prospect prime is indeed prime
        globalPrimes.push_back(prospectPrime);
      }

      currentindex++;
      //increment the index to the next propect prime

      if(currentindex == (int) allnums.size()){
       done.store(true, std::memory_order_relaxed);
       //if index is ouf of bound for the array set done to true for threads to exit
      }
    }
    pthread_barrier_wait(&barrier); //all threads will wait for the above code to run before continuing
  }
}

// // This function takes a list of numbers in nums[] and returns only numbers that
// // are primes.
// //
// // The parameter n_threads indicates how many threads should be created to speed
// // up the computation.
// // -----------------------------------------------------------------------------
std::vector<int64_t> detect_primes(const std::vector<int64_t> & nums, int nthreads)
{
  n_threads = nthreads;
  pthread_barrier_init(&barrier, NULL, n_threads);

  std::vector<int64_t> result;

  allnums = nums;

  pthread_t pool[n_threads];
  struct data args[n_threads];

  cancel.store(false, std::memory_order_relaxed);//set cancel flag to false
  done.store(false, std::memory_order_relaxed);//set done flag to false

  //give each thread an id
  for(int i = 0; i<n_threads; i++){
    args[i].id = i;
  }
  
  //set first prime being looked to the current index (0)
  prospectPrime = allnums[currentindex];

  for(int i = 0; i<n_threads; i++){
    pthread_create(&pool[i], NULL, thread_function, (void *) &args[i]);//create each thread and pass in the thread function
  }

  for(int i=0; i<n_threads; i++){
    pthread_join(pool[i], NULL);//join back all the threads
  }

  pthread_barrier_destroy(&barrier);//destroy barrier
  return globalPrimes;
}

