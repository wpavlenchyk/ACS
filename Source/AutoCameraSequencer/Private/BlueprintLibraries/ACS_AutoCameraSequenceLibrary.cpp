
#include "BlueprintLibraries/ACS_AutoCameraSequenceLibrary.h"

#include "AutomatedLevelSequenceCapture.h"
#include "LevelSequenceActor.h"
#include "SequencerTools.h"
#include "CineCameraActor.h"
#include "MovieSceneToolHelpers.h"
#include "Protocols/AudioCaptureProtocol.h"
#include "Protocols/VideoCaptureProtocol.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieSceneCameraCutTrack.h"

DEFINE_LOG_CATEGORY(LogAutoCameraSequencer);

void UACS_AutoCameraSequenceLibrary::SetLevelSequenceTimeTrack(ULevelSequence* LevelSequence, int32 StartTime, int32 EndTime)
{
	if (LevelSequence == nullptr)
	{
		UE_LOG(LogAutoCameraSequencer, Error, TEXT("LevelSequence is nullptr!"));
		return;
	}

	int32 FrameTickValue = LevelSequence->MovieScene->GetTickResolution().AsDecimal() / LevelSequence->MovieScene->GetDisplayRate().AsDecimal();
	
	LevelSequence->MovieScene->SetPlaybackRange(FFrameNumber(StartTime * FrameTickValue), (EndTime - StartTime) * FrameTickValue);
}

FGuid UACS_AutoCameraSequenceLibrary::AddCameraToSequence(ULevelSequence* LevelSequence, int32 StartTime, int32 EndTime, TArray<FCameraRoutePoint> CameraPoints, ESimplifiedKeyInterpolation KeyInterpolation)
{
	if (LevelSequence == nullptr)
	{
		UE_LOG(LogAutoCameraSequencer, Error, TEXT("LevelSequence is nullptr!"));
		return FGuid();
	}

	AddCameraCutTrackToLevelSequence(LevelSequence);
	
	UMovieScene* MovieScene = LevelSequence->MovieScene;
	UMovieSceneSequence* Sequence = Cast<UMovieSceneSequence>(LevelSequence);
	
	ACineCameraActor* OutActor = NewObject<ACineCameraActor>();
	FGuid Guid = Sequence->CreateSpawnable(OutActor);
	
	int32 FrameTickValue = MovieScene->GetTickResolution().AsDecimal() / MovieScene->GetDisplayRate().AsDecimal();
	int32 StartFrame = FrameTickValue * StartTime;
	int32 EndFrame = FrameTickValue * EndTime;
	
	UMovieScene3DTransformTrack* CameraTransformTrack = MovieScene->AddTrack<UMovieScene3DTransformTrack>(Guid);
	if (CameraTransformTrack == nullptr)
	{
		UE_LOG(LogAutoCameraSequencer, Error, TEXT("Failed to Create camera transform track!"));
		return FGuid();
	}
	
	UMovieScene3DTransformSection* CameraTransformSection = Cast<UMovieScene3DTransformSection>(CameraTransformTrack->CreateNewSection());
	if (CameraTransformSection == nullptr)
	{
		UE_LOG(LogAutoCameraSequencer, Error, TEXT("Failed to Create camera transform section!"));
		return FGuid();
	}
	
	CameraTransformSection->SetUseQuaternionInterpolation(true);
	CameraTransformSection->SetRange(TRange<FFrameNumber>(FFrameNumber(StartFrame), FFrameNumber(EndFrame)));
	CameraTransformSection->SetBlendType(EMovieSceneBlendType::Additive);

	int32 RowIndex = -1;

	for (UMovieSceneSection* ExistingSection : CameraTransformTrack->GetAllSections())
	{
		RowIndex = FMath::Max(RowIndex, ExistingSection->GetRowIndex());	
	}
	CameraTransformSection->SetRowIndex(RowIndex + 1);
	CameraTransformTrack->AddSection(*CameraTransformSection);
	
	if (UMovieSceneCameraCutTrack* MovieSceneCameraCutTrack = Cast<UMovieSceneCameraCutTrack>(MovieScene->GetCameraCutTrack()))
	{
		UMovieSceneCameraCutSection* CameraCutSection =  MovieSceneCameraCutTrack->AddNewCameraCut(UE::MovieScene::FRelativeObjectBindingID(Guid), FFrameNumber(StartFrame));
		
		if (CameraCutSection != nullptr)
		{
			CameraCutSection->SetEndFrame(FFrameNumber(EndFrame));
		}
	}

	if (!CameraPoints.IsEmpty())
	{
		if (CameraPoints.Num() > 1)
		{
			int32 Interval = EndFrame - StartFrame;
			int32 FrameDelayBetweenFrames = Interval / (CameraPoints.Num() - 1);

			for (int Index = 0; Index < CameraPoints.Num(); ++Index)
			{
				AddTransformKeyframe(CameraTransformSection, StartFrame + Index * FrameDelayBetweenFrames, CameraPoints[Index], KeyInterpolation);
			}
		}
		else
		{
			AddTransformKeyframe(CameraTransformSection, StartFrame, CameraPoints[0], KeyInterpolation);
		}
	}
	
	return Guid;
}

