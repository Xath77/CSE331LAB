/* USER CODE BEGIN Header */

/* USER CODE END Header */

#include "main.h"
#include "fonts.h"
#include "ssd1306.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN PV */
// Game state variables
typedef enum {
  PLAYER_BLUE = 0,
  PLAYER_RED = 1
} Player_t;

typedef enum {
  POSITION_EMPTY = 0,
  POSITION_BLUE = 1,
  POSITION_RED = 2
} PositionState_t;

typedef enum {
  GAME_ONGOING = 0,
  GAME_BLUE_WINS = 1,
  GAME_RED_WINS = 2,
  GAME_DRAW = 3
} GameState_t;

Player_t currentPlayer = PLAYER_BLUE;  // Blue starts first
PositionState_t gameBoard[10];  // Position 0 unused, 1-9 are game positions
GameState_t gameState = GAME_ONGOING;
uint8_t moveCount = 0;  // Track number of moves for draw detection
uint8_t winningPositions[9];  // Store ALL winning positions (max 9 if all lines win)
uint8_t winningCount = 0;  // Number of winning positions
bool hasWinningCombo = false;

/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
char Keypad_Read(void);
void Turn_On_Blue_LED(uint8_t position);
void Turn_On_Red_LED(uint8_t position);
void Turn_Off_LED(uint8_t position, Player_t player);
void Turn_Off_All_LEDs(void);
void Reset_Game(void);
void Display_Current_Player(void);
void Display_Invalid_Move(void);
void Display_Winner(Player_t winner);
void Display_Draw(void);
bool Check_Winner(Player_t player);
bool Check_Draw(void);
void Add_Winning_Position(uint8_t pos);
bool Is_Position_In_Winning_List(uint8_t pos);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */


int main(void)
{
  /* USER CODE BEGIN 1 */
  char key = 0;
  uint32_t lastBlinkTime = 0;
  bool blinkState = true;
  /* USER CODE END 1 */

  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C2_Init();

  /* USER CODE BEGIN 2 */
  SSD1306_Init();
  SSD1306_Clear();

  Reset_Game();

  SSD1306_GotoXY(0, 5);
  SSD1306_Puts("TIC-TAC-TOE", &Font_11x18, 1);
  SSD1306_GotoXY(20, 30);
  SSD1306_Puts("Blue vs Red", &Font_7x10, 1);
  SSD1306_GotoXY(10, 50);
  SSD1306_Puts("Press 0 to Reset", &Font_7x10, 1);
  SSD1306_UpdateScreen();

  HAL_Delay(3000);  // Show message for 3 seconds

  // Display first player
  Display_Current_Player();

  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // If game is won, blink ALL winning LEDs
    if((gameState == GAME_BLUE_WINS || gameState == GAME_RED_WINS) && hasWinningCombo)
    {
      if(HAL_GetTick() - lastBlinkTime >= 500)  // Blink every 500ms
      {
        lastBlinkTime = HAL_GetTick();
        blinkState = !blinkState;

        Player_t winner = (gameState == GAME_BLUE_WINS) ? PLAYER_BLUE : PLAYER_RED;

        // Toggle ALL winning LEDs
        for(int i = 0; i < winningCount; i++)
        {
          if(blinkState)
          {
            if(winner == PLAYER_BLUE)
              Turn_On_Blue_LED(winningPositions[i]);
            else
              Turn_On_Red_LED(winningPositions[i]);
          }
          else
          {
            Turn_Off_LED(winningPositions[i], winner);
          }
        }
      }
    }

    // Read keypad
    key = Keypad_Read();

    if(key != 0)  // If a key was pressed
    {
      // If '0' is pressed, reset the game
      if(key == '0')
      {
        Reset_Game();
        Display_Current_Player();
        hasWinningCombo = false;
      }
      // If a number 1-9 is pressed and game is ongoing
      else if(key >= '1' && key <= '9' && gameState == GAME_ONGOING)
      {
        uint8_t position = key - '0';  // Convert char to number

        // Check if position is already taken
        if(gameBoard[position] != POSITION_EMPTY)
        {
          // Invalid move - position already taken
          Display_Invalid_Move();
          HAL_Delay(1500);  // Show error for 1.5 seconds
          Display_Current_Player();  // Return to player display
        }
        else
        {
          // Valid move - claim the position
          if(currentPlayer == PLAYER_BLUE)
          {
            gameBoard[position] = POSITION_BLUE;
            Turn_On_Blue_LED(position);
            moveCount++;

            // Check if Blue wins
            if(Check_Winner(PLAYER_BLUE))
            {
              gameState = GAME_BLUE_WINS;
              hasWinningCombo = true;
              Display_Winner(PLAYER_BLUE);
              lastBlinkTime = HAL_GetTick();
            }
            else if(Check_Draw())
            {
              gameState = GAME_DRAW;
              Display_Draw();
            }
            else
            {
              currentPlayer = PLAYER_RED;  // Switch to red player
              Display_Current_Player();
            }
          }
          else  // currentPlayer == PLAYER_RED
          {
            gameBoard[position] = POSITION_RED;
            Turn_On_Red_LED(position);
            moveCount++;

            // Check if Red wins
            if(Check_Winner(PLAYER_RED))
            {
              gameState = GAME_RED_WINS;
              hasWinningCombo = true;
              Display_Winner(PLAYER_RED);
              lastBlinkTime = HAL_GetTick();
            }
            else if(Check_Draw())
            {
              gameState = GAME_DRAW;
              Display_Draw();
            }
            else
            {
              currentPlayer = PLAYER_BLUE;  // Switch to blue player
              Display_Current_Player();
            }
          }
        }
      }

      // Wait for key release
      while(Keypad_Read() != 0)
      {
        HAL_Delay(10);
      }

      HAL_Delay(50);
    }
  }
  /* USER CODE END 3 */
}

