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

/* 0 - Purely sequential blur without using any pthread functionality */

void threadless_blur(struct picture *pic) {
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  blur_picture(pic);

  save_picture_to_file(pic, "blurtest/threadless.jpg");

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  printf("0. Threadless Blur Elapsed Time: %ld\n\n", delta_us);
} 

/* 1 - Sequential blur using only 1 thread in the threadpool */

void sequential_blur(struct picture *pic) {
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  puts("Making threadpool with 1 threads");
  threadpool thpool = thpool_init(1);
  thpool_add_work(thpool, blur_task, (void*)(uintptr_t) pic);

  thpool_wait(thpool);
  puts("Killing threadpool");
  thpool_destroy(thpool);
  // blur_picture(pic);

  save_picture_to_file(pic, "blurtest/sequential.jpg");

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  printf("1. Sequential Blur Elapsed Time: %ld\n\n", delta_us);
}

// Arguments struct for row_by_row_blur and col_by_col_blur
struct pic_args {
  struct picture *tmp;
  struct picture *pic;
  int index;
};

/* 2 - Row by row parallel blur */

void row_by_row_task(void *pic_args) {
  struct pic_args *temp_pic_args = (struct pic_args *) pic_args;
  struct picture *tmp = temp_pic_args->tmp;
  struct picture *pic = temp_pic_args->pic;
  int j = temp_pic_args->index;

  // printf("Started thread for index %d\n", i);

  for(int i = 1 ; i < tmp->width - 1; i++){
      
    struct pixel rgb;  
    int sum_red = 0;
    int sum_green = 0;
    int sum_blue = 0;
  
    for(int n = -1; n <= 1; n++){
      for(int m = -1; m <= 1; m++){
        rgb = get_pixel(tmp, i+n, j+m);
        sum_red += rgb.red;
        sum_green += rgb.green;
        sum_blue += rgb.blue;
      }
    }
  
    rgb.red = sum_red / BLUR_REGION_SIZE;
    rgb.green = sum_green / BLUR_REGION_SIZE;
    rgb.blue = sum_blue / BLUR_REGION_SIZE;

    set_pixel(pic, i, j, &rgb);
  }

  free(pic_args);
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
  for(int j = 1 ; j < tmp.height - 1; j++){
    struct pic_args *pic_args2 = malloc(sizeof (struct pic_args));
    pic_args2->tmp = &tmp;
    pic_args2->pic = pic;
    pic_args2->index = j;
    thpool_add_work(thpool, row_by_row_task, (void*)(uintptr_t) pic_args2);
  }

  thpool_wait(thpool);
  puts("Killing threadpool");
  thpool_destroy(thpool);

  clear_picture(&tmp);

  save_picture_to_file(pic, "blurtest/row_by_row.jpg");

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  printf("2. Row-by-row Blur Elapsed Time: %ld\n\n", delta_us);
}

/* 3 - Column by column parallel blur */

void col_by_col_task(void *pic_args) {
  struct pic_args *temp_pic_args = (struct pic_args *) pic_args;
  struct picture *tmp = temp_pic_args->tmp;
  struct picture *pic = temp_pic_args->pic;
  int i = temp_pic_args->index;

  // printf("Started thread for index %d\n", i);

  for(int j = 1 ; j < tmp->height - 1; j++){
      
    struct pixel rgb;  
    int sum_red = 0;
    int sum_green = 0;
    int sum_blue = 0;
  
    for(int n = -1; n <= 1; n++){
      for(int m = -1; m <= 1; m++){
        rgb = get_pixel(tmp, i+n, j+m);
        sum_red += rgb.red;
        sum_green += rgb.green;
        sum_blue += rgb.blue;
      }
    }
  
    rgb.red = sum_red / BLUR_REGION_SIZE;
    rgb.green = sum_green / BLUR_REGION_SIZE;
    rgb.blue = sum_blue / BLUR_REGION_SIZE;

    set_pixel(pic, i, j, &rgb);
  }

  free(pic_args);
}

