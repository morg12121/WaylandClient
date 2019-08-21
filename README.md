# WaylandClient
Tries to open a simple window using the Wayland protocol (sway). 

# Important
The code relies on "xdg-shell-protocol.c" and "xdg-shell-client-protocol.h", both of which are created on compile (look at the makefile for further information).

# Features
Currently it only sets up a very basic window (with some bugs as goodies). It shows an animated blue screen for demonstration purposes, rendered in software.
It also should be noted that this program hogs the CPU because I don't know if there is a nice wayland-ish way to avoid that (for a software renderer).
For the moment, one can implement a proper sleep routine if they want to.
I may or may not add more features to this code for educational purposes. 
