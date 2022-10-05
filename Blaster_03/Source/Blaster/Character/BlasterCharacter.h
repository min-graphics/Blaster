// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "GameFramework/Character.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter,public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	void PlayFireMontage(bool bAiming);

	UFUNCTION(NetMulticast,Unreliable)
	void MulticastHit();

	virtual void OnRep_ReplicatedMovement() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	//这是个动作映射，因此不需要参数。
	void EquipButtonPressed();
	void CrouchButtonPressed();
	//对于瞄准，要有按下和松开两种状态
	void AimButtonPressed();
	void AimButtonReleased();
	void CalculateAO_Pitch();

	float CalculateSpeed();
	void AimOffset(float DeltaTime);

	void SimProxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();
private:
	UPROPERTY(VisibleAnywhere,Category=Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere,Category=Camera)
	class UCameraComponent* FollowCamera;

	//这里是网络同步那一节添加的控件，用来显示当前的角色权限。
	//加蓝图可读，必须加上后面那句话，那句话是给定权限的。
	UPROPERTY(EditAnywhere,BlueprintReadOnly,meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	//我们希望它被复制，意味着每当它在服务器被改变时，所有的客户端都会跟着改变。这是一个存储武器的变量
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	//这是一种rep notify,只有当重叠事件发生时才通知，之前是直接写在tick里面了，不用传递任何参数，因为我们没有主动调用，复制函数的时候自动进行调用的
	//如果有参数，也只是最后一个参数，最后一次被调用时的某个变量
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	//只有函数的宏定义内加上服务器和可靠，才会自动补全Implementation
	UFUNCTION(Server,Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	//声明蒙太奇
	UPROPERTY(EditAnywhere,Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	//声明蒙太奇
	UPROPERTY(EditAnywhere,Category = Combat)
	class UAnimMontage* HitReactMontage;
	
	
	//当摄像机靠近玩家时，隐藏玩家
	void HideCameraIfCharacterClose();

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

public:	
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped()const;
	bool IsAiming()const;

	//forceinline,强制内联函数
	FORCEINLINE float GetAO_Yaw() const {return AO_Yaw;}
	FORCEINLINE float GetAO_Pitch() const {return AO_Pitch;}

	AWeapon* GetEquippedWeapon();

	FORCEINLINE ETurningInPlace GetTuringInPlace() const { return TurningInPlace; }

	FVector GetHitTarget() const;

	FORCEINLINE UCameraComponent* GetFollowCamere() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	
};
