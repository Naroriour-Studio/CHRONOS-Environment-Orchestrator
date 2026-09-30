// ============================================================================
// CHRONOS - ChronosDirector.cpp
// ============================================================================
#include "ChronosDirector.h"
#include "ChronosWorldSubsystem.h"
#include "ChronosWeatherPreset.h"

#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/World.h"
#include "Engine/Scene.h"

AChronosDirector::AChronosDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	SetHidden(true);
	SetActorEnableCollision(false);
}

void AChronosDirector::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Live preview: editing any setting on this actor instantly reconfigures the subsystem.
	PushConfigToSubsystem();
}

void AChronosDirector::BeginPlay()
{
	Super::BeginPlay();
	PushConfigToSubsystem();

	if (UChronosWorldSubsystem* Sys = UChronosWorldSubsystem::Get(this))
	{
		Sys->OnPrecipitationChanged.AddDynamic(this, &AChronosDirector::HandlePrecipitationChanged);
	}
}

bool AChronosDirector::ShouldTickIfViewportsOnly() const
{
	// Lets this actor tick in editor viewports while NOT in PIE.
	return bPreviewInEditor;
}

void AChronosDirector::PushConfigToSubsystem()
{
	if (UChronosWorldSubsystem* Sys = UChronosWorldSubsystem::Get(this))
	{
		Sys->Configure(Settings, DefaultWeather);
	}
}

void AChronosDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const bool bGameWorld = World->IsGameWorld();
	if (!bGameWorld && !bPreviewInEditor)
	{
		return;
	}

	UChronosWorldSubsystem* Sys = UChronosWorldSubsystem::Get(this);
	if (!Sys)
	{
		return;
	}

	Sys->TickChronos(DeltaTime);

	const float SunElevation = Sys->GetSunElevation();
	const float SunAzimuth   = Sys->GetSunAzimuth();
	ApplySnapshot(Sys->GetCurrentSnapshot(), SunElevation, SunAzimuth, DeltaTime);

	// Day/night transition events (game only, so editor preview can't spawn things).
	const bool bNight = SunElevation < 0.f;
	if (bGameWorld && bNight != bWasNight)
	{
		bWasNight = bNight;
		if (bNight) { OnBecameNight(); }
		else        { OnBecameDay();  }
	}
}

void AChronosDirector::HandlePrecipitationChanged(EPrecipitationType Type, float Intensity)
{
	OnPrecipitationUpdated(Type, Intensity);
}

void AChronosDirector::ApplySnapshot(const FWeatherSnapshot& S, float SunElevation, float SunAzimuth, float DeltaTime)
{
	const bool bNight = SunElevation < 0.f;
	// 1.0 at solar noon, 0.0 at horizon. Used to fade sun/sky contribution.
	const float Daylight = FMath::Clamp(SunElevation / FMath::Max(Settings.MaxSunElevation, 1.f), 0.f, 1.f);

	// ---------------- Sun / Moon ----------------
	if (SunLight)
	{
		SunLight->SetActorRotation(FRotator(-SunElevation, SunAzimuth, 0.f));

		if (UDirectionalLightComponent* LightComp = SunLight->FindComponentByClass<UDirectionalLightComponent>())
		{
			if (!bNight)
			{
				LightComp->SetIntensity(S.SunIntensity * Daylight);
				LightComp->SetLightColor(S.SunTint);
				LightComp->SetColorTemperature(S.SunColorTemperature);
			}
			else
			{
				// The same directional light moonlights as... the moonlight.
				LightComp->SetIntensity(S.MoonlightIntensity);
				LightComp->SetLightColor(FLinearColor(0.55f, 0.7f, 1.f));
				LightComp->SetColorTemperature(7500.f);
			}
			LightComp->SetUseTemperature(true);
			LightComp->SetVisibility(LightComp->Intensity > KINDA_SMALL_NUMBER);
		}
	}

	// ---------------- SkyLight (throttled recapture: the big perf win) ----------------
	if (SkyLight)
	{
		if (USkyLightComponent* SkyComp = SkyLight->FindComponentByClass<USkyLightComponent>())
		{
			SkyComp->SetIntensity(S.SkyLightIntensity * (bNight ? 0.15f : (0.25f + 0.75f * Daylight)));

			TimeSinceRecapture += DeltaTime;
			const bool bSunMovedEnough = FMath::Abs(SunElevation - LastSunElevation) >= MinSunAngleForRecapture;
			if (TimeSinceRecapture >= SkyRecaptureInterval && bSunMovedEnough)
			{
				SkyComp->RecaptureSky();
				TimeSinceRecapture = 0.f;
				LastSunElevation   = SunElevation;
			}
		}
	}

	// ---------------- Fog ----------------
	if (HeightFog)
	{
		if (UExponentialHeightFogComponent* FogComp = HeightFog->FindComponentByClass<UExponentialHeightFogComponent>())
		{
			FogComp->SetFogDensity(S.FogDensity);
			FogComp->SetFogHeightFalloff(S.FogHeightFalloff);
		}
	}
}
