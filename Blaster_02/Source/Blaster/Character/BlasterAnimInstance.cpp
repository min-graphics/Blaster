// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"

#include "BlasterCharacter.h"

//引入移动组件的头文件
#include "Blaster/Weapon/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	//对应蓝图节点，尝试获取pawn的拥有者，这样便可以和character互联了
	BlasterCharacter=Cast<ABlasterCharacter>(TryGetPawnOwner());
	
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if(BlasterCharacter==nullptr)
	{
		BlasterCharacter=Cast<ABlasterCharacter>(TryGetPawnOwner());
	}
	if(BlasterCharacter==nullptr)return;
	
	//获取速度，之后定义速度，其实是和蓝图一模一样的。
	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z=0.f;
	Speed=Velocity.Size();
	
	//是否在空中和是否加速，都是由移动组件来判断的。
	bIsInAir= BlasterCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating=BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size()>0.f?true:false;
	//其实和蓝图实现思想是一样的，只不过这里改成了cpp实现
	bWeaponEquipped=BlasterCharacter->IsWeaponEquipped();

	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	
	bIsCrouched=BlasterCharacter->bIsCrouched;

	bAiming=BlasterCharacter->IsAiming();

	TurningInPlace = BlasterCharacter->GetTuringInPlace();

	//Offset Yaw for Strafing
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator DelatRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation,AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation,DelatRot,DeltaSeconds,5.f);
	YawOffset = DeltaRotation.Yaw;

	
	//这是一个测试的，为了测试客户端两个yaw角度
	if(!BlasterCharacter->HasAuthority())
	{
		UE_LOG(LogTemp,Warning,TEXT("AimRotation Yaw %f:"),AimRotation.Yaw);
		UE_LOG(LogTemp,Warning,TEXT("MovementRotation Yaw %f:"),MovementRotation.Yaw);
	}
	
	//与lean有关
	CharacterRotationLastFrame=CharacterRotation;
	CharacterRotation=BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation,CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean,Target,DeltaSeconds,6.f);
	Lean = FMath::Clamp(Interp,-90.f,90.f);

	//瞄准偏移的yaw角和pitch角
	AO_Yaw = BlasterCharacter->GetAO_Yaw();
	AO_Pitch=BlasterCharacter->GetAO_Pitch();

	if(bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"));
		FVector OutPosition;
		FRotator OutRotation;
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"),LeftHandTransform.GetLocation(),FRotator::ZeroRotator,OutPosition,OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}
	
	
}
