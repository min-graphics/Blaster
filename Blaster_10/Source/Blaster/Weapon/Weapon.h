// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState:uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName="Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName="Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName="Dropped"),
	
	EWS_Max UMETA(DisplayName="DefaultMAX")
};

UENUM(BlueprintType)
enum class EFireType:uint8
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),
	EFT_Project UMETA(DisplayName = "Projectile Weapon"),
	EFT_Shotgun UMETA(DisplayName = "Shotgun Weapon"),

	EFT_Max UMETA(DisplayName = "DefaultMAX")
};
UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	//网络同步的一般都要加上这一句话。
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;
	void SetHUDAmmo();
	void ShowPickupWidget(bool bShowWidget);
	//先弄一个fire接口，以后会不断的进行扩充
	virtual void Fire(const FVector& HitTarget);
	void Dropped();
	void AddAmmo(int32 AmmoToAdd);
	//这个函数输入起点，目标物，输出散射的向量。
	FVector TraceEndWithScatter(const FVector& HitTarget);
	/*
 * Trace end with scatter
 */

	UPROPERTY(EditAnywhere,Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere,Category = "Weapon Scatter")
	float SphereRadius = 75.f;

	//将服务器倒带变量设置为可复制的
	UPROPERTY(Replicated,EditAnywhere)
	bool bUseServerSideRewind = false;
	
	/*
	 *Textures for the weapon crosshairs
	 **/

	UPROPERTY(EditAnywhere,Category = Crosshairs)
	class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere,Category = Crosshairs)
	UTexture2D* CrosshairsLeft;
	
	UPROPERTY(EditAnywhere,Category = Crosshairs)
	UTexture2D* CrosshairsRight;
	
	UPROPERTY(EditAnywhere,Category = Crosshairs)
	UTexture2D* CrosshairsTop;
	
	UPROPERTY(EditAnywhere,Category = Crosshairs)
	UTexture2D* CrosshairsBottom;
	
	/*
	 *Zoom FOV while aiming
	 */
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	/*
	 *Automatic Fire
	 */
	UPROPERTY(EditAnywhere,Category = Combat)
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere,Category = Combat)
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	/*
	 *Enable or disable custom depth
	 */
	void EnableCustomDepth(bool bEnable);

	bool bDestroyWeapon = false;
	
	UPROPERTY(EditAnywhere)
	EFireType FireType;

	UPROPERTY(EditAnywhere,Category = "Weapon Scatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;

	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;

	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
protected:
	
	virtual void BeginPlay() override;
	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnDropped();
	virtual void OnEquippedSecondary();

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);
	
	UFUNCTION()
	void OnSphereEndOverlap(
			UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex
		);


private:
	UPROPERTY(VisibleAnywhere,Category="Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere,Category="Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing=OnRep_WeaponState,VisibleAnywhere,Category="Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere,Category="Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere,Category="Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	UPROPERTY(EditAnywhere)
	int32 Ammo;

	UFUNCTION(Client,Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client,Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);
	
	UFUNCTION()
	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	//The number of unprocessed server requests for Ammo.
	//Incremented in SpendRound,decremented in ClientUpDateAmmo;
	int32 Sequence = 0;
	
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

public:
	FORCEINLINE void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const {return AreaSphere;}
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const {return WeaponMesh;}
	FORCEINLINE float GetZoomedFOV() const {return ZoomedFOV;}
	FORCEINLINE float GetZoomedInterpSpeed() const {return ZoomInterpSpeed;}
	bool IsEmpty();
	bool IsFull();
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
};
