#pragma once

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <fstream>
#include <ctime>
#include <random>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "generic_functions.h"
#include "entity.h"
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
#define spriteCountX 16
#define spriteCountY 8
#define TILE_SIZE 0.15f
#define MAX_ENEMIES 10
#define MAX_BULLETS 25
#define WORLD_OFFSET_X 0
#define WORLD_OFFSET_Y 0
#define RANDOM_DIRECTION (direction::directions)rand()%2

class Application {
public:
	Application();
	~Application();

	bool UpdateAndRender();
private:
	SDL_Event event;
	SDL_Window* displayWindow;

	GLuint ssID;
	GLuint pokemonSS;

	Mix_Chunk* jump;
	Mix_Chunk* shoot;
	Mix_Music* music;

	Entity player;
	Entity *enemies[MAX_ENEMIES];
	Entity *bullets[MAX_BULLETS];

	const Uint8 *keys;

	unsigned char **levelData;
	int mapWidth;
	int mapHeight;
	int enemyIndex;
	int playerFacing;
	int bulletIndex;
	float shootCD;
	bool isSolid(int);

	float lastFrameTicks;
	float timeLeftOver;

	void Init();
	void update(float);
	void FixedUpdate();
	void render();
	void spawnMap();
	bool readHeader(std::ifstream&);
	bool readLayerData(std::ifstream&);
	bool readEntityData(std::ifstream&);
	void placeEntity(std::string type, float placeX, float placeY);
	void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY);
	void renderGameLevel();
	void doLevelCollisionY(Entity* temp);
	void doLevelCollisionX(Entity* temp);
	float checkPointForGridCollisionY(float x, float y);
	float checkPointForGridCollisionX(float x, float y);
	void shootBullet();
	void renderBullet();
};