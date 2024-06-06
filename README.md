# 项目说明

- 此项目正在开发一个名为MindSyncOS的操作系统。现暂时用于完成北邮操作系统课程作业，如后续有机会将会继续开发。
- This project is developing an operating system called MindSyncOS. It is currently used to complete the homework for the operating system course of BUPT, and will continue to be developed if there is an opportunity in the future.

## 个人信息验证

- 姓名：苏俊杰
- 学号：2022211607
- 班级：2022211807

## 系统运行说明
- 请再`Windows`系统使用`qemu`运行此项目，暂不支持其他操作系统作为宿主机
- 理论上映像文件可以直接在裸机运行，但仍建议在`qemu`进行运行。裸机运行出现任何问题，后果自负
- 请在拉取仓库后，打开命令行，进入 `src`目录，输入 `make run`即可运行操作系统
- 请使用 `git bash`命令行工具，否则可能会出现错误
- 如果修改代码后`make run`出现错误，可以尝试`make clean`清除中间文件后再次`make run`。

## 系统现有功能说明

1. 系统支持鼠标与键盘驱动，可以控制鼠标移动，键盘输入文字
2. 系统支持多图层，图层可以设置上下层关系，可以打开多个窗口，且画面刷新经过优化，提高了画面刷新速度。
3. 系统内存支持动态分区分配，使用首次适应算法。
4. 系统支持多任务，提高并发性。多任务基于优先级算法，设置优先级后鼠标等图像响应速度提高。

## 其他

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
