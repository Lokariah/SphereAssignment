#pragma once
#include "CCircle.h"
#include "Timer.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <Windows.h>
#include <random>

const bool visualisation = true;

const int MAX_WORKERS = 32;

const int CIRCLE_AMOUNT = 100000;
const float X_MIN_COORD = -5000.0f;
const float X_MAX_COORD = 5000.0f;
const float Y_MIN_COORD = -5000.0f;
const float Y_MAX_COORD = 5000.0f;

const float xVelocityPosLimit = 5.0f;
const float xVelocityNegLimit = -5.0f;
const float yVelocityPosLimit = 5.0f;
const float yVelocityNegLimit = -5.0f;

struct Thread {
	std::thread thread;
	std::condition_variable bAvaliableWork;
	std::mutex lock;
};

struct CollisionWork {
	bool bComplete = true;
	std::vector<CircleUpdateData*> dynamicSpheresUpdateData;
	int dynamicSphereStart;
	int numDynamicSpheres;
	std::vector<CircleUpdateData*> staticSpheresUpdateData;
	float frameTime;
};

std::pair<Thread, CollisionWork> collisionWorkers[MAX_WORKERS];
int numWorkers = 0;
Timer timer;


void Setup(std::vector<CCircle>& staticSpheres, std::vector<CCircle>& dynamicSpheres, std::vector<CircleUpdateData*>& staticSpheresUpdateData, std::vector<CircleUpdateData*>& dynamicSpheresUpdateData/*, I3DEngine* myEngine*/);
void collisionThread(int thread);
void ThreadUpdate(std::vector<CircleUpdateData*>& staticSpheresUpdateData, std::vector<CircleUpdateData*>& dynamicSpheresUpdateData, int dynamicSphereStart, int dynamicSpheresAmount, float frameTime);
bool CollisionDetection(CircleUpdateData* staticSphere, CircleUpdateData* dynamicSphere);
bool SortCondition(CircleUpdateData* sphereA, CircleUpdateData* sphereB);
float VectorDistance(vector2 vector);

int main() {

	//Finds out avaliable cores for current machine and dispatches appropiate amount of threads.
	numWorkers = std::thread::hardware_concurrency();
	if (numWorkers == 0) numWorkers = 8;
	numWorkers--;
	for (int i = 0; i < numWorkers; i++) {
		collisionWorkers[i].first.thread = std::thread(&collisionThread, i);
	}

	std::vector<CCircle> staticSpheres;
	std::vector<CCircle> dynamicSpheres;
	std::vector<CircleUpdateData*> staticSpheresUpdateData;
	std::vector<CircleUpdateData*> dynamicSpheresUpdateData;
	CircleUpdateData* check = new CircleUpdateData;

	Setup(staticSpheres, dynamicSpheres, staticSpheresUpdateData, dynamicSpheresUpdateData/*, myEngine*/);
	timer.Start();

	while (true) {
		timer.Tick();
		float frameTime = timer.FrameTime();
		std::cout << "Frame took " << frameTime << std::endl;

		//Sorts dynamic spheres for no current benefit but will benefit moving collision when implemented.
		std::sort(dynamicSpheresUpdateData.begin(), dynamicSpheresUpdateData.end(), [](CircleUpdateData* a, CircleUpdateData* b)
			{
				return a->pos.x < b->pos.x;
			});

		//Sets up each threads work for the frame and sets them off.
		int chunkAmount = dynamicSpheresUpdateData.size() / (numWorkers + 1);
		for (int i = 0; i < numWorkers; i++) {
			auto& work = collisionWorkers[i].second;
			work.dynamicSpheresUpdateData = dynamicSpheresUpdateData;
			work.dynamicSphereStart = i * chunkAmount;
			work.numDynamicSpheres = chunkAmount;
			work.staticSpheresUpdateData = staticSpheresUpdateData;
			work.frameTime = frameTime;

			auto& workThread = collisionWorkers[i].first;
			{
				std::unique_lock<std::mutex> lock(workThread.lock);
				work.bComplete = false;
			}

			workThread.bAvaliableWork.notify_one();
		}

		//Runs remaining spheres collision on main thread
		int remainingSpheres = (dynamicSpheresUpdateData.size() - chunkAmount * numWorkers) - 1;
		ThreadUpdate(staticSpheresUpdateData, dynamicSpheresUpdateData, chunkAmount * numWorkers, remainingSpheres, frameTime);

		//Waits for all threads to sync back up
		for (int i = 0; i < numWorkers; i++) {
			auto& workThread = collisionWorkers[i].first;
			auto& work = collisionWorkers[i].second;

			std::unique_lock<std::mutex> lock(workThread.lock);
			workThread.bAvaliableWork.wait(lock, [&]() {return work.bComplete; });
		}
	}

	for (int i = 0; i < numWorkers; i++) {
		collisionWorkers[i].first.thread.detach();
	}

	for (int i = 0; i < staticSpheresUpdateData.size(); i++) {
		delete staticSpheresUpdateData.at(i);
	}
	for (int i = 0; i < dynamicSpheresUpdateData.size(); i++) {
		delete dynamicSpheresUpdateData.at(i);
	}
	delete check;
	// Delete the 3D engine now we are finished with it
	return 1;
}


