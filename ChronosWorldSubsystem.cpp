// ============================================================================
// CHRONOS - ChronosWorldSubsystem.cpp
// ============================================================================
#include "ChronosWorldSubsystem.h"
#include "ChronosWeatherPreset.h"
#include "Engine/World.h"

void UChronosWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	TimeOfDay      = Settings.StartHour;
	CurrentSnapshot = FWeatherSnapshot(); // safe defaults until Configure() runs
}

void UChronosWorldSubsystem::Deinitialize()
{
	OnHourChanged.Clear();
	OnDayChanged.Clear();
	OnWeatherChanged.Clear();
	OnPrecipitationChanged.Clear();
	Super::Deinitialize();
}

UChronosWorldSubsystem* UChronosWorldSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		return World->GetSubsystem<UChronosWorldSubsystem>();
	}
	return nullptr;
}

void UChronosWorldSubsystem::Configure(const FChronosSettings& InSettings, const UChronosWeatherPreset* DefaultWeather)
{
	Settings      = InSettings;
	TimeOfDay     = FMath::Fmod(Settings.StartHour, 24.f);
	Day           = 0;
	LastWholeHour = INDEX_NONE;
	TargetWeather = nullptr;
	BlendElapsed  = 0.f;
	CurrentWeather = DefaultWeather;
	CurrentSnapshot = Evaluate(DefaultWeather);
	BroadcastHourIfNeeded();
}

void UChronosWorldSubsystem::TickChronos(float DeltaTime)
{
	if (!bPaused && Settings.TimeScale > 0.f)
	{
		// TimeScale = game-seconds per real second -> convert to hours.
		TimeOfDay += (DeltaTime * Settings.TimeScale) / 3600.f;
		if (TimeOfDay >= 24.f)
		{
			TimeOfDay -= 24.f;
			Day++;
			OnDayChanged.Broadcast(Day);
		}
		BroadcastHourIfNeeded();
	}
	UpdateBlend(DeltaTime);
}

void UChronosWorldSubsystem::BroadcastHourIfNeeded()
{
	const int32 WholeHour = FMath::FloorToInt(TimeOfDay);
	if (WholeHour != LastWholeHour)
	{
		LastWholeHour = WholeHour;
		OnHourChanged.Broadcast(WholeHour);
	}
}

void UChronosWorldSubsystem::SetTimeOfDay(float NewHour)
{
	TimeOfDay = FMath::Fmod(FMath::Max(NewHour, 0.f), 24.f);
	BroadcastHourIfNeeded();
}

void UChronosWorldSubsystem::SetTimeScale(float NewScale)
{
	Settings.TimeScale = FMath::Max(NewScale, 0.f);
}

void UChronosWorldSubsystem::SetWeather(const UChronosWeatherPreset* NewWeather, float InBlendDuration)
{
	if (!NewWeather || NewWeather == TargetWeather || (NewWeather == CurrentWeather && !TargetWeather))
	{
		return;
	}

	// Freeze the live snapshot as the blend source so chained blends stay smooth.
	BlendSource   = CurrentSnapshot;
	TargetWeather = NewWeather;
	BlendDuration = FMath::Max(InBlendDuration, 0.01f);
	BlendElapsed  = 0.f;
	OnWeatherChanged.Broadcast(NewWeather);
}

void UChronosWorldSubsystem::SnapToWeather(const UChronosWeatherPreset* NewWeather)
{
	TargetWeather  = nullptr;
	CurrentWeather = NewWeather;
	CurrentSnapshot = Evaluate(NewWeather);

	if (NewWeather)
	{
		OnWeatherChanged.Broadcast(NewWeather);
	}
}

void UChronosWorldSubsystem::UpdateBlend(float DeltaTime)
{
	if (TargetWeather)
	{
		BlendElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(BlendElapsed / BlendDuration, 0.f, 1.f);
		CurrentSnapshot = FWeatherSnapshot::Lerp(BlendSource, Evaluate(TargetWeather), Alpha);

		if (Alpha >= 1.f)
		{
			CurrentWeather = TargetWeather;
			TargetWeather  = nullptr;
		}
	}

	// Discrete precipitation event (spam guard: only on state change).
	const bool bPrecipActive = CurrentSnapshot.PrecipitationIntensity > 0.01f
		&& CurrentSnapshot.Precipitation != EPrecipitationType::None;
	if (CurrentSnapshot.Precipitation != LastPrecipType || bPrecipActive != bLastPrecipActive)
	{
		LastPrecipType  = CurrentSnapshot.Precipitation;
		bLastPrecipActive = bPrecipActive;
		OnPrecipitationChanged.Broadcast(CurrentSnapshot.Precipitation,
			bPrecipActive ? CurrentSnapshot.PrecipitationIntensity : 0.f);
	}
}

float UChronosWorldSubsystem::GetSunElevation() const
{
	// Sunrise ~06:00, solar noon ~12:00, sunset ~18:00, lowest point at midnight.
	const float Phase = FMath::Fmod((TimeOfDay - 6.f) / 24.f, 1.f) * 2.f * PI;
	return Settings.MaxSunElevation * FMath::Sin(Phase);
}

float UChronosWorldSubsystem::GetSunAzimuth() const
{
	return FMath::Fmod((TimeOfDay / 24.f) * 360.f + Settings.AzimuthOffset, 360.f);
}

FWeatherSnapshot UChronosWorldSubsystem::Evaluate(const UChronosWeatherPreset* Preset)
{
	if (!Preset)
	{
		return FWeatherSnapshot(); // struct defaults = pleasant clear day
	}

	FWeatherSnapshot S;
	S.SunIntensity           = Preset->SunIntensity;
	S.SunColorTemperature    = Preset->SunColorTemperature;
	S.SunTint                = Preset->SunTint;
	S.MoonlightIntensity     = Preset->MoonlightIntensity;
	S.SkyLightIntensity      = Preset->SkyLightIntensity;
	S.FogDensity             = Preset->FogDensity;
	S.FogHeightFalloff       = Preset->FogHeightFalloff;
	S.CloudCoverage          = Preset->CloudCoverage;
	S.WindSpeed              = Preset->WindSpeed;
	S.Precipitation          = Preset->Precipitation;
	S.PrecipitationIntensity = Preset->PrecipitationIntensity;
	return S;
}
