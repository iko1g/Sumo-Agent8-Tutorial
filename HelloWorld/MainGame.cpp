#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"
#include <sstream>

int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;

enum Agent8State
{
	STATE_APPEAR = 0,
	STATE_HALT,
	STATE_PLAY,
	STATE_DEAD,
};

struct GameState 
{
	int score = 0;
	Agent8State agentState = STATE_APPEAR;
	int highScore;
};

GameState gameState;

enum GameObjectType 
{
	TYPE_NULL = -1,
	TYPE_AGENT8,
	TYPE_FAN,
	TYPE_TOOL,
	TYPE_COIN,
	TYPE_STAR,
	TYPE_LASER,
	TYPE_DESTROYED,
};

void HandlePlayerControls();
void UpdateFan();
void UpdateTools();
void UpdateCoinAndStars();
void UpdateLasers();
void UpdateDestroyed();
void UpdateAgent8();
void UpdateDebugText();
void PlayAgent8DeathAnim();
void UpdateHighScore();
//void UpdateAgent8AfterDeath();

// The entry point for a PlayBuffer program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::StartAudioLoop("music");
	int id_fan = Play::CreateGameObject(TYPE_FAN, {1140, 217}, 0, "fan");
	Play::GetGameObject(id_fan).velocity = {0, 3};
	Play::GetGameObject(id_fan).animSpeed = 1.0f;
	Play::CreateGameObject(TYPE_AGENT8, { 115, 0 }, 50, "agent8");
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	Play::DrawBackground();
	UpdateAgent8();
	UpdateFan();
	UpdateTools();
	UpdateCoinAndStars();
	UpdateLasers();
	UpdateDestroyed();
	//UpdateDebugText();
	Play::DrawFontText("64px", "ARROW KEYS TO MOVE UP AND DOWN AND SPACE TO FIRE", {DISPLAY_WIDTH/2 , DISPLAY_HEIGHT - 30}, Play::CENTRE);
	Play::DrawFontText("132px", "SCORE:" + std::to_string(gameState.score), { DISPLAY_WIDTH / 2 , 50}, Play::CENTRE);
	Play::PresentDrawingBuffer();
	return Play::KeyDown( VK_ESCAPE );
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();
	return PLAY_OK;
}


//Used to control player movement
void HandlePlayerControls()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);

	if (Play::KeyDown(VK_UP)) 
	{
		obj_agent8.velocity = { 0,-4 };
		Play::SetSprite(obj_agent8, "agent8_climb", 0.25f);
	}
	else if (Play::KeyDown(VK_DOWN)) 
	{
		obj_agent8.acceleration = { 0, 1 };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
	}
	else 
	{
		if (obj_agent8.velocity.y > 5) 
		{
			gameState.agentState = STATE_HALT;
			Play::SetSprite(obj_agent8, "agent8_halt", 0.333f);
			obj_agent8.acceleration = {0, 0};
		}
		else 
		{
			Play::SetSprite(obj_agent8, "agent8_hang", 0.02f);
			obj_agent8.velocity *= 0.5f;
			obj_agent8.acceleration = {0, 0};
		}
	}

	if (Play::KeyPressed(VK_SPACE)) 
	{
		Vector2D firePos = obj_agent8.pos + Vector2D(155, -75);
		int id = Play::CreateGameObject(TYPE_LASER, firePos, 30 , "laser");
		Play::GetGameObject(id).velocity = { 32, 0 };
		Play::PlayAudio("shoot");
	}

	Play::UpdateGameObject(obj_agent8);

	if (Play::IsLeavingDisplayArea(obj_agent8)) 
	{
		obj_agent8.pos = obj_agent8.oldPos;
	}

	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(obj_agent8);
}


//Used to update fan position and based on said position spawn tools and coins
void UpdateFan() 
{
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);

	if (Play::RandomRoll(50) == 50) 
	{
		int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 50, "driver");
		GameObject& obj_tool = Play::GetGameObject(id);
		obj_tool.velocity = Point2f(-8, Play::RandomRollRange(-1, 1) * 6);

		if (Play::RandomRoll(2) == 1) 
		{
			Play::SetSprite(obj_tool, "spanner", 0);
			obj_tool.radius = 100;
			obj_tool.velocity.x = -4;
			obj_tool.rotSpeed = 0.1f;
		}

		Play::PlayAudio("tool");
	}

	if (Play::RandomRoll(150) == 1) 
	{
		int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 40, "coin");
		GameObject& obj_coin = Play::GetGameObject(id);
		obj_coin.velocity = { -3, 0 };
		obj_coin.rotSpeed = 0.1f;
	}

	Play::UpdateGameObject(obj_fan);

	if (Play::IsLeavingDisplayArea(obj_fan)) 
	{
		obj_fan.pos = obj_fan.oldPos;
		obj_fan.velocity.y *= -1;
	}

	Play::DrawObject(obj_fan);

}


