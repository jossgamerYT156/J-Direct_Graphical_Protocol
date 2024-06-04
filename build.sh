# build.sh for J-Direct_Graphical_Protocol.
# to run, you have to stop your XServer, then run the output binary, and what you coded on main.c should render.
# though it might not render completely properly, keep in mind this is still a testing version of J-DGP.

gcc -o J-DGP main.c graphics.c -I./src/ -I/usr/include/libdrm/ -ldrm
