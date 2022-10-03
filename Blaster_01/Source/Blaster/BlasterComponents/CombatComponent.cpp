// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	//一开始都先把tick函数false，只有我们确定要tick一些东西时，才true，这样能提高网络性能。
	PrimaryComponentTick.bCanEverTick = false;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
	// ...
}


// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
	// ...
	
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming=bIsAiming;
	ServerSetAiming(bIsAiming);
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed= bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

	
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if(EquippedWeapon&&Character)
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw=true;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming=bIsAiming;
	//在服务器端也要进行速度设置，否则的话会默认之前的速度。
	//只要服务器知道我们想怎么走，一切都好说
	if(Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed= bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}

	
}


// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UCombatComponent,EquippedWeapon);
	DOREPLIFETIME(UCombatComponent,bAiming);
	
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(Character==nullptr||WeaponToEquip==NULL)return;
	//写到这时，就需要去给骨骼加一个插槽了

	EquippedWeapon=WeaponToEquip;
	//武器状态那个枚举类要进行设置
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(TEXT("RightHandSocket"));
	if(HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon,Character->GetMesh());
	}
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->ShowPickupWidget(false);
	EquippedWeapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw=true;
}

