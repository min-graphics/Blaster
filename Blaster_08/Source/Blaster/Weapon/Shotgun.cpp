// Fill out your copyright notice in the Description page of Project Settings.

#include "Shotgun.h"
#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if(OwnerPawn==nullptr)return;
	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if(MuzzleFlashSocket)
	{
		//这个插槽是射线射出的起始位置，从这里开始进行射线检测，但是由于是霰弹枪，所以希望射线的数量为参数
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		//不用在发射12次时计算12次伤害，而是直接把地图中的成员受到的伤害进行映射，之后在服务器用乘法统一结算，调用一次应用伤害，这样效率会高很多，这样广播也会少很多。
		TMap<ABlasterCharacter*,uint32> HitMap;
		
		for(uint32 i=0;i<NumberOfPellets;i++)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start,HitTarget,FireHit);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if(BlasterCharacter && HasAuthority() && InstigatorController)
			{
				if(HitMap.Contains(BlasterCharacter))
				{
					HitMap[BlasterCharacter]++;
				}
				else
				{
					HitMap.Emplace(BlasterCharacter,1);
				}
			}
			if(ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()
				);
			}
			if(HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					.5f,
					FMath::FRandRange(-.5f,.5f)
				);
			}
		}
		for(auto HitPair : HitMap)
		{
			if(HitPair.Key && HasAuthority() &&InstigatorController)
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key,
					Damage* HitPair.Value,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		}
	}
}

