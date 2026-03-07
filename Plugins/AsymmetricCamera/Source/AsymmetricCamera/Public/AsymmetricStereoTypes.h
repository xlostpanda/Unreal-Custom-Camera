// 立体渲染相关枚举类型定义

#pragma once

#include "CoreMinimal.h"
#include "AsymmetricStereoTypes.generated.h"

/**
 * 立体渲染布局模式
 * 控制左右眼图像的排列方式
 */
UENUM(BlueprintType)
enum class EAsymmetricStereoLayout : uint8
{
	None        UMETA(DisplayName = "Mono"),           // 单目，不做立体渲染
	SideBySide  UMETA(DisplayName = "Side by Side"),   // 左右并排（SBS），左眼在左、右眼在右
	TopBottom   UMETA(DisplayName = "Top / Bottom")    // 上下排列（TB），左眼在上、右眼在下
};

/**
 * FFmpeg 合成输出模式
 * 控制 MRQ 渲染完成后如何处理左右眼图片序列
 */
UENUM(BlueprintType)
enum class EAsymmetricCompositeMode : uint8
{
	Disabled        UMETA(DisplayName = "Disabled"),        // 不合成，保留左右眼分离序列
	ImageSequence   UMETA(DisplayName = "Image Sequence"),  // 每帧输出一张合并图片（默认）
	Video           UMETA(DisplayName = "Video")            // 输出合并视频文件
};

/**
 * FFmpeg 视频编码器
 * 仅在 CompositeMode=Video 时生效
 */
UENUM(BlueprintType)
enum class EFFmpegVideoCodec : uint8
{
	H264        UMETA(DisplayName = "H.264 (libx264)"),  // 兼容性最佳，支持 MP4/MOV/MKV/AVI
	H265        UMETA(DisplayName = "H.265 (libx265)")   // 压缩率更高，强制使用 MKV 容器
};

/**
 * FFmpeg 输出容器格式
 * 仅在 CompositeMode=Video 时生效；H.265 强制使用 MKV
 */
UENUM(BlueprintType)
enum class EFFmpegOutputFormat : uint8
{
	MP4         UMETA(DisplayName = "MP4"),  // 最广泛兼容，推荐搭配 H.264
	MOV         UMETA(DisplayName = "MOV"),  // Apple QuickTime 格式
	MKV         UMETA(DisplayName = "MKV"),  // 开放容器，H.265 默认使用此格式
	AVI         UMETA(DisplayName = "AVI")   // 旧式格式，兼容性较差
};
