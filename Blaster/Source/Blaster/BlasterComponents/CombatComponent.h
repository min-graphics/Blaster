// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

	void EquipWeapon(class AWeapon* WeaponToEquip);
	

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server,Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();


private:
	class ABlasterCharacter* Character;


	//当复制一个变量时，我们需要注册并获取lifetime复制。
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;
	
	UPROPERTY(Replicated)
	bool bAiming;

	//这两个变量时为了控制瞄准时速度降低
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	

public:	
	// Called every frame
	

		
};
