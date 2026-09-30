// ============================================================================
// CHRONOS - Data-Driven Time-of-Day & Weather Orchestration System
// ChronosDirector.h - Level-facing actor. Holds per-level settings, binds to
// scene actors (Sun, SkyLight, Fog), drives the subsystem, renders snapshots.
// Also ticks in the editor viewport so designers can scrub time WITHOUT PIE.
// NOTE: Replace CHRONOS_API with your project's module API macro.
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChronosTypes.h"
#include "ChronosDirector.generated.h"

class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class UChronosWeatherPreset;

UCLASS()
class CHRONOS_API AChronosDirector : public AActor
{
	GENERATED_BODY()

public:
	AChronosDirector();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

	// ---------------- Level bindings (pick actors already in your level) ----------------

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Chronos|Scene")
	TObjectPtr<ADirectionalLight> SunLight;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Chronos|Scene")
	TObjectPtr<ASkyLight> SkyLight;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Chronos|Scene")
	TObjectPtr<AExponentialHeightFog> HeightFog;

	// ---------------- Configuration ----------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chronos|Time", meta = (ShowOnlyInnerProperties))
	FChronosSettings Settings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chronos|Weather")
	TObjectPtr<const UChronosWeatherPreset> DefaultWeather;

	/** Registry the debug panel lists as buttons. In production this would come from the Asset Manager. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chronos|Weather")
	TArray<TObjectPtr<const UChronosWeatherPreset>> AvailablePresets;

	// ---------------- Editor preview ----------------

	/** Tick in editor viewports: scrub time from the details panel / EUW and watch the sky move, no PIE needed. */
	UPROPERTY(EditAnywhere, Category = "Chronos|Editor")
	bool bPreviewInEditor = true;

	// ---------------- Performance knobs ----------------

	/** Minimum seconds between SkyLight recaptures (RecaptureSky is expensive). */
	UPROPERTY(EditAnywhere, Category = "Chronos|Performance", meta = (ClampMin = "0.1"))
	float SkyRecaptureInterval = 0.5f;

	/** Skip recapture if the sun moved less than this many degrees since the last one. */
	UPROPERTY(EditAnywhere, Category = "Chronos|Performance", meta = (ClampMin = "0.0"))
	float MinSunAngleForRecapture = 0.25f;

	// ---------------- Blueprint extension points ----------------

	/** Hook your rain/snow Niagara spawn here (game only). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chronos")
	void OnPrecipitationUpdated(EPrecipitationType Type, float Intensity);

	UFUNCTION(BlueprintImplementableEvent, Category = "Chronos")
	void OnBecameNight();

	UFUNCTION(BlueprintImplementableEvent, Category = "Chronos")
	void OnBecameDay();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandlePrecipitationChanged(EPrecipitationType Type, float Intensity);

private:
	void PushConfigToSubsystem();
	void ApplySnapshot(const FWeatherSnapshot& Snapshot, float SunElevation, float SunAzimuth, float DeltaTime);

	float TimeSinceRecapture = 100000.f;
	float LastSunElevation = 0.f;
	bool bWasNight = false;
};
