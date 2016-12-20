#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_STEPS 10000

typedef struct Dimensions{
	float ymin, ymax, xmin, xmax;
	int pixel_width, pixel_height;
}Dimensions;

typedef struct Pixel{
	unsigned char r,g,b;
}Pixel;

typedef struct Picture{
	int width, height;
	Pixel **pixel_colors;
	char* filename;
}Picture;

typedef struct Complex{
	float x, y;
}Complex;

typedef struct ThreadArgs{
	Pixel **pixel_colors;
	Dimensions d;
	int assignedIndex, assignedCount;
} ThreadArgs;

void read_config_and_populate(Dimensions*, char*);
void create_picture_of_mandelbrot(Picture*, Dimensions, char*);
void create_picture_of_julia(Picture*, Dimensions, char*);
Complex compute_z(Complex, Complex);
unsigned char is_in_mandelbrot(Complex);
unsigned char is_in_julia(Complex, Complex);
void write_picture(Picture);
Pixel* read_colors(char*);
void *thread_function_mandelbrot(void *);

int main(int argc, char** argv){
	Dimensions d;
	read_config_and_populate(&d, "config");	

	Picture p;

	create_picture_of_mandelbrot(&p, d, "fractal.ppm");
	write_picture(p);	

	create_picture_of_julia(&p, d, "julia.ppm");
	write_picture(p);

	return 0;
}

void read_config_and_populate(Dimensions* d, char* config_filename){
	d->ymin = -1;
	d->ymax = 1;
	d->xmax = 2;
	d->xmin = -2;
	d->pixel_width = 1920;
	d->pixel_height = 1080;
}

void write_picture(Picture p){
	FILE *f = fopen(p.filename, "w");

	fprintf(f, "P3\n%d %d\n256\n", p.width, p.height);

	int i, j;
	for(i = 0; i < p.height; i++){
		for(j = 0; j < p.width; j++){
			fprintf(f, "%d %d %d ", p.pixel_colors[i][j].r, p.pixel_colors[i][j].g, p.pixel_colors[i][j].b);
		}
		fprintf(f, "\n");
	}
	fclose(f);

}

void create_picture_of_mandelbrot(Picture* p, Dimensions d, char* filename){
	float x_step = (float)(d.xmax - d.xmin)/(float)d.pixel_width;
	float y_step = (float)(d.ymax - d.ymin)/(float)d.pixel_height;

	Complex c;
	c.x = -0.153;
	c.y = 0.652995;

	int i, j;

	p->width = d.pixel_width;
	p->height = d.pixel_height;
	p->filename = filename;
	
	p->pixel_colors = (Pixel**)malloc(sizeof(Pixel*) * d.pixel_height);

	for(i = 0; i < p->height; i++){
		p->pixel_colors[i] = (Pixel*)malloc(sizeof(Pixel) * d.pixel_width);
	}

	int nprocs = sysconf(_SC_NPROCESSORS_ONLN);

	ThreadArgs *args;
	args = (ThreadArgs*)malloc(nprocs*sizeof(ThreadArgs));
	int crt_index = 0, step = (d.pixel_height/nprocs);
	if( d.pixel_height%nprocs ){
		step++;
	}

	for(i = 0; i < nprocs; i++){
		args[i].pixel_colors = p->pixel_colors;
		args[i].d = d;
		
		args[i].assignedIndex = crt_index;
		args[i].assignedCount = step;

		int ciot = d.pixel_height - crt_index;
		if( ciot <= step ){
			args[i].assignedCount = ciot;
		}

		crt_index += step;
	}

	pthread_t threads[nprocs];
	for(i = 0; i < nprocs; i++){
		pthread_create(&threads[i], NULL, thread_function_mandelbrot, (void*)(&args[i]));
	}

	for(i = 0; i < nprocs; i++){
		pthread_join(threads[i], NULL);
	}

}

void *thread_function(void *args){
	ThreadArgs arg = *((ThreadArgs*)args);
	
	float x_step = (float)(arg.d.xmax - arg.d.xmin)/(float)arg.d.pixel_width;
	float y_step = (float)(arg.d.ymax - arg.d.ymin)/(float)arg.d.pixel_height;

	Complex c;
	c.x = -0.153;
	c.y = 0.652995;

	Pixel* pixels = read_colors("pal.ppm");

	int i,j;
	for(i = arg.assignedIndex; i < arg.assignedIndex + arg.assignedCount; i++){
		for(j = 0; j < arg.d.pixel_width; j++){
			Complex z;
			z.x = j*x_step + arg.d.xmin;
			z.y = i*y_step + arg.d.ymin;
			unsigned char value = is_in_julia(z, c);
			arg.pixel_colors[i][j].r = pixels[value].r;
			arg.pixel_colors[i][j].g = pixels[value].g;
			arg.pixel_colors[i][j].b = pixels[value].b;
		}
	}
}

