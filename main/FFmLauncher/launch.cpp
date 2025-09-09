#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <cstdarg> 
#include <time.h> // 引入时间相关头文件用于日志
#include <stdint.h> // 引入 uint32_t 等类型
#include <sys/wait.h> // 用于 system() 的返回值检查
#include <signal.h> // 信号处理

// --- Camera Config ---
#define DEVICE "/dev/video7"
#define WIDTH 1920
#define HEIGHT 1080
// ---------------------

// --- IPC Config (与 display/main.c 保持一致) ---
#define SHM_NAME "/display_shm" // 共享内存名称
#define SEM_NAME "/display_sem" // 信号量名称
#define BUFFER_SIZE 128         // 消息缓冲区大小
// -----------------------------------------------

// --- 日志宏定义 (简化版，与 display/main.c 风格类似) ---
#define LOG_TAG "[LAUNCH]"
void log_info(const char *format, ...) {
    time_t now;
    struct tm *tm_info;
    char buffer[26];
    time(&now);
    tm_info = localtime(&now);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s] %s [INFO] ", buffer, LOG_TAG);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

void log_error(const char *format, ...) {
    time_t now;
    struct tm *tm_info;
    char buffer[26];
    time(&now);
    tm_info = localtime(&now);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(stderr, "[%s] %s [ERROR] ", buffer, LOG_TAG);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
}

void log_debug(const char *format, ...) {
#ifdef DEBUG
    time_t now;
    struct tm *tm_info;
    char buffer[26];
    time(&now);
    tm_info = localtime(&now);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s] %s [DEBUG] ", buffer, LOG_TAG);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
#endif
}
// --------------------------------------------------------

// 全局变量用于 cleanup (与 display/main.c 保持一致)
static int shm_fd = -1;
static void *shared_memory = MAP_FAILED;
static sem_t *semaphore = SEM_FAILED;
static volatile sig_atomic_t running = 1; // 用于信号处理

// 信号处理函数 (与 display/main.c 保持一致)
void cleanup(int sig) {
    log_info("Received signal %d, cleaning up...", sig);
    running = 0; // 设置运行标志为假

    // 清理共享内存映射
    if (shared_memory != MAP_FAILED) {
        if (munmap(shared_memory, BUFFER_SIZE) == -1) {
            log_error("Failed to unmap shared memory: %s", strerror(errno));
        } else {
            log_debug("Shared memory unmapped.");
        }
        shared_memory = MAP_FAILED;
    }

    // 关闭共享内存文件描述符 (注意：display中未明确close shm_fd，但最好加上)
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
        log_debug("Shared memory file descriptor closed.");
    }

    // 关闭信号量
    if (semaphore != SEM_FAILED) {
        if (sem_close(semaphore) == -1) {
            log_error("Failed to close semaphore: %s", strerror(errno));
        } else {
            log_debug("Semaphore closed.");
        }
        // 注意：通常由创建者 unlink，这里不主动 unlink
        // sem_unlink(SEM_NAME);
        semaphore = SEM_FAILED;
    }

    // 注意：通常由创建者 unlink，这里不主动 unlink 共享内存对象
    // shm_unlink(SHM_NAME);

    log_info("Cleanup completed. Exiting.");
    exit(EXIT_SUCCESS);
}

// 设置曝光和增益
int set_camera_controls()
{
    log_debug("Opening subdev /dev/v4l-subdev2...");
    int subdev_fd = open("/dev/v4l-subdev2", O_RDWR);
    if (subdev_fd < 0) {
        log_error("Failed to open subdev /dev/v4l-subdev2: %s", strerror(errno));
        return -1;
    }
    log_debug("Successfully opened subdev.");

    struct v4l2_control ctrl_exp;
    memset(&ctrl_exp, 0, sizeof(ctrl_exp));
    ctrl_exp.id = V4L2_CID_EXPOSURE;
    ctrl_exp.value = 1300;
    log_debug("Setting exposure to %d...", ctrl_exp.value);

    if (ioctl(subdev_fd, VIDIOC_S_CTRL, &ctrl_exp) < 0) {
        log_error("Failed to set exposure to %d: %s", ctrl_exp.value, strerror(errno));
    } else {
        log_info("Successfully set exposure to %d.", ctrl_exp.value);
    }

    struct v4l2_control ctrl_gain;
    memset(&ctrl_gain, 0, sizeof(ctrl_gain));
    ctrl_gain.id = V4L2_CID_ANALOGUE_GAIN;
    ctrl_gain.value = 200;
    log_debug("Setting analogue gain to %d...", ctrl_gain.value);

    if (ioctl(subdev_fd, VIDIOC_S_CTRL, &ctrl_gain) < 0) {
        log_error("Failed to set analogue gain to %d: %s", ctrl_gain.value, strerror(errno));
    } else {
        log_info("Successfully set analogue gain to %d.", ctrl_gain.value);
    }

    close(subdev_fd);
    log_debug("Closed subdev.");
    return 0;
}