void Add_Winning_Position(uint8_t pos)
{
  // Check if position already exists in list
  if(! Is_Position_In_Winning_List(pos))
  {
    winningPositions[winningCount] = pos;
    winningCount++;
  }
}

bool Is_Position_In_Winning_List(uint8_t pos)
{
  for(int i = 0; i < winningCount; i++)
  {
    if(winningPositions[i] == pos)
      return true;
  }
  return false;
}

bool Check_Winner(Player_t player)
{
  PositionState_t mark = (player == PLAYER_BLUE) ? POSITION_BLUE :   POSITION_RED;
  bool hasWon = false;

  // Reset winning positions
  winningCount = 0;

  // Check all rows
  if(gameBoard[1] == mark && gameBoard[2] == mark && gameBoard[3] == mark)
  {
    Add_Winning_Position(1);
    Add_Winning_Position(2);
    Add_Winning_Position(3);
    hasWon = true;
  }
  if(gameBoard[4] == mark && gameBoard[5] == mark && gameBoard[6] == mark)
  {
    Add_Winning_Position(4);
    Add_Winning_Position(5);
    Add_Winning_Position(6);
    hasWon = true;
  }
  if(gameBoard[7] == mark && gameBoard[8] == mark && gameBoard[9] == mark)
  {
    Add_Winning_Position(7);
    Add_Winning_Position(8);
    Add_Winning_Position(9);
    hasWon = true;
  }

  // Check all columns
  if(gameBoard[1] == mark && gameBoard[4] == mark && gameBoard[7] == mark)
  {
    Add_Winning_Position(1);
    Add_Winning_Position(4);
    Add_Winning_Position(7);
    hasWon = true;
  }
  if(gameBoard[2] == mark && gameBoard[5] == mark && gameBoard[8] == mark)
  {
    Add_Winning_Position(2);
    Add_Winning_Position(5);
    Add_Winning_Position(8);
    hasWon = true;
  }
  if(gameBoard[3] == mark && gameBoard[6] == mark && gameBoard[9] == mark)
  {
    Add_Winning_Position(3);
    Add_Winning_Position(6);
    Add_Winning_Position(9);
    hasWon = true;
  }

  // Check all diagonals
  if(gameBoard[1] == mark && gameBoard[5] == mark && gameBoard[9] == mark)
  {
    Add_Winning_Position(1);
    Add_Winning_Position(5);
    Add_Winning_Position(9);
    hasWon = true;
  }
  if(gameBoard[3] == mark && gameBoard[5] == mark && gameBoard[7] == mark)
  {
    Add_Winning_Position(3);
    Add_Winning_Position(5);
    Add_Winning_Position(7);
    hasWon = true;
  }

  return hasWon;
}

