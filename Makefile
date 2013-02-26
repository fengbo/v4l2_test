CC = gcc
video-test:vmain.o video.o
	$(CC) -o video-test vmain.o video.o
vmain.o:vmain.c video.c video.h
	$(CC) -c vmain.c
video.o:video.c video.h
	$(CC) -c video.c
clean:
	rm file_video.yuv 
	rm video-test 
	rm *.o
