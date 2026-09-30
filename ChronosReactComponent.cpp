// ============================================================================
// CHRONOS - ChronosReactComponent.cpp
// ============================================================================
#include "ChronosReactComponent.h"
#include "ChronosWorldSubsystem.h"
#include "Components/LightComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"

UChronosReactComponent::UChronosReactComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 4 Hz is plenty for time-of-day reactions - and a talking point about tick budgets.
	PrimaryComponentTick.TickInterval = 0.25f;
}

void UChronosReactComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner())
	{
		GetOwner()->GetComponents<ULightComponent>(Lights);
	}
	Apply(); // don't wait 0.25s for the first correct state
}

void UChronosReactComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Apply();
}

void UChronosReactComponent::Apply()
{
	const UChronosWorldSubsystem* Sys = UChronosWorldSubsystem::Get(this);
	if (!Sys)
	{
		return;
	}

	const FWeatherSnapshot S = Sys->GetCurrentSnapshot();
	const float Hour    = Sys->GetTimeOfDay();
	const bool  bNight  = Sys->IsNight();

	// ---------------- Lights ----------------
	if (bControlLights)
	{
		const bool bLightsOn = bOnAtNightOnly ? bNight : true;
		const float CurveMult = IntensityOverDay ? IntensityOverDay->GetFloatValue(Hour) : 1.f;
		const float WeatherMult = 1.f - (StormDimming * S.CloudCoverage);
		const float FinalIntensity = BaseIntensity * CurveMult * WeatherMult;

		for (ULightComponent* Light : Lights)
		{
			if (!Light)
			{
				continue;
			}
			Light->SetVisibility(bLightsOn);
			Light->SetIntensity(FinalIntensity);
			Light->SetLightColor(NightColor);
		}
	}

	// ---------------- Blueprint events (spam-guarded) ----------------
	const int32 WholeHour = FMath::FloorToInt(Hour);
	if (WholeHour != LastHour)
	{
		LastHour = WholeHour;
		OnHourUpdated(WholeHour, bNight);
	}

	if (!FMath::IsNearlyEqual(S.CloudCoverage, LastCloudCoverage, 0.02f) || S.Precipitation != LastPrecipitation)
	{
		LastCloudCoverage = S.CloudCoverage;
		LastPrecipitation = S.Precipitation;
		OnWeatherUpdated(S);
	}
}
