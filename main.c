#include <stdio.h>
#include <stdlib.h>

#define MAX_STEPS 1000

typedef struct Dimensions{
	float ymin, ymax, xmin, xmax;
	int pixel_width, pixel_height;
}Dimensions;

typedef struct Pixel{
	char r,g,b;
}Pixel;

typedef struct Picture{
	int width, height;
	Pixel **pixel_colors;
}Picture;

typedef struct Complex{
	float x, y;
}Complex;

void read_config_and_populate(Dimensions*, char*);
void create_picture(Picture*, Dimensions);
Complex compute_z(Complex, Complex);
char is_in_mandlebrot(Complex);
void write_picture(Picture);

int main(int argc, char** argv){
	Dimensions d;
	read_config_and_populate(&d, "config");	

	Picture p;

	create_picture(&p, d);
	write_picture(p);	

	return 0;
}

void read_config_and_populate(Dimensions* d, char* config_filename){
	d->ymin = -1;
	d->ymax = 1;
	d->xmin = -2;
	d->xmax = 2;
	d->pixel_width = 800;
	d->pixel_height = 600;
}

void write_picture(Picture p){
	FILE *f = fopen("fractal.ppm", "w");

	fprintf(f, "P3\n%d %d\n2\n", p.width, p.height);

	int i, j;
	for(i = 0; i < p.height; i++){
		for(j = 0; j < p.width; j++){
			fprintf(f, "%d %d %d ", p.pixel_colors[i][j].r, p.pixel_colors[i][j].g, p.pixel_colors[i][j].b);
		}
		fprintf(f, "\n");
	}

}

void create_picture(Picture* p, Dimensions d){
	float x_step = (float)(d.xmax - d.xmin)/(float)d.pixel_width;
	float y_step = (float)(d.ymax - d.ymin)/(float)d.pixel_height;

	float y, x;
	int i, j;

	p->width = d.pixel_width;
	p->height = d.pixel_height;
	
	p->pixel_colors = (Pixel**)malloc(sizeof(Pixel*) * d.pixel_height);

	for(i = 0; i < p->height; i++){
		p->pixel_colors[i] = (Pixel*)malloc(sizeof(Pixel) * d.pixel_width);
	}

	i = j = 0;

	for(y = d.ymin; y < d.ymax - y_step; y += y_step){
		for(x = d.xmin; x < d.xmax - x_step; x += x_step){
			Complex c;
			c.x = x;
			c.y = y;
			char value = is_in_mandlebrot(c);
			p->pixel_colors[i][j].r = value;
			p->pixel_colors[i][j].g = value;
			p->pixel_colors[i][j].b = value;
			//printf("%d %d\n", i, j);
			j++;
		}
		i++;
		j=0;
	}

}

char is_in_mandlebrot(Complex c){
	int step = 0;
	
	Complex z = c;

	while(step < MAX_STEPS && z.x * z.x + z.y * z.y < 4){
		step++;
		z = compute_z(z, c);
	}

	if(step < MAX_STEPS){
		return 1;
	}

	return 0;
}

Complex compute_z(Complex z, Complex c){
	Complex new_z;

	new_z.x = z.x * z.x - z.y * z.y + c.x;
	new_z.y = c.y + 2 * z.x * z.y;

	return new_z;
}
