#源文件
SOURCE = sample.cpp detector.cpp objectinfo.cpp

#可执行文件名称
TARGET = demo

#使用g++编译，且以C++11标准
CXX = g++ -Wall -std=c++11

#opencv需要的参数
CFLAGS =`pkg-config opencv --cflags` `pkg-config opencv --libs`
#只使用上面这一名，会提示"libippicv.a"这个库找不到，所以自己添加上
UFLAGS = -L /usr/local/share/OpenCV/3rdparty/lib/

TARGET:
	    $(CXX) -o $(TARGET) $(SOURCE) $(CFLAGS) $(UFLAGS)

clean:
		rm -f *.a *.o $(TARGET) core *~ *.so *.lo *.swp

# useful to make a backup "make tgz"
tgz: clean
	mkdir -p backups
	tar czvf ./backups_`date +"%Y_%m_%d_%H.%M.%S"`.tgz --exclude backups *
