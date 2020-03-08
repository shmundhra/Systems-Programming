#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "png.h"

int width, height;
png_byte color_type;
png_byte bit_depth;

png_bytep *row_pointers = NULL;

void read_png_file(char *filename) {
    FILE *fp = fopen(filename, "rb");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png) abort();

    png_infop info = png_create_info_struct(png);
    if(!info) abort();

    if(setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    png_read_info(png, info);

    width      = png_get_image_width(png, info);
    height     = png_get_image_height(png, info);
    color_type = png_get_color_type(png, info);
    bit_depth  = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if(bit_depth == 16)
        png_set_strip_16(png);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if(png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if(color_type == PNG_COLOR_TYPE_RGB ||
       color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    if(color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }
    png_read_update_info(png, info);

    if (row_pointers) abort();

    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
    }
    png_read_image(png, row_pointers);

    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);
}

void write_png_file(char *filename) {
    int y;

    FILE *fp = fopen(filename, "wb");
    if(!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    if (setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    // png_set_filler(png, 0, PNG_FILLER_AFTER);

    if (!row_pointers) abort();

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    for(int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    fclose(fp);
    png_destroy_write_struct(&png, &info);
}

void write_pgm_file(const char *filename, char* image_equalized, int maxGray)
{
    int i, j;
    int hi, lo;

    FILE* pgmFile = fopen(filename, "wb");
    if (pgmFile == NULL) {
        perror("Cannot open file to write");
        exit(EXIT_FAILURE);
    }

    fprintf(pgmFile, "P5\n");
    fprintf(pgmFile, "%d %d\n", height, width);
    fprintf(pgmFile, "%d\n", maxGray);

    for (i=0; i < height; i++) {
        for (j=0; j < width; j++) {
            // Writing the gray values in the 2D array to the file
            fputc(image_equalized[i*width + j], pgmFile);
        }
        fprintf(pgmFile, "\n");
    }
    fclose(pgmFile);
}

int isvalid(int i, int j) {
    int index = i*width+ j;
    if(index < 0 || index >= width * height)
        return 0;
    return 1;
}

int main(int argc,char *argv[])
{
    int ret, nproc, myid;
    char filename[] = argv[1];

    read_png_file(filename);

    int color_depth = 1<<bit_depth;
    int* histogram = (int*)calloc(color_depth, sizeof(int));
    int* histogram_equalized = (int*)calloc(color_depth, sizeof(int));
    long int image_size = width*height;

    unsigned char* image = (unsigned char*)calloc(image_size, sizeof(unsigned char));
    unsigned char* image_equalized = (unsigned char*)calloc(image_size, sizeof(unsigned char));

    int bytes_per_processor = ((image_size + nproc - 1)/nproc);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    int start_row_id = bytes_per_processor*myid;
    int end_row_id = start_row_id + bytes_per_processor;
    if (end_row_id > image_size) end_row_id = image_size;

    for(int i = start_row_id; i < end_row_id; i++){
        image[i] = (unsigned char)row_pointers[i/width][i%width];
    }

    for(int i = start_row_id; i < end_row_id; i++){
        histogram[image[i]]++;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, histogram, color_depth, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if(myid == 0){
        int counter = 0;
        for(int i = 0; i < color_depth; i++){
            counter += histogram[i];
            histogram_equalized[i] = (color_depth-1)*((float)counter/image_size);
        }
    }
    MPI_Bcast(histogram_equalized, color_depth, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    unsigned char* sub_array = (unsigned char*)malloc(bytes_per_processor);
    MPI_Scatter(image, bytes_per_processor, MPI_BYTE, sub_array, bytes_per_processor, MPI_BYTE, 0, MPI_COMM_WORLD);

    for(int b = start_row_id; b < end_row_id; b++) {
        sub_array[b - start_row_id] = histogram_equalized[image[b]];
    }
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Allgather(sub_array, bytes_per_processor, MPI_BYTE, image_equalized, bytes_per_processor, MPI_BYTE, MPI_COMM_WORLD);
    printf("Debug line %d\n", myid);
    for(int b = start_row_id; b < end_row_id; b++){
        row_pointers[b / width][b % width] = (png_byte)image_equalized[b];
    }
    // MPI_Finalize();
    MPI_Barrier(MPI_COMM_WORLD);

    if (myid == 0) write_pgm_file("histeql.pgm", image_equalized, color_depth-1);

    float sobelX[3][3] = { { -1, 0, 1 },
                           { -2, 0, 2 },
                           { -1, 0, 1 }
                         };

    float sobelY[3][3] = { { -1, -2, -1 },
                           {  0,  0,  0 },
                           {  1,  2,  1 }
                         };

    //Sobel Computation on submatrices starts here
    int *sub_sobelx = (int *)malloc(bytes_per_processor * sizeof(int));
    int *sub_sobely = (int *)malloc(bytes_per_processor * sizeof(int));
    int *sub_sobel = (int *)malloc(bytes_per_processor * sizeof(int));

    int rows_per_proc = height / nproc;
    int a, b, c, d, e, f, x, y;
    for(int i = rows_per_proc*myid; i < rows_per_proc*(myid+1); i++)
    {
        for(int j = 0; j < width; j++)
        {
            a = b = c = d = e = f = 0;
            if(isvalid(i+1, j-1))
                a = image_equalized[(i+1)*width + j-1];
            if(isvalid(i-1, j-1))
                b = image_equalized[(i-1)*width + j-1];
            if(isvalid(i+1, j))
                c = image_equalized[(i+1)*width + j];
            if(isvalid(i-1, j))
                d = image_equalized[(i-1)*width + j];
            if(isvalid(i+1, j+1))
                e = image_equalized[(i+1)*width + j+1];
            if(isvalid(i-1, j+1))
                f = image_equalized[(i-1)*width + j+1];

            sub_sobelx[width*i + j] = (a-b) + 2*(c-d) + (e-f);
            x = sub_sobelx[width*i + j];

            a = b = c = d = e = f = 0;
            if(isvalid(i-1, j+1))
                a = image_equalized[(i-1)*width + j+1];
            if(isvalid(i-1, j-1))
                b = image_equalized[(i-1)*width + j-1];
            if(isvalid(i,j+1))
                c = image_equalized[(i)*width + j+1];
            if(isvalid(i,j-1))
                d = image_equalized[(i)*width + j-1];
            if(isvalid(i+1, j+1))
                e = image_equalized[(i+1)*width + j+1];
            if(isvalid(i+1, j-1))
                f = image_equalized[(i+1)*width + j-1];

            sub_sobely[width*i + j] = (a-b) + 2*(c-d)  + (e-f);
            y = sub_sobely[width*i + j];

            sub_sobel[width*i + j] = sqrt(x*x + y*y);
        }
    }

    unsigned char* sobel;
    if(myid == 0)
    {
        //Initialise sobel to combine sub sobels
        sobel = (unsigned char*)calloc(image_size, sizeof(unsigned char));
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Gather(sub_sobel, bytes_per_processor, MPI_INT, sobel, bytes_per_processor, MPI_INT, 0, MPI_COMM_WORLD);

    if (myid == 0)
    {
        write_pgm_file("final.pgm", sobel, color_depth-1);
    }

    MPI_Finalize();
    return 0;
}