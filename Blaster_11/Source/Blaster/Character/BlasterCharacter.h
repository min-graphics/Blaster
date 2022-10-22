// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "GameFramework/Character.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/TimelineComponent.h"
#include "Blaster/BlasterTypes/Team.h"
#include "BlasterCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLeftGame);

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

	/*
	 * Play Montages
	 */
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();
    /*
	UFUNCTION(NetMulticast,Unreliable)
	void MulticastHit();
	*/
	virtual void OnRep_ReplicatedMovement() override;
	
	//角色淘汰时发生的事
	void Elim(bool bPlayerLeftGame);
	
	UFUNCTION(NetMulticast,Reliable)
	void MulticastElim(bool bPlayerLeftGame);

	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();
	

	void SpawnDefaultWeapon();
	
	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;//要先声明，不加class这里会报错

	bool bFinishedSwapping = false;

	UFUNCTION(Server,Reliable)
	void ServerLeaveGame();

	FOnLeftGame OnLeftGame;

	UFUNCTION(NetMulticast,Reliable)
	void MulticastGainedTheLead();

	UFUNCTION(NetMulticast,Reliable)
	void MulticastLostTheLead();

	void SetTeamColor(ETeam Team);

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
	void ReloadButtonPressed();
	//对于瞄准，要有按下和松开两种状态
	void AimButtonPressed();
	void AimButtonReleased();
	void CalculateAO_Pitch();
	
	void AimOffset(float DeltaTime);

	void SimProxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();
	void GrenadeButtonPressed();
	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor,float Damage, const UDamageType* DamageType,class AController* InstigatorController,AActor* DamageCauser);
	

	//Poll for any relavant class and initialize our HUD
	void PollInit();

	void RotateInPlace(float DeltaTime);

	/**
	 *Hit boxes used for server-side rewind
	 */
	UPROPERTY(EditAnywhere)
	class UBoxComponent* head;

	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;

	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* backpack;

	UPROPERTY(EditAnywhere)
	UBoxComponent* blanket;

	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;

	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;
	

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


	/*
	 * Blaster component
	 */
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;
	
	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* LagCompensation;
	
	//只有函数的宏定义内加上服务器和可靠，才会自动补全Implementation
	UFUNCTION(Server,Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/*
	 * Animation Montages
	 */
	//声明蒙太奇
	UPROPERTY(EditAnywhere,Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	//声明蒙太奇
	UPROPERTY(EditAnywhere,Category = Combat)
	UAnimMontage* HitReactMontage;

	//声明蒙太奇
	UPROPERTY(EditAnywhere,Category = Combat)
	UAnimMontage* ElimMontage;

	//声明蒙太奇
	UPROPERTY(EditAnywhere,Category = Combat)
	UAnimMontage* ReloadMontage;

	//声明蒙太奇
	UPROPERTY(EditAnywhere,Category = Combat)
	UAnimMontage* ThrowGrenadeMontage;

	//声明蒙太奇
	UPROPERTY(EditAnywhere,Category = Combat)
	UAnimMontage* SwapMontage;
	
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
	float CalculateSpeed();

	/*
	 * Player health
	 */
	UPROPERTY(EditAnywhere,Category="Player State")
	float MaxHealth = 100.f;
	UPROPERTY(ReplicatedUsing=OnRep_Health,VisibleAnywhere,Category="Player State")
	float Health = 100.f;
	UFUNCTION()
	void OnRep_Health(float LastHealth);

	/*
	 * Player health
	 */
	UPROPERTY(EditAnywhere,Category="Player State")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing=OnRep_Shield,EditAnywhere,Category="Player State")
	float Shield = 0.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);
	
	class ABlasterPlayerController* BlasterPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;

	UPROPERTY(EditAnywhere)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

	bool bLeftGame = false;
	
	/*
	 * Dissolve effect，c++版的时间轴
	 */
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();
	//我们实时更新的动态实例
	UPROPERTY(VisibleAnywhere,Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	//使用动态材料实例生成的蓝图动态材料实例
	UPROPERTY(EditAnywhere,Category=Elim)
	UMaterialInstance* DissolveMaterialInstance;

	/**
	 *Team colors
	 */

	UPROPERTY(EditAnywhere,Category = Elim)
	UMaterialInstance* RedDissolveMatInst;

	UPROPERTY(EditAnywhere,Category = Elim)
	UMaterialInstance* RedMaterial;

	UPROPERTY(EditAnywhere,Category = Elim)
	UMaterialInstance* BlueDissolveMatInst;

	UPROPERTY(EditAnywhere,Category = Elim)
	UMaterialInstance* BlueMaterial;

	UPROPERTY(EditAnywhere,Category = Elim)
	UMaterialInstance* OriginalMaterial;
	/*
	 * Elim effect
	 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;
	
	UPROPERTY(EditAnywhere)
	USoundCue* ElimBotSound;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;
	
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CrownSystem;
	
	UPROPERTY()
	class UNiagaraComponent* CrownComponent;

	/*
	 * Grenade
	 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	/*
	 * Default Weapon
	 */
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;
	
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
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	
	ECombatState GetCombatState() const;
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE bool GetDisableGemeplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade ; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	bool IsLocallyReloading();
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
};
