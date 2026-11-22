#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

THIRD_PARTY_INCLUDES_START
#include "miniaudio.h"
THIRD_PARTY_INCLUDES_END

#include "BluetoothAudioComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class URZINHO_API UBluetoothAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBluetoothAudioComponent();

	// Frequência de corte quando a boca está tapada (quanto menor, mais abafado. Ex: 350 a 600 Hz)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Bluetooth|Muffle")
	float MuffledCutoffFrequency = 450.0f;

	// Multiplicador de volume quando a boca está tapada (0.0 a 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Bluetooth|Muffle")
	float MuffledVolumeMultiplier = 0.8f;

	// Retrieves a list of all available audio playback devices
	UFUNCTION(BlueprintCallable, Category = "Audio|Bluetooth")
	TArray<FString> GetAudioDevices();

	// Initializes the Miniaudio engine with a specific hardware device index
	UFUNCTION(BlueprintCallable, Category = "Audio|Bluetooth")
	bool InitializeAudioEngine(int32 DeviceIndex);

	// Plays an audio file directly to the initialized Bluetooth device (com opção de Loop)
	UFUNCTION(BlueprintCallable, Category = "Audio|Bluetooth")
	void PlayAudioFile(FString FileName, bool bLoop = false);

	// Pausa o áudio atual mantendo o ponto onde parou
	UFUNCTION(BlueprintCallable, Category = "Audio|Bluetooth")
	void PauseAudio();

	// Retoma o áudio pausado
	UFUNCTION(BlueprintCallable, Category = "Audio|Bluetooth")
	void ResumeAudio();

	// Interrompe imediatamente e descarrega o áudio que está tocando
	UFUNCTION(BlueprintCallable, Category = "Audio|Bluetooth")
	void StopAudio();

	// Ativa ou desativa o efeito de voz abafada em tempo real
	UFUNCTION(BlueprintCallable, Category = "Audio|Bluetooth")
	void SetVoiceMuffled(bool bMuffled);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Helper interno para limpar a engine, filtro e som com segurança
	void ShutdownAudioEngine();

	ma_engine AudioEngine;
	ma_sound CurrentSound;
	ma_lpf_node MuffleFilterNode;

	bool bIsEngineInitialized = false;
	bool bIsSoundInitialized = false;
	bool bIsFilterInitialized = false;
	bool bIsCurrentlyMuffled = false;
};