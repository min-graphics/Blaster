// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"

#include "Projectile.h"
#include "Engine/SkeletalMeshSocket.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	//if(!HasAuthority()) return;//这是用来确保只在服务器端进行执行。
	
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	UWorld* World = GetWorld();
	if(MuzzleFlashSocket && World)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		//这是从枪口闪光插槽到从十字准星下的轨迹击中位置。
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = InstigatorPawn;

		AProjectile* SpawnedProjectile = nullptr;
		
		if(bUseServerSideRewind)
		{
			if(InstigatorPawn->HasAuthority()) //server
				{
				if(InstigatorPawn->IsLocallyControlled()) //server ,host -use replicated projectile
					{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass,SocketTransform.GetLocation(),TargetRotation,SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->Damage = Damage;
					SpawnedProjectile->HeadShotDamage = HeadShotDamage;
					}
				else//server , not locally controlled - spawn non-replicated projectile ,no SSR
					{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass,SocketTransform.GetLocation(),TargetRotation,SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
					
					}
				}
			else//client ,using SSR
				{
				if(InstigatorPawn->IsLocallyControlled()) //client,locally controlled - spawn non-replicated projectile,use SSR
					{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass,SocketTransform.GetLocation(),TargetRotation,SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
					SpawnedProjectile->Damage = Damage;
					}
				else//client,not locally controlled - spawn non-replicated projectile,no SSR
					{
					SpawnedProjectile = World->SpawnActor<AProjectile>(ServerSideRewindProjectileClass,SocketTransform.GetLocation(),TargetRotation,SpawnParams);
					SpawnedProjectile->bUseServerSideRewind = false;
					}
				}
		}
		else //weapon not using SSR
		{
			if(InstigatorPawn->HasAuthority())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>(ProjectileClass,SocketTransform.GetLocation(),TargetRotation,SpawnParams);
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage;
				SpawnedProjectile->HeadShotDamage = HeadShotDamage;
			}
		}
	}
};
