#include "NidMode.hpp"

#include "Paddle.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

//inline helper function for vector rotation
//tbh didn't understand this format for inline functions but wanted to try it out, TODO ask a question about it next class
//was giving me a ton of errors, went the normal route of typing helper functions
void rotate_vector(glm::vec2& vector, float theta) {
	glm::mat2 rotation_matrix(cos(theta), sin(theta), -1.0f * sin(theta), cos(theta)); //Column-Major order
	vector = rotation_matrix * vector;
};

NidMode::NidMode() {

	//set up trail as if ball has been here for 'forever':
	ball_trail.clear();
	ball_trail.emplace_back(ball, trail_length);
	ball_trail.emplace_back(ball, 0.0f);

	
	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	//vertex array mapping buffer for color_texture_program:
	{ 
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of NidMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	//solid white texture:
	{ 
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

NidMode::~NidMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool NidMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	// Good debugging tool (or cheat ;) )
	//if (evt.type == SDL_MOUSEMOTION) {
	//	//convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
	//	glm::vec2 clip_mouse = glm::vec2(
	//		(evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
	//		(evt.motion.y + 0.5f) / window_size.y *-2.0f + 1.0f
	//	);
	//	left_paddle.center.y = (clip_to_court * glm::vec3(clip_mouse, 1.0f)).y;
	//	left_paddle.center.x = (clip_to_court * glm::vec3(clip_mouse, 1.0f)).x;
	//}

	return false;
}

void NidMode::update(float elapsed) {

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	//----- paddle update -----

	//Update the key states
	SDL_PumpEvents();
	const Uint8* state = SDL_GetKeyboardState(NULL);

	//left player controls, WASD, space to rotate
	{
		if (state[SDL_SCANCODE_W]) { left_paddle.center.y = left_paddle.center.y + left_paddle.speed * elapsed; }
		if (state[SDL_SCANCODE_S]) { left_paddle.center.y = left_paddle.center.y - left_paddle.speed * elapsed; }
		if (state[SDL_SCANCODE_A]) { left_paddle.center.x = left_paddle.center.x - left_paddle.speed * elapsed; }
		if (state[SDL_SCANCODE_D]) { left_paddle.center.x = left_paddle.center.x + left_paddle.speed * elapsed; }
		if (state[SDL_SCANCODE_F]) { left_paddle.rotation += left_paddle.rotation_speed * elapsed;  }
		if (state[SDL_SCANCODE_G]) { left_paddle.rotation -= left_paddle.rotation_speed * elapsed; }
	}

	//left player controls, Arrow keys, right_ctrl to rotate
	{
		if (state[SDL_SCANCODE_UP])     { right_paddle.center.y = right_paddle.center.y + right_paddle.speed * elapsed; }
		if (state[SDL_SCANCODE_DOWN])   { right_paddle.center.y = right_paddle.center.y - right_paddle.speed * elapsed; }
		if (state[SDL_SCANCODE_LEFT])   { right_paddle.center.x = right_paddle.center.x - right_paddle.speed * elapsed; }
		if (state[SDL_SCANCODE_RIGHT])  { right_paddle.center.x = right_paddle.center.x + right_paddle.speed * elapsed; }
		if (state[SDL_SCANCODE_RSHIFT]) { right_paddle.rotation += right_paddle.rotation_speed * elapsed; }
		if (state[SDL_SCANCODE_RCTRL])  { right_paddle.rotation -= right_paddle.rotation_speed * elapsed; }
	}
	
	

	//clamp paddles to court:
	right_paddle.center.y = std::max(right_paddle.center.y, -court_radius.y + right_paddle.radius.y);
	right_paddle.center.y = std::min(right_paddle.center.y,  court_radius.y - right_paddle.radius.y);
	right_paddle.center.x = std::max(right_paddle.center.x, -court_radius.x + right_paddle.radius.x);
	right_paddle.center.x = std::min(right_paddle.center.x,  court_radius.x - right_paddle.radius.x);

	left_paddle.center.y = std::max(left_paddle.center.y, -court_radius.y + left_paddle.radius.y);
	left_paddle.center.y = std::min(left_paddle.center.y,  court_radius.y - left_paddle.radius.y);
	left_paddle.center.x = std::max(left_paddle.center.x, -court_radius.x + left_paddle.radius.x);
	left_paddle.center.x = std::min(left_paddle.center.x,  court_radius.x - left_paddle.radius.x);

	//----- ball update -----

	//speed of ball doubles every four points:
	//float speed_multiplier = 4.0f * std::pow(2.0f, (left_score + right_score) / 4.0f);

	//velocity cap, though (otherwise ball can pass through paddles):
	//speed_multiplier = std::min(speed_multiplier, 1.0f);

	float constant_magnitude_multipiler = 5.0f / length(ball_velocity);

	ball += elapsed * constant_magnitude_multipiler * ball_velocity;

	//---- collision handling ----

	//paddles:
	auto paddle_vs_ball = [this](Paddle const &paddle) {

		glm::vec2 ball_translated = ball - paddle.center;
		glm::vec2 ball_translated_rotated = ball_translated;
		rotate_vector(ball_translated_rotated, paddle.rotation * -2.0f * (float)M_PI);
		glm::vec2 velocity_rotated = ball_velocity;
		rotate_vector(velocity_rotated, paddle.rotation * -2.0f * (float)M_PI);

		float x_radius = ball_radius.x + paddle.radius.x;
		float y_radius = ball_radius.y + paddle.radius.y;

		float x_dist = abs(ball_translated_rotated.x) - x_radius;
		float y_dist = abs(ball_translated_rotated.y) - y_radius;

		//if no overlap, no collision:
		if (x_dist > 0 || y_dist > 0) { return; }

		if (x_dist > y_dist) { // bounce in x direction
			if (ball_translated_rotated.x > 0) {
				ball_translated_rotated.x = paddle.center.x + paddle.radius.x + ball_radius.x;
				velocity_rotated.x = glm::max(abs(velocity_rotated.x), 1.0f);
			}
			else {
				ball_translated_rotated.x = paddle.center.x - paddle.radius.x - ball_radius.x;
				velocity_rotated.x = -1.0f * glm::max(velocity_rotated.x, 1.0f);
			}
		}
		else { // bounce in y direction
			if (ball_translated_rotated.y > 0) {
				ball_translated_rotated.y = paddle.center.y + paddle.radius.y + ball_radius.y;
				velocity_rotated.y = glm::max(abs(velocity_rotated.y), 1.0f);
			}
			else {
				ball_translated_rotated.y = paddle.center.y - paddle.radius.y - ball_radius.y;
				velocity_rotated.y = -1.0f * glm::max(velocity_rotated.y, 1.0f);
			}
		}

		//warp y velocity based on offset from paddle center:
		float vel = (ball_translated_rotated.y) / (paddle.radius.y + ball_radius.y);
		velocity_rotated.y = glm::mix(velocity_rotated.y, vel, 0.75f);
		
		rotate_vector(velocity_rotated, paddle.rotation * 2.0f * (float)M_PI);
		ball_velocity = velocity_rotated;

	};
	paddle_vs_ball(left_paddle);
	paddle_vs_ball(right_paddle);

	//court walls:
	if (ball.y > court_radius.y - ball_radius.y) {
		ball.y = court_radius.y - ball_radius.y;
		if (ball_velocity.y > 0.0f) {
			ball_velocity.y = -ball_velocity.y;
		}
	}
	if (ball.y < -court_radius.y + ball_radius.y) {
		ball.y = -court_radius.y + ball_radius.y;
		if (ball_velocity.y < 0.0f) {
			ball_velocity.y = -ball_velocity.y;
		}
	}

	if (ball.x > court_radius.x - ball_radius.x) {
		ball.x = court_radius.x - ball_radius.x;
		if (ball_velocity.x > 0.0f) {
			ball_velocity.x = -ball_velocity.x;
			left_score += 1;
			ball = glm::vec2(0.0f, 0.0f);
			ball_velocity = glm::vec2( (mt()/(float)(mt.max())) - .5f, (mt() / (float)(mt.max())) - .5f);
		}
	}
	if (ball.x < -court_radius.x + ball_radius.x) {
		ball.x = -court_radius.x + ball_radius.x;
		if (ball_velocity.x < 0.0f) {
			ball_velocity.x = -ball_velocity.x;
			right_score += 1;
			ball = glm::vec2(0.0f, 0.0f);
			ball_velocity = glm::vec2((mt() / (float)(mt.max())) - .5f, (mt() / (float)(mt.max())) - .5f);
		}
	}

	//----- rainbow trails -----

	//age up all locations in ball trail:
	for (auto &t : ball_trail) {
		t.z += elapsed;
	}
	//store fresh location at back of ball trail:
	ball_trail.emplace_back(ball, 0.0f);

	//trim any too-old locations from back of trail:
	//NOTE: since trail drawing interpolates between points, only removes back element if second-to-back element is too old:
	while (ball_trail.size() >= 2 && ball_trail[1].z > trail_length) {
		ball_trail.pop_front();
	}
}



void NidMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 left_color = HEX_TO_U8VEC4(0xFF0000FF);
	const glm::u8vec4 right_color = HEX_TO_U8VEC4(0x0000FFFF);
	
	//I know this is exceedingly dumb, but it works, and I would get issues otherwise
	const glm::u8vec4 left_score_color = HEX_TO_U8VEC4(0x08000000 * (uint8_t)glm::clamp((float)left_score - (float)right_score, .0f, 8.f));
	const glm::u8vec4 right_score_color = HEX_TO_U8VEC4(0x00000800 * (uint8_t)glm::clamp((float)right_score - (float)left_score, .0f, 8.f));
	const glm::u8vec4 total_score_color = HEX_TO_U8VEC4(0x08000800 * (uint8_t)glm::clamp(left_score + right_score, (uint32_t)0, (uint32_t)8));
	const glm::u8vec4 new_bg_color = HEX_TO_U8VEC4(0x070007FF) + total_score_color + right_score_color + left_score_color;

	//const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x171714ff);
	//const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0xd1bb54ff);
	const glm::u8vec4 new_fg_color = HEX_TO_U8VEC4(0xd1ff54ff);

	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0x604d29ff);
	const std::vector< glm::u8vec4 > rainbow_colors = {
		HEX_TO_U8VEC4(0x604d29ff), HEX_TO_U8VEC4(0x624f29fc), HEX_TO_U8VEC4(0x69542df2),
		HEX_TO_U8VEC4(0x6a552df1), HEX_TO_U8VEC4(0x6b562ef0), HEX_TO_U8VEC4(0x6b562ef0),
		HEX_TO_U8VEC4(0x6d572eed), HEX_TO_U8VEC4(0x6f592feb), HEX_TO_U8VEC4(0x725b31e7),
		HEX_TO_U8VEC4(0x745d31e3), HEX_TO_U8VEC4(0x755e32e0), HEX_TO_U8VEC4(0x765f33de),
		HEX_TO_U8VEC4(0x7a6234d8), HEX_TO_U8VEC4(0x826838ca), HEX_TO_U8VEC4(0x977840a4),
		HEX_TO_U8VEC4(0x96773fa5), HEX_TO_U8VEC4(0xa07f4493), HEX_TO_U8VEC4(0xa1814590),
		HEX_TO_U8VEC4(0x9e7e4496), HEX_TO_U8VEC4(0xa6844887), HEX_TO_U8VEC4(0xa9864884),
		HEX_TO_U8VEC4(0xad8a4a7c),
	};
	#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, float rotation, glm::u8vec4 const &color) {
		
		// Rectangle's points are represented by ABCD in left->right, top->bot order

		glm::vec2 vec_a = glm::vec2(-radius.x,  radius.y);
		glm::vec2 vec_b = glm::vec2( radius.x,  radius.y);
		glm::vec2 vec_c = glm::vec2(-radius.x, -radius.y);
		glm::vec2 vec_d = glm::vec2( radius.x, -radius.y);

		rotate_vector(vec_a, rotation * 2.0f * (float)M_PI);
		rotate_vector(vec_b, rotation * 2.0f * (float)M_PI);
		rotate_vector(vec_c, rotation * 2.0f * (float)M_PI);
		rotate_vector(vec_d, rotation * 2.0f * (float)M_PI);
		
		//draw rectangle as two CCW-oriented triangles:
		//Triangle 1 is CDB, 2 is CBA
		vertices.emplace_back(glm::vec3(center + vec_c, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center + vec_d, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center + vec_b, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center + vec_c, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center + vec_b, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center + vec_a, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//shadows for everything (except the trail):
	//TODO add rotation here
	glm::vec2 s = glm::vec2(0.0f,-shadow_offset);

	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f)+s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), 0.0f, shadow_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f)+s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), 0.0f, shadow_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius)+s, glm::vec2(court_radius.x, wall_radius), 0.0f, shadow_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius)+s, glm::vec2(court_radius.x, wall_radius), 0.0f, shadow_color);
	//draw_rectangle(left_paddle.center+s, left_paddle.radius, left_paddle.rotation, shadow_color);
	//draw_rectangle(right_paddle.center+s, right_paddle.radius, right_paddle.rotation, shadow_color);
	//draw_rectangle(ball+s, ball_radius, 0.0f, shadow_color);

	//ball's trail:
	if (ball_trail.size() >= 2) {
		//start ti at second element so there is always something before it to interpolate from:
		std::deque< glm::vec3 >::iterator ti = ball_trail.begin() + 1;
		//draw trail from oldest-to-newest:
		for (uint32_t i = uint32_t(rainbow_colors.size())-1; i < rainbow_colors.size(); --i) {
			//time at which to draw the trail element:
			float t = (i + 1) / float(rainbow_colors.size()) * trail_length;
			//advance ti until 'just before' t:
			while (ti != ball_trail.end() && ti->z > t) ++ti;
			//if we ran out of tail, stop drawing:
			if (ti == ball_trail.end()) break;
			//interpolate between previous and current trail point to the correct time:
			glm::vec3 a = *(ti-1);
			glm::vec3 b = *(ti);
			glm::vec2 at = (t - a.z) / (b.z - a.z) * (glm::vec2(b) - glm::vec2(a)) + glm::vec2(a);
			//draw:
			draw_rectangle(at, ball_radius, 0.0f, rainbow_colors[i]);
		}
	}

	//solid objects:

	//walls:
	draw_rectangle(glm::vec2(-court_radius.x-wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), 0.0f, new_fg_color);
	draw_rectangle(glm::vec2( court_radius.x+wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), 0.0f, new_fg_color);
	draw_rectangle(glm::vec2( 0.0f,-court_radius.y-wall_radius), glm::vec2(court_radius.x, wall_radius), 0.0f, new_fg_color);
	draw_rectangle(glm::vec2( 0.0f, court_radius.y+wall_radius), glm::vec2(court_radius.x, wall_radius), 0.0f, new_fg_color);

	//paddles: //TODO add rotation to paddle // TODO add colors to paddles
	draw_rectangle(left_paddle.center, left_paddle.radius, left_paddle.rotation, left_color);
	draw_rectangle(right_paddle.center, right_paddle.radius, right_paddle.rotation, right_color);
	

	//ball:
	draw_rectangle(ball, ball_radius, 0.0f, new_fg_color);

	//scores:
	glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);
	for (uint32_t i = 0; i < left_score; ++i) {
		draw_rectangle(glm::vec2( -court_radius.x + (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, 0.0f, new_fg_color);
	}
	for (uint32_t i = 0; i < right_score; ++i) {
		draw_rectangle(glm::vec2( court_radius.x - (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, 0.0f, new_fg_color);
	}


	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f * score_radius.y + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(new_bg_color.r / 255.0f, new_bg_color.g / 255.0f, new_bg_color.b / 255.0f, new_bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}