void Setup(std::vector<CCircle>& staticSpheres, std::vector<CCircle>& dynamicSpheres, std::vector<CircleUpdateData*>& staticSpheresUpdateData, std::vector<CircleUpdateData*>& dynamicSpheresUpdateData/*, I3DEngine* myEngine*/) {
	int halfAmount = CIRCLE_AMOUNT / 2;
	int remainingAmount = CIRCLE_AMOUNT - halfAmount;

	std::default_random_engine gen;
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	gen.seed(currTime);

	for (int i = 0; i < halfAmount; i++) {
		CircleUpdateData* tempUpdate = new CircleUpdateData();
		std::uniform_real_distribution<> xPosDistribution(X_MIN_COORD, X_MAX_COORD);
		std::uniform_real_distribution<> yPosDistribution(Y_MIN_COORD, Y_MAX_COORD);

		tempUpdate->pos = { float(xPosDistribution(gen)), float(yPosDistribution(gen)) };
		tempUpdate->velocity = { 0.0f, 0.0f };
		tempUpdate->radius = 10;
		tempUpdate->id = i;
		staticSpheresUpdateData.emplace_back(tempUpdate);
		staticSpheres.emplace_back(CCircle{ false/*, sphereMesh*/, i, tempUpdate });
	}
	for (int i = 0; i < remainingAmount; i++) {

		CircleUpdateData* tempUpdate = new CircleUpdateData();
		std::uniform_real_distribution<> xPosDistribution(X_MIN_COORD, X_MAX_COORD);
		std::uniform_real_distribution<> yPosDistribution(Y_MIN_COORD, Y_MAX_COORD);
		std::uniform_real_distribution<> xVelocDistribution(xVelocityNegLimit, xVelocityPosLimit);
		std::uniform_real_distribution<> yVelocDistribution(yVelocityNegLimit, yVelocityPosLimit);

		tempUpdate->pos = { float(xPosDistribution(gen)), float(yPosDistribution(gen)) };
		tempUpdate->velocity = { float(xVelocDistribution(gen)), float(yVelocDistribution(gen)) };
		tempUpdate->radius = 10;
		tempUpdate->id = i;
		dynamicSpheresUpdateData.emplace_back(tempUpdate);
		dynamicSpheres.emplace_back(CCircle{ true/*, sphereMesh*/, i, tempUpdate });
	}

	std::sort(staticSpheresUpdateData.begin(), staticSpheresUpdateData.end(), [](CircleUpdateData* a, CircleUpdateData* b)
		{
			return a->pos.x < b->pos.x;
		});
}

void collisionThread(int thread) {
	auto& worker = collisionWorkers[thread].first;
	auto& work = collisionWorkers[thread].second;
	while (true) {
		{
			std::unique_lock<std::mutex> lock(worker.lock);
			worker.bAvaliableWork.wait(lock, [&]() {return !work.bComplete; });
		}
		//collision work
		ThreadUpdate(work.staticSpheresUpdateData, work.dynamicSpheresUpdateData, work.dynamicSphereStart, work.numDynamicSpheres, work.frameTime);

		{
			std::unique_lock<std::mutex> lock(worker.lock);
			work.bComplete = true;
		}

		worker.bAvaliableWork.notify_one();
	}
}

