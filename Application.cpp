#include "Application.h"
using namespace std;

Application::Application() {
	Init();
}

Application::~Application() {
	SDL_Quit();
	Mix_FreeChunk(jump);
	Mix_FreeChunk(shoot);
	Mix_FreeMusic(music);
}

void Application::Init() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Sidescroller", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	glViewport(0, 0, 800, 600);
	glMatrixMode(GL_PROJECTION);
	glOrtho(-2.66, 2.66, -2.0, 2.0, -2.0, 2.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	srand(time(NULL));
	music = Mix_LoadMUS("untitled.wav");
	jump = Mix_LoadWAV("jump.wav");
	shoot = Mix_LoadWAV("shoot.wav");
	lastFrameTicks = 0.0f;
	timeLeftOver = 0.0f;
	enemyIndex = 0;
	bulletIndex = 0;
	shootCD = 0.2f;
	ssID = LoadTexture("sprites.png", GL_RGBA);
	pokemonSS = LoadTexture("pokemon_ss.png", GL_RGBA);
	keys = SDL_GetKeyboardState(NULL);
	playerFacing = direction::RIGHT;

	for (int i = 0; i < MAX_ENEMIES; ++i) {
		enemies[i] = NULL;
	}
	for (int i = 0; i < MAX_BULLETS; ++i) {
		bullets[i] = NULL;
	}
	spawnMap();
	Mix_PlayMusic(music, -1);
}

bool Application::UpdateAndRender() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			return true;
		}
	}
	// timestep 
	float fixedElapsed = elapsed + timeLeftOver;
	if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
		fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
	}
	while (fixedElapsed >= FIXED_TIMESTEP) {
		fixedElapsed -= FIXED_TIMESTEP;
		FixedUpdate();
		update(elapsed);
	}

	timeLeftOver = fixedElapsed;

	render();
	return false;
}

void Application::update(float elapsed) {
	player.setXAccel(0.0f);
	if (player.getCollideBottom()) {
		if (keys[SDL_SCANCODE_UP]) {
			player.setYVel(3.5f);
			Mix_PlayChannel(-1, jump, 0);
		}
	}
	if (keys[SDL_SCANCODE_LEFT]) {
		player.setXAccel(-3.5f);
		playerFacing = direction::LEFT;
	}
	else if (keys[SDL_SCANCODE_RIGHT]) {
		player.setXAccel(3.5f);
		playerFacing = direction::RIGHT;
	}
	if (keys[SDL_SCANCODE_SPACE]) {
		shootBullet();
	}
	// update player
	player.setX(player.getX() + player.getXVel() * FIXED_TIMESTEP);
	player.setY(player.getY() + player.getYVel() * FIXED_TIMESTEP);
	// update enemies
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		if (enemies[i] != NULL) {
			enemies[i]->setX(enemies[i]->getX() + enemies[i]->getXVel() * FIXED_TIMESTEP);
			enemies[i]->setY(enemies[i]->getY() + enemies[i]->getYVel() * FIXED_TIMESTEP);
		}
	}
	// update bullets
	for (int i = 0; i < MAX_BULLETS; ++i) {
		if (bullets[i] != NULL) {
			bullets[i]->setX(bullets[i]->getX() + bullets[i]->getXVel() * FIXED_TIMESTEP);
		}
	}
	shootCD += FIXED_TIMESTEP;
}

void Application::FixedUpdate() {
	player.setCollideBottom(false);
	player.setCollideTop(false);
	player.setCollideLeft(false);
	player.setCollideRight(false);
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		if (enemies[i] != NULL) {
			enemies[i]->setCollideBottom(false);
			enemies[i]->setCollideTop(false);
			enemies[i]->setCollideLeft(false);
			enemies[i]->setCollideRight(false);
		}
	}
	player.setXVel(lerp(player.getXVel(), 0.0f, FIXED_TIMESTEP * player.getXFriction()));
	player.setYVel(lerp(player.getYVel(), 0.0f, FIXED_TIMESTEP * player.getYFriction()));
	player.setXVel(player.getXVel() + player.getXAccel() * FIXED_TIMESTEP);
	player.setYVel(player.getYVel() + player.getYAccel() * FIXED_TIMESTEP);
	player.setXVel(player.getXVel() + x_gravity * FIXED_TIMESTEP);
	player.setYVel(player.getYVel() + y_gravity * FIXED_TIMESTEP);

	// enemies gravity logic
	for (int i = 0; i < MAX_ENEMIES - 1; ++i) {
		if (enemies[i] != NULL) {
			enemies[i]->setXVel(enemies[i]->getXVel() + x_gravity * FIXED_TIMESTEP);
			enemies[i]->setYVel(enemies[i]->getYVel() + y_gravity * FIXED_TIMESTEP);
		}
	}

	// player collision
	doLevelCollisionY(&player);
	doLevelCollisionX(&player);
	// enemy collision
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		if (enemies[i] != NULL) {
			doLevelCollisionY(enemies[i]);
			doLevelCollisionX(enemies[i]);
			if (enemies[i]->getCollideLeft()) {
				enemies[i]->setXVel(-1 * enemies[i]->getXVel());
				enemies[i]->setDirection(direction::RIGHT);
			}
			else if (enemies[i]->getCollideRight()) {
				enemies[i]->setXVel(-1 * enemies[i]->getXVel());
				enemies[i]->setDirection(direction::LEFT);
			}
		}
	}
	// bullet collision with static entity
	for (int i = 0; i < MAX_BULLETS; ++i) {
		if (bullets[i] != NULL) {
			doLevelCollisionX(bullets[i]);
			if (bullets[i]->getCollideLeft()) {
				delete bullets[i];
				bullets[i] = NULL;
			}
			else if (bullets[i]->getCollideRight()) {
				delete bullets[i];
				bullets[i] = NULL;
			}
		}
	}
	// bullet collision with enemies
	for (int i = 0; i < MAX_BULLETS; ++i) {
		if (bullets[i] != NULL) {
			for (int j = 0; j < MAX_ENEMIES; ++j) {
				if (enemies[j] != NULL) {
					if (bullets[i]->collidesWith(enemies[j])) {
						delete bullets[i];
						bullets[i] = NULL;
						delete enemies[j];
						enemies[j] = NULL;
						break;
					}
				}
			}
		}
	}
}