// 采集一帧 NV12
int capture_nv12_frame(const char *output_file)
{
    log_info("Starting frame capture to %s...", output_file);
    log_debug("Opening video device %s...", DEVICE);
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        log_error("Failed to open video device %s: %s", DEVICE, strerror(errno));
        return -1;
    }
    log_debug("Successfully opened video device.");

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = WIDTH;
    fmt.fmt.pix_mp.height = HEIGHT;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    log_debug("Setting format: %dx%d, NV12...", WIDTH, HEIGHT);

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        log_error("Failed to set video format: %s", strerror(errno));
        close(fd);
        return -1;
    }
    log_debug("Successfully set video format.");

    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 3;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;
    log_debug("Requesting %d buffers...", req.count);

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        log_error("Failed to request buffers: %s", strerror(errno));
        close(fd);
        return -1;
    }
    log_debug("Successfully requested buffers.");

    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    memset(&buf, 0, sizeof(buf));
    memset(planes, 0, sizeof(planes));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    buf.m.planes = planes;
    buf.length = 1; // 假设只使用一个平面
    log_debug("Querying buffer info for index %d...", buf.index);

    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        log_error("Failed to query buffer: %s", strerror(errno));
        close(fd);
        return -1;
    }
    log_debug("Successfully queried buffer. Length: %u, Offset: %u",
              buf.m.planes[0].length, buf.m.planes[0].m.mem_offset);

    log_debug("Mapping buffer memory...");
    void *buffer_start = mmap(NULL, buf.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.planes[0].m.mem_offset);
    if (buffer_start == MAP_FAILED) {
        log_error("Failed to mmap buffer: %s", strerror(errno));
        close(fd);
        return -1;
    }
    log_debug("Successfully mapped buffer memory.");

    log_debug("Queueing buffer (index %d)...", buf.index);
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        log_error("Failed to queue buffer: %s", strerror(errno));
        munmap(buffer_start, buf.m.planes[0].length);
        close(fd);
        return -1;
    }
    log_debug("Successfully queued buffer.");

    log_debug("Starting streaming (type %d)...", buf.type);
    if (ioctl(fd, VIDIOC_STREAMON, &buf.type) < 0) {
        log_error("Failed to start streaming: %s", strerror(errno));
        munmap(buffer_start, buf.m.planes[0].length);
        close(fd);
        return -1;
    }
    log_debug("Streaming started.");

    log_debug("Dequeuing buffer (waiting for frame)...");
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        log_error("Failed to dequeue buffer (capture frame): %s", strerror(errno));
        ioctl(fd, VIDIOC_STREAMOFF, &buf.type);
        munmap(buffer_start, buf.m.planes[0].length);
        close(fd);
        return -1;
    }
    log_info("Frame captured successfully. Size: %u bytes.", buf.m.planes[0].bytesused);

    log_debug("Opening output file %s for writing...", output_file);
    FILE *fp = fopen(output_file, "wb");
    if (!fp) {
        log_error("Failed to open output file %s: %s", output_file, strerror(errno));
        ioctl(fd, VIDIOC_STREAMOFF, &buf.type);
        munmap(buffer_start, buf.m.planes[0].length);
        close(fd);
        return -1;
    }

    log_debug("Writing %u bytes to output file...", buf.m.planes[0].bytesused);
    size_t written = fwrite(buffer_start, buf.m.planes[0].bytesused, 1, fp);
    if (written != 1) {
        log_error("Failed to write complete frame to file: %s", strerror(errno));
        fclose(fp);
        ioctl(fd, VIDIOC_STREAMOFF, &buf.type);
        munmap(buffer_start, buf.m.planes[0].length);
        close(fd);
        return -1;
    }
    fclose(fp);
    log_info("Frame data written to %s.", output_file);

    log_debug("Stopping streaming...");
    if (ioctl(fd, VIDIOC_STREAMOFF, &buf.type) < 0) {
         log_error("Failed to stop streaming (ignoring): %s", strerror(errno));
    } else {
        log_debug("Streaming stopped.");
    }

    log_debug("Unmapping buffer memory...");
    if (munmap(buffer_start, buf.m.planes[0].length) == -1) {
        log_error("Failed to unmap buffer: %s", strerror(errno));
    } else {
        log_debug("Buffer memory unmapped.");
    }

    close(fd);
    log_debug("Video device closed.");
    log_info("Frame capture completed successfully.");
    return 0;
}