void *thread_function_mandelbrot(void *args){
	ThreadArgs arg = *((ThreadArgs*)args);
	
	float x_step = (float)(arg.d.xmax - arg.d.xmin)/(float)arg.d.pixel_width;
	float y_step = (float)(arg.d.ymax - arg.d.ymin)/(float)arg.d.pixel_height;

	Complex c;
	c.x = -0.153;
	c.y = 0.652995;

	Pixel* pixels = read_colors("pal.ppm");

	int i,j;
	for(i = arg.assignedIndex; i < arg.assignedIndex + arg.assignedCount; i++){
		for(j = 0; j < arg.d.pixel_width; j++){
			Complex z;
			z.x = j*x_step + arg.d.xmin;
			z.y = i*y_step + arg.d.ymin;
			unsigned char value = is_in_mandelbrot(z);
			arg.pixel_colors[i][j].r = pixels[value].r;
			arg.pixel_colors[i][j].g = pixels[value].g;
			arg.pixel_colors[i][j].b = pixels[value].b;
		}
	}
}

void create_picture_of_julia(Picture* p, Dimensions d, char* filename){
	float x_step = (float)(d.xmax - d.xmin)/(float)d.pixel_width;
	float y_step = (float)(d.ymax - d.ymin)/(float)d.pixel_height;

	Complex c;
	c.x = -0.153;
	c.y = 0.652995;

	int i, j;

	p->width = d.pixel_width;
	p->height = d.pixel_height;
	p->filename = filename;
	
	p->pixel_colors = (Pixel**)malloc(sizeof(Pixel*) * d.pixel_height);

	for(i = 0; i < p->height; i++){
		p->pixel_colors[i] = (Pixel*)malloc(sizeof(Pixel) * d.pixel_width);
	}

	int nprocs = sysconf(_SC_NPROCESSORS_ONLN);

	ThreadArgs *args;
	args = (ThreadArgs*)malloc(nprocs*sizeof(ThreadArgs));
	int crt_index = 0, step = (d.pixel_height/nprocs);
	if( d.pixel_height%nprocs ){
		step++;
	}

	for(i = 0; i < nprocs; i++){
		args[i].pixel_colors = p->pixel_colors;
		args[i].d = d;
		
		args[i].assignedIndex = crt_index;
		args[i].assignedCount = step;

		int ciot = d.pixel_height - crt_index;
		if( ciot <= step ){
			args[i].assignedCount = ciot;
		}

		crt_index += step;
	}

	pthread_t threads[nprocs];
	for(i = 0; i < nprocs; i++){
		pthread_create(&threads[i], NULL, thread_function, (void*)(&args[i]));
	}

	for(i = 0; i < nprocs; i++){
		pthread_join(threads[i], NULL);
	}
}

unsigned char is_in_mandelbrot(Complex c){
	int step = 0;
	
	Complex z = c;

	while(step < MAX_STEPS && z.x * z.x + z.y * z.y < 4){
		step++;
		z = compute_z(z, c);
	}

	if(step < MAX_STEPS){
		return step%256;
	}

	return 0;
}

unsigned char is_in_julia(Complex z, Complex c){
	int step = 0;
	
	while(step < MAX_STEPS && z.x * z.x + z.y * z.y < 4){
		step++;
		z = compute_z(z, c);
	}

	if(step < MAX_STEPS){
		return step%256;
	}

	return 0;
}

Complex compute_z(Complex z, Complex c){
	Complex new_z;

	new_z.x = z.x * z.x - z.y * z.y + c.x;
	new_z.y = c.y + 2 * z.x * z.y;

	return new_z;
}

Pixel* read_colors(char* filename){
	int width = 256;
	Pixel* pixels = (Pixel*)malloc(sizeof(Pixel) * width);	
	FILE* f = fopen(filename, "rb");
	int endline_count = 0, i;
	unsigned char buffer;

	do{
		fread(&buffer, sizeof(unsigned char), 1, f);
		if(buffer == 10){
			endline_count++;
		}
	}while(endline_count < 3);

	for(i = 0; i < width; i++){
		fread(pixels, sizeof(unsigned char), width*3, f);
	}

	fclose(f);
	return pixels;
}
