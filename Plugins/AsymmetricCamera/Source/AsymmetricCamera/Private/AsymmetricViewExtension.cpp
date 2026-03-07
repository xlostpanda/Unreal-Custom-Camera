// 场景视图扩展实现，在这里覆盖投影矩阵

#include "AsymmetricViewExtension.h"
#include "AsymmetricCameraComponent.h"
#include "AsymmetricScreenComponent.h"
#include "Math/InverseRotationMatrix.h"
#include "SceneView.h"

DEFINE_LOG_CATEGORY_STATIC(LogAsymmetricCamera, Log, All);

// 调试日志计数器：只在前几帧打印，避免每帧刷屏
static int32 GAsymmetricDebugLogFrames = 0;

FAsymmetricViewExtension::FAsymmetricViewExtension(
	const FAutoRegister& AutoRegister,
	UWorld* InWorld,
	UAsymmetricCameraComponent* InComponent)
	: FWorldSceneViewExtension(AutoRegister, InWorld)
	, CameraComponent(InComponent)
{
}

void FAsymmetricViewExtension::SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData)
{
	// 运行时路径：MRQ 不走这里，走 SetupView
	if (!CameraComponent.IsValid() || !CameraComponent->bUseAsymmetricProjection)
	{
		return;
	}

	FVector EyePosition = CameraComponent->GetEyePosition();

	FRotator ViewRotation;
	FMatrix ProjectionMatrix;

	if (!CameraComponent->CalculateOffAxisProjection(EyePosition, ViewRotation, ProjectionMatrix))
	{
		return;
	}

	// 用屏幕旋转构建 ViewRotationMatrix，和 LocalPlayer.cpp:1244 一样：
	//   FInverseRotationMatrix(ViewRotation) * SwizzleMatrix
	// SwizzleMatrix 将 UE 坐标系（X=前，Y=右，Z=上）转换为渲染坐标系（X=右，Y=上，Z=前）
	static const FMatrix SwizzleMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1)
	);
	const FMatrix ViewRotationMatrix = FInverseRotationMatrix(ViewRotation) * SwizzleMatrix;

	// 调试日志（只打前 3 帧，排查投影矩阵是否正确）
	if (GAsymmetricDebugLogFrames < 3)
	{
		GAsymmetricDebugLogFrames++;
		UE_LOG(LogAsymmetricCamera, Warning, TEXT("=== AsymmetricCamera Debug Frame %d ==="), GAsymmetricDebugLogFrames);
		UE_LOG(LogAsymmetricCamera, Warning, TEXT("  EyePosition: %s"), *EyePosition.ToString());
		UE_LOG(LogAsymmetricCamera, Warning, TEXT("  ViewRotation: %s"), *ViewRotation.ToString());
		UE_LOG(LogAsymmetricCamera, Warning, TEXT("  Original ViewOrigin: %s"), *InOutProjectionData.ViewOrigin.ToString());
		UE_LOG(LogAsymmetricCamera, Warning, TEXT("  ProjMatrix[0]: %.4f %.4f %.4f %.4f"), ProjectionMatrix.M[0][0], ProjectionMatrix.M[0][1], ProjectionMatrix.M[0][2], ProjectionMatrix.M[0][3]);
		UE_LOG(LogAsymmetricCamera, Warning, TEXT("  ProjMatrix[1]: %.4f %.4f %.4f %.4f"), ProjectionMatrix.M[1][0], ProjectionMatrix.M[1][1], ProjectionMatrix.M[1][2], ProjectionMatrix.M[1][3]);
		UE_LOG(LogAsymmetricCamera, Warning, TEXT("  ProjMatrix[2]: %.4f %.4f %.4f %.4f"), ProjectionMatrix.M[2][0], ProjectionMatrix.M[2][1], ProjectionMatrix.M[2][2], ProjectionMatrix.M[2][3]);
		UE_LOG(LogAsymmetricCamera, Warning, TEXT("  ProjMatrix[3]: %.4f %.4f %.4f %.4f"), ProjectionMatrix.M[3][0], ProjectionMatrix.M[3][1], ProjectionMatrix.M[3][2], ProjectionMatrix.M[3][3]);
		UE_LOG(LogAsymmetricCamera, Warning, TEXT("  ViewRect: %d,%d - %d,%d"), InOutProjectionData.ViewRect.Min.X, InOutProjectionData.ViewRect.Min.Y, InOutProjectionData.ViewRect.Max.X, InOutProjectionData.ViewRect.Max.Y);
	}

	InOutProjectionData.ViewOrigin = EyePosition;
	InOutProjectionData.ViewRotationMatrix = ViewRotationMatrix;
	InOutProjectionData.ProjectionMatrix = ProjectionMatrix;

	// 约束视口比例匹配屏幕宽高比（防止拉伸）
	// 当视口比例和屏幕比例不一致时，加上 Pillarbox（左右黑边）或 Letterbox（上下黑边）
	if (CameraComponent->bMatchViewportAspectRatio && CameraComponent->ScreenComponent)
	{
		const FVector2D ScreenSize = CameraComponent->ScreenComponent->GetScreenSize();
		const float ScreenW = ScreenSize.X;
		const float ScreenH = ScreenSize.Y;
		if (ScreenW <= SMALL_NUMBER || ScreenH <= SMALL_NUMBER)
		{
			return;
		}

		const float ScreenAspect = ScreenW / ScreenH;

		const FIntRect FullRect = InOutProjectionData.ViewRect;
		const int32 ViewW = FullRect.Width();
		const int32 ViewH = FullRect.Height();
		if (ViewW <= 0 || ViewH <= 0)
		{
			return;
		}

		const float ViewportAspect = static_cast<float>(ViewW) / static_cast<float>(ViewH);
		if (FMath::IsNearlyEqual(ScreenAspect, ViewportAspect, 0.001f))
		{
			return;
		}

		int32 NewW, NewH;
		int32 OffX = FullRect.Min.X;
		int32 OffY = FullRect.Min.Y;

		if (ScreenAspect < ViewportAspect)
		{
			// 左右黑边（Pillarbox）：屏幕比视口窄
			NewH = ViewH;
			NewW = FMath::RoundToInt(static_cast<float>(ViewH) * ScreenAspect);
			OffX += (ViewW - NewW) / 2;
		}
		else
		{
			// 上下黑边（Letterbox）：屏幕比视口宽
			NewW = ViewW;
			NewH = FMath::RoundToInt(static_cast<float>(ViewW) / ScreenAspect);
			OffY += (ViewH - NewH) / 2;
		}

		const FIntRect ConstrainedRect(OffX, OffY, OffX + NewW, OffY + NewH);
		InOutProjectionData.SetConstrainedViewRectangle(ConstrainedRect);
	}
}

void FAsymmetricViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	// MRQ（Movie Render Queue）不调用 SetupViewProjectionMatrix，
	// 所以在这里对离线渲染应用非对称投影。
	if (!InView.bIsOfflineRender)
	{
		return;
	}

	if (!CameraComponent.IsValid()
		|| !CameraComponent->bUseAsymmetricProjection
		|| !CameraComponent->bEnableMRQSupport)
	{
		return;
	}

	// 直接使用视图已设定的 ViewLocation 作为眼睛位置，
	// 这样 MoviePipelineAsymmetricStereoPass::GetCameraInfo 已应用的左右眼偏移会被正确保留。
	// 如果改用组件中心位置，左右眼会得到相同的投影矩阵（没有视差）。
	FVector EyePosition = InView.ViewLocation;
	FRotator ViewRotation;
	FMatrix ProjectionMatrix;

	if (!CameraComponent->CalculateOffAxisProjection(EyePosition, ViewRotation, ProjectionMatrix))
	{
		return;
	}

	// 判断当前是哪只眼（0=左眼/单目，1=右眼），每眼独立维护前帧数据。
	// 不区分眼别时，第二只眼会把第一只眼当前帧的位置当成"前帧"，导致运动模糊向量错误。
	int32 EyeIdx = 0;
	if (CameraComponent->EyeSeparation > SMALL_NUMBER)
	{
		FVector ScreenBL, ScreenBR, ScreenTL, ScreenTR;
		CameraComponent->GetEffectiveScreenCorners(ScreenBL, ScreenBR, ScreenTL, ScreenTR);
		const FVector ScreenRight = (ScreenBR - ScreenBL).GetSafeNormal();
		const FVector BaseEyePos = CameraComponent->GetEyePosition();
		// 点积正数说明眼睛在屏幕右侧（右眼），负数在左侧（左眼）
		const float Side = FVector::DotProduct(EyePosition - BaseEyePos, ScreenRight);
		EyeIdx = (Side >= 0.0f) ? 1 : 0;
	}

	// 写入前帧变换数据，供运动模糊速度缓冲区计算使用。
	// 第一帧没有前帧数据，不设 PreviousViewTransform（首帧无运动模糊，是 MRQ 固有限制）。
	FPerEyePreviousData& PrevData = PrevDataPerEye[EyeIdx];
	if (PrevData.bHasData)
	{
		InView.PreviousViewTransform = FTransform(PrevData.ViewRotation.Quaternion(), PrevData.EyePosition);
	}

	// 缓存当前帧数据，供下一帧使用
	PrevData.EyePosition = EyePosition;
	PrevData.ViewRotation = ViewRotation;
	PrevData.bHasData = true;

	// 更新投影矩阵、视图位置和视图矩阵
	InView.UpdateProjectionMatrix(ProjectionMatrix);
	InView.ViewLocation = EyePosition;
	InView.ViewRotation = ViewRotation;
	InView.UpdateViewMatrix();
}