// 处理拍照、压缩和传输
void process_capture()
{
    log_info("=== Starting Capture Process ===");
    // 设置相机控制参数
    log_debug("Setting camera controls...");
    if (set_camera_controls() != 0) {
        log_error("Failed to set camera controls. Continuing with capture...");
        // 根据需求决定是否继续或返回
        // return; // 如果认为控制失败是致命的，可以取消注释
    } else {
        log_debug("Camera controls set successfully.");
    }

    // 拍照并保存为 NV12 格式
    log_debug("Capturing NV12 frame...");
    if (capture_nv12_frame("/opt/frame_nv12.raw") != 0) {
        log_error("Capture failed.");
        return;
    }
    log_info("NV12 frame captured successfully.");

    // 使用 ffmpeg 压缩为 JPEG 格式
    log_info("Running ffmpeg to convert NV12 to JPEG...");
    log_debug("Executing command: ffmpeg -y -f rawvideo -pixel_format nv12 -s 1920x1080 -i /opt/frame_nv12.raw -vf scale=512:288 -q:v 5 -f image2 /opt/frame.jpg");
    int ffmpeg_result = system("ffmpeg -y -f rawvideo -pixel_format nv12 -s 1920x1080 -i /opt/frame_nv12.raw -vf scale=512:288 -q:v 5 -f image2 /opt/frame.jpg");
    if (ffmpeg_result == -1) {
        log_error("system() call for ffmpeg failed: %s", strerror(errno));
        return;
    } else if (WIFEXITED(ffmpeg_result) && WEXITSTATUS(ffmpeg_result) != 0) {
        log_error("FFmpeg conversion failed with exit code: %d", WEXITSTATUS(ffmpeg_result));
        return;
    } else if (WIFSIGNALED(ffmpeg_result)) {
        log_error("FFmpeg conversion was terminated by signal: %d", WTERMSIG(ffmpeg_result));
        return;
    }
    log_info("FFmpeg conversion completed successfully. Output: /opt/frame.jpg");

    // 触发 BLE 传输
    log_info("Triggering BLE transmission...");
    log_debug("Copying JPEG to /tmp/123.jpg...");
    int cp_result = system("cp /opt/frame.jpg /tmp/123.jpg");
    if (cp_result == -1) {
        log_error("system() call for cp failed: %s", strerror(errno));
        return;
    } else if (WIFEXITED(cp_result) && WEXITSTATUS(cp_result) != 0) {
        log_error("Failed to copy JPEG to /tmp: %d", WEXITSTATUS(cp_result));
        return;
    } else if (WIFSIGNALED(cp_result)) {
        log_error("cp command was terminated by signal: %d", WTERMSIG(cp_result));
        return;
    }

    log_debug("Creating trigger file /tmp/send...");
    int touch_result = system("touch /tmp/send");
    
    if (touch_result == -1) {
        log_error("system() call for touch failed: %s", strerror(errno));
        return;
    } else if (WIFEXITED(touch_result) && WEXITSTATUS(touch_result) != 0) {
        log_error("Failed to create trigger file /tmp/send: %d", WEXITSTATUS(touch_result));
        return;
    } else if (WIFSIGNALED(touch_result)) {
        log_error("touch command was terminated by signal: %d", WTERMSIG(touch_result));
        return;
    }

    // 发送"PhotoCaptured"信号到共享内存
    log_debug("Sending PhotoCaptured signal to shared memory...");
    const char* photo_captured_msg = "PhotoCaptured";
    memset(shared_memory, 0, BUFFER_SIZE); // 清空共享内存
    strncpy((char*)shared_memory, photo_captured_msg, BUFFER_SIZE - 1);
    ((char*)shared_memory)[BUFFER_SIZE - 1] = '\0'; // 确保字符串结束
    
    // 通知其他进程
    if (semaphore != SEM_FAILED) {
        if (sem_post(semaphore) == -1) {
            log_error("Failed to post semaphore for PhotoCaptured signal: %s", strerror(errno));
        } else {
            log_info("PhotoCaptured signal sent successfully to shared memory.");
        }
    } else {
        log_error("Semaphore not available, cannot notify other processes.");
    }

    log_info("BLE transmission triggered successfully.");
    log_info("=== Capture Process Completed ===");
}

