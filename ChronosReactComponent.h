// ============================================================================
// CHRONOS - Data-Driven Time-of-Day & Weather Orchestration System
// ChronosReactComponent.h - Drop on ANY actor to make it react to time/weather.
// Ships with data-driven light control; Blueprint events let designers extend
// behavior (window materials, NPC schedules, audio) without touching C++.
// NOTE: Replace CHRONOS_API with your project's module API macro.
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChronosTypes.h"
#include "ChronosReactComponent.generated.h"

class ULightComponent;
class UCurveFloat;

UCLASS(ClassGroup = (Chronos), meta = (BlueprintSpawnableComponent))
class CHRONOS_API UChronosReactComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UChronosReactComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ---------------- Light control ----------------

	UPROPERTY(EditAnywhere, Category = "Chronos|Light")
	bool bControlLights = true;

	/** On at night, off during the day. Disable to keep lights on 24/7 and only modulate intensity. */
	UPROPERTY(EditAnywhere, Category = "Chronos|Light")
	bool bOnAtNightOnly = true;

	/** Intensity of point/spot lights owned by this actor (units of the light, e.g. candelas). */
	UPROPERTY(EditAnywhere, Category = "Chronos|Light", meta = (ClampMin = "0.0"))
	float BaseIntensity = 5000.f;

	/** Optional: X = hour of day [0..24], Y = intensity multiplier. Lets a lamp fade in at dusk. */
	UPROPERTY(EditAnywhere, Category = "Chronos|Light")
	TObjectPtr<UCurveFloat> IntensityOverDay;

	/** How much heavy cloud dims this light (0 = ignore weather, 1 = fully dark in overcast). */
	UPROPERTY(EditAnywhere, Category = "Chronos|Light", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StormDimming = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Chronos|Light")
	FLinearColor NightColor = FLinearColor(1.f, 0.72f, 0.4f);

	// ---------------- Blueprint extension points ----------------

	/** Fires when the whole hour changes. Perfect for NPC schedules ("shop closes at 18"). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chronos")
	void OnHourUpdated(int32 Hour, bool bIsNight);

	/** Fires when blended weather meaningfully changes (spam-guarded). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chronos")
	void OnWeatherUpdated(const FWeatherSnapshot& Snapshot);

private:
	void Apply();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULightComponent>> Lights;

	int32 LastHour = INDEX_NONE;
	float LastCloudCoverage = -1.f;
	EPrecipitationType LastPrecipitation = EPrecipitationType::None;
};