void UACS_AutoCameraSequenceLibrary::StartLevelSequenceRender(ULevelSequence* LevelSequence)
{
	if (LevelSequence == nullptr)
	{
		UE_LOG(LogAutoCameraSequencer, Error, TEXT("LevelSequence is nullptr!"));
		return;
	}

	UAutomatedLevelSequenceCapture* Capture = NewObject<UAutomatedLevelSequenceCapture>();

	Capture->LevelSequenceAsset = LevelSequence;

	FDirectoryPath OutputDirectory;
	OutputDirectory.Path = FPaths::ProjectSavedDir();

	Capture->Settings.OutputDirectory = OutputDirectory;

	Capture->SetImageCaptureProtocolType(UVideoCaptureProtocol::StaticClass());

	Cast<UVideoCaptureProtocol>(Capture->ImageCaptureProtocol)->bUseCompression = true;
	Cast<UVideoCaptureProtocol>(Capture->ImageCaptureProtocol)->CompressionQuality = 75;

	Capture->SetAudioCaptureProtocolType(UMasterAudioSubmixCaptureProtocol::StaticClass());

	USequencerToolsFunctionLibrary::RenderMovie(Capture, FOnRenderMovieStopped());
 }

void UACS_AutoCameraSequenceLibrary::AddCameraCutTrackToLevelSequence(ULevelSequence* LevelSequence)
{
	if (LevelSequence == nullptr)
	{
		UE_LOG(LogAutoCameraSequencer, Error, TEXT("LevelSequence is nullptr!"));
		return;
	}

	if (LevelSequence->MovieScene->GetCameraCutTrack() != nullptr)
	{
		return;
	}
	
	LevelSequence->MovieScene->AddCameraCutTrack(UMovieSceneCameraCutTrack::StaticClass());
}

void UACS_AutoCameraSequenceLibrary::AddTransformKeyframe(UMovieScene3DTransformSection* TransformSection, int FrameNumber, FCameraRoutePoint CameraRoutePoint, ESimplifiedKeyInterpolation KeyInterpolation)
{
	RemoveTransformKeyframe(TransformSection, FrameNumber);

	AddValueToSpecificChannel(TransformSection, 0, FrameNumber, CameraRoutePoint.Location.X, KeyInterpolation);
	AddValueToSpecificChannel(TransformSection, 1, FrameNumber, CameraRoutePoint.Location.Y, KeyInterpolation);
	AddValueToSpecificChannel(TransformSection, 2, FrameNumber, CameraRoutePoint.Location.Z, KeyInterpolation);

	AddValueToSpecificChannel(TransformSection, 3, FrameNumber, CameraRoutePoint.Rotation.Roll, KeyInterpolation);
	AddValueToSpecificChannel(TransformSection, 4, FrameNumber, CameraRoutePoint.Rotation.Pitch, KeyInterpolation);
	AddValueToSpecificChannel(TransformSection, 5, FrameNumber, CameraRoutePoint.Rotation.Yaw, KeyInterpolation);
}

void UACS_AutoCameraSequenceLibrary::AddValueToSpecificChannel(UMovieScene3DTransformSection* TransformSection, int ChannelIndex, int FrameNumber, double Value, ESimplifiedKeyInterpolation KeyInterpolation)
{
	FMovieSceneDoubleChannel* Channel = TransformSection->GetChannelProxy().GetChannel<FMovieSceneDoubleChannel>(ChannelIndex);
	if (Channel == nullptr)
	{
		UE_LOG(LogAutoCameraSequencer, Error, TEXT("LevelSequence is nullptr!"));
		return;
	}

	switch (KeyInterpolation)
	{
	case ESimplifiedKeyInterpolation::CubicKey:
		Channel->AddCubicKey(FrameNumber, Value);
		break;
	case ESimplifiedKeyInterpolation::ConstantKey:
		Channel->AddConstantKey(FrameNumber, Value);
		break;
	case ESimplifiedKeyInterpolation::LinearKey:
	default:
		Channel->AddLinearKey(FrameNumber, Value);
		break;	
	}

	TransformSection->Modify();
}

void UACS_AutoCameraSequenceLibrary::RemoveTransformKeyframe(UMovieScene3DTransformSection* TransformSection, int FrameNumber)
{
	RemoveValueToSpecificChannel(TransformSection, 0, FrameNumber);
	RemoveValueToSpecificChannel(TransformSection, 1, FrameNumber);
	RemoveValueToSpecificChannel(TransformSection, 2, FrameNumber);

	RemoveValueToSpecificChannel(TransformSection, 3, FrameNumber);
	RemoveValueToSpecificChannel(TransformSection, 4, FrameNumber);
	RemoveValueToSpecificChannel(TransformSection, 5, FrameNumber);
}

void UACS_AutoCameraSequenceLibrary::RemoveValueToSpecificChannel(UMovieScene3DTransformSection* TransformSection, int ChannelIndex, int FrameNumber)
{
	FMovieSceneDoubleChannel* Channel = TransformSection->GetChannelProxy().GetChannel<FMovieSceneDoubleChannel>(ChannelIndex);
	if (Channel == nullptr)
	{
		UE_LOG(LogAutoCameraSequencer, Error, TEXT("LevelSequence is nullptr!"));
		return;
	}

	TArray<FFrameNumber> UnusedKeyTimes;
	TArray<FKeyHandle> KeyHandles;
	Channel->GetKeys(TRange<FFrameNumber>(FrameNumber,FrameNumber), &UnusedKeyTimes, &KeyHandles);

	Channel->DeleteKeys(KeyHandles);

	TransformSection->Modify();
}