bool Check_Draw(void)
{
  return (moveCount >= 9);
}

void Reset_Game(void)
{
  // Clear game board
  for(int i = 0; i < 10; i++)
  {
    gameBoard[i] = POSITION_EMPTY;
  }

  // Clear winning positions
  for(int i = 0; i < 9; i++)
  {
    winningPositions[i] = 0;
  }
  winningCount = 0;

  // Turn off all LEDs
  Turn_Off_All_LEDs();

  // Reset game state
  currentPlayer = PLAYER_BLUE;
  gameState = GAME_ONGOING;
  moveCount = 0;
}

void Display_Current_Player(void)
{
  SSD1306_Clear();
  SSD1306_GotoXY(10, 0);
  SSD1306_Puts("Current Turn:", &Font_7x10, 1);

  if(currentPlayer == PLAYER_BLUE)
  {
    SSD1306_GotoXY(20, 25);
    SSD1306_Puts("BLUE", &Font_16x26, 1);
  }
  else
  {
    SSD1306_GotoXY(30, 25);
    SSD1306_Puts("RED", &Font_16x26, 1);
  }

  SSD1306_GotoXY(15, 55);
  SSD1306_Puts("Press 1-9", &Font_7x10, 1);

  SSD1306_UpdateScreen();
}

void Display_Invalid_Move(void)
{
  SSD1306_Clear();
  SSD1306_GotoXY(10, 15);
  SSD1306_Puts("INVALID", &Font_11x18, 1);
  SSD1306_GotoXY(20, 40);
  SSD1306_Puts("MOVE!", &Font_11x18, 1);
  SSD1306_UpdateScreen();
}

void Display_Winner(Player_t winner)
{
  SSD1306_Clear();

  if(winner == PLAYER_BLUE)
  {
    SSD1306_GotoXY(25, 10);
    SSD1306_Puts("BLUE", &Font_11x18, 1);
  }
  else
  {
    SSD1306_GotoXY(30, 10);
    SSD1306_Puts("RED", &Font_11x18, 1);
  }

  SSD1306_GotoXY(20, 35);
  SSD1306_Puts("WINS!", &Font_11x18, 1);

  SSD1306_GotoXY(10, 55);
  SSD1306_Puts("Press 0 to Reset", &Font_7x10, 1);

  SSD1306_UpdateScreen();
}

void Display_Draw(void)
{
  SSD1306_Clear();
  SSD1306_GotoXY(20, 20);
  SSD1306_Puts("DRAW!", &Font_16x26, 1);

  SSD1306_GotoXY(10, 50);
  SSD1306_Puts("Press 0 to Reset", &Font_7x10, 1);

  SSD1306_UpdateScreen();
}

void Turn_On_Blue_LED(uint8_t position)
{
  switch(position)
  {
    case 1:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 1);  // Blue LED 1
      break;
    case 2:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, 1);  // Blue LED 2
      break;
    case 3:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, 1);  // Blue LED 3
      break;
    case 4:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, 1);  // Blue LED 4
      break;
    case 5:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 1);  // Blue LED 5
      break;
    case 6:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 1);  // Blue LED 6
      break;
    case 7:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);  // Blue LED 7
      break;
    case 8:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 1);  // Blue LED 8
      break;
    case 9:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, 1);  // Blue LED 9
      break;
    default:
      break;
  }
}

void Turn_On_Red_LED(uint8_t position)
{
  switch(position)
  {
    case 1:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, 1);  // Red LED 1
      break;
    case 2:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, 1);  // Red LED 2
      break;
    case 3:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 1);  // Red LED 3
      break;
    case 4:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 1);   // Red LED 4
      break;
    case 5:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 1);   // Red LED 5
      break;
    case 6:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, 1);  // Red LED 6
      break;
    case 7:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 1);  // Red LED 7
      break;
    case 8:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 1);  // Red LED 8
      break;
    case 9:
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);  // Red LED 9
      break;
    default:
      break;
  }
}

