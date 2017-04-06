#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw_gl3.h>
#include <vector>
#include <glm\glm.hpp>
#include <iostream>

using namespace std;
using namespace glm;

float posSphere[3] = { 0.0f,1.0f,0.0f };
float radiusSphere = 1.0f;

float forceX = 0.0f;
float forceY = -0.8f;
float forceZ = 0.0f;

//Normales de los planos del eje X
vec3 normalXRight;
vec3 normalXLeft;

//Normales de los planos del eje Y
vec3 normalYDown;
vec3 normalYTop;

//Normales de los planos del eje Z
vec3 normalZFront;
vec3 normalZBack;

float dDown, dTop, dRight, dLeft, dFront, dBack;

//Parametros Stretch
float springConstStretch = 1.0f;
float dampingStretch = 0.1f;

//Parametros Bend
float springConstBend = 1.0f;
float dampingBend = 0.1f;

//Parametros Shear
float springConstShear = 1.0f;
float dampingShear = 0.1f;

struct particle {

	vec3 position;
	vec3 postPos;
	vec3 vel;
	vec3 postVel;

	vec3 force = vec3(0.0f);

	int row, col;

	//vector<particle*> nearPart;
	vector<pair<int,particle*>> nearPart; //Stretch = 0, Bend = 1, Shear = 2

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
	}

	{	//Propiedades
		ImGui::Begin("Propiedades Esfera:");
		ImGui::DragFloat3("Position", posSphere, 0.1f);
		ImGui::DragFloat("Radius", &radiusSphere, 0.1f, 0.1f, 10.0f);

		ImGui::Text("\nPropiedades Particulas:");
		ImGui::DragFloat("Distance", &partDist, 0.1f, 0.1f, 2.0f);

		ImGui::Text("\nPropiedades Stretch:");
		ImGui::DragFloat("Spring Stretch", &springConstStretch, 0.1f, 0.1f, 10.0f);
		ImGui::DragFloat("Damping Stretch", &dampingStretch, 0.1f, 0.1f, 10.0f);

		ImGui::Text("\nPropiedades Bend:");
		ImGui::DragFloat("Spring Bend", &springConstBend, 0.1f, 0.1f, 10.0f);
		ImGui::DragFloat("Damping Bend", &dampingBend, 0.1f, 0.1f, 10.0f);

		ImGui::Text("\nPropiedades Shear:");
		ImGui::DragFloat("Spring Shear", &springConstShear, 0.1f, 0.1f, 10.0f);
		ImGui::DragFloat("Damping Shear", &dampingShear, 0.1f, 0.1f, 10.0f);

		ImGui::End();
	}


	// ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if(show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

