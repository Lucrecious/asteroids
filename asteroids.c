#include <stdio.h>
#include <stdbool.h>

#include <raylib.h>
#include <raymath.h>

#include <math.h>

#include <assert.h>


// player
//
#define MAX_PLAYER_VELOCITY 400.0

typedef struct {
    Vector2 position;
    float rotation;

    float acceleration;
    Vector2 velocity;
} player_t;

static void init_player(player_t *player) {
    player->position.x = GetScreenWidth() / 2;
    player->position.y = GetScreenHeight() / 2;
    player->rotation = PI / 2.0;

    player->acceleration = 100.0;
    player->velocity = Vector2Zero();
}

#define PLAYER_HEIGHT 10
static void draw_player(player_t *player) {
    Vector2 top = { 0, -PLAYER_HEIGHT };
    Vector2 left = { -PLAYER_HEIGHT, PLAYER_HEIGHT };
    Vector2 right = { PLAYER_HEIGHT, PLAYER_HEIGHT };

    top = Vector2Rotate(top, player->rotation);
    left = Vector2Rotate(left, player->rotation);
    right = Vector2Rotate(right, player->rotation);

    Vector2 t = Vector2Add(top, player->position);
    Vector2 l = Vector2Add(left, player->position);
    Vector2 r = Vector2Add(right, player->position);

    Vector2 copy_positions[] = {
	(Vector2) { 0.0f, 0.0f },
	(Vector2) { -GetScreenWidth(), 0.0f },
	(Vector2) { GetScreenWidth(), 0.0f },
	(Vector2) { 0.0f, -GetScreenHeight() },
	(Vector2) { 0.0f, GetScreenHeight() },
    };

    for (int i = 0; i < sizeof(copy_positions)/sizeof(copy_positions[0]); ++i) {
	Vector2 relative = copy_positions[i];

	DrawLineEx(Vector2Add(t, relative), Vector2Add(l, relative), 3.0f, RED);
	DrawLineEx(Vector2Add(t, relative), Vector2Add(r, relative), 3.0f, RED);
	DrawLineEx(Vector2Add(l, relative), Vector2Add(r, relative), 3.0f, RED);
    }
}

static void wrap_player(player_t *player) {
    if (player->position.x < 0) {
	player->position.x += GetScreenWidth();
    } else if (player->position.x >= GetScreenWidth()) {
	player->position.x -= GetScreenWidth();
    }

    if (player->position.y < 0) {
	player->position.y += GetScreenHeight();
    } else if (player->position.y >= GetScreenHeight()) {
	player->position.y -= GetScreenHeight();
    }
}

// bullets

#define MAX_BULLET_COUNT 100
#define BULLET_VELOCITY 1000.0

typedef struct {
    Vector2 position;
    Vector2 direction;
} bullet_t;

int bullet_count = 0;
bullet_t bullets[MAX_BULLET_COUNT];

bullet_t *spawn_bullet() {
    if (bullet_count >= MAX_BULLET_COUNT) return NULL;
    bullet_t* bullet = (bullets + bullet_count);
    bullet_count++;
    return bullet;
}

static void draw_bullets() {
    for (size_t i = 0; i < bullet_count; i++) {
	bullet_t *bullet = (bullets + i);

	DrawLineEx(bullet->position,
		Vector2Add(bullet->position, Vector2Scale(bullet->direction, -10)), 3.0f, RED);
    }
}

static void bullet_reclaim(int i) {
    assert(i < bullet_count && "this should only be called when there are live bullets");
    bullets[i] = bullets[bullet_count - 1];
    bullet_count--;
}


// asteroids

#define MAX_ASTEROID_VERTS 12
typedef struct {
    Vector2 position;
    Vector2 velocity;

    int point_count;
    Vector2 points[MAX_ASTEROID_VERTS];

    bool is_big;

    bool has_entered_screen;
} asteroid_t;

#define MAX_ASTEROID_COUNT 256
#define ASTEROID_BIG_WH 32
#define ASTEROID_SMALL_WH 16
int asteroid_count = 0;
asteroid_t asteroids[MAX_ASTEROID_COUNT];

