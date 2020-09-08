#include "Paddle.hpp"
#include <glm/glm.hpp>

Paddle::Paddle(glm::vec2 center_vector) {
	center.x = center_vector.x;
	center.y = center_vector.y;
}