void Application::render() {
	SDL_GL_SwapWindow(displayWindow);
	glClearColor(0.0392f, 0.596f, 0.6745f, 0.0f);
	//glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glLoadIdentity();
	glTranslatef(-player.getX(), -player.getY(), 0.0f);
	renderGameLevel();
	for (int i = 0; i < MAX_ENEMIES; ++i) {
		if (enemies[i] != NULL) {
			enemies[i]->ssDraw();
		}
	}
	renderBullet();
	player.ssDraw();
}

void Application::spawnMap() {
	ifstream infile("new.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				return;
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[Objects]") {
			readEntityData(infile);
		}
	}
}

bool Application::readHeader(ifstream& stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }

		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);

		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height"){
			mapHeight = atoi(value.c_str());
		}
	}

	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new unsigned char*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned char[mapWidth];
		}
		return true;
	}
}

bool Application::readLayerData(ifstream& stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					unsigned char val = (unsigned char)atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 12;
					}
				}
			}
		}
	}
	return true;
}

bool Application::readEntityData(ifstream& stream) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
			float placeX = atoi(xPosition.c_str()) / 16 * TILE_SIZE;
			float placeY = atoi(yPosition.c_str()) / 16 * -TILE_SIZE;
			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}

void Application::placeEntity(string type, float placeX, float placeY) {
	if (type == "Start")  {
		Spritesheet tmp = Spritesheet(pokemonSS, 0.0f / 128.0f, 86.0f / 128.0f, 28.0f / 128.0f, 32.0f / 128.0f);
		player = Entity(tmp, placeX, placeY);
	}
	else if (type == "Enemy") {
		if (enemyIndex < MAX_ENEMIES) {
			float u = (float)(80 % spriteCountX) / (float)spriteCountX;
			float v = (float)(80 / spriteCountX) / (float)spriteCountY;
			float spriteWidth = 1.0f / (float)spriteCountX;
			float spriteHeight = 1.0f / (float)spriteCountY;
			Spritesheet tmp = Spritesheet(ssID, u, v, spriteWidth, spriteHeight);
			enemies[enemyIndex] = new Entity(tmp, placeX, placeY);
			enemies[enemyIndex]->setDirection(RANDOM_DIRECTION);
			enemies[enemyIndex]->setScale(0.5f);
			if (enemies[enemyIndex]->getDirection() == direction::RIGHT)
				enemies[enemyIndex]->setXVel(1.5f);
			else
				enemies[enemyIndex]->setXVel(-1.5f);
			++enemyIndex;
		}
	}
}

void Application::renderGameLevel() {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ssID);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	vector<float> vertexData;
	vector<float> texCoordData;
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			float u = (float)(((int)levelData[y][x]) % spriteCountX) / (float)spriteCountX;
			float v = (float)(((int)levelData[y][x]) / spriteCountX) / (float)spriteCountY;
			float spriteWidth = 1.0f / (float)spriteCountX * 0.999f;
			float spriteHeight = 1.0f / (float)spriteCountY * 0.999f;
			vertexData.insert(vertexData.end(), {
				TILE_SIZE * x, -TILE_SIZE * y,
				TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
			});
			texCoordData.insert(texCoordData.end(), { u, v,
				u, v + (spriteHeight),
				u + spriteWidth, v + (spriteHeight),
				u + spriteWidth, v
			});
		}
	}
	glVertexPointer(2, GL_FLOAT, 0, vertexData.data());
	glEnableClientState(GL_VERTEX_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoordData.data());
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDrawArrays(GL_QUADS, 0, mapWidth * mapHeight * 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void Application::worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)((worldX + (WORLD_OFFSET_X)) / TILE_SIZE);
	*gridY = (int)((-worldY + (WORLD_OFFSET_Y)) / TILE_SIZE);
}

