#pragma once
#include "CCircle.h"
#include <Windows.h>
#include <random>



CCircle::CCircle()
{
}

CCircle::CCircle(bool bUpdate/*, tle::IMesh* SphereMesh*/, int id, CircleUpdateData* UpdateData)
{
	bMoving = bUpdate;
	//sphereMesh = SphereMesh;
	sphereId = id;
	updateData = UpdateData;
	name = id;
	if (bUpdate) colour = { 1.0f, 0.5f, 0.5f };
	else colour = { 0.5f, 0.0f, 1.0f };
	//sphereModel = sphereMesh->CreateModel(updateData->pos.x, updateData->pos.y, 0.0f);
	//if (bUpdate)sphereModel->SetSkin("RedBall.jpg");
	//else sphereModel->SetSkin("Baize.jpg");
}

CCircle::CCircle(CCircle& circle)
{
	updateData = circle.updateData;
	sphereId = circle.sphereId;
	bMoving = bMoving;
	name = circle.name;
	colour = circle.colour;
	//sphereMesh = circle.sphereMesh;
	//sphereModel = circle.sphereModel;
}

CCircle::CCircle(const CCircle& circle) {
	updateData = circle.updateData;
	sphereId = circle.sphereId;
	bMoving = bMoving;
	name = circle.name;
	colour = circle.colour;
	//sphereMesh = circle.sphereMesh;
	//sphereModel = circle.sphereModel;
}

void CCircle::MomentumUpdate()
{
	updateData->pos.x += updateData->velocity.x;
	updateData->pos.y += updateData->velocity.y;
	//sphereModel->SetPosition(updateData->pos.x, updateData->pos.y, 0.0f);
}

void CCircle::MomentumUpdate(float frameTime)
{
	updateData->pos.x += updateData->velocity.x * frameTime;
	updateData->pos.y += updateData->velocity.y * frameTime;
	//sphereModel->SetPosition(updateData->pos.x, updateData->pos.y, 0.0f);
}

void CCircle::PositionSync()
{
	//sphereModel->SetPosition(updateData->pos.x, updateData->pos.y, 0.0f);
}

void CCircle::CollisionResolution(vector2 newPos, vector2 newMomentum)
{
	updateData->pos = newPos;
	updateData->velocity = newMomentum;
}

void CCircle::FlipHoriMomentum(bool bRight, const float leftBarrier, const float rightBarrier)
{
	updateData->velocity.x = -updateData->velocity.x;
	if (bRight) updateData->pos.x = rightBarrier;
	else updateData->pos.x = leftBarrier;
}

void CCircle::FlipVertMomentum(bool bTop, const float topBarrier, const float bottomBarrier)
{
	updateData->velocity.y = -updateData->velocity.y;
	if (bTop) updateData->pos.y = topBarrier;
	else updateData->pos.y = bottomBarrier;
}

vector2 operator+(const vector2& x, const vector2& y)
{
	return { x.x + y.x, x.y + y.y };
}

vector2 operator-(const vector2& x, const vector2& y)
{
	return { x.x - y.x, x.y - y.y };
}

vector2 operator*(const vector2& x, float y)
{
	return { x.x * y, x.y * y };
}

vector2 operator*(float y, const vector2& x)
{
	return { x.x * y, x.y * y };
}

vector2 operator/(const vector2& x, float& y)
{
	return { x.x / y, x.y / y };
}