static asteroid_t *spawn_asteroid(int x, int y, bool is_big) {
    if (asteroid_count >= MAX_ASTEROID_COUNT) return NULL;

    asteroid_t *asteroid = (asteroids + asteroid_count);
    asteroid_count++;

    asteroid->position.x = x;
    asteroid->position.y = y;

    asteroid->is_big = is_big;

    asteroid->has_entered_screen = true;
    if (x < 0 || x > GetScreenWidth() || y < 0 || y > GetScreenHeight()) {
	asteroid->has_entered_screen = false;
    }
    
    int sides = GetRandomValue(5, MAX_ASTEROID_VERTS - 1);
    asteroid->point_count = sides + 1;

    float r = is_big ? ASTEROID_BIG_WH : ASTEROID_SMALL_WH;

    for (int i = 0; i < sides; ++i) {
	asteroid->points[i].x = r * cosf(2.0f*PI*i/sides);
	asteroid->points[i].y = r * sinf(2.0f*PI*i/sides);

	asteroid->points[i].x += (GetRandomValue(-100, 100) / 100.0) * 5.0f;
	asteroid->points[i].y += (GetRandomValue(-100, 100) / 100.0) * 5.0f;
    }

    asteroid->points[sides] = asteroid->points[0];

    return asteroid;
}

static bool is_colliding_with_asteroid(Vector2 position, asteroid_t *asteroid) {
    Vector2 local_to_asteroid = Vector2Subtract(position, asteroid->position);
    return CheckCollisionPointPoly(local_to_asteroid, asteroid->points, asteroid->point_count);
}

static void wrap_asteroid(asteroid_t *asteroid) {
    if (!asteroid->has_entered_screen) return;

    if (asteroid->position.x < 0) {
	asteroid->position.x += GetScreenWidth();
    } else if (asteroid->position.x >= GetScreenWidth()) {
	asteroid->position.x -= GetScreenWidth();
    }

    if (asteroid->position.y < 0) {
	asteroid->position.y += GetScreenHeight();
    } else if (asteroid->position.y >= GetScreenHeight()) {
	asteroid->position.y -= GetScreenHeight();
    }
}

static void asteroid_reclaim(int index) {
    assert(index < asteroid_count && "should only be called on live asteroids");

    asteroids[index] = asteroids[asteroid_count - 1]; 
    asteroid_count--;
}

static void draw_asteroids() {
    Vector2 copy_positions[] = {
	(Vector2) { 0.0f, 0.0f },
	(Vector2) { -GetScreenWidth(), 0.0f },
	(Vector2) { GetScreenWidth(), 0.0f },
	(Vector2) { 0.0f, -GetScreenHeight() },
	(Vector2) { 0.0f, GetScreenHeight() },
    };

    for (size_t i = 0; i < asteroid_count; ++i) {

	asteroid_t *asteroid = (asteroids + i);

	for (int p = 0; p < asteroid->point_count - 1; ++p) {
	    int copy_count = sizeof(copy_positions)/sizeof(copy_positions[0]);
	    if (!asteroid->has_entered_screen) {
		copy_count = 1;
	    }

	    for (int j = 0; j < copy_count; ++j) {
		Vector2 p1 = Vector2Add(asteroid->points[p], asteroid->position);
		Vector2 p2 = Vector2Add(asteroid->points[p + 1], asteroid->position);

		DrawLineEx(Vector2Add(p1, copy_positions[j]), Vector2Add(p2, copy_positions[j]), 3.0, RED);
	    }
	}
    }
}

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float sec;
} particle_t;

#define MAX_PARTICLE_COUNT 10000
int particle_count = 0;
particle_t particles[MAX_PARTICLE_COUNT];
 
static particle_t *spawn_particle() {
    return &particles[particle_count++];
}

static void reclaim_particle(int index) {
    particles[index] = particles[particle_count - 1];
    particle_count--;
}

// destroy asteroids

