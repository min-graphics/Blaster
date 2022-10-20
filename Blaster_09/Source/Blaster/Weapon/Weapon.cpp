// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Casing.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/BlasterComponents/CombatComponent.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick =false;
	bReplicates=true;
	SetReplicateMovement(true);
	
	//初始化武器网格体，并且绑定到根组件上
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	SetRootComponent(WeaponMesh);

	//对所有通道都进行阻拦，但是对于pawn实施忽略,并且一开始设置无碰撞
	WeaponMesh->SetCollisionResponseToChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn,ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//给网格体设置深度值，这里不想hardcode，直接写个宏比较好。
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	AreaSphere=CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToChannels(ECollisionResponse::ECR_Ignore);
	//设置的在所有客户端上碰撞都会阻挡，但是以后会在服务器上打开,在beginplay当中打开
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget=CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if(WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	//当当前有权威时，就开启碰撞了，其实意思就是，只在服务器上开启，if内也可以用函数HasAuthority()
	//写在if下的函数体内，表示只在服务器端进行这些处理
	//if(GetLocalRole()==ENetRole::ROLE_Authority)//
	//在优化的时候把上面的那一行if给取消了。
	//{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn,ECollisionResponse::ECR_Overlap);
		//第一个是参数是对象，第二个参数是回调函数。
		
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this,&AWeapon::OnSphereOverlap);
		//好像说这里是一个回调。。。
		AreaSphere->OnComponentEndOverlap.AddDynamic(this,&AWeapon::OnSphereEndOverlap);
	//}
	if(PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
	/*
	 *模拟客户端作弊
	if(!HasAuthority())
	{
		FireDelay = 0.001f;
	}
	*/
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	//值复制，复制weaponstate
	DOREPLIFETIME(AWeapon,WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon,bUseServerSideRewind,COND_OwnerOnly);
}


//只想在服务器端进行武器碰撞检测
void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if(BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if(BlasterCharacter)
	{
		//直接置空就行了，就像万喜说的一样，不能通过一个空指针来调用函数。
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo-1,0,MagCapacity);
	SetHUDAmmo();
	if(HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	}
	else
	{
		++Sequence;
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if(HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo();
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd,0,MagCapacity);
	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd);
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if(HasAuthority()) return;
	Ammo = FMath::Clamp(Ammo+AmmoToAdd,0,MagCapacity);
	BlasterOwnerCharacter = BlasterOwnerCharacter==nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if(BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombat() && IsFull())
	{
		BlasterOwnerCharacter->GetCombat()->JumpToShotgunEnd();	
	}
	SetHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if(Owner == nullptr)
	{
		BlasterOwnerCharacter=nullptr;
		BlasterOwnerController=nullptr;
	}
	else
	{
		BlasterOwnerCharacter = BlasterOwnerCharacter==nullptr?Cast<ABlasterCharacter>(Owner):BlasterOwnerCharacter;
		if(BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() && BlasterOwnerCharacter->GetEquippedWeapon() == this)
		{
			SetHUDAmmo();
		}
	}
}

void AWeapon::SetHUDAmmo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()):BlasterOwnerCharacter;
	if(BlasterOwnerCharacter)
	{
		BlasterOwnerController= BlasterOwnerController==nullptr?Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller):BlasterOwnerController;
		if(BlasterOwnerController)
		{
			BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	OnWeaponStateSet();
}
void AWeapon::OnWeaponStateSet()
{
	switch(WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquipped();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		OnEquippedSecondary();
		break;
	case EWeaponState::EWS_Dropped:
		OnDropped();
		break;
	}
}
void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}
void AWeapon::OnEquipped()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if(WeaponType==EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToChannels(ECollisionResponse::ECR_Ignore);
	}
	EnableCustomDepth(false);//

	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if(BlasterOwnerCharacter && bUseServerSideRewind)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr?Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller):BlasterOwnerController;
		if(BlasterOwnerController && HasAuthority() && !BlasterOwnerController->HighPingDelegate.IsBound())
		{
			BlasterOwnerController->HighPingDelegate.AddDynamic(this,&AWeapon::OnPingTooHigh);
		}
	}
}
void AWeapon::OnDropped()
{
	if(HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn,ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,ECollisionResponse::ECR_Ignore);

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if(BlasterOwnerCharacter && bUseServerSideRewind)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr?Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller):BlasterOwnerController;
		if(BlasterOwnerController && HasAuthority() && BlasterOwnerController->HighPingDelegate.IsBound())
		{
			BlasterOwnerController->HighPingDelegate.RemoveDynamic(this,&AWeapon::OnPingTooHigh);
		}
	}
}

void AWeapon::OnEquippedSecondary()
{
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if(WeaponType==EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToChannels(ECollisionResponse::ECR_Ignore);
	}

	if(WeaponMesh)
	{
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);
		WeaponMesh->MarkRenderStateDirty();
	}

	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if(BlasterOwnerCharacter && bUseServerSideRewind)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr?Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller):BlasterOwnerController;
		if(BlasterOwnerController && HasAuthority() && BlasterOwnerController->HighPingDelegate.IsBound())
		{
			BlasterOwnerController->HighPingDelegate.RemoveDynamic(this,&AWeapon::OnPingTooHigh);
		}
	}
}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}

bool AWeapon::IsFull()
{
	return Ammo == MagCapacity;
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if(PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
	
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if(FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation,false);
	}
	if(CasingClass)
	{
		//这段代码是在发射物的基础上修改的，原理是一样的。
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if(AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			
			UWorld* World = GetWorld();
			if(World)
			{
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
					);
			}
		}
	}
	//if(HasAuthority())//将这个去掉后就会变成客户端预测，如果没有加服务器协调的话，就会导致抖动。
	//{
		SpendRound();
	//}
}
void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld,true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	BlasterOwnerCharacter=nullptr;
	BlasterOwnerController=nullptr;
}

FVector AWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if(MuzzleFlashSocket==nullptr) return FVector();
	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();
	
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f,SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart; 
	
	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}
