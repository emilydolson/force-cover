# Force-cover

[![Build Status](https://travis-ci.org/emilydolson/force-cover.svg?branch=master)](https://travis-ci.org/emilydolson/force-cover)

Getting accurate test coverage information about C++ code containing templates is challenging; uninstantiated templates don't make it into the compiled binary, so compilers don't instrument them for coverage tracking (i.e. if you never use a template the compiler thinks it isn't runnable code and doesn't count it as lines that should be covered). Since templates with no test coverage are likely to never get instantiated this results in overly accurate test coverage metrics.

Force-cover is a set of tools for dealing with this problem. It consists of two parts: 
* a C++ program (built with Clang Libtooling) that reads your C++ code, finds the templates, and sticks comments before and after them to indicate that they should be covered.
* a python program that looks at the final test coverage output, finds the macros, and adjusts the file as necessary to indicate that uncovered template code should be counted as uncovered code.

The workflow for using Force-cover is as follows:
* Run all of your C++ code through the force_cover C++ program to insert comments.
* Compile your program using appropriate flags for your compiler to indicate that you want to measure test coverage on this program
* Run your program
* Run your coverage program 
* Run the python script on the coverage program's output