void col_by_col_blur(struct picture *pic) {
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
    struct pic_args *pic_args2 = malloc(sizeof (struct pic_args));
    pic_args2->tmp = &tmp;
    pic_args2->pic = pic;
    pic_args2->index = i;
    thpool_add_work(thpool, col_by_col_task, (void*)(uintptr_t) pic_args2);
  }

  thpool_wait(thpool);
  puts("Killing threadpool");
  thpool_destroy(thpool);

  clear_picture(&tmp);

  save_picture_to_file(pic, "blurtest/col_by_col.jpg");

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  printf("3. Column-by-column Blur Elapsed Time: %ld\n\n", delta_us);
}

/* 4 - Sector-by-sector (1/2) parallel blur */

// Arguments struct for half_sector_blur
struct half_sector_pic_args {
  struct picture *tmp;
  struct picture *pic;
  int start_x_coord;
  int end_x_coord;
};

void half_sector_task(void *pic_args) {
  struct half_sector_pic_args *temp_pic_args = (struct half_sector_pic_args *) pic_args;
  struct picture *tmp = temp_pic_args->tmp;
  struct picture *pic = temp_pic_args->pic;
  int start_x_coord = temp_pic_args->start_x_coord;
  int end_x_coord = temp_pic_args->end_x_coord;

  for(int i = start_x_coord ; i < end_x_coord; i++){
    for(int j = 1 ; j < tmp->height - 1; j++){  
      struct pixel rgb;  
      int sum_red = 0;
      int sum_green = 0;
      int sum_blue = 0;
    
      for(int n = -1; n <= 1; n++){
        for(int m = -1; m <= 1; m++){
          rgb = get_pixel(tmp, i+n, j+m);
          sum_red += rgb.red;
          sum_green += rgb.green;
          sum_blue += rgb.blue;
        }
      }
    
      rgb.red = sum_red / BLUR_REGION_SIZE;
      rgb.green = sum_green / BLUR_REGION_SIZE;
      rgb.blue = sum_blue / BLUR_REGION_SIZE;

      set_pixel(pic, i, j, &rgb);
    }
  }

  free(pic_args);
}

void half_sector_blur(struct picture *pic) {
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  puts("Making threadpool with 1 threads");
  threadpool thpool = thpool_init(NO_THREADS);

  // Split picture into a list of processes
  struct picture tmp;
  tmp.img = copy_image(pic->img);
  tmp.width = pic->width;
  tmp.height = pic->height;  

  // Calculate midpoint of image (horizontally) to divide image into 2 sectors
  int midpoint = pic->width / 2;

  // Assign arguments and allocate a thread to blur left half of image
  struct half_sector_pic_args *pic_args1 = malloc(sizeof (struct half_sector_pic_args));
  pic_args1->tmp = &tmp;
  pic_args1->pic = pic;
  pic_args1->start_x_coord = 1;
  pic_args1->end_x_coord = midpoint;
  thpool_add_work(thpool, half_sector_task, (void*)(uintptr_t) pic_args1);

  // Assign arguments and allocate a thread to blur right half of image
  struct half_sector_pic_args *pic_args2 = malloc(sizeof (struct half_sector_pic_args));
  pic_args2->tmp = &tmp;
  pic_args2->pic = pic;
  pic_args2->start_x_coord = midpoint;
  pic_args2->end_x_coord = pic->width - 1;
  thpool_add_work(thpool, half_sector_task, (void*)(uintptr_t) pic_args2);

  thpool_wait(thpool);
  puts("Killing threadpool");
  thpool_destroy(thpool);

  clear_picture(&tmp);

  save_picture_to_file(pic, "blurtest/half_sector.jpg");

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  printf("4. Half-sector Blur Elapsed Time: %ld\n\n", delta_us);
}

/* 5 - Sector-by-sector (1/4) parallel blur */

// Arguments struct for quarter_sector_blur
struct quarter_sector_pic_args {
  struct picture *tmp;
  struct picture *pic;
  int start_x_coord;
  int end_x_coord;
  int start_y_coord;
  int end_y_coord;
};

