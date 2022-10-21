// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "NiagaraFunctionLibrary.h"
#include "GameNetworkManagerSettings.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Blaster.h"

// Sets default values
AProjectile::AProjectile()
{
 	
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	//设置盒体碰撞
	CollisionBox=CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//先设置成全部忽略，之后可以再对特殊的几个打开碰撞阻拦。
	CollisionBox->SetCollisionResponseToChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility,ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic,ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh,ECollisionResponse::ECR_Block);

	//生成默认发射物，同时，打开受重力旋转,但是不需要这些运动组件了，就简单定义一下，下面开始用多态来实现就行了。
	//ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	//ProjectileMovementComponent->bRotationFollowsVelocity=true;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if(Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}
	if(HasAuthority())
	{
		//弹丸只会在服务器上注册命中事件。
		CollisionBox->OnComponentHit.AddDynamic(this,&AProjectile::OnHit);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	/*
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if(BlasterCharacter)
	{
		BlasterCharacter->MulticastHit();
	}
	*/
	Destroy();
}

void AProjectile::SpawnTrailSystem()
{
	if(TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimerFinished,
		DestroyTime
	);
	
}
void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::ExplodeDamage()
{
	APawn* FiringPawn = GetInstigator();
	if(FiringPawn&&HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if(FiringController)//only play damage on the server
			{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,//World context object
				Damage,//Base Damage
				10.f,//MinimumDamage
				GetActorLocation(),//Origin
				DamageInnerRadius,//DamageInnerRadius
				DamageOuterRadius,//DamageOuterRadius
				1.f,//DamageFallOff
				UDamageType::StaticClass(),//DamageTypeClass
				TArray<AActor*>(),//IgnoreActors
				this,//DamageCauser
				FiringController//InstigatorController
			);
			}
	}
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
	if(ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),ImpactParticles,GetActorTransform());
	}
	if(ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this,ImpactSound,GetActorLocation());
	}
}

