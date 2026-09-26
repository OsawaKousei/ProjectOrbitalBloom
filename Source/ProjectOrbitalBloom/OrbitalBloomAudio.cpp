#include "OrbitalBloomAudio.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "GameFramework/Actor.h"

namespace
{
constexpr int32 SampleRate = 22050;
TArray<uint8> Synth(float Duration, float Frequency, bool bScore)
{
    const int32 Count = static_cast<int32>(Duration * SampleRate);
    TArray<uint8> PCM;
    PCM.SetNumUninitialized(Count * sizeof(int16));
    const double Chords[] = {130.8128, 174.6141, 146.8324, 195.9977};
    for (int32 I = 0; I < Count; ++I)
    {
        const double T = static_cast<double>(I) / SampleRate;
        double Sample;
        if (bScore)
        {
            const double Local = FMath::Fmod(T, 2.0);
            const double Root = Chords[FMath::Min(3, static_cast<int32>(T / 2))];
            const double Envelope = FMath::Sin(PI * Local / 2) * FMath::Sin(PI * Local / 2);
            Sample = Envelope * (FMath::Sin(2 * PI * Root * T) + 0.45 * FMath::Sin(2 * PI * Root * 1.5 * T)
                + 0.25 * FMath::Sin(2 * PI * Root * 2.5 * T)) * 0.06;
        }
        else
        {
            const double Envelope = FMath::Min(1.0, T / 0.008) * FMath::Exp(-T * 20);
            Sample = FMath::Sin(2 * PI * Frequency * T + 180 * T * T) * Envelope * 0.22;
        }
        const int16 Value = static_cast<int16>(FMath::Clamp(Sample, -1.0, 1.0) * 32767);
        FMemory::Memcpy(PCM.GetData() + I * sizeof(int16), &Value, sizeof(int16));
    }
    return PCM;
}
}

UOrbitalBloomAudio::UOrbitalBloomAudio() { PrimaryComponentTick.bCanEverTick = true; }

void UOrbitalBloomAudio::BeginPlay()
{
    Super::BeginPlay();
    auto Wave = [this]()
    {
        USoundWaveProcedural* Result = NewObject<USoundWaveProcedural>(this);
        Result->SetSampleRate(SampleRate);
        Result->NumChannels = 1;
        Result->Duration = INDEFINITELY_LOOPING_DURATION;
        Result->bLooping = false;
        return Result;
    };
    auto Component = [this](USoundWaveProcedural* Sound)
    {
        UAudioComponent* Result = NewObject<UAudioComponent>(GetOwner());
        GetOwner()->AddInstanceComponent(Result);
        Result->bAutoActivate = false;
        Result->bIsUISound = true;
        Result->SetSound(Sound);
        Result->RegisterComponent();
        return Result;
    };
    MusicWave = Wave(); EffectWave = Wave();
    Music = Component(MusicWave); Effect = Component(EffectWave);
    Loop = Synth(8, 0, true);
    MusicWave->QueueAudio(Loop.GetData(), Loop.Num());
    Music->Play();
}

void UOrbitalBloomAudio::SetTactical(bool bValue)
{
    if (!Music || bTactical == bValue) return;
    bTactical = bValue;
    Music->SetLowPassFilterEnabled(bValue);
    Music->SetLowPassFilterFrequency(bValue ? 650 : 20000);
    Music->AdjustVolume(0.15f, bValue ? 0.3f : 1.0f);
    Accent(bValue ? 420 : 880);
}

void UOrbitalBloomAudio::Accent(float Frequency)
{
    if (!Effect) return;
    Effect->Stop();
    EffectWave->ResetAudio();
    const TArray<uint8> PCM = Synth(0.25f, Frequency, false);
    EffectWave->QueueAudio(PCM.GetData(), PCM.Num());
    Effect->Play();
    EffectRemaining = 0.25f;
}

void UOrbitalBloomAudio::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (MusicWave && MusicWave->GetAvailableAudioByteCount() < SampleRate * 2)
        MusicWave->QueueAudio(Loop.GetData(), Loop.Num());
    if (EffectRemaining > 0)
    {
        EffectRemaining -= DeltaTime;
        if (EffectRemaining <= 0) Effect->Stop();
    }
}
