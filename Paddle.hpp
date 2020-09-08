#pragma once
#include <glm/glm.hpp>
struct Paddle {
	Paddle(glm::vec2 center_vector);
	glm::vec2 center = glm::vec2(0.0f, 0.0f);
	glm::vec2 radius = glm::vec2(0.2f, 1.0f);
	float rotation = 0.0f;
	float speed = 4.0f;
	float rotation_speed = 1.0f;
};