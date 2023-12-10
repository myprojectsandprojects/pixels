#include <stdint.h>

struct PixelBuffer {
	uint32_t *data;
	int32_t width;
	int32_t height;
};

struct Point {
	int32_t x, y;
};

void draw_line(Point point1, Point point2, uint32_t line_color, PixelBuffer *pixels) {
	uint32_t *pixel_ptr = pixels->data;

	int32_t start, end;

	//@ vertical lines (cant calculate slope for them)
	if((point2.x - point1.x) == 0){
		if((point2.y - point1.y) > 0){
			start = point1.y;
			end = point2.y;
		}else{
			start = point2.y;
			end = point1.y;
		}

		for(int y = start; y <= end; ++y){
			pixel_ptr[y * pixels->width + point1.x] = line_color;
		}

		return;
	}

	double slope = (double)(point2.y - point1.y) / (double)(point2.x - point1.x);
	double y_intercept = point1.y - (slope * point1.x);

	if(slope > -1.0 && slope < 1.0){
		if((point2.x - point1.x) > 0){
			start = point1.x;
			end = point2.x;
		}else{
			start = point2.x;
			end = point1.x;
		}

		for(int x = start; x <= end; ++x){
			double y = x * slope + y_intercept;
			int32_t y_round = (int32_t)y;
			pixel_ptr[y_round * pixels->width + x] = line_color;
		}
	}else{
		if((point2.y - point1.y) > 0){
			start = point1.y;
			end = point2.y;
		}else{
			start = point2.y;
			end = point1.y;
		}

		for(int y = start; y <= end; ++y){
			double x = (y - y_intercept) / slope;
			int32_t x_round = (int32_t)x;
			pixel_ptr[y * pixels->width + x_round] = line_color;
		}
	}
	

	pixel_ptr[point1.y * pixels->width + point1.x] = 0x00ff00ff;
	pixel_ptr[point2.y * pixels->width + point2.x] = 0x0000ffff;
}