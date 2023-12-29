#include <iostream>
#include <gst/gst.h>

int main(int, char**){
    gst_init(NULL, NULL);
    std::cout << "Hello, from gstlive!\n";
}
