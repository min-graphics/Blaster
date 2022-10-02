// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BlasterAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	//相当于begin play
	virtual void NativeInitializeAnimation() override;
	//相当于tick
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	
	UPROPERTY(BlueprintReadOnly,Category=Character,meta=(AllowPrivateAccess="true"))
	class ABlasterCharacter* BlasterCharacter;
	
	UPROPERTY(BlueprintReadOnly,Category=Movement,meta=(AllowPrivateAccess="true"))
	float Speed;
	
	UPROPERTY(BlueprintReadOnly,Category=Movement,meta=(AllowPrivateAccess="true"))
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly,Category=Movement,meta=(AllowPrivateAccess="true"))
	bool bIsAccelerating;
	
};