void ThreadUpdate(std::vector<CircleUpdateData*>& staticSpheresUpdateData, std::vector<CircleUpdateData*>& dynamicSpheresUpdateData, int dynamicSphereStart, int dynamicSpheresAmount, float frameTime) {
	CircleUpdateData* check = new CircleUpdateData;

	for (int i = 0; i < dynamicSpheresAmount; i++) {
		auto currDynamicSphere = dynamicSpheresUpdateData.at(dynamicSphereStart + i);
		//currDynamicSphere.MomentumUpdate(frameTime);
		currDynamicSphere->pos.x += currDynamicSphere->velocity.x;// * frameTime;
		currDynamicSphere->pos.y += currDynamicSphere->velocity.y;// * frameTime;

		check->pos = currDynamicSphere->pos;
		check->velocity = { 0.0f, 0.0f };

		//Retrieves first sphere where the comparison fails to sweep left and right from
		auto currStaticSphere = std::lower_bound(staticSpheresUpdateData.begin(), staticSpheresUpdateData.end(), check, [](CircleUpdateData* a, CircleUpdateData* b)
			{
				return a->pos.x < b->pos.x;
			});

		if (currStaticSphere != staticSpheresUpdateData.end()) {

			auto sweepRight = currStaticSphere;

			const float dynamicX = currDynamicSphere->pos.x;
			const float dynamicRadius = currDynamicSphere->radius;
			float xDiff = abs((*sweepRight)->pos.x - dynamicX);

			//Rightwards sweep until collective radiuses is greater than x axis distance between 
			while (xDiff < ((*sweepRight)->radius + dynamicRadius)) {

				if (CollisionDetection((*sweepRight), currDynamicSphere)) {
					//std::cout << "collisionOccured between sphere " << currDynamicSphere->id << " with hp " << currDynamicSphere->hp << " and " << (*sweepRight)->id << " with hp " << (*sweepRight)->hp << " at " << timer.TotalTime() << "\n";
				}
				if (sweepRight != staticSpheresUpdateData.end()) {
					sweepRight++;
					if (sweepRight == staticSpheresUpdateData.end())break;
					xDiff = abs((*sweepRight)->pos.x - dynamicX);
				}
			}
			auto sweepLeft = currStaticSphere;

			xDiff = abs(dynamicX - (*sweepLeft)->pos.x);

			//Rightwards sweep until collective radiuses is greater than x axis distance between 
			while (xDiff < ((*sweepLeft)->radius + dynamicRadius)) {

				if (CollisionDetection((*sweepLeft), currDynamicSphere)) {
					//std::cout << "collisionOccured between sphere " << currDynamicSphere->id << " with hp " << currDynamicSphere->hp << " and " << (*sweepLeft)->id << " with hp " << (*sweepLeft)->hp << " at " << timer.TotalTime() << "\n";
				}
				if (sweepLeft != staticSpheresUpdateData.begin()) {
					sweepLeft--;
					if (sweepLeft == staticSpheresUpdateData.begin()) break;
					xDiff = abs(dynamicX - (*sweepLeft)->pos.x);
				}
				else break;
			}
		}

		//Wall boundry collision code
		const vector2 spherePos = currDynamicSphere->pos;
		bool bVertUpdate = false;
		bool bTopBreach = false;
		bool bHoriUpdate = false;
		bool bRightBreach = false;

		if (spherePos.x >= X_MAX_COORD) {
			bHoriUpdate = true;
			bRightBreach = true;
		}
		else if (spherePos.x <= X_MIN_COORD) bHoriUpdate = true;

		if (spherePos.y >= Y_MAX_COORD) {
			bVertUpdate = true;
			bTopBreach = true;
		}
		else if (spherePos.y <= Y_MIN_COORD) bVertUpdate = true;

		if (bHoriUpdate) {
			if (bRightBreach) currDynamicSphere->pos.x = X_MAX_COORD;
			else currDynamicSphere->pos.x = X_MIN_COORD;
			currDynamicSphere->velocity.x = -currDynamicSphere->velocity.x;
		}
		if (bVertUpdate) {
			if (bTopBreach) currDynamicSphere->pos.y = Y_MAX_COORD;
			else currDynamicSphere->pos.y = Y_MIN_COORD;
			currDynamicSphere->velocity.y = -currDynamicSphere->velocity.y;
		}

	}
	delete check;
}

