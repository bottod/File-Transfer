VPATH = ./src:./src/inc

SRCPATH = ./src
BUILDPATH = ./build

CC = gcc
CFLAGS = -lpthread 

all: server client
	@echo "Make Succeed!!!"

server:
	@mkdir -p $(BUILDPATH)
	@$(CC) $(SRCPATH)/server.c $(SRCPATH)/inc/sha1.c $(CFLAGS) -o $(BUILDPATH)/$@

client:
	@mkdir -p $(BUILDPATH)
	@$(CC) $(SRCPATH)/client.c $(SRCPATH)/inc/sha1.c $(CFLAGS) -o $(BUILDPATH)/$@


.PHONY:clean all
clean:
	rm -rf $(BUILDPATH)/client $(BUILDPATH)/server 