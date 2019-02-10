# ValgrindedSockets
Scaffolding for a multi-platform socket library with C interfaces (C facilitates its WebAssembly version utilizing browser APIs) intended for Valgrind testing

## Build  
To build it on any platform, run `build.sh` Bash script (on Windows, run it in _Git Bash_).  
This code compiles cleanly with GCC 8.2 and all the recommended warnings:
```
g++ -std=c++17 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wsign-conversion -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2 -pthread main.cpp
```  

## Testing with Valgrind  
To test with valgrind (which can be done in WSL), build as:  
```
g++ -std=c++17 -g -O0 -pthread main.cpp
```
then diagnose threading problems:  
```
valgrind --tool=helgrind ./a.out
```
or to check for leaks:  
```
valgrind --leak-check=yes ./a.out
```