//void Update(std::vector<CircleUpdateData*>& staticSpheresUpdateData, std::vector<CircleUpdateData*>& dynamicSpheresUpdateData, std::vector<CCircle>& dynamicSpheres, CircleUpdateData* check, float frameTime) {
//
//	for (int i = 0; i < dynamicSpheresUpdateData.size(); i++) {
//
//		dynamicSpheres.at(i).MomentumUpdate(frameTime);
//
//		check->pos = dynamicSpheresUpdateData.at(i)->pos;
//		check->velocity = { 0.0f, 0.0f };
//
//		auto currStaticSphere = std::lower_bound(staticSpheresUpdateData.begin(), staticSpheresUpdateData.end(), check, [](CircleUpdateData* a, CircleUpdateData* b)
//			{
//				return a->pos.x < b->pos.x;
//			});
//
//		if (currStaticSphere != staticSpheresUpdateData.end()) {
//
//			auto sweepRight = currStaticSphere;
//
//			const float dynamicX = dynamicSpheresUpdateData.at(i)->pos.x;
//			const float dynamicRadius = dynamicSpheresUpdateData.at(i)->radius;
//			float xDiff = abs((*sweepRight)->pos.x - dynamicX);
//
//			while (xDiff < ((*sweepRight)->radius + dynamicRadius)) {
//
//				if (CollisionDetection((*sweepRight), dynamicSpheresUpdateData.at(i))) {
//				}
//				if (sweepRight != staticSpheresUpdateData.end()) {
//					sweepRight++;
//					if (sweepRight == staticSpheresUpdateData.end())break;
//					xDiff = abs((*sweepRight)->pos.x - dynamicX);
//				}
//			}
//			auto sweepLeft = currStaticSphere;
//
//			xDiff = abs(dynamicX - (*sweepLeft)->pos.x);
//			while (xDiff < ((*sweepLeft)->radius + dynamicRadius)) {
//
//				if (CollisionDetection((*sweepLeft), dynamicSpheresUpdateData.at(i))) {
//				}
//				if (sweepLeft != staticSpheresUpdateData.begin()) {
//					sweepLeft--;
//					if (sweepLeft == staticSpheresUpdateData.begin()) break;
//					xDiff = abs(dynamicX - (*sweepLeft)->pos.x);
//				}
//				else break;
//			}
//		}
//
//		const vector2 spherePos = dynamicSpheresUpdateData.at(i)->pos;
//		bool bVertUpdate = false;
//		bool bTopBreach = false;
//		bool bHoriUpdate = false;
//		bool bRightBreach = false;
//
//		if (spherePos.x >= X_MAX_COORD) {
//			bHoriUpdate = true;
//			bRightBreach = true;
//		}
//		else if (spherePos.x <= X_MIN_COORD) bHoriUpdate = true;
//
//		if (spherePos.y >= Y_MAX_COORD) {
//			bVertUpdate = true;
//			bTopBreach = true;
//		}
//		else if (spherePos.y <= Y_MIN_COORD) bVertUpdate = true;
//
//		if (bHoriUpdate) {
//			if (bRightBreach) dynamicSpheresUpdateData.at(i)->pos.x = X_MAX_COORD;
//			else dynamicSpheresUpdateData.at(i)->pos.x = X_MIN_COORD;
//			dynamicSpheresUpdateData.at(i)->velocity.x = -dynamicSpheresUpdateData.at(i)->velocity.x;
//		}
//		if (bVertUpdate) {
//			if (bTopBreach) dynamicSpheresUpdateData.at(i)->pos.y = Y_MAX_COORD;
//			else dynamicSpheresUpdateData.at(i)->pos.y = Y_MIN_COORD;
//			dynamicSpheresUpdateData.at(i)->velocity.y = -dynamicSpheresUpdateData.at(i)->velocity.y;
//		}
//
//	}
//}

bool CollisionDetection(CircleUpdateData* staticSphere, CircleUpdateData* dynamicSphere) {
	vector2 staticSpherePos = staticSphere->pos;
	vector2 dynamicSpherePos = dynamicSphere->pos;
	vector2 vectBetweenSpheres = staticSpherePos - dynamicSpherePos;

	//Checks collision occurs
	float vectDist = VectorDistance(vectBetweenSpheres);
	float sphereRadiusCombined = staticSphere->radius + dynamicSphere->radius;
	if (vectDist <= sphereRadiusCombined) {
		//Works out reflected vector
		vector2 normVectorBetweenSpheres = vectBetweenSpheres / vectDist;
		float dotSpheresVectorNormVector = vectBetweenSpheres.x * normVectorBetweenSpheres.x + vectBetweenSpheres.y * normVectorBetweenSpheres.y;
		vector2 reflectedDynamicMomentum = vectBetweenSpheres - 2 * dotSpheresVectorNormVector * normVectorBetweenSpheres;

		//Works out new position in direction of reflected vector
		float reflectDist = VectorDistance(reflectedDynamicMomentum);
		vector2 normReflectedVec = reflectedDynamicMomentum / reflectDist;

		vector2 newPosition = dynamicSpherePos + (normReflectedVec * (vectDist - sphereRadiusCombined + 0.1f) * 0.5f);

		//Preserves momentum from before collision
		float momentumDist = VectorDistance(dynamicSphere->velocity);
		dynamicSphere->pos = newPosition;
		dynamicSphere->velocity = normReflectedVec * momentumDist;
		return true;
	}
	else return false;
}

bool SortCondition(CircleUpdateData* sphereA, CircleUpdateData* sphereB) {
	return sphereA->pos.x < sphereB->pos.x;
}

float VectorDistance(vector2 vector) {
	return sqrt(vector.x * vector.x + vector.y * vector.y);
}