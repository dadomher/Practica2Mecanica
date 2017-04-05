#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw_gl3.h>
#include <vector>
#include <glm\glm.hpp>
#include <iostream>

using namespace std;
using namespace glm;

float forceX = 0.0f;
float forceY = -9.8f;
float forceZ = 0.0f;

//Normales de los planos del eje X
/*float normalXRight[3] = { 0.f };
float normalXLeft[3] = { 0.f };*/
vec3 normalXRight;
vec3 normalXLeft;

//Normales de los planos del eje Y
/*float normalYDown[3] = { 0.f };
float normalYTop[3] = { 0.f };*/
vec3 normalYDown;
vec3 normalYTop;

//Normales de los planos del eje Z
/*float normalZFront[3] = { 0.f };
float normalZBack[3] = { 0.f };*/
vec3 normalZFront;
vec3 normalZBack;

float dDown, dTop, dRight, dLeft, dFront, dBack;

struct particle {

	vec3 position;
	vec3 postPos;
	vec3 vel;
	vec3 postVel;

	int row, col;

	vector<particle*> nearPart;

	float mass;

};

int maxPart = 14 * 18;
particle *particles = new particle[14*18];
float partDist = 0.5f;

namespace ClothMesh {
	extern void setupClothMesh();
	extern void cleanupClothMesh();
	extern void updateClothMesh(float* array_data);
	extern void drawClothMesh();
}

namespace LilSpheres {
	extern const int maxParticles;
	extern void setupParticles(int numTotalParticles, float radius = 0.05f);
	extern void cleanupParticles();
	extern void updateParticles(int startIdx, int count, float* array_data);
	extern void drawParticles(int startIdx, int count);
}

