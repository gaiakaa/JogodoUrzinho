// Fill out your copyright notice in the Description page of Project Settings.

THIRD_PARTY_INCLUDES_START
#define MINIAUDIO_IMPLEMENTATION
THIRD_PARTY_INCLUDES_END

#include "BluetoothAudioComponent.h"
#include "Misc/Paths.h"

UBluetoothAudioComponent::UBluetoothAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBluetoothAudioComponent::BeginPlay()
{
	Super::BeginPlay();
}

TArray<FString> UBluetoothAudioComponent::GetAudioDevices()
{
	TArray<FString> DeviceList;
	ma_context Context;

	if (ma_context_init(NULL, 0, NULL, &Context) == MA_SUCCESS)
	{
		ma_device_info* pPlaybackInfos;
		ma_uint32 PlaybackCount;

		if (ma_context_get_devices(&Context, &pPlaybackInfos, &PlaybackCount, NULL, NULL) == MA_SUCCESS)
		{
			for (ma_uint32 i = 0; i < PlaybackCount; ++i)
			{
				DeviceList.Add(FString(UTF8_TO_TCHAR(pPlaybackInfos[i].name)));
			}
		}
		ma_context_uninit(&Context);
	}

	return DeviceList;
}

bool UBluetoothAudioComponent::InitializeAudioEngine(int32 DeviceIndex)
{
	// Protection: Uninitialize if already running before starting a new one
	if (bIsEngineInitialized)
	{
		ma_engine_uninit(&AudioEngine);
		bIsEngineInitialized = false;
	}

	ma_context Context;
	if (ma_context_init(NULL, 0, NULL, &Context) != MA_SUCCESS) return false;

	ma_device_info* pPlaybackInfos;
	ma_uint32 PlaybackCount;

	if (ma_context_get_devices(&Context, &pPlaybackInfos, &PlaybackCount, NULL, NULL) != MA_SUCCESS) return false;

	// Prevent out-of-bounds array access
	if (DeviceIndex < 0 || (ma_uint32)DeviceIndex >= PlaybackCount) return false;

	// Configure the engine to use the specific hardware ID
	ma_engine_config EngineConfig = ma_engine_config_init();
	EngineConfig.pPlaybackDeviceID = &pPlaybackInfos[DeviceIndex].id;

	if (ma_engine_init(&EngineConfig, &AudioEngine) == MA_SUCCESS)
	{
		bIsEngineInitialized = true;
	}

	ma_context_uninit(&Context);
	return bIsEngineInitialized;
}

void UBluetoothAudioComponent::PlayAudioFile(FString FileName)
{
	if (bIsEngineInitialized)
	{
		FString RelativePath = FPaths::ProjectContentDir() + TEXT("BearAudio/") + FileName;
		FString FullPath = FPaths::ConvertRelativePathToFull(RelativePath);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("A procurar audio em: %s"), *FullPath));
		}

		ma_engine_play_sound(&AudioEngine, TCHAR_TO_UTF8(*FullPath), NULL);
	}
}

void UBluetoothAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Safely destroy the audio engine to prevent memory leaks and crashes
	if (bIsEngineInitialized)
	{
		ma_engine_uninit(&AudioEngine);
		bIsEngineInitialized = false;
	}

	Super::EndPlay(EndPlayReason);
}

void UBluetoothAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}