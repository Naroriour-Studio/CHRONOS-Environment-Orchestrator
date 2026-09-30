// ============================================================================
// CHRONOS - Data-Driven Time-of-Day & Weather Orchestration System
// ChronosWorldSubsystem.h - Owns all time & weather STATE and LOGIC.
// Deliberately has no scene knowledge: the Director actor feeds it time and
// renders the result. Pure-logic subsystems are testable and world-agnostic.
// NOTE: Replace CHRONOS_API with your project's module API macro.
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ChronosTypes.h"
#include "ChronosWorldSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChronosHourChanged, int32, NewHour);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChronosDayChanged, int32, NewDay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChronosWeatherChanged, const UChronosWeatherPreset*, NewWeather);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChronosPrecipitationChanged, EPrecipitationType, Type, float, Intensity);

UCLASS()
class CHRONOS_API UChronosWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Context-safe accessor usable from ANY Blueprint (pass any actor or 'self' as context). */
	UFUNCTION(BlueprintCallable, Category = "Chronos", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, Keywords = "chronos time weather"))
	static UChronosWorldSubsystem* Get(const UObject* WorldContextObject);

	/** Pushes per-level configuration in. Called by the Director (OnConstruction + BeginPlay). */
	void Configure(const FChronosSettings& InSettings, const UChronosWeatherPreset* DefaultWeather);

	/** Advances time and weather blending. Called every frame by AChronosDirector::Tick. */
	void TickChronos(float DeltaTime);

	// ---------------- Designer API (callable from Blueprints / UI) ----------------

	UFUNCTION(BlueprintCallable, Category = "Chronos|Time")
	void SetTimeOfDay(float NewHour);

	UFUNCTION(BlueprintCallable, Category = "Chronos|Time")
	void SetTimeScale(float NewScale);

	UFUNCTION(BlueprintCallable, Category = "Chronos|Time")
	void SetPaused(bool bInPaused) { bPaused = bInPaused; }

	/** Blends from the current state into NewWeather over BlendDuration seconds. */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Weather")
	void SetWeather(const UChronosWeatherPreset* NewWeather, float BlendDuration = 5.f);

	/** Teleports to a weather state instantly (no blend). */
	UFUNCTION(BlueprintCallable, Category = "Chronos|Weather")
	void SnapToWeather(const UChronosWeatherPreset* NewWeather);

	// ---------------- Queries ----------------

	UFUNCTION(BlueprintPure, Category = "Chronos|Time") float GetTimeOfDay() const { return TimeOfDay; }
	UFUNCTION(BlueprintPure, Category = "Chronos|Time") int32 GetDay() const { return Day; }
	UFUNCTION(BlueprintPure, Category = "Chronos|Time") bool IsPaused() const { return bPaused; }
	UFUNCTION(BlueprintPure, Category = "Chronos|Time") float GetTimeScale() const { return Settings.TimeScale; }
	UFUNCTION(BlueprintPure, Category = "Chronos|Sun") float GetSunElevation() const;
	UFUNCTION(BlueprintPure, Category = "Chronos|Sun") float GetSunAzimuth() const;
	UFUNCTION(BlueprintPure, Category = "Chronos|Time") bool IsNight() const { return GetSunElevation() < 0.f; }
	UFUNCTION(BlueprintPure, Category = "Chronos|Weather") FWeatherSnapshot GetCurrentSnapshot() const { return CurrentSnapshot; }
	UFUNCTION(BlueprintPure, Category = "Chronos|Weather") bool IsBlending() const { return TargetWeather != nullptr; }

	// ---------------- Events ----------------

	UPROPERTY(BlueprintAssignable, Category = "Chronos|Events") FOnChronosHourChanged OnHourChanged;
	UPROPERTY(BlueprintAssignable, Category = "Chronos|Events") FOnChronosDayChanged OnDayChanged;
	/** Fires when a NEW weather target is set (start of blend). */
	UPROPERTY(BlueprintAssignable, Category = "Chronos|Events") FOnChronosWeatherChanged OnWeatherChanged;
	/** Fires only on discrete precipitation changes (None->Rain etc.) so audio/VFX don't get spammed. */
	UPROPERTY(BlueprintAssignable, Category = "Chronos|Events") FOnChronosPrecipitationChanged OnPrecipitationChanged;

private:
	static FWeatherSnapshot Evaluate(const UChronosWeatherPreset* Preset);
	void UpdateBlend(float DeltaTime);
	void BroadcastHourIfNeeded();

	FChronosSettings Settings;

	float TimeOfDay = 9.f;		// hours, [0..24)
	int32 Day = 0;
	int32 LastWholeHour = INDEX_NONE;
	bool bPaused = false;

	const UChronosWeatherPreset* CurrentWeather = nullptr;
	const UChronosWeatherPreset* TargetWeather = nullptr;
	FWeatherSnapshot BlendSource;
	float BlendDuration = 0.f;
	float BlendElapsed = 0.f;

	FWeatherSnapshot CurrentSnapshot;
	EPrecipitationType LastPrecipType = EPrecipitationType::None;
	bool bLastPrecipActive = false;
};