bool show_test_window = false;
void GUI() {
	{	//FrameRate
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		//TODO
	}

	// ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if(show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

void NormalPlane(vec3 pointA, vec3 pointB, vec3 pointC, vec3* normal) {

	/*float vectorA[3] = { pointA[0] - pointB[0], pointA[1] - pointB[1], pointA[2] - pointB[2] };
	float vectorB[3] = { pointC[0] - pointB[0], pointC[1] - pointB[1], pointC[2] - pointB[2] };*/
	vec3 vectorA = vec3(pointA.x - pointB.x, pointA.y - pointB.y, pointA.z - pointB.z);
	vec3 vectorB = vec3(pointC.x - pointB.x, pointC.y - pointB.y, pointC.z - pointB.z);

	/*normal[0] = vectorA[1] * vectorB[2] - vectorA[2] * vectorB[1];
	normal[1] = vectorA[2] * vectorB[0] - vectorA[0] * vectorB[2];
	normal[2] = vectorA[0] * vectorB[1] - vectorA[1] * vectorB[0];*/
	normal->x = vectorA.y * vectorB.z - vectorA.z * vectorB.y;
	normal->y = vectorA.z * vectorB.x - vectorA.x * vectorB.z;
	normal->z = vectorA.x * vectorB.y - vectorA.y * vectorB.x;

	//Normalizar el vector
	float modulo = sqrt(normal->x * normal->x + normal->y * normal->y + normal->z * normal->z);
	normal->x /= modulo;
	normal->y /= modulo;
	normal->z /= modulo;
}

void findNearParts(particle *actualPart, particle *particles) {
	for (int i = 0; i < maxPart; i++) {
		/*if ((particles[i].row == actualPart->row + 1 || particles[i].row == actualPart->row - 1)
			|| (particles[i].row == actualPart->row + 2 || particles[i].row == actualPart->row - 2) 
			&& particles[i].col == actualPart->col) {
			actualPart->nearPart.push_back(&particles[i]);
		}
		if ((particles[i].col == actualPart->col + 1 || particles[i].col == actualPart->col - 1)
			|| (particles[i].col == actualPart->col + 2 || particles[i].col == actualPart->col - 2)
			&& particles[i].row == actualPart->row) {
			actualPart->nearPart.push_back(&particles[i]);
		}
		if (particles[i].col == actualPart->col + 1 && (particles[i].row == actualPart->row + 1 || particles[i].row == actualPart->row - 1)
			|| particles[i].col == actualPart->col - 1 && (particles[i].row == actualPart->row + 1 || particles[i].row == actualPart->row - 1)){
			actualPart->nearPart.push_back(&particles[i]);
		}*/

		if ((particles[i].row == actualPart->row + 1 || particles[i].row == actualPart->row - 1)
			&& particles[i].col == actualPart->col) {
			actualPart->nearPart.push_back(&particles[i]);
		}
		if ((particles[i].col == actualPart->col + 1 || particles[i].col == actualPart->col - 1)
			&& particles[i].row == actualPart->row) {
			actualPart->nearPart.push_back(&particles[i]);
		}
		if ((particles[i].row == actualPart->row + 2 || particles[i].row == actualPart->row - 2)
			&& particles[i].col == actualPart->col) {
			actualPart->nearPart.push_back(&particles[i]);
		}
		if ((particles[i].col == actualPart->col + 2 || particles[i].col == actualPart->col - 2)
			&& particles[i].row == actualPart->row) {
			actualPart->nearPart.push_back(&particles[i]);
			//cout << "PART R: " << actualPart->row << " PART C: " << actualPart->col << " Comp R " << particles[i].row << " Comp C " << particles[i].col << endl;
		}
		if (particles[i].col == actualPart->col + 1 && (particles[i].row == actualPart->row + 1 || particles[i].row == actualPart->row - 1)
			|| particles[i].col == actualPart->col - 1 && (particles[i].row == actualPart->row + 1 || particles[i].row == actualPart->row - 1)) {
			actualPart->nearPart.push_back(&particles[i]);
		}

		/*float diagonalDist = sqrt(pow(partDist, 2) + pow(partDist, 2));
		float distX = sqrt(pow(particles[i].position.x, 2) - pow(actualPart->position.x, 2));
		float distZ = sqrt(pow(particles[i].position.z, 2) - pow(actualPart->position.z, 2));
		if ((distX == partDist || distX == partDist*2) && distZ == 0) {
			actualPart->nearPart.push_back(&particles[i]);
		}
		if ((distZ == partDist || distZ == partDist*2) && distX == 0) {
			actualPart->nearPart.push_back(&particles[i]);
		}
		if (distX == diagonalDist && distZ == diagonalDist) {
			actualPart->nearPart.push_back(&particles[i]);
		}*/
	}
}

void PhysicsInit() {
	//Calcular la normal de los planos
	//Plano bajo
	vec3 pointA = vec3(-5.0f, 0.0f, -5.0f);
	vec3 pointB = vec3(-5.0f, 0.0f, 5.0f);
	vec3 pointC = vec3(5.0f, 0.0f, 5.0f);
	NormalPlane(pointA, pointB, pointC, &normalYDown);
	dDown = -(normalYDown.x * pointA.x) - (normalYDown.y * pointA.y) - (normalYDown.z * pointA.z);

	//Plano alto
	pointA = vec3(5.0f, 10.0f, 5.0f);
	pointB = vec3(-5.0f, 10.0f, 5.0f);
	pointC = vec3(-5.0f, 10.0f, -5.0f);
	NormalPlane(pointA, pointB, pointC, &normalYTop);
	dTop = -(normalYTop.x * pointA.x) - (normalYTop.y * pointA.y) - (normalYTop.z * pointA.z);

	//Plano derecha
	pointA = vec3(5.0f, 0.0f, -5.0f);
	pointB = vec3(5.0f, 0.0f, 5.0f);
	pointC = vec3(5.0f, 10.0f, 5.0f);
	NormalPlane(pointA, pointB, pointC, &normalXRight);
	dRight = -(normalXRight.x * pointA.x) - (normalXRight.y * pointA.y) - (normalXRight.z * pointA.z);

	//Plano izquierda
	pointA = vec3(-5.0f, 10.0f, 5.0f);
	pointB = vec3(-5.0f, 0.0f, 5.0f);
	pointC = vec3(-5.0f, 0.0f, -5.0f);
	NormalPlane(pointA, pointB, pointC, &normalXLeft);
	dLeft = -(normalXLeft.x * pointA.x) - (normalXLeft.y * pointA.y) - (normalXLeft.z * pointA.z);

	//Plano frontal
	pointA = vec3(5.0f, 0.0f, 5.0f);
	pointB = vec3(-5.0f, 0.0f, 5.0f);
	pointC = vec3(-5.0f, 10.0f, 5.0f);
	NormalPlane(pointA, pointB, pointC, &normalZFront);
	dFront = -(normalZFront.x * pointA.x) - (normalZFront.y * pointA.y) - (normalZFront.z * pointA.z);

	//Plano trasero
	pointA = vec3(-5.0f, 10.0f, -5.0f);
	pointB = vec3(-5.0f, 0.0f, -5.0f);
	pointC = vec3(5.0f, 0.0f, -5.0f);
	NormalPlane(pointA, pointB, pointC, &normalZBack);
	dBack = -(normalZBack.x * pointA.x) - (normalZBack.y * pointA.y) - (normalZBack.z * pointA.z);

	//TODO
	float initX = -5.0;
	float initZ = -5.0;
	int part = 0;
	for (int i = 0; i < 18; i++) {
		for (int j = 0; j < 14; j++) {
			particles[part].position = vec3(initX + partDist*j, 7.5f, initZ + partDist*i);
			particles[part].vel = vec3(0.0f, -3.0f, 0.0f);
			particles[part].row = j;
			particles[part].col = i;
			part++;
		}
	}
	for (int i = 0; i < maxPart; i++) {
		findNearParts(&particles[i], particles);
		if (i==37) {
			for (int j = 0; j < particles[i].nearPart.size(); j++) {
				cout << 1 << endl;
			}
		}
		
	}
	particles[0].position.y += 1;
	particles[0].vel.y = 0;
	particles[13].position.y += 1;
	particles[13].vel.y = 0;

}

void PhysicsUpdate(float dt) {
	//TODO
	for (int i = 0; i < maxPart; i++) {
		particles[i].postPos.x = particles[i].position.x + dt * particles[i].vel.x;
		particles[i].postPos.y = particles[i].position.y + dt * particles[i].vel.y;
		particles[i].postPos.z = particles[i].position.z + dt * particles[i].vel.z;

		//Calcular velocidad
		particles[i].postVel.x = particles[i].vel.x + dt * (forceX / particles[i].mass);
		particles[i].postVel.y = particles[i].vel.y + dt * (forceY / particles[i].mass);
		particles[i].postVel.z = particles[i].vel.z + dt * (forceZ / particles[i].mass);

		particles[i].position.x = particles[i].postPos.x;
		particles[i].position.y = particles[i].postPos.y;
		particles[i].position.z = particles[i].postPos.z;

		particles[i].vel.x = particles[i].postVel.x;
		particles[i].vel.y = particles[i].postVel.y;
		particles[i].vel.z = particles[i].postVel.z;
	}

	float *partVerts = new float[(14*18) * 3];
	for (int i = 0; i < (14 * 18); ++i) {
		partVerts[i * 3 + 0] = particles[i].position.x;
		partVerts[i * 3 + 1] = particles[i].position.y;
		partVerts[i * 3 + 2] = particles[i].position.z;
	}
	LilSpheres::updateParticles(0, (14 * 18), partVerts);
	ClothMesh::updateClothMesh(partVerts);
	delete[] partVerts;
}

void PhysicsCleanup() {
	//TODO
}