void quarter_sector_task(void *pic_args) {
  struct quarter_sector_pic_args *temp_pic_args = (struct quarter_sector_pic_args *) pic_args;
  struct picture *tmp = temp_pic_args->tmp;
  struct picture *pic = temp_pic_args->pic;
  int start_x_coord = temp_pic_args->start_x_coord;
  int end_x_coord = temp_pic_args->end_x_coord;
  int start_y_coord = temp_pic_args->start_y_coord;
  int end_y_coord = temp_pic_args->end_y_coord;

  for(int i = start_x_coord ; i < end_x_coord; i++){
    for(int j = start_y_coord ; j < end_y_coord; j++){  
      struct pixel rgb;  
      int sum_red = 0;
      int sum_green = 0;
      int sum_blue = 0;
    
      for(int n = -1; n <= 1; n++){
        for(int m = -1; m <= 1; m++){
          rgb = get_pixel(tmp, i+n, j+m);
          sum_red += rgb.red;
          sum_green += rgb.green;
          sum_blue += rgb.blue;
        }
      }
    
      rgb.red = sum_red / BLUR_REGION_SIZE;
      rgb.green = sum_green / BLUR_REGION_SIZE;
      rgb.blue = sum_blue / BLUR_REGION_SIZE;

      set_pixel(pic, i, j, &rgb);
    }
  }
  free(pic_args);
}

void quarter_sector_blur(struct picture *pic) {
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  printf("Making threadpool with %d threads", NO_THREADS);
  threadpool thpool = thpool_init(NO_THREADS);

  // Split picture into a list of processes
  struct picture tmp;
  tmp.img = copy_image(pic->img);
  tmp.width = pic->width;
  tmp.height = pic->height;  

  // Calculate midpoint of image (horizontally and vertically) to divide image into 2 sectors
  int x_midpoint = pic->height / 2;
  int y_midpoint = pic->width / 2;

  // Assign arguments and allocate a thread to blur top-left of image
  struct quarter_sector_pic_args *pic_args1 = malloc(sizeof (struct quarter_sector_pic_args));
  pic_args1->tmp = &tmp;
  pic_args1->pic = pic;
  pic_args1->start_x_coord = 1;
  pic_args1->end_x_coord = x_midpoint;
  pic_args1->start_y_coord = 1;
  pic_args1->end_y_coord = y_midpoint;
  thpool_add_work(thpool, quarter_sector_task, (void*)(uintptr_t) pic_args1);

  // Assign arguments and allocate a thread to blur top-right of image
  struct quarter_sector_pic_args *pic_args2 = malloc(sizeof (struct quarter_sector_pic_args));
  pic_args2->tmp = &tmp;
  pic_args2->pic = pic;
  pic_args2->start_x_coord = x_midpoint;
  pic_args2->end_x_coord = pic->width - 1;
  pic_args2->start_y_coord = 1;
  pic_args2->end_y_coord = y_midpoint;
  thpool_add_work(thpool, quarter_sector_task, (void*)(uintptr_t) pic_args2);

  // Assign arguments and allocate a thread to blur bottom-left of image
  struct quarter_sector_pic_args *pic_args3 = malloc(sizeof (struct quarter_sector_pic_args));
  pic_args3->tmp = &tmp;
  pic_args3->pic = pic;
  pic_args3->start_x_coord = 1;
  pic_args3->end_x_coord = x_midpoint;
  pic_args3->start_y_coord = y_midpoint;
  pic_args3->end_y_coord = pic->height - 1;
  thpool_add_work(thpool, quarter_sector_task, (void*)(uintptr_t) pic_args3);

  // Assign arguments and allocate a thread to blur top-right of image
  struct quarter_sector_pic_args *pic_args4 = malloc(sizeof (struct quarter_sector_pic_args));
  pic_args4->tmp = &tmp;
  pic_args4->pic = pic;
  pic_args4->start_x_coord = x_midpoint;
  pic_args4->end_x_coord = pic->width - 1;
  pic_args4->start_y_coord = y_midpoint;
  pic_args4->end_y_coord = pic->height - 1;
  thpool_add_work(thpool, quarter_sector_task, (void*)(uintptr_t) pic_args4);

  thpool_wait(thpool);
  puts("Killing threadpool");
  thpool_destroy(thpool);

  clear_picture(&tmp);

  save_picture_to_file(pic, "blurtest/quarter_sector.jpg");

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  printf("5. Quarter-sector Blur Elapsed Time: %ld\n\n", delta_us);
}


