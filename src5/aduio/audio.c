#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rk_defines.h"
#include "rk_mpi_ai.h"
#include "rk_mpi_ao.h"
#include "rk_mpi_sys.h"

// 音频设备上下文结构体
typedef struct _MyRecorderCtx {
    RK_S32      s32DeviceSampleRate;  // 输入采样率
    RK_S32      s32DeviceChannel;     // 输入声道数
    RK_S32      s32BitWidth;          // 输入位宽
    RK_S32      s32DevId;             // 设备ID
    RK_S32      s32ChnIndex;          // 通道索引
    RK_S32      s32PlaybackSampleRate;// 输出采样率
    RK_S32      s32PlaybackChannels;  // 输出声道数
    RK_S32      s32PlaybackBitWidth;  // 输出位宽
} MY_RECORDER_CTX_S;

// 全局状态标记
static RK_BOOL gAoInitialized = RK_FALSE;  // 音频输出初始化标记

// 工具函数：转换声道数到枚举类型
static AUDIO_SOUND_MODE_E find_sound_mode(RK_S32 ch) {
    switch (ch) {
        case 1: return AUDIO_SOUND_MODE_MONO;
        case 2: return AUDIO_SOUND_MODE_STEREO;
        default: return AUDIO_SOUND_MODE_MONO;
    }
}

// 工具函数：转换位宽到枚举类型
static AUDIO_BIT_WIDTH_E find_bit_width(RK_S32 bit) {
    switch (bit) {
        case 16: return AUDIO_BIT_WIDTH_16;
        case 24: return AUDIO_BIT_WIDTH_24;
        case 32: return AUDIO_BIT_WIDTH_32;
        default: return AUDIO_BIT_WIDTH_16;
    }
}

/**
 * 音频输入初始化
 * @param ctx 音频上下文
 * @return 成功返回RK_SUCCESS，失败返回RK_FAILURE
 */
static RK_S32 audio_input_init(MY_RECORDER_CTX_S *ctx) {
    RK_S32 ret;
    AI_CHN_ATTR_S ai_attr;

    if (!ctx) {
        printf("ERROR: Invalid audio input context\n");
        return RK_FAILURE;
    }

    // 初始化MPI系统
    ret = RK_MPI_SYS_Init();
    if (ret != RK_SUCCESS) {
        printf("ERROR: RK_MPI_SYS_Init failed\n");
        return RK_FAILURE;
    }

    // 配置音频输入属性
    memset(&ai_attr, 0, sizeof(AI_CHN_ATTR_S));
    ai_attr.enSampleRate = (AUDIO_SAMPLE_RATE_E)ctx->s32DeviceSampleRate;
    ai_attr.enBitWidth = find_bit_width(ctx->s32BitWidth);
    ai_attr.enSoundMode = find_sound_mode(ctx->s32DeviceChannel);
    ai_attr.u32BufCnt = 3;    // 缓冲区数量
    ai_attr.u32FrameCnt = 10; // 帧数量

    // 创建音频输入通道
    ret = RK_MPI_AI_CreateChn(ctx->s32DevId, ctx->s32ChnIndex, &ai_attr);
    if (ret != RK_SUCCESS) {
        printf("ERROR: RK_MPI_AI_CreateChn failed\n");
        RK_MPI_SYS_Exit();
        return RK_FAILURE;
    }

    // 启动音频输入通道
    ret = RK_MPI_AI_StartChn(ctx->s32DevId, ctx->s32ChnIndex);
    if (ret != RK_SUCCESS) {
        printf("ERROR: RK_MPI_AI_StartChn failed\n");
        RK_MPI_AI_DestroyChn(ctx->s32DevId, ctx->s32ChnIndex);
        RK_MPI_SYS_Exit();
        return RK_FAILURE;
    }

    printf("INFO: Audio input initialized - Rate:%d, Channels:%d, BitWidth:%d\n",
           ctx->s32DeviceSampleRate, ctx->s32DeviceChannel, ctx->s32BitWidth);
    return RK_SUCCESS;
}

/**
 * 读取音频输入数据
 * @param ctx 音频上下文
 * @param data 数据缓冲区
 * @param max_len 缓冲区最大长度
 * @param actual_len 实际读取长度
 * @return 成功返回RK_SUCCESS，失败返回RK_FAILURE
 */
