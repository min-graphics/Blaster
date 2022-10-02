// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);

private:
	UPROPERTY(VisibleAnywhere,Category=Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere,Category=Camera)
	class UCameraComponent* FollowCamera;

	//这里是网络同步那一节添加的控件，用来显示当前的角色权限。
	//加蓝图可读，必须加上后面那句话，那句话是给定权限的。
	UPROPERTY(EditAnywhere,BlueprintReadOnly,meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

public:	
	

};
