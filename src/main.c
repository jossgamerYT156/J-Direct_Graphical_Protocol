// example of how to use this Graphical "Protocol"
#include <Include/graphical/graphics.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
    for(int i = 0; i < 5; i++) { // repeats itself five times.
        create_window(400, 300, "Test Window"); // args needed are heigh, width and name... though it can't render text yet.
        jFillScreen('R');
        sleep(5); // waits five secs.
    }
    return 0; // returns/quits
}
// change this at will, though some functions that you want to apply and aren't part of graphics.c won't work quite well.
// jFillScreen('R/G/B/K'); will fill the screen with Red, Blue, Green, Black respectly to what leter you int.
// R for Red.
// G for Green.
// B for Blue.
// K for Black.
// Automatically by using create_window(); J-DGP draws na simple mouse ("/") that you can control with W,A,S,D.
