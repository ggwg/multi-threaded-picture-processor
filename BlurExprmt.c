#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h> // remove later?
#include "Utils.h"
#include "Picture.h"
#include "PicProcess.h"
#include "C-Thread-Pool/thpool.h"

#include <unistd.h>

void blur_task(void *pic) {
  blur_picture((struct picture *) pic);
}

void task(void *arg){
  sleep(3);
	printf("Thread #%u working on %d\n", (int)pthread_self(), (int) arg);
  
}

void sequential_blur(struct picture *pic) {
  puts("Making threadpool with 1 threads");

  threadpool thpool = thpool_init(1);
  thpool_add_work(thpool, blur_task, (void*)(uintptr_t) pic);

  thpool_wait(thpool);
  puts("Killing threadpool");
  thpool_destroy(thpool);
  // blur_picture(pic);

  save_picture_to_file(pic, "blurtest_example.jpg");

    // puts("Making threadpool with 4 threads");
    // threadpool thpool = thpool_init(8);

    // puts("Adding 40 tasks to threadpool");
    // int i;
    // for (i=0; i<40; i++){
    //   thpool_add_work(thpool, task, (void*)(uintptr_t)i);
    // };

    // thpool_wait(thpool);
    // puts("Killing threadpool");
    // thpool_destroy(thpool);
}

// ---------- MAIN PROGRAM ---------- \\

  int main(int argc, char **argv){

    printf("Support Code for Running the Blur Optimisation Experiments... \n");
    
    char input_file[20] = "images/test.jpg";

    struct picture pic;
    init_picture_from_file(&pic, input_file);

    sequential_blur(&pic);
    

    //TODO: implement your blur optimisation experiments in this file

    /*
    TODO:
    1) Row-by-row
    2) Column-by-column
    3) Sector-by-sector
    4) pixel-by-pixel
    */

    // puts("Making threadpool with 4 threads");
    // threadpool thpool = thpool_init(8);

    // puts("Adding 40 tasks to threadpool");
    // int i;
    // for (i=0; i<40; i++){
    //   thpool_add_work(thpool, task, (void*)(uintptr_t)i);
    // };

    // thpool_wait(thpool);
    // puts("Killing threadpool");
    // thpool_destroy(thpool);
    
    // return 0;

  }