static RK_S32 audio_input_read(MY_RECORDER_CTX_S *ctx, void *data, size_t max_len, size_t *actual_len) {
    RK_S32 ret;
    AUDIO_FRAME_S frame;
    struct timeval timeout = {1, 0};  // 1秒超时

    if (!ctx || !data || !actual_len || max_len == 0) {
        printf("ERROR: Invalid parameters for audio read\n");
        return RK_FAILURE;
    }

    // 获取音频帧
    ret = RK_MPI_AI_GetFrame(ctx->s32DevId, ctx->s32ChnIndex, &frame, &timeout);
    if (ret != RK_SUCCESS) {
        printf("WARNING: RK_MPI_AI_GetFrame failed\n");
        return RK_FAILURE;
    }

    // 检查缓冲区大小
    *actual_len = frame.u32Len;
    if (*actual_len > max_len) {
        printf("ERROR: Audio frame too large (%zu > %zu)\n", *actual_len, max_len);
        RK_MPI_AI_ReleaseFrame(ctx->s32DevId, ctx->s32ChnIndex, &frame);
        return RK_FAILURE;
    }

    // 复制数据
    memcpy(data, frame.pVirAddr, *actual_len);

    // 释放帧
    ret = RK_MPI_AI_ReleaseFrame(ctx->s32DevId, ctx->s32ChnIndex, &frame);
    if (ret != RK_SUCCESS) {
        printf("WARNING: RK_MPI_AI_ReleaseFrame failed\n");
    }

    return RK_SUCCESS;
}

/**
 * 销毁音频输入
 * @param ctx 音频上下文
 * @return 成功返回RK_SUCCESS，失败返回RK_FAILURE
 */
static RK_S32 audio_input_deinit(MY_RECORDER_CTX_S *ctx) {
    RK_S32 ret = RK_SUCCESS;

    if (!ctx) return RK_FAILURE;

    // 停止并销毁输入通道
    ret |= RK_MPI_AI_StopChn(ctx->s32DevId, ctx->s32ChnIndex);
    ret |= RK_MPI_AI_DestroyChn(ctx->s32DevId, ctx->s32ChnIndex);
    ret |= RK_MPI_SYS_Exit();

    printf("INFO: Audio input deinitialized\n");
    return ret;
}

/**
 * 初始化音频输出
 * @param ctx 音频上下文
 * @return 成功返回RK_SUCCESS，失败返回RK_FAILURE
 */
static RK_S32 audio_output_init(MY_RECORDER_CTX_S *ctx) {
    RK_S32 ret;
    AO_CHN_ATTR_S ao_attr;

    if (!ctx || gAoInitialized) return RK_SUCCESS;

    // 初始化MPI系统
    ret = RK_MPI_SYS_Init();
    if (ret != RK_SUCCESS) {
        printf("ERROR: RK_MPI_SYS_Init failed for output\n");
        return RK_FAILURE;
    }

    // 配置音频输出属性
    memset(&ao_attr, 0, sizeof(AO_CHN_ATTR_S));
    ao_attr.enSampleRate = (AUDIO_SAMPLE_RATE_E)ctx->s32PlaybackSampleRate;
    ao_attr.enBitWidth = find_bit_width(ctx->s32PlaybackBitWidth);
    ao_attr.enSoundMode = find_sound_mode(ctx->s32PlaybackChannels);
    ao_attr.u32BufCnt = 3;
    ao_attr.u32FrameCnt = 10;

    // 创建音频输出通道
    ret = RK_MPI_AO_CreateChn(ctx->s32DevId, ctx->s32ChnIndex, &ao_attr);
    if (ret != RK_SUCCESS) {
        printf("ERROR: RK_MPI_AO_CreateChn failed\n");
        RK_MPI_SYS_Exit();
        return RK_FAILURE;
    }

    // 启动音频输出通道
    ret = RK_MPI_AO_StartChn(ctx->s32DevId, ctx->s32ChnIndex);
    if (ret != RK_SUCCESS) {
        printf("ERROR: RK_MPI_AO_StartChn failed\n");
        RK_MPI_AO_DestroyChn(ctx->s32DevId, ctx->s32ChnIndex);
        RK_MPI_SYS_Exit();
        return RK_FAILURE;
    }

    gAoInitialized = RK_TRUE;
    printf("INFO: Audio output initialized - Rate:%d, Channels:%d, BitWidth:%d\n",
           ctx->s32PlaybackSampleRate, ctx->s32PlaybackChannels, ctx->s32PlaybackBitWidth);
    return RK_SUCCESS;
}

/**
 * 播放音频数据
 * @param ctx 音频上下文
 * @param data 音频数据
 * @param data_len 数据长度
 * @return 成功返回RK_SUCCESS，失败返回RK_FAILURE
 */
