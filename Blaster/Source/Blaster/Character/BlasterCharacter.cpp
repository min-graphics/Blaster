// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	
	
	

}


void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}
//这里是设置角色的输入组件
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	
	//动作映射，键盘绑定
	PlayerInputComponent->BindAction("Jump",IE_Pressed,this,&ACharacter::Jump);

	//绑定轴，
	PlayerInputComponent->BindAxis("MoveForward",this,&ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight",this,&ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn",this,&ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp",this,&ABlasterCharacter::LookUp);

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

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}



