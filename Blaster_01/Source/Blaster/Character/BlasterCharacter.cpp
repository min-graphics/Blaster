// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	//创建类，声明类的父类，并且取名字
	CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	//绑定到mesh上
	CameraBoom->SetupAttachment(GetMesh());
	//设定长度为600.f
	CameraBoom->TargetArmLength=600.f;
	//可以用来控制旋转
	CameraBoom->bUsePawnControlRotation=true;

	//创建类，声明类的父类，并且取名字。
	FollowCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	//创建好类之后，定义好依附关系
	FollowCamera->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);
	//关闭旋转
	FollowCamera->bUsePawnControlRotation=false;

	//我们不希望角色和控制器一起转
	bUseControllerRotationYaw=false;
	//设置玩家朝向运动方向
	GetCharacterMovement()->bOrientRotationToMovement=true;

	//在构造函数中创建控件
	OverheadWidget=CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	//将控件绑定到根组件上
	OverheadWidget->SetupAttachment(RootComponent);

	//战斗系统初始化
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	//战斗系统被复制
	Combat->SetIsReplicated(true);

	//设置是否可以蹲伏
	GetCharacterMovement()->NavAgentProps.bCanCrouch=true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,ECollisionResponse::ECR_Ignore);
	GetCharacterMovement()->RotationRate = FRotator(0.f,0.f,850.f);

	
	//默认初始是不转向的
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

}


void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}
void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AimOffset(DeltaTime);
}



//这里是设置角色的输入组件
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	
	//动作映射，键盘绑定
	PlayerInputComponent->BindAction("Jump",IE_Pressed,this,&ABlasterCharacter::Jump);
	PlayerInputComponent->BindAction("Equip",IE_Pressed,this,&ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch",IE_Pressed,this,&ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim",IE_Pressed,this,&ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim",IE_Released,this,&ABlasterCharacter::AimButtonReleased);

	//绑定轴，
	PlayerInputComponent->BindAxis("MoveForward",this,&ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight",this,&ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn",this,&ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp",this,&ABlasterCharacter::LookUp);

}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//在武器类overlap函数当中初始化overlappingweapon
	DOREPLIFETIME_CONDITION(ABlasterCharacter,OverlappingWeapon,COND_OwnerOnly);
	
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if(Combat)
	{
		Combat->Character=this;
	}
}

void ABlasterCharacter::MoveForward(float Value)
{
	//当控制器不为空指针，并且有输入的时候
	if(Controller != nullptr && Value != 0.f)
	{
		//旋转是要控制控制器的旋转，不是人物的旋转。
		const FRotator YawRotation(0.f,Controller->GetControlRotation().Yaw,0.f);
		//绑定轴
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		//如果要控制速度的话，并不是在这里方向乘2，而是有专门的移动组件
		AddMovementInput(Direction,Value);
		
	}
		
}
void ABlasterCharacter::MoveRight(float Value)
{
	//当控制器不为空指针，并且有输入的时候
	if(Controller != nullptr && Value != 0.f)
	{
		//旋转是要控制控制器的旋转，不是人物的旋转。
		const FRotator YawRotation(0.f,Controller->GetControlRotation().Yaw,0.f);
		//绑定轴
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction,Value);
		
	}
}
void ABlasterCharacter::Turn(float Value)
{
	//其实就是角色蓝图里的获得控制器旋转
	AddControllerYawInput(Value);
}
void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

//装备武器弹药这些，要服务器来进行调度，不能让客户端决定什么时候拿起武器。而是客户端向服务器发出请求，服务器上先拿起了，再复制给客户端。
void ABlasterCharacter::EquipButtonPressed()
{
	//只会在服务器端调用这个事件，之后服务器再进行广播
	if(Combat)
	{
		if(HasAuthority())//服务器直接拿武器
		{
			Combat->EquipWeapon(OverlappingWeapon);	
		}
		else//客户端要请求服务器来拿武器
		{
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	//这个蹲伏函数是角色父类直接封装好的，可以直接用
	if(bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if(Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if(Combat)
	{
		Combat->SetAiming(false);
	}
}

//我们只计算静止不动时的yaw和pitch
void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if(Combat&&Combat->EquippedWeapon==nullptr) return;
	//获取速度，之后定义速度，其实是和蓝图一模一样的。
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if(Speed == 0.f&& !bIsInAir)//standing still.not jumping
	{
		FRotator CurrentAimRotation = FRotator(0.f,GetBaseAimRotation().Yaw,0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation,StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if(TurningInPlace==ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
		
	}
	if(Speed >0.f || bIsInAir)//runing ,or Jumping
	{
		StartingAimRotation=FRotator(0.f,GetBaseAimRotation().Yaw,0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw=true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	AO_Pitch = GetBaseAimRotation().Pitch;
	if(AO_Pitch>90.f && !IsLocallyControlled())
	{
		//map pitch from [270,360) to [-90,0)
		FVector2D InRange(270.f,360.f);
		FVector2D OutRange(-90.f,0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange,OutRange,AO_Pitch);
	}
}

void ABlasterCharacter::Jump()
{
	if(bIsCrouched)
	{
		UnCrouch();	
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if(LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
	
}
//这里要改变RPC的实现，
void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	//这里不用检查，因为RPC只会在服务器上调用
	if(Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);	
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if(AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if(AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if(TurningInPlace !=ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw,0.f,DeltaTime,4.f);
		AO_Yaw = InterpAO_Yaw;
		if(FMath::Abs(AO_Yaw)<15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f,GetBaseAimRotation().Yaw,0.f);
		}
	}
}

//只要在服务器上发生变化，新的值就会复制到客户端，
void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	
	OverlappingWeapon=Weapon;
	if(IsLocallyControlled())
	{
		if(OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
		
	
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()const
{
	return (Combat && Combat->bAiming);	
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if(Combat==nullptr)return nullptr;
	
	return Combat->EquippedWeapon;
}





