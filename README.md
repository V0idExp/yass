[![Build Status](https://travis-ci.org/V0idExp/yass.svg?branch=master)](https://travis-ci.org/V0idExp/yass)

YASS
====

What is YASS? It's just another 2D space shooter, created for fun. It serves as
a sandbox project in which (square) wheels can be reinvented, (bad) design
decisions tried out and (poor) implementation done for learning. It is not meant
in any way to be fun, engaging, original or something else which a good game
should be. A bit of effort is made though to keep the codebase clean, apply a
consistent coding style and have an overall robust and performant
implementation. Code is split in modules tied as less as possible, in order to
make it easily copy-pastable to other projects.

*Build and play on your own risk! :)*

Did you like the project or found it somehow useful? Offer me a
[beer](http://paypal.me/voidexp/5), I'll appreciate!

# License
The code is BSD licensed. Basically, you can do whatever you desire with it.
Assets are bought from [Kenney](http://www.kenney.nl) and are licensed with
Creative Commons Zero (CC0), thus, you can do whatever you want with them too.
All credits for cool assets go to Kenney game studio.

# Dependencies

The list of dependencies is (hopefully) short and easy to satisfy:

 * SDL2
 * SDL2_image
 * GLEW
 * Basic Linear Algebra Subroutines (BLAS) compatible library
   (`libblas-dev` on Ubuntu, `Accelerate` framework on OSX)

# Build

After having ensured all listed dependencies are installed, try:

    $ make

Tested and ran on Mac OS X and Linux.

# Run

Not that difficult either:

    $ ./game
