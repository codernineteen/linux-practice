# when we build c language files, it is okay to type commands manually
# but it would be better to write macro files and use it when we need to build
# we just need to modify makefile when there are any changes
CC = gcc
TARGET = main.exe

$(TARGET) : main.o
	$(CC) -o $(TARGET) main.o

main.o : main.c
	$(CC) -c -o main.o main.c

dependency.o : dependency.h
	$(CC) -h -o dependency.o dependency.h

clean :
	rm *.o main_exe