// 主函数
int main()
{
    log_info("=== Launch Program Started ===");

    // 注册信号处理函数 (与 display/main.c 保持一致)
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    // --- 初始化 IPC (监听已存在的 display_sem 和 display_shm) ---
    // 1. 打开已存在的共享内存 (由 display 程序创建)
    log_debug("Opening existing shared memory object %s...", SHM_NAME);
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        log_error("shm_open failed: %s", strerror(errno));
        cleanup(0); // 使用 cleanup 进行统一清理
        return -1;
    }
    log_debug("Shared memory object opened successfully (fd: %d).", shm_fd);

    // 2. 映射共享内存到进程地址空间
    log_debug("Mapping shared memory...");
    shared_memory = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        log_error("mmap failed: %s", strerror(errno));
        cleanup(0);
        return -1;
    }
    log_debug("Shared memory mapped successfully at %p.", shared_memory);

    // 3. 关闭文件描述符 (因为我们已经映射了)
    close(shm_fd);
    shm_fd = -1; // 标记为已关闭
    log_debug("Shared memory file descriptor closed.");

    // 4. 打开已存在的信号量 (由 display 程序创建)
    log_debug("Opening existing semaphore %s...", SEM_NAME);
    semaphore = sem_open(SEM_NAME, 0); // 不创建，只打开
    if (semaphore == SEM_FAILED) {
        log_error("sem_open failed: %s", strerror(errno));
        cleanup(0);
        return -1;
    }
    log_debug("Semaphore opened successfully.");
    // --- IPC 初始化完成 ---

    log_info("Listening for signals on shared memory %s via semaphore %s (created by display program)...", SHM_NAME, SEM_NAME);

    // --- 主循环：监听共享内存变化 (逻辑与 display/main.c 中的 display_update_thread 类似) ---
    char last_message[BUFFER_SIZE] = {0};
    while (running) { // 使用 running 标志控制循环
        // 等待信号量
        log_debug("Waiting on semaphore...");
        if (sem_wait(semaphore) == -1) {
            if (errno == EINTR) {
                log_info("sem_wait interrupted by signal, checking running flag...");
                continue; // 处理中断信号，检查 running 标志
            }
            log_error("sem_wait failed: %s", strerror(errno));
            break; // Exit loop on other errors
        }
        log_debug("Semaphore acquired.");

        // 检查共享内存中的信号
        char current_message[BUFFER_SIZE] = {0}; // 初始化为0
        // 使用 memcpy 确保不会因为源内存未完全初始化而读取垃圾
        memcpy(current_message, (char *)shared_memory, BUFFER_SIZE - 1);
        current_message[BUFFER_SIZE - 1] = '\0'; // 确保字符串结束
        log_info("Received signal from shared memory: '%s'", current_message); // 打印原始信号

        // --- 核心逻辑：解析信号并触发拍照 ---
        // 移除重复检查，每次都处理
        if (strlen(current_message) > 0) {
            strncpy(last_message, current_message, BUFFER_SIZE - 1);
            log_info("Raw current_message (hex): ");
            for (int i = 0; i < 20 && current_message[i]; i++) {
                printf("%02X ", (unsigned char)current_message[i]);
            }
            printf("\n");
            // 解析信号 (根据需求触发拍照)
            // 与 display/main.c 不同，这里只关心 "BLE:" 开头的信号
            if (strncmp(current_message, "BLE:4C 41 55 4E 43 48 0A", 4) == 0) {
                log_info("Detected BLE trigger signal. Starting capture process.");
                process_capture();
                // 重置信号量，确保下次能收到信号
            } else if (strlen(current_message) > 0) { // 检查是否是空字符串
                 log_info("Unknown signal received: '%s'. Ignoring.", current_message);
            } else {
                 log_debug("Received empty signal (likely initialization or spurious wake-up).");
            }
        }
        // --- 信号处理完成 ---

        // 注意：不像 display/main.c 那样清空 shared_memory，
        // 因为其他进程（如 btgatt-server）可能需要读取它。
        // 如果需要清空，取消下面一行的注释。
        // memset(shared_memory, 0, BUFFER_SIZE);
    }
    // --- 主循环结束 ---

    // 正常退出时也会调用 cleanup
    log_info("Main loop exited. Calling cleanup...");
    cleanup(0);
    return 0;
}