# 项目说明

- 此项目正在开发一个名为MindSyncOS的操作系统。现暂时用于完成北邮操作系统课程作业，如后续有机会将会继续开发。
- This project is developing an operating system called MindSyncOS. It is currently used to complete the homework for the operating system course of BUPT, and will continue to be developed if there is an opportunity in the future.
- 具体的实验报告请见 `docs/实验报告.md`或 `docs/实验报告.pdf`。

## 个人信息

- 姓名：苏俊杰
- 学号：2022211607
- 班级：2022211807

## 系统运行说明

- 请在裸机使用软盘启动，或在 `Windows`系统使用 `qemu`运行此项目，暂不支持其他启动方式。
- 理论上，映像文件可以直接在裸机运行，但仍建议在 `qemu`进行运行。裸机运行出现任何问题，后果自负。
- 请在拉取仓库后，打开命令行，进入 `src`目录，输入 `make run`即可运行操作系统。
- 请使用 `git bash`命令行工具，否则可能会出现错误。
- 如果修改代码后 `make run`出现错误，可以尝试 `make clean`清除中间文件后再次 `make run`。
- 可以使用 `ctrl + alt`将控制权退出到 `qemu`外部。

## 系统现有功能说明

1. 处理机管理：
   1. 系统支持多任务并发运行，设置任务优先级，高优先级优先运行，同级任务按时间片轮转法调度。
   2. 系统支持绝大多数任务调度原语，能够将任务在创建、就绪、运行、阻塞、挂起、终止几个状态合理切换。
   3. 通过FIFO实现消息队列对任务进行控制，配合开关中断，支持任务的互斥与同步。
2. 存储器管理：
   1. 系统支持动态内存分配与释放，使用首次适应算法，且对内核与用户区域进行了设计与划分。
   2. 系统控制台支持使用 `mem`指令查看内存使用情况。
3. 设备管理：
   1. 系统支持键盘与鼠标控制，编写中断处理程序处理用户输入。
   2. 能够使用键盘输入文字，使用 `Tab`切换窗口，使用鼠标移动窗口。
4. 文件管理：
   1. 系统控制台支持使用 `dir`指令查看磁盘目录，使用 `type`指令查看文件内容。
   2. 支持检索并运行存储在系统硬盘的用户程序，系统内有一个示例程序 `hello.hrb`，可以输入 `hello`或 `hello.hrb`运行。
5. OS与用户接口：
   1. 系统具有可视界面，以窗口形式显示应用程序画面，支持多图层的叠加处理。
   2. 用户可以使用鼠标与系统交互。
   3. 用户可以使用键盘输入指令，并在屏幕现实的控制台查看回显。
   4. 用户可以使用系统提供的API进行编程，实现用户程序在控制台输出文字。

## 系统哲学

以下为MindSyncOS的哲学理念与未来展望

# MindSyncOS - The Cognitive Companion for Your Digital Workflow

Welcome to the repository for MindSyncOS, an innovative operating system designed to elevate the way you interact with your digital workspace. Our philosophy is centered around three core principles: Intelligence, Synchronization, and Personalization.

## Intelligence

We believe that an operating system should be more than just a platform for running applications—it should be an extension of your mind. MindSyncOS is built with advanced algorithms that learn and adapt to your habits, allowing it to predict your needs and streamline your workflow.

## Synchronization

In a world where we switch between multiple devices and applications, we've made sure that MindSyncOS keeps everything in sync. Whether it's remembering the last page you read in an eBook, the line of code you last edited, or the research article you were browsing, MindSyncOS ensures that your digital journey is seamless and uninterrupted.

## Personalization

Every user is unique, and so should be their operating system. MindSyncOS is designed to be highly customizable, enabling you to tailor the system to your specific requirements. From setting up personalized work and study schedules to creating custom shortcuts and commands, MindSyncOS puts you in the driver's seat.

## Features

- Smart Tracking: Keep track of your reading and browsing progress across various documents and web pages.
- Behavioral Memory: OS remembers your preferences and patterns to assist you more effectively.
- Schedule Management: Plan and manage your work and study schedules with built-in tools.
- Customization: Adapt the interface and functionalities to match your workflow and preferences.
- Developer-Friendly: Robust support for coding with features like code snippet management and project tracking.

## Our Vision

To create an operating system that not only manages your digital tasks but also enhances your cognitive abilities. MindSyncOS aims to become the ultimate cognitive companion for users who demand more from their technology.

## Get Involved

We welcome contributions from the community. Whether you're a developer looking to add features or a user with feedback, your involvement helps make MindSyncOS better for everyone.

- Star this repository to bookmark it.
- Fork it to start your own MindSyncOS project.
- Open issues to report bugs or suggest new features.
- Submit pull requests for bug fixes or new functionalities.

## License

MindSyncOS is open-source software, released under the GNU General Public v3 License. Feel free to explore its source code and modify it according to your needs.
