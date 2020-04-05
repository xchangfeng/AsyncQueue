# AsyncQueue
A simple code for Asynchronous queue in Win &amp; Linux. It is used in multithreading program for exchange data.

This code was inpired by glib: https://github.com/GNOME/glib. But the Whole code of glib is too huge to used in a small project. So, a small code example of Asynchronous queue is extracted from glib.

一个在Win和Linux下可运行的异步队列简单代码。代码用于多线程程序的数据同步。

本代码受glib启发：https://github.com/GNOME/glib 。但是由于整个glib代码过于庞大，不适宜在一些小项目中使用。所以，本代码从glib中摘取异步队列相关的代码。


## 1 Test
To test this code. Just clone the code to you system. In Win, just run "AsyncQueue.sln" in Visual Studio. In Linux, just run "make" to generate asyncqueue. Then, the result can be seen. Those code are test in Visual Studio 2015 Community and Ubuntu 18.04.

想要测试这个代码，只需要克隆到你的系统。在Win下，在Visual Studio下运行"AsyncQueue.sln"。 在Linux下，只要运行"make"来生成asyncqueue。然后就可以看到运行的结果。这个代码在Visual Studio 2015社区版和Ubuntu 18.04下测试通过。

## 2 Using the code
### 2.1 Direct
Copy AsyncQueue.h and AsyncQueue.c to your project. And use the APIs in AsyncQueue.h. Those files are works well in Windows and Linux system. Only one thing need to be confirmed: Macro definition of _WIN32 should define first in Windows but not in Linux.

All the APIs are shown in AsyncQueue.h.

将 AsyncQueue.h 和 AsyncQueue.c 复制到你的项目中去。然后就可以使用AsyncQueue.h中的API了。这些文件在Win和Linux下都可以运行。只需要注意一点，在Windows下，_WIN32已经宏定义了，但Linux下没有定义。

所有API都在 AsyncQueue.h 中有详细介绍。

### 2.2 Library
If you want use it by library, It is recommend that use glib library. But if you insist, you can recompile AsyncQueue.h and AsyncQueue.c to .dll file in Windows and .so file in Linux.

如果你想用库调用的方式，这里推荐使用glib的库。但如果非要使用这个文件生成的库，可以将AsyncQueue.h and AsyncQueue.c重新编译，在Windows下，编译成.dll文件，在Linux，编译成.so文件。