//Used to change game state when the player collides with tool and update the score accordingly 
void UpdateTools() 
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);

	for (int id : vTools) 
	{
		GameObject& obj_tool = Play::GetGameObject(id);

		if (gameState.agentState != STATE_DEAD && Play::IsColliding(obj_tool, obj_agent8)) 
		{
			Play::StopAudioLoop("music");
			Play::PlayAudio("die");
			//UpdateHighScore();
			gameState.agentState = STATE_DEAD;
		}

		Play::UpdateGameObject(obj_tool);

		if (Play::IsLeavingDisplayArea(obj_tool, Play::VERTICAL)) 
		{
			obj_tool.pos = obj_tool.oldPos;
			obj_tool.velocity.y *= -1;
		}

		Play::DrawObjectRotated(obj_tool);

		if (!Play::IsVisible(obj_tool)) 
		{
			Play::DestroyGameObject(id);
		}
	}
}


//Used to update the score if the player collides/touches with the coin
void UpdateCoinAndStars() 
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	for (int id_coin : vCoins) 
	{
		GameObject& obj_coin = Play::GetGameObject(id_coin);
		bool hasCollided = false;

		if (Play::IsColliding(obj_coin, obj_agent8)) 
		{
			for (float rad{0.25f}; rad < 2.0f; rad += 0.5f)
			{
				int id = Play::CreateGameObject(TYPE_STAR, obj_agent8.pos, 0 , "star");
				GameObject& obj_star = Play::GetGameObject(id);
				obj_star.rotSpeed = 0.01f;
				obj_star.acceleration = { 0.0f, 0.5f};
				Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);
			}

			if (gameState.agentState == STATE_DEAD) 
			{
				gameState.score = gameState.score;
			}
			else 
			{
				hasCollided = true;
				gameState.score += 500;
				Play::PlayAudio("collect");
			}
		}

		Play::UpdateGameObject(obj_coin);
		Play::DrawObjectRotated(obj_coin);

		if (!Play::IsVisible(obj_coin)|| hasCollided) 
		{
			Play::DestroyGameObject(id_coin);
		}
	}

	std::vector<int> vStars = Play::CollectGameObjectIDsByType(TYPE_STAR);

	for (int id_star : vStars) 
	{
		GameObject& obj_star = Play::GetGameObject(id_star);

		Play::UpdateGameObject(obj_star);
		Play::DrawObjectRotated(obj_star);

		if (!Play::IsVisible(obj_star)) 
		{
			Play::DestroyGameObject(id_star);
		}
	}
}


//Used to update score accordingly if the player shoots at a coin or a tool
//and update objects when they collide with a laser
void UpdateLasers() 
{
	std::vector<int> vLasers = Play::CollectGameObjectIDsByType(TYPE_LASER);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	for (int id_laser : vLasers) 
	{
		GameObject& obj_laser = Play::GetGameObject(id_laser);
		bool hasCollided = false;

		for (int id_tool : vTools) 
		{
			GameObject& obj_tool = Play::GetGameObject(id_tool);

			if (Play::IsColliding(obj_laser, obj_tool)) 
			{
				hasCollided = true;
				obj_tool.type = TYPE_DESTROYED;
				gameState.score += 100;
			}
		}

		for (int id_coin : vCoins) 
		{
			GameObject& obj_coin = Play::GetGameObject(id_coin);

			if (Play::IsColliding(obj_laser, obj_coin)) 
			{
				hasCollided = true;
				obj_coin.type = TYPE_DESTROYED;
				Play::PlayAudio("error");
				gameState.score -= 300;
			}
		}

		if (gameState.score < 0) 
		{
			gameState.score = 0;
		}

		Play::UpdateGameObject(obj_laser);
		Play::DrawObject(obj_laser);

		if (!Play::IsVisible(obj_laser) || hasCollided) 
		{
			Play::DestroyGameObject(id_laser);
		}
	}
}


//Used to show fade out animation of destroyed tools or coins and
//delete said objects after being 'dead'
void UpdateDestroyed() 
{
	std::vector<int> vDead = Play::CollectGameObjectIDsByType(TYPE_DESTROYED);

	for (int id_dead : vDead) 
	{
		GameObject& obj_dead = Play::GetGameObject(id_dead);
		obj_dead.animSpeed = 0.2f;
		Play::UpdateGameObject(obj_dead);

		if (obj_dead.frame % 2)
		{
			Play::DrawObjectRotated(obj_dead, (10 - obj_dead.frame) / 10.0f);
		}

		if (!Play::IsVisible(obj_dead) || obj_dead.frame >= 10) 
		{
			Play::DestroyGameObject(id_dead);
		}
	}
}

