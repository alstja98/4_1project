CC:= /opt/arm-linux-4.1.1/bin/arm-linux-gcc-4.1.1

TARGET = 7_project
OBJS = 7_project.o device.o
SRCS = 7_project.c device.c

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) -lpthread

7_project.o: 7_project.c device.h
	$(CC) -c 7_project.c

device.o: device.c device.h
	$(CC) -c device.c

clean:
	rm -rf *.o
	rm -f $(TARGET)