void Turn_Off_LED(uint8_t position, Player_t player)
{
  if(player == PLAYER_BLUE)
  {
    switch(position)
    {
      case 1: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 0); break;
      case 2: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, 0); break;
      case 3: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, 0); break;
      case 4: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, 0); break;
      case 5: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 0); break;
      case 6: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 0); break;
      case 7: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 0); break;
      case 8: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 0); break;
      case 9: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, 0); break;
      default: break;
    }
  }
  else  // PLAYER_RED
  {
    switch(position)
    {
      case 1: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, 0); break;
      case 2: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, 0); break;
      case 3: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 0); break;
      case 4: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0); break;
      case 5: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0); break;
      case 6: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, 0); break;
      case 7: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 0); break;
      case 8: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 0); break;
      case 9: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0); break;
      default: break;
    }
  }
}

void Turn_Off_All_LEDs(void)
{
  // Turn off Blue LEDs
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, 0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, 0);

  // Turn off Red LEDs
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, 0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, 0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);
}


char Keypad_Read(void)
{
  // Your physical keypad layout:
  // 1 2 3 A
  // 4 5 6 B
  // 7 8 9 C
  // * 0 # D

  // Pin mapping:
  // Rows:   PB9, PB8, PB7, PB6
  // Columns: PB5, PB4, PB3, PA15

  // Reset all rows to LOW
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, 0);

  // Check ROW-1 (PB9) - Keys:        1, 2, 3, A
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, 1);
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, 0);
    return '1';
  }
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, 0);
    return '2';
  }
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, 0);
    return '3';
  }
  if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, 0);
    return 'A';
  }
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, 0);

  // Check ROW-2 (PB8) - Keys: 4, 5, 6, B
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 1);
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 0);
    return '4';
  }
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 0);
    return '5';
  }
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 0);
    return '6';
  }
  if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 0);
    return 'B';
  }
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 0);

  // Check ROW-3 (PB7) - Keys: 7, 8, 9, C
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 1);
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 0);
    return '7';
  }
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 0);
    return '8';
  }
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 0);
    return '9';
  }
  if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 0);
    return 'C';
  }
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 0);

  // Check ROW-4 (PB6) - Keys: *, 0, #, D
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 1);
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
    return '*';
  }
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
    return '0';
  }
  if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
    return '#';
  }
  if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == 1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
    return 'D';
  }
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);

  return 0;  // No key pressed
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.       OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.    HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.   HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.   PLL.    PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.   ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.   SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.  AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.  APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.  APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C2_Init(void)
{
  hi2c2.Instance = I2C2;
  hi2c2.Init.      ClockSpeed = 400000;
  hi2c2.Init.   DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.     OwnAddress1 = 0;
  hi2c2.Init.   AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.  DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.   OwnAddress2 = 0;
  hi2c2.Init.  GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level - Blue LEDs (PA0-PA7) */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
                           GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level - Blue LED 9 (PB0) and Red LEDs (PB12-PB15) */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level - Red LEDs (PA8-PA12) */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level - Keypad Rows */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0-PA7 (Blue LEDs 1-8) */
  GPIO_InitStruct.       Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
                        GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.       Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.   Pull = GPIO_NOPULL;
  GPIO_InitStruct.    Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins :       PA8-PA12 (Red LEDs 5-1) */
  GPIO_InitStruct.   Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.    Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.   Pull = GPIO_NOPULL;
  GPIO_InitStruct.   Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 (Blue LED 9) */
  GPIO_InitStruct.   Pin = GPIO_PIN_0;
  GPIO_InitStruct.    Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.  Pull = GPIO_NOPULL;
  GPIO_InitStruct.    Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12-PB15 (Red LEDs 9-6) */
  GPIO_InitStruct.   Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.   Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.   Pull = GPIO_NOPULL;
  GPIO_InitStruct.  Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA15 (Keypad Column input) */
  GPIO_InitStruct.  Pin = GPIO_PIN_15;
  GPIO_InitStruct.  Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.   Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB6 PB7 PB8 PB9 (Keypad Row outputs) */
  GPIO_InitStruct.  Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct. Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.  Pull = GPIO_NOPULL;
  GPIO_InitStruct. Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 PB5 (Keypad Column inputs) */
  GPIO_InitStruct. Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct. Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct. Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
