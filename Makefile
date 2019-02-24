CC = gcc
CFLAGS = -Wall
OBJS = x_conf_reader.c

all: x_conf_reader

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

rbcfg: $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	rm -f x_conf_reader *.o
