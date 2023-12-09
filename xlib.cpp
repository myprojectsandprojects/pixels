
//@ if the window is sufficiently large, then resizing the window causes flickering. flicker color is the background color.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/time.h>
// #include <x86intrin.h>

const int initial_window_width = 1000;
const int initial_window_height = 600;

void render_frame(XImage *image)
{
	for (int y = 0; y < image->height; ++y)
	{
		for (int x = 0; x < image->width; ++x)
		{
			// Wait! This is much slower than the alternative below. Even with -O2. Why?
			// char *p = image->data + 4 * x + 4 * image->width * y;
			// *p = 0; ++p; // blue
			// *p = 255; ++p; // green
			// *p = 0; ++p; // red
			// *p = 0;    // padding

			uint32_t *ptr = (uint32_t *)image->data;
			*(ptr + (y * image->width + x)) = 0x00ff0000;
		}
	}
}

int main(int argc, char **argv)
{
	Display *display = XOpenDisplay(NULL);
	if(display == NULL){
		fprintf(stderr, "error: XOpenDisplay()\n");
		return 1;
	}

	int s = DefaultScreen(display);
	unsigned long black = XBlackPixel(display, s);
	unsigned long white = XWhitePixel(display, s);

	Window root_window = DefaultRootWindow(display);
	GC graphics_context = XCreateGC(display, root_window, 0, 0);

	Window window = XCreateSimpleWindow(
		display,                        // display
		root_window,    				// parent window
		1, 1,                         // x, y
		initial_window_width, initial_window_height,  // width, height
		0,                            // border width
		black,                        // border
		white                         // background
	);

	XStoreName(display, window, "Handmadehero gradient");

	/* process window close event through event handler so XNextEvent does not fail */
	Atom del_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(display, window, &del_window, 1);

	XMapWindow(display, window);


	XSelectInput(display, window, ExposureMask|StructureNotifyMask);


	XImage *window_buffer = XCreateImage(
		display,                     // connection
		XDefaultVisual(display, s),  // visual
		XDisplayPlanes(display, s),  // depth
		ZPixmap,               // format
		0,                     // offset
		(char *) malloc(initial_window_width * initial_window_height * sizeof(int)),
		initial_window_width, initial_window_height,
		8,                     // pad
		0);                    // ?

	struct timeval previous_time;
	gettimeofday(&previous_time, NULL);

	bool should_quit = false;

	while(!should_quit)
	{
		while (XPending(display)) {
			XEvent e;
			XNextEvent(display, &e);
	
			switch(e.type) {
				case ClientMessage: {
					// printf("ClientMessage event!\n");
					// if(e.xclient.data.l[0] == del_window)
					// // if(e.xclient.data.l[0] == wmDeleteMessage)
					// 	should_quit = true;

					XClientMessageEvent client_msg = e.xclient;
					if (client_msg.data.l[0] == del_window) {
						printf("User closed the window, so we are quitting the application...\n");
						should_quit = true;
					}

					break;
				}
				case ConfigureNotify: {
					XConfigureEvent xce = e.xconfigure;
	
					if (window_buffer->width != xce.width || window_buffer->height != xce.height) {
						printf("window was resized (%d x %d)\n", xce.width, xce.height);

						XDestroyImage(window_buffer); // also frees data
	
						window_buffer = XCreateImage(
							display,                     // connection
							XDefaultVisual(display, s),  // visual
							XDisplayPlanes(display, s),  // depth
							ZPixmap,               // format
							0,                     // offset
							(char *) malloc(xce.width * xce.height * sizeof(int)),
							xce.width, xce.height,
							8,                     // pad
							0);                    // ?
	
						// render_frame(window_buffer);
					}
					break;
				}
// 				case Expose:
// 				{
// 					printf("EXPOSE\n");
// 					XExposeEvent ex = e.xexpose;

// //					if (ex.count) break;
	
// 					XPutImage(d, w, g, window_buffer,
// 						0, 0, // src
// 						0, 0, // dest
// 						window_buffer->width, window_buffer->height);

// 					break;
// 				}
			}
		}

		render_frame(window_buffer);
	
		XPutImage(display, window, graphics_context, window_buffer,
			0, 0, // src
			0, 0, // dest
			window_buffer->width, window_buffer->height);

		struct timeval current_time;
		gettimeofday(&current_time, NULL);

		double elapsed_ms =
			1000 * (current_time.tv_sec - previous_time.tv_sec)
			+ 0.001 * (current_time.tv_usec - previous_time.tv_usec);
		
		printf("%f ms\n", elapsed_ms);

		previous_time = current_time;
	}

	XCloseDisplay(display);

	return 0;
}