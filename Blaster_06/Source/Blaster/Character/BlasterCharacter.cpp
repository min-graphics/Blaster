// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"

#include "Blaster/Blaster.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/Weapon/Weapon.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Particles/ParticleSystemComponent.h"

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

	//设置胶囊体，网格体的碰撞
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility,ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f,0.f,850.f);
	
	//默认初始是不转向的
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	//溶解时间轴
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}


void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	if(GetLocalRole() == ENetRole::ROLE_SimulatedProxy)
	{
		SimProxiesTurn();
	}
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::Elim()
{
	if(Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastElim();
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFinished,
		ElimDelay
	);
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	if(BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}
	bElimmed=true;
	PlayElimMontage();

	//Start dissolve effect
	if(DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance=UMaterialInstanceDynamic::Create(DissolveMaterialInstance,this);
		GetMesh()->SetMaterial(0,DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"),0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"),200.f);
	}
	StartDissolve();

	//Disable character movement
	GetCharacterMovement()->DisableMovement();//这个会禁止移动，但是可以拖动视角。
	GetCharacterMovement()->StopMovementImmediately();//不能再用鼠标旋转角色，与上面两者结合可以让角色停在原地。

	//在冷却时间内，禁用部分输入
	bDisableGameplay = true;
	if(Combat)
	{
		Combat->FireButtonPressed(false);
	}
	if(BlasterPlayerController)
	{
		//禁用输入，不能再开火
		DisableInput(BlasterPlayerController);
	}
	//关闭碰撞，包括关闭胶囊体碰撞和mesh碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//spawn elim bot
	if(ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X,GetActorLocation().Y,GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
			);
	}
	if(ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
}
void ABlasterCharacter::ElimTimerFinished()
{
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if(BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this,Controller);
	}
}
void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();
	if(ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState()!=MatchState::InProgress;
	if(Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}
void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if(BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health,MaxHealth);
	}
}

void ABlasterCharacter::PollInit()
{
	if(BlasterPlayerState==nullptr)
	{
		BlasterPlayerState=GetPlayerState<ABlasterPlayerState>();
		if(BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToScore(0);
		}
	}
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if(bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	//Enet ROLE在在枚举中是有顺序的，none<simulatedproxy<autonomousproxy<authority<max
	if(GetLocalRole()>ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if(TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
	
}
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateHUDHealth();
	if(HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this,&ABlasterCharacter::ReceiveDamage);
	}
}
void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RotateInPlace(DeltaTime);
	//else//意味着是模拟代理的情况
	//{
	//	SimProxiesTurn();
	//}
	HideCameraIfCharacterClose();

	PollInit();
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
	PlayerInputComponent->BindAction("Fire",IE_Pressed,this,&ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire",IE_Released,this,&ABlasterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload",IE_Pressed,this,&ABlasterCharacter::ReloadButtonPressed);

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
	DOREPLIFETIME(ABlasterCharacter,Health);
	DOREPLIFETIME(ABlasterCharacter,bDisableGameplay);
	
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if(Combat)
	{
		Combat->Character=this;
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if(Combat == nullptr||Combat->EquippedWeapon==nullptr)
	{
		return;
	}
	//创建一个动画实例
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		//播放时要选择正确的部分来播放，根据bAiming
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if(Combat == nullptr||Combat->EquippedWeapon==nullptr)
	{
		return;
	}
	//创建一个动画实例
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		//播放时要选择正确的部分来播放，根据bAiming
		FName SectionName;
		
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	//创建一个动画实例
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	//这个保留，因为角色动画是有武器的，没武器会很奇怪
	if(Combat == nullptr||Combat->EquippedWeapon==nullptr)
	{
		return;
	}
	//创建一个动画实例
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		//播放时要选择正确的部分来播放，根据bAiming
		FName SectionName("FromFront");
		//switch(SectionName)
	//	{
			
	//	}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health-Damage,0.f,MaxHealth);
	//onclambhealth只会在客户端调用，而不是服务器上。ReceiveDamage是在服务器上调用的
	UpdateHUDHealth();
	PlayHitReactMontage();

	if(Health == 0.f)
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if(BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this,BlasterPlayerController,AttackerController);
		}
	}
}

void ABlasterCharacter::MoveForward(float Value)
{
	if(bDisableGameplay)return;
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
	if(bDisableGameplay)return;
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
	if(bDisableGameplay)return;
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
	if(bDisableGameplay)return;
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

void ABlasterCharacter::ReloadButtonPressed()
{
	if(bDisableGameplay)return;
	if(Combat)
	{
		Combat->Reload();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if(bDisableGameplay)return;
	if(Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if(bDisableGameplay)return;
	if(Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if(AO_Pitch>90.f && !IsLocallyControlled())
	{
		//map pitch from [270,360) to [-90,0)
		FVector2D InRange(270.f,360.f);
		FVector2D OutRange(-90.f,0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange,OutRange,AO_Pitch);
	}
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}


//我们只计算静止不动时的yaw和pitch
void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if(Combat&&Combat->EquippedWeapon==nullptr) return;
	//获取速度，之后定义速度，其实是和蓝图一模一样的。
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if(Speed == 0.f&& !bIsInAir)//standing still.not jumping
	{
		bRotateRootBone = true;
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
		bRotateRootBone=false;
		StartingAimRotation=FRotator(0.f,GetBaseAimRotation().Yaw,0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw=true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABlasterCharacter::SimProxiesTurn()
{
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	bRotateRootBone = false;
	float Speed = CalculateSpeed();
	if(Speed>0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation,ProxyRotationLastFrame).Yaw;

	if(FMath::Abs(ProxyYaw)>TurnThreshold)
	{
		if(ProxyYaw>TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if(ProxyYaw<-TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::Jump()
{
	if(bDisableGameplay)return;
	if(bIsCrouched)
	{
		UnCrouch();	
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if(bDisableGameplay)return;
	if(Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if(bDisableGameplay)return;
	if(Combat)
	{
		Combat->FireButtonPressed(false);
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
/*
void ABlasterCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}
*/
void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if(!IsLocallyControlled()) return;
	if((FollowCamera->GetComponentLocation()-GetActorLocation()).Size()<CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if(DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"),DissolveValue);
	}
	
}

//c++版的时间轴
void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this,&ABlasterCharacter::UpdateDissolveMaterial);
	if(DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve,DissolveTrack);
		DissolveTimeline->Play();
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
//第75集,用来debug枪口旋转的射线。
FVector ABlasterCharacter::GetHitTarget() const
{
	if(Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if(Combat==nullptr)return ECombatState::ECS_MAX;
	return Combat->CombatState;
}