float Application::checkPointForGridCollisionY(float x, float y) {
	int gridX, gridY;
	worldToTileCoordinates(x, y, &gridX, &gridY);
	if (gridX < 0 || gridX > 100 || gridY < 0 || gridY > 100) {
		return 0.0f;
	}
	if (isSolid(levelData[gridY][gridX])) {
		float yCoord = (gridY * TILE_SIZE);
		return -y - yCoord;
	}
	return 0.0f;
}

float Application::checkPointForGridCollisionX(float x, float y) {
	int gridX, gridY;
	worldToTileCoordinates(x, y, &gridX, &gridY);
	if (gridX < 0 || gridX > 100 || gridY < 0 || gridY > 100) {
		return 0.0f;
	}
	if (isSolid(levelData[gridY][gridX])) {
		float xCoord = (gridX * TILE_SIZE);
		return -x - xCoord;
	}
	return 0.0f;
}

void Application::doLevelCollisionY(Entity* temp) {
	float adjust = checkPointForGridCollisionY(temp->getX(), temp->getY() - temp->getHeight() * 0.5f);
	if (adjust != 0.0f) {
		temp->setY(temp->getY() + adjust + 0.0001f);
		temp->setYVel(0.0f);
		temp->setCollideBottom(true);
	}
	adjust = checkPointForGridCollisionY(temp->getX(), temp->getY() + temp->getHeight() * 0.5f);
	if (adjust != 0.0f) {
		temp->setY(temp->getY() - adjust - 0.0001f);
		temp->setYVel(0.0f);
		temp->setCollideTop(true);
	}
}

void Application::doLevelCollisionX(Entity* temp) {
	float adjust = checkPointForGridCollisionX(temp->getX() - temp->getWidth() * 0.5f, temp->getY());
	if (adjust != 0.0f) {
		temp->setX(temp->getX() - adjust * TILE_SIZE *0.008f);
		temp->setXVel(0.0f);
		temp->setCollideLeft(true);
	}
	adjust = checkPointForGridCollisionX(temp->getX() + temp->getWidth() * 0.5f, temp->getY());
	if (adjust != 0.0f) {
		temp->setX(temp->getX() + adjust * TILE_SIZE * 0.001);
		temp->setXVel(0.0f);
		temp->setCollideRight(true);
	}
}

bool Application::isSolid(int tileID) {
	switch (tileID) {
	case 0:
		return true;
		break;
	case 1:
		return true;
		break;
	case 2:
		return true;
		break;
	case 3:
		return true;
		break;
	case 4:
		return true;
		break;
	case 5:
		return true;
		break;
	case 6:
		return true;
		break;
	case 7:
		return true;
		break;
	case 16:
		return true;
		break;
	case 17:
		return true;
		break;
	case 18:
		return true;
		break;
	case 19:
		return true;
		break;
	case 20:
		return true;
		break;
	case 32:
		return true;
		break;
	case 33:
		return true;
		break;
	case 34:
		return true;
		break;
	case 35:
		return true;
		break;
	default:
		return false;
		break;
	}
}

void Application::shootBullet() {
	if (bulletIndex < MAX_BULLETS) {
		if (shootCD >= 0.3f) {
			if (bullets[bulletIndex] != NULL) {
				delete bullets[bulletIndex];
			}
			bullets[bulletIndex] = new Entity(Spritesheet(pokemonSS, 0.0f / 128.0f, 52.0f / 128.0f, 32.0f / 128.0f, 32.0f / 128.0f), player.getX(), player.getY());
			if (playerFacing == direction::RIGHT)
				bullets[bulletIndex]->setXVel(3.0f);
			else
				bullets[bulletIndex]->setXVel(-3.0f);
			bullets[bulletIndex]->setScale(0.5f);
			bulletIndex++;
			Mix_PlayChannel(-1, shoot, 0);
			shootCD = 0.0f;
		}
	}
	else {
		if (shootCD >= 0.3f) {
			bulletIndex = 0;
			if (bullets[bulletIndex] != NULL) {
				delete bullets[bulletIndex];
			}
			bullets[bulletIndex] = new Entity(Spritesheet(pokemonSS, 0.0f / 128.0f, 52.0f / 128.0f, 32.0f / 128.0f, 32.0f / 128.0f), player.getX(), player.getY());
			if (playerFacing == direction::RIGHT)
				bullets[bulletIndex]->setXVel(3.0f);
			else
				bullets[bulletIndex]->setXVel(-3.0f);
			bullets[bulletIndex]->setScale(0.5f);
			bulletIndex++;
			Mix_PlayChannel(-1, shoot, 0);
			shootCD = 0.0f;
		}
	}
}

void Application::renderBullet() {
	for (int i = 0; i < MAX_BULLETS; ++i) {
		if (bullets[i] != NULL) {
			bullets[i]->ssDraw();
		}
	}
}