
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <sys/time.h>

const int initial_window_width = 1000;
const int initial_window_height = 600;

void render_frame(uint32_t *pixels, int32_t width, int32_t height, uint8_t offset)
{
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			uint8_t red = x + offset;
			uint8_t green = 0;
			uint8_t blue = 0;
			*(pixels + (y * width + x)) = (red << 16) | (green << 8) | (blue);
		}
	}
}

int main() {
	// Connect to the X server.
	xcb_connection_t *c = xcb_connect(
        NULL,  // display name
        NULL); // preferred screen number
    if(xcb_connection_has_error(c) > 0){
        fprintf(stderr, "error: xcb_connect()\n");
        exit(EXIT_FAILURE);
    }

	const struct xcb_setup_t *setup = xcb_get_setup(c); // Must not be freed.

	xcb_screen_t *screen = xcb_setup_roots_iterator(setup).data; // Get the first screen (?)
	printf("screen: width: %d, height: %d\n",
		screen->width_in_pixels, screen->height_in_pixels);

	/* Create a window */
	uint32_t const value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t const value_list[] = {screen->white_pixel, XCB_EVENT_MASK_EXPOSURE};
    xcb_drawable_t const app_window = xcb_generate_id(c);
    xcb_create_window(
    	c,
        XCB_COPY_FROM_PARENT,          /* depth */
        app_window,                        /* window Id */
        screen->root,                  /* parent window */
        0, 0,                          /* x, y */
        initial_window_width, initial_window_height, /* width, height */
        10,                             /* border_width */
        XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class */
        XCB_COPY_FROM_PARENT,          // visual's id
        // screen->root_visual,           // visual's id
        value_mask, value_list);   /* masks */

    const char *title = "This Is The Title";
    xcb_change_property(
    	c, // connection
    	XCB_PROP_MODE_REPLACE, // mode
    	app_window, // window
    	XCB_ATOM_WM_NAME, // property
    	XCB_ATOM_STRING, // type
    	8, // format
    	strlen(title), // data length
    	title); // data

    /* Map the window on the screen and flush*/
    xcb_map_window(c, app_window);

    /*
	We want to be notified about the window being closed by the user. If that happens we want to quit the program.
    */
    const char *property_name = "WM_PROTOCOLS";
    const char *value_name = "WM_DELETE_WINDOW";
    xcb_intern_atom_cookie_t ck1 = xcb_intern_atom(c, 1, strlen(property_name), property_name);
    xcb_intern_atom_cookie_t ck2 = xcb_intern_atom(c, 0, strlen(value_name), value_name);
    xcb_intern_atom_reply_t *rp1 = xcb_intern_atom_reply(c, ck1, NULL);
    xcb_intern_atom_reply_t *rp2 = xcb_intern_atom_reply(c, ck2, NULL);
    xcb_change_property(
    	c, // connection
    	XCB_PROP_MODE_REPLACE, // mode
    	app_window, // window
    	rp1->atom, // property
    	XCB_ATOM_ATOM, // type
    	32, // format
    	1, // data length
    	&(rp2->atom)); // data
    xcb_atom_t window_deleted_atom = rp2->atom; // We need this later to recognize this event
    free(rp1);
    free(rp2);

    /* Create black (foreground) graphic context */
    xcb_gcontext_t const gc = xcb_generate_id(c);
    uint32_t const gc_mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t const gc_values[] = {screen->black_pixel, 0};
    xcb_create_gc(c, gc, app_window, gc_mask, gc_values);

    uint32_t total_pixels = initial_window_width * initial_window_height;
    uint32_t *pixels = (uint32_t *) calloc(total_pixels, sizeof(uint32_t));
    uint8_t red = 0;
    for (int i = 0; i < total_pixels; ++i) {
    	pixels[i] = 0x00000000 | (red << 16);
        red += 1;
    }

    // xcb_drawable_t pixmap = xcb_generate_id(c);
    // xcb_void_cookie_t cookie = xcb_create_pixmap(
    // 	c,
    // 	24,           // depth
    // 	pixmap,       // pixmap id
    // 	app_window,       // drawable
    // 	IMAGE_WIDTH,  // width
    // 	IMAGE_HEIGHT);// height

    xcb_flush(c);

    struct timeval previous_time;
	gettimeofday(&previous_time, NULL);

	uint8_t offset = 0;
    /* Event loop */
    bool run = true;
    // for(xcb_generic_event_t *event; (event = xcb_wait_for_event(c)); free(event)){
    xcb_generic_event_t *event;
    while (run) {
    	while(event = xcb_poll_for_event(c)) {
	        switch(event->response_type & ~0x80 )
	        {
		        case XCB_EXPOSE: {
		           	printf("expose event\n");
		        }
		        break;
		        case XCB_CLIENT_MESSAGE: {
		        	printf("XCB_CLIENT_MESSAGE\n");
		        	xcb_client_message_event_t *client_msg = (xcb_client_message_event_t *)event;
		        	if(client_msg->data.data32[0] == window_deleted_atom) {
		        		printf("User closed the window, so we are quitting the application...\n");
		        		run = false;
		        	}
		        }
		        break;
		        default: {
		        	printf("unknown event\n");
		        }
	        }
    		free(event);
    	}

    	render_frame(pixels, initial_window_width, initial_window_height, offset);
    	offset += 1;

    	//@ why not just put the buffer directly into the window without any intermediary pixmap?
       	xcb_put_image(
	    	c,
	    	XCB_IMAGE_FORMAT_Z_PIXMAP,
	    	// pixmap,
	    	app_window,
	    	gc,
	    	initial_window_width, initial_window_height,
	    	0, 0,
	    	0,
	    	24,
	    	initial_window_width * initial_window_height * 4,
	    	(uint8_t *) pixels);

      //  	xcb_copy_area(
	    	// c,
      //       pixmap,
	    	// app_window,
	    	// gc,
	    	// 0, 0,
	    	// 0, 0,
	    	// IMAGE_WIDTH, IMAGE_HEIGHT);

       	struct timeval current_time;
		gettimeofday(&current_time, NULL);

		double elapsed_ms =
			1000 * (current_time.tv_sec - previous_time.tv_sec)
			+ 0.001 * (current_time.tv_usec - previous_time.tv_usec);
		
		// printf("%f ms\n", elapsed_ms);

		previous_time = current_time;	
    }

    xcb_disconnect(c); // Close the connection and free the structure.
	return EXIT_SUCCESS;
}