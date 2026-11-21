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
	// Limpa tudo com segurança se já estiver rodando
	ShutdownAudioEngine();

	ma_context Context;
	if (ma_context_init(NULL, 0, NULL, &Context) != MA_SUCCESS) return false;

	ma_device_info* pPlaybackInfos;
	ma_uint32 PlaybackCount;

	if (ma_context_get_devices(&Context, &pPlaybackInfos, &PlaybackCount, NULL, NULL) != MA_SUCCESS)
	{
		ma_context_uninit(&Context);
		return false;
	}

	if (DeviceIndex < 0 || (ma_uint32)DeviceIndex >= PlaybackCount)
	{
		ma_context_uninit(&Context);
		return false;
	}

	ma_engine_config EngineConfig = ma_engine_config_init();
	EngineConfig.pPlaybackDeviceID = &pPlaybackInfos[DeviceIndex].id;

	if (ma_engine_init(&EngineConfig, &AudioEngine) == MA_SUCCESS)
	{
		bIsEngineInitialized = true;

		// Inicializa o filtro Low-Pass (abafador) no grafo da engine
		ma_uint32 Channels = ma_engine_get_channels(&AudioEngine);
		ma_uint32 SampleRate = ma_engine_get_sample_rate(&AudioEngine);
		double InitialCutoff = bIsCurrentlyMuffled ? (double)MuffledCutoffFrequency : 20000.0;

		// Ordem 4 deixa o corte bem acentuado (parecido com mão tapando a boca)
		ma_lpf_node_config LpfConfig = ma_lpf_node_config_init(Channels, SampleRate, InitialCutoff, 4);

		if (ma_lpf_node_init(ma_engine_get_node_graph(&AudioEngine), &LpfConfig, NULL, &MuffleFilterNode) == MA_SUCCESS)
		{
			bIsFilterInitialized = true;
			// Conecta a saída do filtro na saída principal do dispositivo Bluetooth
			ma_node_attach_output_bus(&MuffleFilterNode, 0, ma_engine_get_endpoint(&AudioEngine), 0);
			ma_node_set_output_bus_volume(&MuffleFilterNode, 0, bIsCurrentlyMuffled ? MuffledVolumeMultiplier : 1.0f);
		}
	}

	ma_context_uninit(&Context);
	return bIsEngineInitialized;
}

void UBluetoothAudioComponent::PlayAudioFile(FString FileName)
{
	if (!bIsEngineInitialized) return;

	// Se já existe um som carregado/tocando, para e libera antes de tocar o próximo
	if (bIsSoundInitialized)
	{
		ma_sound_stop(&CurrentSound);
		ma_sound_uninit(&CurrentSound);
		bIsSoundInitialized = false;
	}

	FString RelativePath = FPaths::ProjectContentDir() + TEXT("BearAudio/") + FileName;
	FString FullPath = FPaths::ConvertRelativePathToFull(RelativePath);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Tocando audio: %s"), *FullPath));
	}

	// Se o filtro existe, iniciamos sem conexão padrão para rotear pelo filtro primeiro
	ma_uint32 Flags = bIsFilterInitialized ? MA_SOUND_FLAG_NO_DEFAULT_ATTACHMENT : 0;

	if (ma_sound_init_from_file(&AudioEngine, TCHAR_TO_UTF8(*FullPath), Flags, NULL, NULL, &CurrentSound) == MA_SUCCESS)
	{
		bIsSoundInitialized = true;

		if (bIsFilterInitialized)
		{
			ma_node_attach_output_bus(&CurrentSound, 0, &MuffleFilterNode, 0);
		}

		ma_sound_start(&CurrentSound);
	}
}

void UBluetoothAudioComponent::StopAudio()
{
	if (bIsSoundInitialized)
	{
		ma_sound_stop(&CurrentSound);
		ma_sound_uninit(&CurrentSound);
		bIsSoundInitialized = false;
	}
}

void UBluetoothAudioComponent::SetVoiceMuffled(bool bMuffled)
{
	bIsCurrentlyMuffled = bMuffled;

	if (!bIsEngineInitialized || !bIsFilterInitialized) return;

	ma_uint32 Channels = ma_engine_get_channels(&AudioEngine);
	ma_uint32 SampleRate = ma_engine_get_sample_rate(&AudioEngine);

	// 20000 Hz deixa passar todas as frequências audíveis (voz normal)
	// MuffledCutoffFrequency (~450 Hz) corta os agudos e deixa só o grave abafado
	double TargetCutoff = bMuffled ? (double)MuffledCutoffFrequency : 20000.0;

	ma_lpf_config NewConfig = ma_lpf_config_init(ma_format_f32, Channels, SampleRate, TargetCutoff, 4);
	ma_lpf_node_reinit(&NewConfig, &MuffleFilterNode);

	// Também reduz levemente o volume para dar mais realismo físico
	ma_node_set_output_bus_volume(&MuffleFilterNode, 0, bMuffled ? MuffledVolumeMultiplier : 1.0f);
}

void UBluetoothAudioComponent::ShutdownAudioEngine()
{
	// A ordem de destruição importa: primeiro o som, depois o filtro, por último a engine
	if (bIsSoundInitialized)
	{
		ma_sound_stop(&CurrentSound);
		ma_sound_uninit(&CurrentSound);
		bIsSoundInitialized = false;
	}

	if (bIsFilterInitialized)
	{
		ma_lpf_node_uninit(&MuffleFilterNode, NULL);
		bIsFilterInitialized = false;
	}

	if (bIsEngineInitialized)
	{
		ma_engine_uninit(&AudioEngine);
		bIsEngineInitialized = false;
	}
}

void UBluetoothAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownAudioEngine();
	Super::EndPlay(EndPlayReason);
}

void UBluetoothAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}