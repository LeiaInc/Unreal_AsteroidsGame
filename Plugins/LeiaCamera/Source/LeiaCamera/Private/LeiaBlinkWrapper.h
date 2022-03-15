// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class LeiaBlinkWrapper
{
public:
	LeiaBlinkWrapper();
	~LeiaBlinkWrapper();

	void Start();
	int Tick(FVector& primaryFace, float deltaTime, float cameraCenterX, float cameraCenterY);
	void Shutdown();

private:
	FVector lastPrimaryFace;
	FVector lastMouse;
	int lastFaceCount;
	float initDelay = .1f;
};