static void destroy_asteroid(size_t i) {
    asteroid_t *asteroid = (asteroids + i);

    int explosion_particle_count = 20;
    Vector2 up = { 0.0f, -10.0f };
    for (int i = 0; i < explosion_particle_count; ++i) {
	particle_t *particle = spawn_particle();
	if (particle == NULL) continue;

	Vector2 random_offset = Vector2Scale(Vector2Rotate(up, 2.0 * PI * GetRandomValue(0, 100) / 100.0f), 1.0f);
	particle->position = Vector2Add(asteroid->position, random_offset);

	Vector2 random_direction = Vector2Rotate(up, 2.0f * PI * GetRandomValue(0, 100) / 100.0f);
	particle->velocity = Vector2Scale(random_direction, GetRandomValue(10.0f, 50.0f));
	particle->sec = 0.3f;
    }

    asteroid_reclaim(i);
}

static void destroy_asteroids() {
    for (size_t i = 0; i < asteroid_count; i++) {
	destroy_asteroid(i);
    }
}

#define ASTEROID_SPAWN_SEC 2.0f

#define THRUSTER_INTERVAL_SEC 0.05f

typedef struct {
    player_t player;

    float asteroid_spawn_sec;

    int score;

} game_t;

Sound laser_fire;
Sound explode;

float thruster_interval_sec;

float time_factor = 1.0f;
float GetMyFrameTime() {
    return GetFrameTime() * time_factor;
}

bool gameover = false;

