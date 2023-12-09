g++ -o xlib-example xlib.cpp -lX11
g++ -o xlib-mit-shm-example xlib-mit-shm.cpp -lX11 -lXext
g++ -o xcb-example xcb.cpp -lxcb
g++ -o xcb-mit-shm-example xcb-mit-shm.cpp -lxcb -lxcb-shm