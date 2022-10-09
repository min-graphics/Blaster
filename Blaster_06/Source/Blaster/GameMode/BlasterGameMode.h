// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

namespace MatchState
{
	extern BLASTER_API const FName Cooldown;//Match duration has been reach.Display winner and begin play timer.
}




/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ABlasterGameMode();
	virtual void Tick(float DeltaSeconds) override;
	virtual void PlayerEliminated(class ABlasterCharacter* ElimmedCharacter,class ABlasterPlayerController* VictimController,ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter,AController* ElimmedController);
	
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;
	
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime =10.f;
	
	float LevelStartingTime = 0.f;
	
protected:
	void BeginPlay() override;

	virtual void OnMatchStateSet() override;
	
private:
	//倒计时，因此要在tick当中进行添加
	float CountdownTime =0.f;
public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