static RK_S32 audio_output_play(MY_RECORDER_CTX_S *ctx, const void *data, size_t data_len) {
    RK_S32 ret;
    AUDIO_FRAME_S frame;

    if (!ctx || !data || data_len == 0) {
        printf("ERROR: Invalid parameters for audio play\n");
        return RK_FAILURE;
    }

    // 确保输出已初始化
    if (!gAoInitialized) {
        if (audio_output_init(ctx) != RK_SUCCESS) {
            return RK_FAILURE;
        }
    }

    // 配置音频帧
    memset(&frame, 0, sizeof(AUDIO_FRAME_S));
    frame.enType = AUDIO_FRAME_TYPE_RAW;
    frame.pVirAddr = (void *)data;
    frame.u32Len = data_len;
    frame.enBitWidth = find_bit_width(ctx->s32PlaybackBitWidth);
    frame.enSoundMode = find_sound_mode(ctx->s32PlaybackChannels);
    frame.enSampleRate = (AUDIO_SAMPLE_RATE_E)ctx->s32PlaybackSampleRate;

    // 发送音频帧到输出设备
    ret = RK_MPI_AO_SendFrame(ctx->s32DevId, ctx->s32ChnIndex, &frame, -1);
    if (ret != RK_SUCCESS) {
        printf("ERROR: RK_MPI_AO_SendFrame failed\n");
        return RK_FAILURE;
    }

    return RK_SUCCESS;
}

/**
 * 销毁音频输出
 * @param ctx 音频上下文
 * @return 成功返回RK_SUCCESS，失败返回RK_FAILURE
 */
static RK_S32 audio_output_deinit(MY_RECORDER_CTX_S *ctx) {
    RK_S32 ret = RK_SUCCESS;

    if (!ctx || !gAoInitialized) return RK_SUCCESS;

    // 停止并销毁输出通道
    ret |= RK_MPI_AO_StopChn(ctx->s32DevId, ctx->s32ChnIndex);
    ret |= RK_MPI_AO_DestroyChn(ctx->s32DevId, ctx->s32ChnIndex);
    ret |= RK_MPI_SYS_Exit();

    gAoInitialized = RK_FALSE;
    printf("INFO: Audio output deinitialized\n");
    return ret;
}

// 测试函数：录制5秒音频后立即播放
static void test_audio_io() {
    MY_RECORDER_CTX_S ctx = {
        .s32DeviceSampleRate = 16000,    // 输入采样率16kHz
        .s32DeviceChannel = 1,           // 单声道输入
        .s32BitWidth = 16,               // 16位宽
        .s32DevId = 0,                   // 设备ID 0
        .s32ChnIndex = 0,                // 通道索引0
        .s32PlaybackSampleRate = 16000,  // 输出采样率16kHz
        .s32PlaybackChannels = 1,        // 单声道输出
        .s32PlaybackBitWidth = 16        // 16位宽输出
    };

    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    size_t read_len;
    int record_seconds = 5;
    int frames_per_second = ctx.s32DeviceSampleRate * ctx.s32DeviceChannel * (ctx.s32BitWidth / 8);
    int total_frames = frames_per_second * record_seconds;
    int recorded_frames = 0;

    // 初始化音频输入
    if (audio_input_init(&ctx) != RK_SUCCESS) {
        printf("测试失败：音频输入初始化失败\n");
        return;
    }

    printf("开始录制%d秒音频...\n", record_seconds);
    
    // 录制音频（简单循环，实际应用需考虑线程）
    while (recorded_frames < total_frames && !gRecorderExit) {
        if (audio_input_read(&ctx, buffer, BUFFER_SIZE, &read_len) == RK_SUCCESS && read_len > 0) {
            recorded_frames += read_len;
            
            // 实时播放（回声测试）
            // audio_output_play(&ctx, buffer, read_len);
        }
    }

    printf("录制完成，共录制%d字节\n", recorded_frames);
    
    // 销毁输入设备
    audio_input_deinit(&ctx);

    // 初始化输出设备并播放
    printf("开始播放录制的音频...\n");
    if (audio_output_init(&ctx) != RK_SUCCESS) {
        printf("测试失败：音频输出初始化失败\n");
        return;
    }

    // 此处应添加实际存储的音频数据播放逻辑
    // 简化测试：播放一个提示音（实际应用需替换为录制的完整数据）
    audio_output_play(&ctx, buffer, BUFFER_SIZE);

    // 等待播放完成
    sleep(record_seconds);

    // 销毁输出设备
    audio_output_deinit(&ctx);
    printf("音频测试完成\n");
}

// 可通过调用test_audio_io()函数进行测试