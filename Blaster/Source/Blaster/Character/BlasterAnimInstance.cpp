// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"

#include "BlasterCharacter.h"

//引入移动组件的头文件
#include "GameFramework/CharacterMovementComponent.h"

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

	
}
