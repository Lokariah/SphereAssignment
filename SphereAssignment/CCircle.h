#pragma once
#include <TL-Engine.h>	// TL-Engine include file and namespace
#include <string>

struct vector3 {
	float x;
	float y;
	float z;
};

struct vector2 {
	float x;
	float y;

};

struct CircleUpdateData {
	vector2 pos = { 0.0f, 0.0f };					//8
	vector2 velocity = { 0.0f, 0.0f };				//16
	float radius = 10.0f;							//20
	int id = -1;									//24
	int hp = 100.0f;								//28
	float padding;									//32
};
vector2 operator+ (const vector2& x, const vector2& y);

vector2 operator- (const vector2& x, const vector2& y);

vector2 operator* (const vector2& x, float y);

vector2 operator* (float y, const vector2& x);

vector2 operator/ (const vector2& x, float& y);


class CCircle
{
public:
	CCircle();
	CCircle(bool bUpdate, tle::IMesh* SphereMesh, int id, CircleUpdateData* UpdateData);
	CCircle(CCircle& circle);
	CCircle(const CCircle& circle);
	void MomentumUpdate();
	void MomentumUpdate(float frameTime);
	void PositionSync();
	void CollisionResolution(vector2 newPos, vector2 newMomentum);
	void FlipHoriMomentum(bool bRight, const float leftBarrier, const float rightBarrier);
	void FlipVertMomentum(bool bTop, const float topBarrier, const float bottomBarrier);

	vector2 GetPos() { return updateData->pos; }
	vector2 GetVelocity() { return updateData->velocity; }
	float GetRadius() { return updateData->radius; }

	CircleUpdateData* updateData;

private:
	tle::IModel* sphereModel = nullptr;
	tle::IMesh* sphereMesh = nullptr;

	bool bMoving = false;

	std::string name = "Error";
	int sphereId = -1;
	//int hp = 100;
	vector3 colour = { 1.0f, 1.0f, 1.0f };
};

