#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>

#define MAX_STEPS 10000

int myid, numprocs;

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

typedef struct Info{
	int width, height;
	int startingColumn;
	int numOfColumns;
}Info;

void read_config_and_populate(Dimensions*, char*);
void create_picture_of_mandelbrot(Picture*, Dimensions, char*);
void create_picture_of_julia(Picture*, Dimensions, char*);
Complex compute_z(Complex, Complex);
unsigned char is_in_mandlebrot(Complex);
unsigned char is_in_julia(Complex, Complex);
void write_picture(Picture);
Pixel* read_colors(char*);
void init_openmpi(int, char **);

int main(int argc, char** argv){
	Dimensions d;
	init_openmpi(argc, argv);
	read_config_and_populate(&d, "config");	

	Picture p;

	create_picture_of_mandelbrot(&p, d, "fractal.ppm");
	write_picture(p);	

	create_picture_of_julia(&p, d, "julia.ppm");
	write_picture(p);

	MPI_Finalize();

	return 0;
}

void init_openmpi(int argc, char** argv){
	MPI_Status status;
 
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);
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

	int i, j;

	if(myid == 0){
		Info info;
		int size = sizeof(info);
		char message[size];

		info.width = d.pixel_width;
		info.height = d.pixel_height;

		int chunkDimension = d.pixel_width / (numprocs-1);

		for(i = 1; i < numprocs; i++){
			info.startingColumn = chunkDimension * (i-1);
			if(i == numprocs-1){
				info.numOfColumns = d.pixel_width - chunkDimension * (numprocs-2);
			}else{
				info.numOfColumns = chunkDimension;
			}
			memcpy(message, &info, size);
			MPI_Send(message,size,MPI_CHAR,i,1,MPI_COMM_WORLD);
		}
	}else{
		MPI_Status status;
		Info info;
		int size = sizeof(info);
		char message[size];
		MPI_Recv(&message,size,MPI_CHAR,0,1,MPI_COMM_WORLD,&status);
		memcpy(&info, message, size);

		Pixel* pixels = read_colors("pal.ppm");

		Pixel pixel_colors[info.width][info.height];

		for(j = info.startingColumn; j < info.startingColumn+info.numOfColumns; j++){
			for(i = 0; i < info.height; i++){
				Complex c;
				c.x = j*x_step + d.xmin;
				c.y = i*y_step + d.ymin;
				unsigned char value = is_in_mandlebrot(c);
				pixel_colors[i][j].r = pixels[value].r;
				pixel_colors[i][j].g = pixels[value].g;
				pixel_colors[i][j].b = pixels[value].b;
			}
		}
		MPI_Send(pixel_colors,info.height*info.width*3,MPI_UNSIGNED_CHAR ,0,2,MPI_COMM_WORLD);
	}

	if(myid == 0){
		p->width = d.pixel_width;
		p->height = d.pixel_height;
		p->filename = filename;
		MPI_Status status;
	
		p->pixel_colors = (Pixel**)calloc(sizeof(Pixel*), d.pixel_height);

		for(i = 0; i < p->height; i++){
			p->pixel_colors[i] = (Pixel*)calloc(sizeof(Pixel), d.pixel_width);
		}

		int chunkDimension = d.pixel_width / (numprocs-1);

		for(i = 1; i < numprocs; i++){
			int startingColumn = chunkDimension * (i-1);
			int numOfColumns;
			if(i == numprocs-1){
				numOfColumns = p->width - chunkDimension * (numprocs-2);
			}else{
				numOfColumns = chunkDimension;
			}
			Pixel pixel_colors[p->width][p->height];
			MPI_Recv(pixel_colors,p->width*p->height*3,MPI_UNSIGNED_CHAR ,i,2,MPI_COMM_WORLD,&status);
			int _i, _j;
			for(_i = 0; _i < p->height; _i++){
				for(_j = startingColumn; _j < startingColumn+numOfColumns; _j++){
					p->pixel_colors[_i][_j].r = pixel_colors[_i][_j].r;
					p->pixel_colors[_i][_j].g = pixel_colors[_i][_j].g;
					p->pixel_colors[_i][_j].b = pixel_colors[_i][_j].b;
				}
			}
		}
	}
}

void create_picture_of_julia(Picture* p, Dimensions d, char* filename){
	float x_step = (float)(d.xmax - d.xmin)/(float)d.pixel_width;
	float y_step = (float)(d.ymax - d.ymin)/(float)d.pixel_height;

	int i, j;

	Complex c;
	c.x = -0.153;
	c.y = 0.652995;

	if(myid == 0){
		Info info;
		int size = sizeof(info);
		char message[size];

		info.width = d.pixel_width;
		info.height = d.pixel_height;

		int chunkDimension = d.pixel_width / (numprocs-1);

		for(i = 1; i < numprocs; i++){
			info.startingColumn = chunkDimension * (i-1);
			if(i == numprocs-1){
				info.numOfColumns = d.pixel_width - chunkDimension * (numprocs-2);
			}else{
				info.numOfColumns = chunkDimension;
			}
			memcpy(message, &info, size);
			MPI_Send(message,size,MPI_CHAR,i,1,MPI_COMM_WORLD);
		}
	}else{
		MPI_Status status;
		Info info;
		int size = sizeof(info);
		char message[size];
		MPI_Recv(&message,size,MPI_CHAR,0,1,MPI_COMM_WORLD,&status);
		memcpy(&info, message, size);

		Pixel* pixels = read_colors("pal.ppm");

		Pixel pixel_colors[info.width][info.height];

		for(j = info.startingColumn; j < info.startingColumn+info.numOfColumns; j++){
			for(i = 0; i < info.height; i++){
				Complex z;
				z.x = j*x_step + d.xmin;
				z.y = i*y_step + d.ymin;
				unsigned char value = is_in_julia(z, c);
				pixel_colors[i][j].r = pixels[value].r;
				pixel_colors[i][j].g = pixels[value].g;
				pixel_colors[i][j].b = pixels[value].b;
			}
		}
		MPI_Send(pixel_colors,info.height*info.width*3,MPI_UNSIGNED_CHAR ,0,2,MPI_COMM_WORLD);
	}

	if(myid == 0){
		p->width = d.pixel_width;
		p->height = d.pixel_height;
		p->filename = filename;
		MPI_Status status;
	
		p->pixel_colors = (Pixel**)calloc(sizeof(Pixel*), d.pixel_height);

		for(i = 0; i < p->height; i++){
			p->pixel_colors[i] = (Pixel*)calloc(sizeof(Pixel), d.pixel_width);
		}

		int chunkDimension = d.pixel_width / (numprocs-1);

		for(i = 1; i < numprocs; i++){
			int startingColumn = chunkDimension * (i-1);
			int numOfColumns;
			if(i == numprocs-1){
				numOfColumns = p->width - chunkDimension * (numprocs-2);
			}else{
				numOfColumns = chunkDimension;
			}
			Pixel pixel_colors[p->width][p->height];
			MPI_Recv(pixel_colors,p->width*p->height*3,MPI_UNSIGNED_CHAR ,i,2,MPI_COMM_WORLD,&status);
			int _i, _j;
			for(_i = 0; _i < p->height; _i++){
				for(_j = startingColumn; _j < startingColumn+numOfColumns; _j++){
					p->pixel_colors[_i][_j].r = pixel_colors[_i][_j].r;
					p->pixel_colors[_i][_j].g = pixel_colors[_i][_j].g;
					p->pixel_colors[_i][_j].b = pixel_colors[_i][_j].b;
				}
			}
		}
	}
}

unsigned char is_in_mandlebrot(Complex c){
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
