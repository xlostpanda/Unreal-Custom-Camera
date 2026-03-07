// 场景视图扩展，用来覆盖玩家相机的投影矩阵

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

class UAsymmetricCameraComponent;

/**
 * 场景视图扩展，把玩家相机的投影矩阵替换成离轴非对称投影。
 * 高性能路径：直接改主相机的投影，不需要 Render Target。
 */
class FAsymmetricViewExtension : public FWorldSceneViewExtension
{
public:
	FAsymmetricViewExtension(const FAutoRegister& AutoRegister, UWorld* InWorld, UAsymmetricCameraComponent* InComponent);

	// ISceneViewExtension 接口
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData) override;

private:
	TWeakObjectPtr<UAsymmetricCameraComponent> CameraComponent;

	// 每眼前帧数据，用于立体运动模糊。
	// 索引 0=左眼（或单目），索引 1=右眼。
	// 固定 2 元素数组，不做堆分配，覆盖所有使用场景。
	struct FPerEyePreviousData
	{
		bool bHasData = false;           // 是否有前帧数据（第一帧时为 false）
		FVector EyePosition = FVector::ZeroVector;   // 前帧眼睛世界坐标
		FRotator ViewRotation = FRotator::ZeroRotator; // 前帧视图旋转
	};
	FPerEyePreviousData PrevDataPerEye[2];
};