/* 6 - Pixel by pixel parallel blur */

// Arugments for pixel_by_pixel_blur to be passed to individual threads
struct pixel_pic_args {
  struct picture *tmp;
  struct picture *pic;
  int i;
  int j;
};

void pixel_by_pixel_task(void *pixel_pic_args) {
  struct pixel_pic_args *temp_pic_args = (struct pixel_pic_args *) pixel_pic_args;
  struct picture *tmp = temp_pic_args->tmp;
  struct picture *pic = temp_pic_args->pic;
  int i = temp_pic_args->i;
  int j = temp_pic_args->j;

  struct pixel rgb;  
  int sum_red = 0;
  int sum_green = 0;
  int sum_blue = 0;

  for(int n = -1; n <= 1; n++){
    for(int m = -1; m <= 1; m++){
      rgb = get_pixel(tmp, i+n, j+m);
      sum_red += rgb.red;
      sum_green += rgb.green;
      sum_blue += rgb.blue;
    }
  }

  rgb.red = sum_red / BLUR_REGION_SIZE;
  rgb.green = sum_green / BLUR_REGION_SIZE;
  rgb.blue = sum_blue / BLUR_REGION_SIZE;

  set_pixel(pic, i, j, &rgb);

  free(pixel_pic_args);
}

void pixel_by_pixel_blur(struct picture *pic) {
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);

  printf("Making threadpool with %d threads", NO_THREADS);
  threadpool thpool = thpool_init(NO_THREADS);

  // Split picture into a list of processes
  struct picture tmp;
  tmp.img = copy_image(pic->img);
  tmp.width = pic->width;
  tmp.height = pic->height;  

  // Iterates over the columns of the image
  for(int i = 1 ; i < tmp.width - 1; i++){
    for(int j = 1 ; j < tmp.height - 1; j++){
      struct pixel_pic_args *pic_args = malloc(sizeof (struct pixel_pic_args));
      pic_args->tmp = &tmp;
      pic_args->pic = pic;
      pic_args->i = i;
      pic_args->j = j;
      thpool_add_work(thpool, pixel_by_pixel_task, (void*)(uintptr_t) pic_args);
    }
  }

  thpool_wait(thpool);
  puts("Killing threadpool");
  thpool_destroy(thpool);

  clear_picture(&tmp);

  save_picture_to_file(pic, "blurtest/pixel_by_pixel.jpg");

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  printf("6. Pixel-by-pixel Blur Elapsed Time: %ld\n\n", delta_us);
}

// ---------- MAIN PROGRAM ---------- \\

  int main(int argc, char **argv){

    printf("Support Code for Running the Blur Optimisation Experiments... \n");
    
    char input_file[20] = "images/lake.jpeg";

    struct picture pic;
    init_picture_from_file(&pic, input_file);

    struct picture threadless_pic;
    copy_picture(&threadless_pic, &pic);
    threadless_blur(&threadless_pic);

    struct picture sequential_pic;
    copy_picture(&sequential_pic, &pic);
    sequential_blur(&sequential_pic);

    struct picture row_by_row_pic;
    copy_picture(&row_by_row_pic, &pic);
    row_by_row_blur(&row_by_row_pic);

    struct picture col_by_col_pic;
    copy_picture(&col_by_col_pic, &pic);
    col_by_col_blur(&col_by_col_pic);

    struct picture half_sector_pic;
    copy_picture(&half_sector_pic, &pic);
    half_sector_blur(&half_sector_pic);

    struct picture quarter_sector_pic;
    copy_picture(&quarter_sector_pic, &pic);
    quarter_sector_blur(&quarter_sector_pic);

    struct picture pixel_by_pixel_pic;
    copy_picture(&pixel_by_pixel_pic, &pic);
    pixel_by_pixel_blur(&pixel_by_pixel_pic);




    clear_picture(&pic);
    clear_picture(&threadless_pic);
    clear_picture(&sequential_pic);
    clear_picture(&row_by_row_pic);
    clear_picture(&col_by_col_pic);
    clear_picture(&half_sector_pic);
    clear_picture(&quarter_sector_pic);
    clear_picture(&pixel_by_pixel_pic);

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