//Used to update agent/game state
void UpdateAgent8() {
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);

	switch (gameState.agentState)
	{
		case STATE_APPEAR:
			obj_agent8.velocity = {0,12};
			obj_agent8.acceleration = {0, 0.5f};
			Play::SetSprite(obj_agent8, "agent8_fall", 0);
			obj_agent8.rotation = 0;

			if (obj_agent8.pos.y >= DISPLAY_HEIGHT / 3) 
			{
				gameState.agentState = STATE_PLAY;
			}
			break;

		case STATE_HALT:
			obj_agent8.velocity *= 0.9f;
			if (Play::IsAnimationComplete(obj_agent8)) 
			{
				gameState.agentState = STATE_PLAY;
			}
			break;

		case STATE_PLAY:
			HandlePlayerControls();
			break;

		case STATE_DEAD:
			PlayAgent8DeathAnim();

			Play::DrawFontText("132px", "HIGHSCORE:" + std::to_string(gameState.highScore), {DISPLAY_WIDTH / 2, DISPLAY_HEIGHT/ 2}, Play::CENTRE);

			if (gameState.score > gameState.highScore) 
			{
				gameState.highScore = gameState.score;

				for (float rad{ 0.25f }; rad < 2.0f; rad += 0.25f)
				{
					int id = Play::CreateGameObject(TYPE_STAR, { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, 0, "star");
					GameObject& obj_star = Play::GetGameObject(id);
					obj_star.rotSpeed = 0.01f;
					obj_star.acceleration = { 0.0f, 0.5f };
					Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);
				}

				std::vector<int> vStars = Play::CollectGameObjectIDsByType(TYPE_STAR);

				for (int id_star : vStars)
				{
					GameObject& obj_star = Play::GetGameObject(id_star);

					Play::UpdateGameObject(obj_star);
					Play::DrawObjectRotated(obj_star);

					if (!Play::IsVisible(obj_star))
					{
						Play::DestroyGameObject(id_star);
					}
				}
			}
			
			if (Play::KeyPressed(VK_SPACE) == true) 
			{
				Play::CreateGameObject(TYPE_AGENT8, { 115, 0 }, 50, "agent8");
				gameState.agentState = STATE_APPEAR;
				obj_agent8.pos = { 115, 0};
				obj_agent8.velocity = { 0, 0};
				obj_agent8.frame = 0;
				Play::StartAudioLoop("music");
				gameState.score = 0;

				for (int tool_obj : Play::CollectGameObjectIDsByType(TYPE_TOOL)) 
				{
					Play::GetGameObject(tool_obj).type = TYPE_DESTROYED;
				}

				for (int coin_obj : Play::CollectGameObjectIDsByType(TYPE_COIN))
				{
					Play::GetGameObject(coin_obj).type = TYPE_DESTROYED;
				}
			}
			break;
	}

	Play::UpdateGameObject(obj_agent8);

	if (Play::IsLeavingDisplayArea(obj_agent8) && gameState.agentState != STATE_DEAD) 
	{
		obj_agent8.pos = obj_agent8.oldPos;
	}

	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(obj_agent8);
}


//Used for debugging player position when:
//Player position during play, Player leaving screen after death , Player position after death
void UpdateDebugText() 
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);
	bool leavingV = Play::IsLeavingDisplayArea(obj_agent8, Play::VERTICAL);
	bool leavingH = Play::IsLeavingDisplayArea(obj_agent8, Play::HORIZONTAL);

	if (leavingH) 
	{
		Play::DrawDebugText({ obj_agent8.pos.x,obj_agent8.pos.y }, "HORIZONTAL", Play::cGreen, true);
	}

	if (leavingV) 
	{
		Play::DrawDebugText({ obj_agent8.pos.x,obj_agent8.pos.y }, "VERTICAL", Play::cGreen, true);
	}

	if (!Play::IsColliding(obj_agent8, obj_fan))
	{
		DrawDebugText({ DISPLAY_WIDTH / 2 , 100 }, "fan and agent are touching", Play::cGreen, true);
	}

	Play::DrawDebugText({obj_agent8.pos.x,obj_agent8.pos.y },"HERE!!!!!!!!!!!!!!", Play::cGreen, true);
}

//Used to 'play' the player death animation
void PlayAgent8DeathAnim() 
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	obj_agent8.rotation += 0.25f;
	obj_agent8.acceleration = { -0.3f, 0.5f };
}

//Updates High Score value
void UpdateHighScore() 
{
	if (gameState.highScore == 0) 
	{
		gameState.highScore = gameState.score;
	}
	else 
	{
		if (gameState.score > gameState.highScore)
		{
			gameState.highScore = gameState.score;
		}
	}
}

/*
* -------------------------------------------------------------------------------
void UpdateAgent8AfterDeath()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);
	std::vector<int> vAgent8 = Play::CollectGameObjectIDsByType(TYPE_AGENT8);
	int id_agent8 = vAgent8.front();
	Play::DestroyGameObject(id_agent8);
}
-------------------------------------------------------------------------------
Initialy made because the player was still getting coins after death. This increased
the score. Though most players would either keep playing the game after death or close the
game I wanted to introduce a high score system so this bug had to be fixed. I used the
UpdateAgent8DebugText to figure out that the player was colliding with the fan on the right after death and
when a coin was spawned it would immediately collide with the not visible but still there agent8, increasing the score.
So intitially I thought I could delete the player after death (as seen above). This led to some death animations not working so I
thought of another solution. I thought to stop spawning items after death, stop spawning coins after death and so on. But the simplest solution
was just that if the state is STATE_DEAD then the score will not be updated, else then you can update the score if the player and
coin collide.
--------------------------------------------------------------------------------
*/