bool simulate(game_t *game) {
    Vector2 screen_rect[] = {
	Vector2Zero(),
	(Vector2) { 0.0f, GetScreenHeight() },
	(Vector2) { GetScreenWidth(), GetScreenHeight() }, 
	(Vector2) { GetScreenWidth() , 0.0f },
	Vector2Zero(),
    };

    int screen_rect_points = sizeof(screen_rect)/sizeof(screen_rect[0]);

    player_t *player = &game->player;

    Vector2 mouse_position = GetMousePosition();
    Vector2 to_cursor = Vector2Subtract(mouse_position, player->position);
    Vector2 player_dir = Vector2Normalize(to_cursor);

    bool is_player_moving = false;

    if (!gameover) {
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
	    // TODO: make sure to cursor is not zero vector
	    float a = player->acceleration * GetMyFrameTime();
	    player->velocity = Vector2Add(Vector2Scale(player_dir, a), player->velocity);
	    is_player_moving = true;
	}
	
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
	    bullet_t *bullet = spawn_bullet();
	    if (bullet) {
		PlaySound(laser_fire);
		bullet->position = Vector2Add(player->position, Vector2Scale(player_dir, PLAYER_HEIGHT));
		bullet->direction = player_dir;
	    }
	}
    }

    game->asteroid_spawn_sec -= GetMyFrameTime();
    if (!gameover && game->asteroid_spawn_sec < 0) {
	game->asteroid_spawn_sec = ASTEROID_SPAWN_SEC + game->asteroid_spawn_sec;
	bool is_big = GetRandomValue(0, 1);

	Vector2 random_direction = Vector2Rotate((Vector2){ 0, -1 }, PI * 2 * GetRandomValue(0, 100) / 100.0);
	Vector2 signs = { random_direction.x >= 0 ? 1 : -1, random_direction.y >= 0 ? 1 : -1 };
	Vector2 relative = (Vector2){
	    (GetScreenWidth()/2 + 50) * -signs.x,
	    (GetScreenHeight()/2 + 50) * -signs.y
	};

	Vector2 screen_middle = { GetScreenWidth() / 2, GetScreenHeight() / 2 };
	Vector2 spawn_position = Vector2Add(screen_middle, relative);

	asteroid_t* asteroid = spawn_asteroid(spawn_position.x, spawn_position.y, is_big);

	if (asteroid != NULL) {
	    asteroid->velocity = Vector2Scale(random_direction, 100.0f);
	}
    }

    // player update
    float angle = Vector2Angle((Vector2){ 0, -1 }, to_cursor);
    player->rotation = angle;

    if (Vector2Length(player->velocity) > MAX_PLAYER_VELOCITY) {
	player->velocity = Vector2Scale(Vector2Normalize(player->velocity), MAX_PLAYER_VELOCITY);
    }

    player->position = Vector2Add(Vector2Scale(player->velocity, GetMyFrameTime()), player->position);

    if (is_player_moving) {
	thruster_interval_sec -= GetMyFrameTime();
    }

    if (thruster_interval_sec < 0) {
	thruster_interval_sec = THRUSTER_INTERVAL_SEC;

	particle_t *particle = spawn_particle();
	if (particle != NULL) {
	    Vector2 direction = Vector2Rotate((Vector2){ 0.0f, -1.0f }, player->rotation + (PI / 10) * GetRandomValue(-100, 100) / 100.0f);
	    particle->position = Vector2Add(player->position, Vector2Scale(player_dir, -PLAYER_HEIGHT));
	    particle->velocity = player->velocity;
	    particle->sec = 0.35f;
	    particle->velocity = Vector2Scale(direction, -100.0f);
	}
    }

    wrap_player(player);

    // bullet update
    for (size_t i = 0; i < bullet_count; i++) {
	bullet_t *bullet = (bullets + i);

	Vector2 velocity = Vector2Scale(bullet->direction, BULLET_VELOCITY);
	bullet->position = Vector2Add(bullet->position, Vector2Scale(velocity, GetMyFrameTime()));

	bool dead = false;
if (bullet->position.x < -20 || bullet->position.x > GetScreenWidth() + 20 ||
	    bullet->position.y < -20 || bullet->position.y > GetScreenHeight() + 20) {
	    bullet_reclaim(i);
	    dead = true;
	}

	if (dead) continue;

	for (size_t j = 0; j < asteroid_count; j++) {
	    asteroid_t *asteroid = (asteroids + j);

	    bool bullet_in_asteroid = is_colliding_with_asteroid(bullet->position, asteroid);
	    if (!bullet_in_asteroid) continue;

	    game->score++;

	    if (asteroid->is_big) {
		float velocity_magnitude = Vector2Length(asteroid->velocity);
		for (int i = 0; i < 4; i++) {
		    asteroid_t *child_asteroid = spawn_asteroid(asteroid->position.x, asteroid->position.y, false);
		    if (child_asteroid == NULL) break;

		    Vector2 velocity = { 0, -1 };
		    velocity = Vector2Rotate(velocity, (GetRandomValue(0, 100) / 100.0f) * PI * 2.0);
		    velocity = Vector2Scale(velocity, velocity_magnitude);
		    child_asteroid->velocity = velocity;
		}
	    }

	    PlaySound(explode);

	    destroy_asteroid(j);
	    bullet_reclaim(i);
	    break;
	}
    }

    // asteroid update
    for (size_t i = 0; i < asteroid_count; ++i) {
	asteroid_t *asteroid = (asteroids + i);

	for (size_t j = 0; j < asteroid_count; ++j) {
	    if (j == i) continue;

	    asteroid_t *other = (asteroids + j);

	    Vector2 collision_point;
	    for (int p1 = 0; p1 < asteroid->point_count - 1; ++p1) {
		Vector2 a1 = Vector2Add(asteroid->points[p1], asteroid->position);
		Vector2 b1 = Vector2Add(asteroid->points[p1 + 1], asteroid->position);

		for (int p2 = 0; p2 < other->point_count - 1; ++p2) {
		    Vector2 a2 = Vector2Add(other->points[p2], other->position);
		    Vector2 b2 = Vector2Add(other->points[p2 + 1], other->position);
		    bool is_colliding = CheckCollisionLines(a1, b1, a2, b2, &collision_point);

		    if (is_colliding) {
			Vector2 relative = Vector2Subtract(asteroid->position, collision_point);//other->position);
			relative = Vector2Normalize(relative);

			float speed = Vector2Length(asteroid->velocity);
			asteroid->velocity = Vector2Scale(relative, speed);
		    }
		}
	    }
	}
	
	asteroid->position = Vector2Add(asteroid->position, Vector2Scale(asteroid->velocity, GetMyFrameTime()));

	// TODO: put this in a function so it's clearer whats happening here
	if (!asteroid->has_entered_screen) {
	    if (CheckCollisionPointPoly(asteroid->position, screen_rect, screen_rect_points)) {
		Vector2 collision_point;
		bool edges_intersect_screen_rect = false;
		for (int p = 0; p < asteroid->point_count - 1; ++p) {
		    Vector2 a1 = Vector2Add(asteroid->points[p], asteroid->position);
		    Vector2 b1 = Vector2Add(asteroid->points[p + 1], asteroid->position);
		    for (int s = 0; s < screen_rect_points - 1; ++s) {
			Vector2 a2 = screen_rect[s];
			Vector2 b2 = screen_rect[s + 1];

			if (CheckCollisionLines(a1, b1, a2, b2, &collision_point)) {
			    edges_intersect_screen_rect = true;
			    break;
			}
		    }
		    if (edges_intersect_screen_rect) break;
		}

		asteroid->has_entered_screen = !edges_intersect_screen_rect;
	    }
	}

	wrap_asteroid(asteroid);

	bool player_hit_asteroid = is_colliding_with_asteroid(player->position, asteroid);

	if (player_hit_asteroid) {
	    gameover = true;
	}
    }

    // update particles
    for (size_t i = 0; i < particle_count; ++i) {
	particle_t *particle = &particles[i];

	particle->sec -= GetMyFrameTime();
	if (particle->sec < 0) {
	    reclaim_particle(i);
	    continue;
	}

	particle->position = Vector2Add(particle->position, Vector2Scale(particle->velocity, GetMyFrameTime()));

	DrawRectangle(particle->position.x - 2, particle->position.y - 2, 4, 4, RED);
    }

    return gameover;
}