void NormalPlane(vec3 pointA, vec3 pointB, vec3 pointC, vec3* normal) {

	vec3 vectorA = vec3(pointA.x - pointB.x, pointA.y - pointB.y, pointA.z - pointB.z);
	vec3 vectorB = vec3(pointC.x - pointB.x, pointC.y - pointB.y, pointC.z - pointB.z);
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
	pair<int, particle*> newPart;
	for (int i = 0; i < maxPart; i++) {
		if ((particles[i].row == actualPart->row + 1 || particles[i].row == actualPart->row - 1)
			&& particles[i].col == actualPart->col) {
			newPart.first = 0; //Stretch
			newPart.second = &particles[i];
			actualPart->nearPart.push_back(newPart);
			//actualPart->nearPart.push_back(&particles[i]);
		}
		if ((particles[i].col == actualPart->col + 1 || particles[i].col == actualPart->col - 1)
			&& particles[i].row == actualPart->row) {
			newPart.first = 0; //Stretch
			newPart.second = &particles[i];
			actualPart->nearPart.push_back(newPart);
			//actualPart->nearPart.push_back(&particles[i]);
		}
		if ((particles[i].row == actualPart->row + 2 || particles[i].row == actualPart->row - 2)
			&& particles[i].col == actualPart->col) {
			newPart.first = 1; //Bend
			newPart.second = &particles[i];
			actualPart->nearPart.push_back(newPart);
			//actualPart->nearPart.push_back(&particles[i]);
		}
		if ((particles[i].col == actualPart->col + 2 || particles[i].col == actualPart->col - 2)
			&& particles[i].row == actualPart->row) {
			newPart.first = 1; //Bend
			newPart.second = &particles[i];
			actualPart->nearPart.push_back(newPart);
			//actualPart->nearPart.push_back(&particles[i]);
		}
		if (particles[i].col == actualPart->col + 1 && (particles[i].row == actualPart->row + 1 || particles[i].row == actualPart->row - 1)
			|| particles[i].col == actualPart->col - 1 && (particles[i].row == actualPart->row + 1 || particles[i].row == actualPart->row - 1)) {
			newPart.first = 2; //Shear
			newPart.second = &particles[i];
			actualPart->nearPart.push_back(newPart);
			//actualPart->nearPart.push_back(&particles[i]);
		}
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
	float initX = -2.0;
	float initZ = -5.0;
	int part = 0;
	for (int i = 0; i < 18; i++) {
		for (int j = 0; j < 14; j++) {
			particles[part].position = vec3(initX + partDist*j, 9.0f, initZ + partDist*i);
			particles[part].vel = vec3(0.0f, -3.0f, 0.0f);
			particles[part].row = j;
			particles[part].col = i;
			particles[part].mass = 1;
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
	particles[0].vel = vec3(0.0f);
	particles[13].vel = vec3(0.0f);

}

void PhysicsUpdate(float dt) {
	//TODO
	for (int i = 0; i < maxPart; i++) {
		if (i != 0 && i != 13) {
			//Calcular fuerzas con las particulas adyacentes
			particles[i].force = vec3(0.0f);
			for (int j = 0; j < particles[i].nearPart.size(); j++) {
				vec3 partVec = vec3(particles[i].position.x - particles[i].nearPart[j].second->position.x,
									particles[i].position.y - particles[i].nearPart[j].second->position.y,
									particles[i].position.z - particles[i].nearPart[j].second->position.z);

				float dist = sqrt(pow(partVec.x, 2) + pow(partVec.y, 2) + pow(partVec.z, 2));

				partVec.x /= dist;
				partVec.y /= dist;
				partVec.z /= dist;

				vec3 partVecVel = vec3(particles[i].vel.x - particles[i].nearPart[j].second->vel.x,
										particles[i].vel.y - particles[i].nearPart[j].second->vel.y,
										particles[i].vel.z - particles[i].nearPart[j].second->vel.z);

				if (particles[i].nearPart[j].first == 0) {
					particles[i].force += -(springConstStretch * (dist - partDist) + (dampingStretch * partVecVel) * (partVec))*(partVec);
				} else if (particles[i].nearPart[j].first == 1) {
					particles[i].force += -(springConstBend * (dist - partDist*2) + (dampingBend * partVecVel) * (partVec))*(partVec);
				} else if (particles[i].nearPart[j].first == 2) {
					particles[i].force += -(springConstShear * (dist - partDist) + (dampingShear * partVecVel) * (partVec))*(partVec);
				}

			}

			//Calcular posicion
			particles[i].postPos.x = particles[i].position.x + dt * particles[i].vel.x;
			particles[i].postPos.y = particles[i].position.y + dt * particles[i].vel.y;
			particles[i].postPos.z = particles[i].position.z + dt * particles[i].vel.z;

			//Calcular velocidad
			particles[i].postVel.x = particles[i].vel.x + dt * (forceX+particles[i].force.x / particles[i].mass);
			particles[i].postVel.y = particles[i].vel.y + dt * (forceY+particles[i].force.y / particles[i].mass);
			particles[i].postVel.z = particles[i].vel.z + dt * (forceZ+particles[i].force.z / particles[i].mass);

			//Detectar Colisiones
			float dotProductDown = (normalYDown[0] * particles[i].position.x + normalYDown[1] * particles[i].position.y + normalYDown[2] * particles[i].position.z);
			float dotProductPostDown = (normalYDown[0] * particles[i].postPos.x + normalYDown[1] * particles[i].postPos.y + normalYDown[2] * particles[i].postPos.z);

			//Plano bajo
			if ((dotProductDown + dDown) * (dotProductPostDown + dDown) <= 0) {

					float dotProductPostVelDown = (normalYDown[0] * particles[i].postVel.x + normalYDown[1] * particles[i].postVel.y + normalYDown[2] * particles[i].postVel.z);
					float dotProductVelDown = (normalYDown[0] * particles[i].vel.x + normalYDown[1] * particles[i].vel.y + normalYDown[2] * particles[i].vel.z);
					float normalVel[3] = { dotProductVelDown * normalYDown[0], dotProductVelDown * normalYDown[1], dotProductVelDown * normalYDown[2] };
					float tangVel[3] = { particles[i].vel.x - normalVel[0], particles[i].vel.y - normalVel[1], particles[i].vel.z - normalVel[2] };

					particles[i].postPos.x = particles[i].postPos.x - 1.1 * (dotProductPostDown + dDown)*normalYDown[0];
					particles[i].postPos.y = particles[i].postPos.y - 1.1 * (dotProductPostDown + dDown)*normalYDown[1];
					particles[i].postPos.z = particles[i].postPos.z - 1.1 * (dotProductPostDown + dDown)*normalYDown[2];

					particles[i].postVel.x = particles[i].postVel.x - 1.1 * (dotProductPostVelDown)*normalYDown[0];
					particles[i].postVel.y = particles[i].postVel.y - 1.1 * (dotProductPostVelDown)*normalYDown[1];
					particles[i].postVel.z = particles[i].postVel.z - 1.1 * (dotProductPostVelDown)*normalYDown[2];

					particles[i].postVel.x = particles[i].postVel.x - 1 * tangVel[0];
					particles[i].postVel.y = particles[i].postVel.y - 1 * tangVel[1];
					particles[i].postVel.z = particles[i].postVel.z - 1 * tangVel[2];
			}

			float dotProductTop = (normalYTop[0] * particles[i].position.x + normalYTop[1] * particles[i].position.y + normalYTop[2] * particles[i].position.z);
			float dotProductPostTop = (normalYTop[0] * particles[i].postPos.x + normalYTop[1] * particles[i].postPos.y + normalYTop[2] * particles[i].postPos.z);

			//Plano alto
			if ((dotProductTop + dTop) * (dotProductPostTop + dTop) <= 0) {

					float dotProductPostVelTop = (normalYTop[0] * particles[i].postVel.x + normalYTop[1] * particles[i].postVel.y + normalYTop[2] * particles[i].postVel.z);
					float dotProductVelTop = (normalYTop[0] * particles[i].vel.x + normalYTop[1] * particles[i].vel.y + normalYTop[2] * particles[i].vel.z);
					float normalVel[3] = { dotProductVelTop * normalYTop[0], dotProductVelTop * normalYTop[1], dotProductVelTop * normalYTop[2] };
					float tangVel[3] = { particles[i].vel.x - normalVel[0], particles[i].vel.y - normalVel[1], particles[i].vel.z - normalVel[2] };

					particles[i].postPos.x = particles[i].postPos.x - 1.1 * (dotProductPostTop + dTop)*normalYTop[0];
					particles[i].postPos.y = particles[i].postPos.y - 1.1 * (dotProductPostTop + dTop)*normalYTop[1];
					particles[i].postPos.z = particles[i].postPos.z - 1.1 * (dotProductPostTop + dTop)*normalYTop[2];

					particles[i].postVel.x = particles[i].postVel.x - 1.1 * (dotProductPostVelTop)*normalYTop[0];
					particles[i].postVel.y = particles[i].postVel.y - 1.1 * (dotProductPostVelTop)*normalYTop[1];
					particles[i].postVel.z = particles[i].postVel.z - 1.1 * (dotProductPostVelTop)*normalYTop[2];

					particles[i].postVel.x = particles[i].postVel.x - 1 * tangVel[0];
					particles[i].postVel.y = particles[i].postVel.y - 1 * tangVel[1];
					particles[i].postVel.z = particles[i].postVel.z - 1 * tangVel[2];
			}

			float dotProductRight = (normalXRight[0] * particles[i].position.x + normalXRight[1] * particles[i].position.y + normalXRight[2] * particles[i].position.z);
			float dotProductPostRight = (normalXRight[0] * particles[i].postPos.x + normalXRight[1] * particles[i].postPos.y + normalXRight[2] * particles[i].postPos.z);

			//Plano darecha
			if ((dotProductRight + dRight) * (dotProductPostRight + dRight) <= 0) {

					float dotProductPostVelRight = (normalXRight[0] * particles[i].postVel.x + normalXRight[1] * particles[i].postVel.y + normalXRight[2] * particles[i].postVel.z);
					float dotProductVelRight = (normalXRight[0] * particles[i].vel.x + normalXRight[1] * particles[i].vel.y + normalXRight[2] * particles[i].vel.z);
					float normalVel[3] = { dotProductVelRight * normalXRight[0], dotProductVelRight * normalXRight[1], dotProductVelRight * normalXRight[2] };
					float tangVel[3] = { particles[i].vel.x - normalVel[0], particles[i].vel.y - normalVel[1], particles[i].vel.z - normalVel[2] };

					particles[i].postPos.x = particles[i].postPos.x - 1.1 * (dotProductPostRight + dRight)*normalXRight[0];
					particles[i].postPos.y = particles[i].postPos.y - 1.1 * (dotProductPostRight + dRight)*normalXRight[1];
					particles[i].postPos.z = particles[i].postPos.z - 1.1 * (dotProductPostRight + dRight)*normalXRight[2];

					particles[i].postVel.x = particles[i].postVel.x - 1.1 * (dotProductPostVelRight)*normalXRight[0];
					particles[i].postVel.y = particles[i].postVel.y - 1.1 * (dotProductPostVelRight)*normalXRight[1];
					particles[i].postVel.z = particles[i].postVel.z - 1.1 * (dotProductPostVelRight)*normalXRight[2];

					particles[i].postVel.x = particles[i].postVel.x - 1 * tangVel[0];
					particles[i].postVel.y = particles[i].postVel.y - 1 * tangVel[1];
					particles[i].postVel.z = particles[i].postVel.z - 1 * tangVel[2];
			}

			float dotProductLeft = (normalXLeft[0] * particles[i].position.x + normalXLeft[1] * particles[i].position.y + normalXLeft[2] * particles[i].position.z);
			float dotProductPostLeft = (normalXLeft[0] * particles[i].postPos.x + normalXLeft[1] * particles[i].postPos.y + normalXLeft[2] * particles[i].postPos.z);

			//Plano izquierda
			if ((dotProductLeft + dLeft) * (dotProductPostLeft + dLeft) <= 0) {

					float dotProductPostVelLeft = (normalXLeft[0] * particles[i].postVel.x + normalXLeft[1] * particles[i].postVel.y + normalXLeft[2] * particles[i].postVel.z);
					float dotProductVelLeft = (normalXLeft[0] * particles[i].vel.x + normalXLeft[1] * particles[i].vel.y + normalXLeft[2] * particles[i].vel.z);
					float normalVel[3] = { dotProductVelLeft * normalXLeft[0], dotProductVelLeft * normalXLeft[1], dotProductVelLeft * normalXLeft[2] };
					float tangVel[3] = { particles[i].vel.x - normalVel[0], particles[i].vel.y - normalVel[1], particles[i].vel.z - normalVel[2] };

					particles[i].postPos.x = particles[i].postPos.x - 1.1 * (dotProductPostLeft + dLeft)*normalXLeft[0];
					particles[i].postPos.y = particles[i].postPos.y - 1.1 * (dotProductPostLeft + dLeft)*normalXLeft[1];
					particles[i].postPos.z = particles[i].postPos.z - 1.1 * (dotProductPostLeft + dLeft)*normalXLeft[2];

					particles[i].postVel.x = particles[i].postVel.x - 1.1 * (dotProductPostVelLeft)*normalXLeft[0];
					particles[i].postVel.y = particles[i].postVel.y - 1.1 * (dotProductPostVelLeft)*normalXLeft[1];
					particles[i].postVel.z = particles[i].postVel.z - 1.1 * (dotProductPostVelLeft)*normalXLeft[2];

					particles[i].postVel.x = particles[i].postVel.x - 1 * tangVel[0];
					particles[i].postVel.y = particles[i].postVel.y - 1 * tangVel[1];
					particles[i].postVel.z = particles[i].postVel.z - 1 * tangVel[2];
			}

			float dotProductFront = (normalZFront[0] * particles[i].position.x + normalZFront[1] * particles[i].position.y + normalZFront[2] * particles[i].position.z);
			float dotProductPostFront = (normalZFront[0] * particles[i].postPos.x + normalZFront[1] * particles[i].postPos.y + normalZFront[2] * particles[i].postPos.z);

			//Plano frontal
			if ((dotProductFront + dFront) * (dotProductPostFront + dFront) <= 0) {

					float dotProductPostVelFront = (normalZFront[0] * particles[i].postVel.x + normalZFront[1] * particles[i].postVel.y + normalZFront[2] * particles[i].postVel.z);
					float dotProductVelFront = (normalZFront[0] * particles[i].vel.x + normalZFront[1] * particles[i].vel.y + normalZFront[2] * particles[i].vel.z);
					float normalVel[3] = { dotProductVelFront * normalZFront[0], dotProductVelFront * normalZFront[1], dotProductVelFront * normalZFront[2] };
					float tangVel[3] = { particles[i].vel.x - normalVel[0], particles[i].vel.y - normalVel[1], particles[i].vel.z - normalVel[2] };

					particles[i].postPos.x = particles[i].postPos.x - 1.1 * (dotProductPostFront + dFront)*normalZFront[0];
					particles[i].postPos.y = particles[i].postPos.y - 1.1 * (dotProductPostFront + dFront)*normalZFront[1];
					particles[i].postPos.z = particles[i].postPos.z - 1.1 * (dotProductPostFront + dFront)*normalZFront[2];

					particles[i].postVel.x = particles[i].postVel.x - 1.1 * (dotProductPostVelFront)*normalZFront[0];
					particles[i].postVel.y = particles[i].postVel.y - 1.1 * (dotProductPostVelFront)*normalZFront[1];
					particles[i].postVel.z = particles[i].postVel.z - 1.1 * (dotProductPostVelFront)*normalZFront[2];

					particles[i].postVel.x = particles[i].postVel.x - 1 * tangVel[0];
					particles[i].postVel.y = particles[i].postVel.y - 1 * tangVel[1];
					particles[i].postVel.z = particles[i].postVel.z - 1 * tangVel[2];
			}

			float dotProductBack = (normalZBack[0] * particles[i].position.x + normalZBack[1] * particles[i].position.y + normalZBack[2] * particles[i].position.z);
			float dotProductPostBack = (normalZBack[0] * particles[i].postPos.x + normalZBack[1] * particles[i].postPos.y + normalZBack[2] * particles[i].postPos.z);

			//Plano trasero
			if ((dotProductBack + dBack) * (dotProductPostBack + dBack) <= 0) {

					float dotProductPostVelBack = (normalZBack[0] * particles[i].postVel.x + normalZBack[1] * particles[i].postVel.y + normalZBack[2] * particles[i].postVel.z);
					float dotProductVelBack = (normalZBack[0] * particles[i].vel.x + normalZBack[1] * particles[i].vel.y + normalZBack[2] * particles[i].vel.z);
					float normalVel[3] = { dotProductVelBack * normalZBack[0], dotProductVelBack * normalZBack[1], dotProductVelBack * normalZBack[2] };
					float tangVel[3] = { particles[i].vel.x - normalVel[0], particles[i].vel.y - normalVel[1], particles[i].vel.z - normalVel[2] };

					particles[i].postPos.x = particles[i].postPos.x - 1.1 * (dotProductPostBack + dBack)*normalZBack[0];
					particles[i].postPos.y = particles[i].postPos.y - 1.1 * (dotProductPostBack + dBack)*normalZBack[1];
					particles[i].postPos.z = particles[i].postPos.z - 1.1 * (dotProductPostBack + dBack)*normalZBack[2];

					particles[i].postVel.x = particles[i].postVel.x - 1.1 * (dotProductPostVelBack)*normalZBack[0];
					particles[i].postVel.y = particles[i].postVel.y - 1.1 * (dotProductPostVelBack)*normalZBack[1];
					particles[i].postVel.z = particles[i].postVel.z - 1.1 * (dotProductPostVelBack)*normalZBack[2];

					particles[i].postVel.x = particles[i].postVel.x - 1 * tangVel[0];
					particles[i].postVel.y = particles[i].postVel.y - 1 * tangVel[1];
					particles[i].postVel.z = particles[i].postVel.z - 1 * tangVel[2];
			}

			//Colisiones Esfera
			float dist = sqrt(pow(particles[i].postPos.x - posSphere[0], 2) + pow(particles[i].postPos.y - posSphere[1], 2) + pow(particles[i].postPos.z - posSphere[2], 2));
			if (dist <= radiusSphere) {

				//Calcular punto de colision
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				float P[3] = { particles[i].position.x, particles[i].position.y, particles[i].position.z };
				float Q[3] = { particles[i].postPos.x, particles[i].postPos.y, particles[i].postPos.z };
				float V[3] = { Q[0] - P[0], Q[1] - P[1], Q[2] - P[2] };

				float a = (pow(V[0], 2) + pow(V[1], 2) + pow(V[2], 2));
				float b = (2 * P[0] * V[0] - 2 * V[0] * posSphere[0] + 2 * P[1] * V[1] - 2 * V[1] * posSphere[1] + 2 * P[2] * V[2] - 2 * V[2] * posSphere[2]);
				float c = (pow(P[0], 2) - 2 * P[0] * posSphere[0] + pow(posSphere[0], 2) + pow(P[1], 2) - 2 * P[1] * posSphere[1] + pow(posSphere[1], 2) + pow(P[2], 2) - 2 * P[2] * posSphere[2] + pow(posSphere[2], 2)) - pow(radiusSphere, 2);

				float alpha[2] = { (-b + sqrt(pow(b,2) - 4 * a*c)) / (2 * a),
					(-b - sqrt(pow(b,2) - 4 * a*c)) / (2 * a),
				};

				float colision1[3] = { P[0] + V[0] * alpha[0], P[1] + V[1] * alpha[0] , P[2] + V[2] * alpha[0] };
				float colision2[3] = { P[0] + V[0] * alpha[1], P[1] + V[1] * alpha[1] , P[2] + V[2] * alpha[1] };

				float distCol1 = sqrt(pow(P[0] - colision1[0], 2) + pow(P[1] - colision1[1], 2) + pow(P[2] - colision1[2], 2));
				float distCol2 = sqrt(pow(P[0] - colision2[0], 2) + pow(P[1] - colision2[1], 2) + pow(P[2] - colision2[2], 2));

				float puntoColision[3] = { colision1[0], colision1[1], colision1[2] };

				if (distCol1 > distCol2) {
					puntoColision[0] = colision2[0];
					puntoColision[1] = colision2[1];
					puntoColision[2] = colision2[2];
				}
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				//Calcular colision con el plano tangencial a la esfera con el punto de colision
				float normalColision[3] = { puntoColision[0] - posSphere[0], puntoColision[1] - posSphere[1], puntoColision[2] - posSphere[2] };
				/*float normalColMod = sqrt(pow(normalColision[0], 2) + pow(normalColision[1], 2) + pow(normalColision[2], 2));
				normalColision[0] /= normalColMod;
				normalColision[1] /= normalColMod;
				normalColision[2] /= normalColMod;*/
				//float dColision = -(normalColision[0] * puntoColision[0]) - (normalColision[1] * puntoColision[1]) - (normalColision[2] * puntoColision[2]);
				float dColision = -(normalColision[0] * puntoColision[0] + normalColision[1] * puntoColision[1] + normalColision[2] * puntoColision[2]);

				float dotProductColision = (normalColision[0] * particles[i].position.x + normalColision[1] * particles[i].position.y + normalColision[2] * particles[i].position.z);
				float dotProductPostColision = (normalColision[0] * particles[i].postPos.x + normalColision[1] * particles[i].postPos.y + normalColision[2] * particles[i].postPos.z);

					float dotProductPostVelColision = (normalColision[0] * particles[i].postVel.x + normalColision[1] * particles[i].postVel.y + normalColision[2] * particles[i].postVel.z);
					float dotProductVelColision = (normalColision[0] * particles[i].vel.x + normalColision[1] * particles[i].vel.y + normalColision[2] * particles[i].vel.z);
					float normalVel[3] = { dotProductVelColision * normalColision[0], dotProductVelColision * normalColision[1], dotProductVelColision * normalColision[2] };
					float tangVel[3] = { particles[i].vel.x - normalVel[0], particles[i].vel.y - normalVel[1], particles[i].vel.z - normalVel[2] };

					particles[i].postPos.x = particles[i].postPos.x - 1.1 * (dotProductPostColision + dColision)*normalColision[0];
					particles[i].postPos.y = particles[i].postPos.y - 1.1 * (dotProductPostColision + dColision)*normalColision[1];
					particles[i].postPos.z = particles[i].postPos.z - 1.1 * (dotProductPostColision + dColision)*normalColision[2];

					particles[i].postVel.x = particles[i].postVel.x - 1.1 * (dotProductPostVelColision)*normalColision[0];
					particles[i].postVel.y = particles[i].postVel.y - 1.1 * (dotProductPostVelColision)*normalColision[1];
					particles[i].postVel.z = particles[i].postVel.z - 1.1 * (dotProductPostVelColision)*normalColision[2];

					particles[i].postVel.x = particles[i].postVel.x - 1 * tangVel[0];
					particles[i].postVel.y = particles[i].postVel.y - 1 * tangVel[1];
					particles[i].postVel.z = particles[i].postVel.z - 1 * tangVel[2];

			}
			
			//Aplicar nueva posicion
			particles[i].position.x = particles[i].postPos.x;
			particles[i].position.y = particles[i].postPos.y;
			particles[i].position.z = particles[i].postPos.z;

			//Aplicar nueva velocidad
			//particles[i].vel.x = particles[i].postVel.x;
			particles[i].vel.y = particles[i].postVel.y;
			particles[i].vel.z = particles[i].postVel.z;
		}
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