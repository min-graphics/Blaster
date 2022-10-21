// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "Blaster/Weapon/Projectile.h"
#include "Blaster/Weapon/Weapon.h"
#include "CombatComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//因为战斗组件是专门为角色弄得，两者深度绑定的，所以最好直接弄个友元
	//声明类声明错的时候，编译器不会报错，就算是rider都不会报错。
	friend class ABlasterCharacter;

	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBackPack(AActor* ActorToAttach);
	void UpdateCarriedAmmo();
	void PlayEquipWeaponSound(AWeapon* WeaponToEquip);
	void ReloadEmptyWeapon();
	void ShowAttachedGrenade(bool bShowGrenade);
	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);
	
	void EquipWeapon(class AWeapon* WeaponToEquip);
	void SwapWeapons();
	void Reload();
	
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
	void FinishSwap();

	UFUNCTION(BlueprintCallable)
	void FinishSwapAttachWeapons();
	
	void UpdateAmmoValues();

	void FireButtonPressed(bool bPressed);

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunEnd();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

	UFUNCTION(Server,Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	void PickupAmmo(EWeaponType WeaponType,int32 AmmoAmount);

	bool bLocallyReloading = false;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server,Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();
	
	void Fire();
	void FireProjectileWeapon();
	void FireHitScanWeapon();
	void FireShotgun();
	void LocalFire(const FVector_NetQuantize& TraceHitTarget);
	void ShotgunLocalFire(const TArray<FVector_NetQuantize>& TraceHitTargets);
	
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget,float FireDelay);

	UFUNCTION(NetMulticast,Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets,float FireDelay);

	UFUNCTION(Server,Reliable)
	void MulticastShotgunFire(const TArray<FVector_NetQuantize>& TraceHitTargets);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server,Reliable)
	void ServerReload();
	
	void HandleReload();

	int32 AmountToReload();

	void ThrowGrenade();

	UFUNCTION(Server,Reliable)
	void ServerThrowGrenade();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;

	
	void DropEquippedWeapon();

private:
	class ABlasterCharacter* Character;
	class ABlasterPlayerController* Controller;
	class ABlasterHUD* HUD;
	
	//当复制一个变量时，我们需要注册并获取lifetime复制。
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;
	
	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming=false;

	//这个才是真正的反映了客户端aim的真实值，到底有没有按下去
	bool bAimButtonPressed = false;
	
	UFUNCTION()
	void OnRep_Aiming();

	//这两个变量时为了控制瞄准时速度降低
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	//开火按钮的状态
	bool bFireButtonPressed;

	//不在需要使用存储命中目标的命中目标变量
	//FVector HitTarget;

	/**
	 *HUD and crosshairs
    */
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	//第75集，用来debug射线的
	FVector HitTarget;

	FHUDPackage HUDPackage;
	/*
	 * Aiming and FOV
	 */

	//Field of view when not aiming;set to  the camera's base FOV in BeginPlay
	float DefaultFOV;

	UPROPERTY(EditAnywhere,Category = Combat)
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere,Category = Combat)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	/*
	 * Automatic fire
	 */
	FTimerHandle FireTimer;
	
	bool bCanFire = true;
	
	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

	//Carried ammo for the currently-equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	//TMap底层使用了哈希表，无法被复制
	TMap<EWeaponType,int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 500;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;
	
	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 0;
	
	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateShotgunAmmoValues();

	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 Grenades = 4;
	
	UFUNCTION()
	void OnRep_Grenades();
	
	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;

	void UpdateHUDGrenades();

public:	
	// Called every frame
	FORCEINLINE int32 GetGrenades() const {return Grenades;}
	bool ShouldSwapWeapons();
};
