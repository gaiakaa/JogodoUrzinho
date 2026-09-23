#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

// Third-party includes must be wrapped to avoid Unreal build tool errors
THIRD_PARTY_INCLUDES_START
#include "miniaudio.h"
THIRD_PARTY_INCLUDES_END

#include "BluetoothAudioComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class URZINHO_API UBluetoothAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// default values 
	UBluetoothAudioComponent();

	// Retrieves a list of all available audio playback devices
	UFUNCTION(BlueprintCallable, Category = "Audio|Bluetooth")
	TArray<FString> GetAudioDevices();

	// Initializes the Miniaudio engine with a specific hardware device index
	UFUNCTION(BlueprintCallable, Category = "Audio|Bluetooth")
	bool InitializeAudioEngine(int32 DeviceIndex);

	// Plays an audio file directly to the initialized Bluetooth device
	UFUNCTION(BlueprintCallable, Category = "Audio|Bluetooth")
	void PlayAudioFile(FString FileName);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called when the game ends or the component is destroyed
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Miniaudio engine instance
	ma_engine AudioEngine;

	// Flag to track if the engine is currently running
	bool bIsEngineInitialized = false;
};
