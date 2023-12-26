# when we build c language files, it is okay to type commands manually
# but it would be better to write macro files and use it when we need to build
# we just need to modify makefile when there are any changes
CC = gcc
TARGET = main_exe
OBJECTS = main.o dependency.o

all : $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) -o $@  $(OBJECTS)

clean :
	rm *.o main_exe
