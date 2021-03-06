#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>           
#include <fcntl.h>            
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>        
#include <linux/videodev2.h>

#define CAMERA_DEVICE "/dev/video0"
#define CAPTURE_FILE "frame_%d_.raw"
#define COMPRESS_FILE "compress_%d_.x"
#define UNCOMPRESS_FILE "uncompress_%d_.unx"
#define COMPRESS "sudo ./x %s %s"
#define UNCOMPRESS "sudo ./unx %s %s"

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480
#define VIDEO_FORMAT V4L2_PIX_FMT_YUYV
#define BUFFER_COUNT 4

typedef struct VideoBuffer {
    void   *start;
    size_t  length;
} VideoBuffer;

int main()
{
    int i, ret;

    int fd;
    fd = open(CAMERA_DEVICE, O_RDWR, 0);
    if (fd < 0) {
        printf("Open %s failed\n", CAMERA_DEVICE);
        return -1;
    }

    struct v4l2_capability cap;
    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        printf("VIDIOC_QUERYCAP failed (%d)\n", ret);
        return ret;
    }
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index=0;//formnumber
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;//frametype
    while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1 ){
				printf("VIDIOC_ENUM_FMTsuccess.->fmt.fmt.pix.pixelformat:%s\n",fmtdesc.description);
				fmtdesc.index++;
		}
    // Print capability infomations
    printf("Capability Informations:\n");
    printf(" driver: %s\n", cap.driver);
    printf(" card: %s\n", cap.card);
    printf(" bus_info: %s\n", cap.bus_info);
    printf(" version: %08X\n", cap.version);
    printf(" capabilities: %08X\n", cap.capabilities);

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = VIDEO_WIDTH;
    fmt.fmt.pix.height      = VIDEO_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;//V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        printf("VIDIOC_S_FMT failed (%d)\n", ret);
        return ret;
    }

		memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0) {
        printf("VIDIOC_G_FMT failed (%d)\n", ret);
        return ret;
    }
    // Print Stream Format
    printf("Stream Format Informations:\n");
    printf(" type: %d\n", fmt.type);
    printf(" width: %d\n", fmt.fmt.pix.width);
    printf(" height: %d\n", fmt.fmt.pix.height);
    char fmtstr[8];
    memset(fmtstr, 0, 8);
    memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
    printf(" pixelformat: %s\n", fmtstr);
    printf(" field: %d\n", fmt.fmt.pix.field);
    printf(" bytesperline: %d\n", fmt.fmt.pix.bytesperline);
    printf(" sizeimage: %d\n", fmt.fmt.pix.sizeimage);
    printf(" colorspace: %d\n", fmt.fmt.pix.colorspace);
    printf(" priv: %d\n", fmt.fmt.pix.priv);
    printf(" raw_date: %s\n", fmt.fmt.raw_data);

    struct v4l2_requestbuffers reqbuf;
    
    reqbuf.count = BUFFER_COUNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    
    ret = ioctl(fd , VIDIOC_REQBUFS, &reqbuf);
    if(ret < 0) {
        printf("VIDIOC_REQBUFS failed (%d)\n", ret);
        return ret;
    }

    VideoBuffer*  buffers = calloc( reqbuf.count, sizeof(*buffers) );
    struct v4l2_buffer buf;

    for (i = 0; i < reqbuf.count; i++) 
    {
    		//memset(&buf,0,sizeof(buf));
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(fd , VIDIOC_QUERYBUF, &buf);
        if(ret < 0) {
            printf("VIDIOC_QUERYBUF (%d) failed (%d)\n", i, ret);
            return ret;
        }

        // mmap buffer
        buffers[i].length = buf.length;
        buffers[i].start = (char *) mmap( NULL, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            printf("mmap (%d) failed: %s\n", i, strerror(errno));
            return -1;
        }
    
        // Queen buffer
        ret = ioctl(fd , VIDIOC_QBUF, &buf);
        if (ret < 0) {
            printf("VIDIOC_QBUF (%d) failed (%d)\n", i, ret);
            return -1;
        }

        printf("Frame buffer %d: address=0x%x, length=%d\n", i, (unsigned int)buffers[i].start, buffers[i].length);
    }

    char filename[32];
    char compress_name[32];
    char compress_cmd[64];
    char uncompress_cmd[64];
    
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        printf("VIDIOC_STREAMON failed (%d)\n", ret);
        return ret;
    }
  	for (i=0; i < BUFFER_COUNT; i++ ) 
    {
		    // Get frame
		    ret = ioctl(fd, VIDIOC_DQBUF, &buf);
		    if (ret < 0) {
		        printf("VIDIOC_DQBUF failed (%d)\n", ret);
		        return ret;
		    }

		    // Process the frame	    
		    memset(filename, 0 , 32);
		    memset(compress_name, 0 , 32);
		    memset(compress_cmd, 0, 64);
		    
		    sprintf(filename, CAPTURE_FILE, buf.index);
		    sprintf(compress_name, COMPRESS_FILE, buf.index);
		    sprintf(compress_cmd, COMPRESS, filename, compress_name);
		    
		    FILE *fp = fopen(filename, "wb");
		    if (fp < 0) {
		        printf("open frame data file failed\n");
		        return -1;
		    }
		    
		    printf("Frame buffer %d: address=0x%x, length=%d %d\n", buf.index, (unsigned int)buffers[buf.index].start,  buf.length, buffers[buf.index].length);
		    		    
		    fwrite(buffers[buf.index].start, 1, buf.length, fp);
		    fclose(fp);
		    printf("Capture one frame saved in %s\n", filename);

		    // Re-queen buffer
		    ret = ioctl(fd, VIDIOC_QBUF, &buf);
		    if (ret < 0) {
		        printf("VIDIOC_QBUF failed (%d)\n", ret);
		        return ret;
		    }
		    
		    //Compress
		    system(compress_cmd);
		}
    // Release the resource
    for (i = 0; i < BUFFER_COUNT; i++ ) 
    {
        munmap(buffers[i].start, buffers[i].length);
    }

    // uncompress
    for (i = 0; i < BUFFER_COUNT; i++ ) 
    {
     	memset(filename, 0, 32);
    	memset(compress_name, 0, 32);
    	memset(uncompress_cmd, 0, 64);
    	  
			sprintf(compress_name, COMPRESS_FILE, i);
    	sprintf(filename, UNCOMPRESS_FILE, i);
			sprintf(uncompress_cmd, UNCOMPRESS, compress_name, filename);
			
      system(uncompress_cmd);
      sleep(1);
    }
    close(fd);
    printf("Camera test Done.\n");
    return 0;
}
