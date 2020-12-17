#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h> // remove later?
#include "Utils.h"
#include "Picture.h"
#include "PicProcess.h"
#include "C-Thread-Pool/thpool.h"

#include <unistd.h>

// Blur pre-definied MACROS
#define NO_RGB_COMPONENTS 3
#define BLUR_REGION_SIZE 9

// Number of threads for our process
#define NO_THREADS 4

void copy_picture(struct picture *new_pic, struct picture* pic);

struct timespec start, end;

// Example task called by a thread to demonstrate thread functionality.
void task(void *arg){
  sleep(1);
	printf("Thread #%u working on %d\n", (int)pthread_self(), (int) arg);
}

void blur_task(void *pic) {
  blur_picture((struct picture *) pic);
}

void sequential_blur(struct picture *pic) {
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  puts("Making threadpool with 1 threads");
  threadpool thpool = thpool_init(1);
  thpool_add_work(thpool, blur_task, (void*)(uintptr_t) pic);

  thpool_wait(thpool);
  puts("Killing threadpool");
  thpool_destroy(thpool);
  // blur_picture(pic);

  save_picture_to_file(pic, "blurtest_example.jpg");

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  printf("1. Sequential Blur Elapsed Time: %ld\n", delta_us);
}

void row_by_row_blur(struct picture *pic) {
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  puts("Making threadpool with 1 threads");
  threadpool thpool = thpool_init(NO_THREADS);

  // Split picture into a list of processes
  struct picture tmp;
  tmp.img = copy_image(pic->img);
  tmp.width = pic->width;
  tmp.height = pic->height;  

  // Iterates over the columns of the image
  for(int i = 1 ; i < tmp.width - 1; i++){
    // Create new thread 
    thpool_add_work(thpool, task, (void*)(uintptr_t) pic);
    // for(int j = 1 ; j < tmp.height - 1; j++){
        
    //   struct pixel rgb;  
    //   int sum_red = 0;
    //   int sum_green = 0;
    //   int sum_blue = 0;
    
    //   for(int n = -1; n <= 1; n++){
    //     for(int m = -1; m <= 1; m++){
    //       rgb = get_pixel(&tmp, i+n, j+m);
    //       sum_red += rgb.red;
    //       sum_green += rgb.green;
    //       sum_blue += rgb.blue;
    //     }
    //   }
    
    //   rgb.red = sum_red / BLUR_REGION_SIZE;
    //   rgb.green = sum_green / BLUR_REGION_SIZE;
    //   rgb.blue = sum_blue / BLUR_REGION_SIZE;
    
    //   set_pixel(pic, i, j, &rgb);
    // }
  }

  thpool_wait(thpool);
  puts("Killing threadpool");
  thpool_destroy(thpool);

  clear_picture(&tmp);

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  printf("1. Sequential Blur Elapsed Time: %ld\n", delta_us);
}

// ---------- MAIN PROGRAM ---------- \\

  int main(int argc, char **argv){

    printf("Support Code for Running the Blur Optimisation Experiments... \n");
    
    char input_file[20] = "images/test.jpg";

    struct picture pic;
    init_picture_from_file(&pic, input_file);

    struct picture sequential_pic;
    copy_picture(&sequential_pic, &pic);
    sequential_blur(&sequential_pic);

    struct picture row_by_row_pic;
    copy_picture(&row_by_row_pic, &pic);
    row_by_row_blur(&row_by_row_pic);




    clear_picture(&pic);
    clear_picture(&sequential_pic);
    clear_picture(&row_by_row_pic);

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

void copy_picture(struct picture *new_pic, struct picture* pic) {
  new_pic->img = copy_image(pic->img);
  new_pic->width = pic->width;
  new_pic->height = pic->height;
}