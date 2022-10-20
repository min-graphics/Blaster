// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "RocketMovementComponent.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}
void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();
	if(!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this,&AProjectileRocket::OnHit);
	}
	SpawnTrailSystem();
	if(ProjectileLoop && LoopingSoundAttenuation)
	{
		//生成一个声音并绑定
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
	StartDestroyTimer();
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
							  FVector NormalImpulse, const FHitResult& Hit)
{
	if(OtherActor == GetOwner())
	{
		return;
	}
	ExplodeDamage();
	//StartDestroyTimer();
}

void AProjectileRocket::Destroyed()
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
	if(ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}
	if(CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if(TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}
	if(ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

