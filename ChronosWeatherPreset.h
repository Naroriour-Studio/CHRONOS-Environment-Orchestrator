// ============================================================================
// CHRONOS - Data-Driven Time-of-Day & Weather Orchestration System
// ChronosWeatherPreset.h - Data Asset describing one weather state.
// Designers create as many of these as they want: Content Browser ->
// right-click -> Miscellaneous -> Data Asset -> ChronosWeatherPreset.
// NOTE: Replace CHRONOS_API with your project's module API macro.
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ChronosTypes.h"
#include "ChronosWeatherPreset.generated.h"

UCLASS(BlueprintType)
class CHRONOS_API UChronosWeatherPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	FText DisplayName;

	// --- Day sun ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun", meta = (ClampMin = "0.0"))
	float SunIntensity = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun", meta = (ClampMin = "1000.0", ClampMax = "12000.0"))
	float SunColorTemperature = 6500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
	FLinearColor SunTint = FLinearColor::White;

	// --- Night ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Night", meta = (ClampMin = "0.0"))
	float MoonlightIntensity = 0.05f;

	// --- Ambient ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sky", meta = (ClampMin = "0.0"))
	float SkyLightIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sky", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CloudCoverage = 0.f;

	// --- Fog ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fog", meta = (ClampMin = "0.0"))
	float FogDensity = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fog", meta = (ClampMin = "0.0"))
	float FogHeightFalloff = 0.2f;

	// --- Gameplay-facing scalars (wind audio, cloth, particles) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind", meta = (ClampMin = "0.0"))
	float WindSpeed = 2.f;

	// --- Precipitation ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Precipitation")
	EPrecipitationType Precipitation = EPrecipitationType::None;

	/** 0..1, drives spawn rate of the rain/snow Niagara system you hook up in the Director's event graph. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Precipitation",
		meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "Precipitation != EPrecipitationType::None"))
	float PrecipitationIntensity = 0.f;
};
