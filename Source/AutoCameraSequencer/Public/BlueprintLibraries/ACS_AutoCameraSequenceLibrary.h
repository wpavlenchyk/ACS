#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ACS_AutoCameraSequenceLibrary.generated.h"

class UMovieScene3DTransformSection;
class UMovieSceneSection;
class ULevelSequence;

DECLARE_LOG_CATEGORY_EXTERN(LogAutoCameraSequencer, Error, All);

/* key interpolation */
UENUM(BlueprintType)
enum class ESimplifiedKeyInterpolation : uint8
{
	CubicKey		UMETA(DisplayName = "Cubic Key"),
	LinearKey		UMETA(DisplayName = "Linear Key"),
	ConstantKey		UMETA(DisplayName = "Constant Key")
};

/* Location and rotation of camera on the route for frame */
USTRUCT(BlueprintType)
struct FCameraRoutePoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotation;
};

/* Task implemented as Blueprint Function Library to have opportunity to call functions from editor utility widget
 * Task requires automation of adding camera keyframes and transforms.
 * To have an easy access to camera adding interface it was implemented as editor utility widget
 */
UCLASS()
class AUTOCAMERASEQUENCER_API UACS_AutoCameraSequenceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/* Set level sequence time track */
	UFUNCTION(BlueprintCallable, Category = "Automation Camera Sequencer")
	static void SetLevelSequenceTimeTrack(ULevelSequence* LevelSequence, int32 StartTime, int32 EndTime);

	/* Add camera with given routes to level sequence */
	UFUNCTION(BlueprintCallable, Category = "Automation Camera Sequencer")
	static FGuid AddCameraToSequence(ULevelSequence* LevelSequence, int32 StartTime, int32 EndTime, TArray<FCameraRoutePoint> CameraPoints, ESimplifiedKeyInterpolation KeyInterpolation);
	
	/* Launch level sequence render process */
	UFUNCTION(BlueprintCallable, Category = "Automation Camera Sequencer")
	static void StartLevelSequenceRender(ULevelSequence* LevelSequence);

private:
	/* Add camera cut track, if it is not presented in level sequence */
	static void AddCameraCutTrackToLevelSequence(ULevelSequence* LevelSequence);

	/* Add transform on selected KeyInterpolation to specific frame */
	static void AddTransformKeyframe(UMovieScene3DTransformSection* TransformSection, int FrameNumber, FCameraRoutePoint CameraRoutePoint, ESimplifiedKeyInterpolation KeyInterpolation);
	/* Add value to specific transform channel on selected KeyInterpolation to specific frame */
	static void AddValueToSpecificChannel(UMovieScene3DTransformSection* TransformSection, int ChannelIndex, int FrameNumber, double Value, ESimplifiedKeyInterpolation KeyInterpolation);

	/* Remove transform from selected KeyInterpolation to specific frame */
	static void RemoveTransformKeyframe(UMovieScene3DTransformSection* TransformSection, int FrameNumber);
	/* Remove value from specific transform channel on selected KeyInterpolation to specific frame */
	static void RemoveValueToSpecificChannel(UMovieScene3DTransformSection* TransformSection, int ChannelIndex, int FrameNumber);
};