static void init_game(game_t *game) {
    init_player(&game->player);
    game->score = 0;
    game->asteroid_spawn_sec = ASTEROID_SPAWN_SEC;
}

int main(int argc, char** argv) {
    float asteroid_spawn_sec = ASTEROID_SPAWN_SEC;

    float thruster_interval_sec = THRUSTER_INTERVAL_SEC;

    bool spawned = false;

    InitAudioDevice();

    laser_fire = LoadSound("laserSmall_001.ogg");
    SetSoundVolume(laser_fire, 0.5f);

    explode = LoadSound("explosion.ogg");
    SetSoundVolume(explode, 0.5f);

    InitWindow(800, 600, "Asteroids");

    game_t game;
    init_game(&game);

    bool paused = false;

    while (!WindowShouldClose()) {
	if (IsKeyPressed(KEY_P) && !gameover) {
	    paused = !paused;
	}

	if (gameover && IsKeyPressed(KEY_R)) {
	    gameover = false;
	    init_game(&game);
	}

	if (IsKeyPressed(KEY_S)) {
	    time_factor = time_factor < 0.9f ? 1.0f : 0.1f;
	}

	if (!paused) {
	    gameover = simulate(&game);

	    if (gameover) {
		destroy_asteroids();
	    }
	}

	BeginDrawing();

	ClearBackground(BLACK);

	if (!gameover) {
	    draw_player(&game.player);
	}

	draw_bullets();
	draw_asteroids();

	DrawText(TextFormat("Score: %d", game.score), 0, 0, 64, RED);

	if (paused) {
	    int text_size = 64;
	    int text_width = MeasureText("Paused", text_size);
	    DrawText("Paused", GetScreenWidth() / 2 - text_width / 2.0, GetScreenHeight() / 2 - text_size / 2.0, text_size, RED);
	}

	if (gameover) {
	    int text_size = 32;
	    const char* gameover_text = "Game Over (R to Restart)";
	    int text_width = MeasureText(gameover_text, text_size);
	    DrawText(gameover_text, GetScreenWidth() / 2 - text_width / 2.0, GetScreenHeight() / 2 - text_size / 2.0f, text_size, RED);
	}

	EndDrawing();
    }

    UnloadSound(laser_fire);
    UnloadSound(explode);

    CloseAudioDevice();

    CloseWindow();
    return 0;
}




