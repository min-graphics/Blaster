// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundCue.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	

	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;


	/**
	 *Used with server-side rewind
	 */
	bool bUseServerSideRewind = false;
	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;

	//Only set this for Grenades and Rockets
	UPROPERTY(EditAnywhere)
	float Damage=20.f;

	//Doesn't matter for Grenades and Rockets
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;
private:
	
	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;
	
	class UParticleSystemComponent* TracerComponent;

protected:
	virtual void BeginPlay() override;
	void StartDestroyTimer();
	void DestroyTimerFinished();
	void ExplodeDamage();
	//命中函数，命中后处理声音，粒子，销毁等
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp,AActor* OtherActor,UPrimitiveComponent* OtherComp,FVector NormalImpulse,const FHitResult& Hit);



	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;
	
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;\

	void SpawnTrailSystem();

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

public:	
	
};
