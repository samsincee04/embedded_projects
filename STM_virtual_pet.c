#include "stm32f0xx_hal.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdio.h>
#include "gfx01m2.h" // your LCD + joystick helpers
// ===================== RTOS Objects =====================
static osThreadId_t renderTaskHandle;
static osThreadId_t inputTaskHandle;
static osThreadId_t logicTaskHandle;
static osMutexId_t lcdMutex;
static osMessageQueueId_t inputQueue;
// ===================== Game State =====================
typedef enum {
APP_IDLE = 0,
APP_MENU,
APP_PLAYING,
APP_FEEDING,
APP_WAVING,
APP_RAN_AWAY,
APP_DEAD
} AppState;
typedef enum {
BTN_NONE = 0,
BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_CENTER
} BtnEvent;
typedef struct {
int happiness; // 0..100
int hunger; // 0..100 (fullness)
AppState state;
int menuIndex; // 0: Feed, 1: Play, 2: Exit
} Pet;
static Pet pet = { .happiness = 80, .hunger = 80, .state = APP_IDLE, .menuIndex = 0
};
static uint8_t clear_screen_flag = 0;
// ===================== Palette + Sprite “XPM-lite” =====================
//
// 20x20 ASCII sprites; characters map to RGB565 colors.
// . = background, B = body, E = eye, M = mouth, F = food, P = play ball/star
// You can tweak shapes directly in these arrays.
//
#define SPR_W 20
#define SPR_H 20
static const char *IDLE_0[SPR_H] = {
"....................",
"....................",
".......BBBBBB.......",
".....BBBBBBBBBB.....",
"....BBBBBBBBBBBB....",
"...BBBBBBBBBBBBBB...",
"...BBBBE BBB EBBB...",
"..BBBBBBBBMBBBBBB...",

"..BBBBBBBBBBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"...BBBBBBBBBBBBB....",
"...BBBBBBBBBBBBB....",
"....BBBBBBBBBBB.....",
".....BBBBBBBBB......",
"......BBBBBBB......B",
".......BBBBB.......B",
"........BBB.......BB",
".........B........BB",
"...................."
};
static const char *IDLE_1[SPR_H] = {
"....................",
"....................",
".......BBBBBB.......",
".....BBBBBBBBBB.....",
"....BBBBBBBBBBBB....",
"...BBBBBBBBBBBBBB...",
"...BBBBE BBB EBBB...",
"..BBBBBBBB BBBBBB...",
"..BBBBBBBBMBBBBB....",
"..BBBBBBBBBBBBBB....",
"..BBBBBBBBBBBBBB....",
"...BBBBBBBBBBBBB....",
"...BBBBBBBBBBBBB....",
"....BBBBBBBBBBB.....",
".....BBBBBBBBB......",
"......BBBBBBB......B",
".......BBBBB.......B",
"........BBB.......BB",
".........B........BB",
"...................."
};
static const char *EAT_0[SPR_H] = {
"....................",
"....................",
".......BBBBBB.......",
".....BBBBBBBBBB.....",
"....BBBBBBBBBBBB....",
"...BBBBBBBBBBBBBB...",
"...BBBBE BBB EBBB...",
"..BBBBBBBBMBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"...BBBBBBBBBBBBB....",
"...BBBBBBBBBBBBB....",
"....BBBBBBBBBBB.....",
".....BBBBBBBBB...FFB",
"......BBBBBBB.FFFFFB",
".......BBBBB.....FFB",
"........BBB.......BB",
".........B........BB",
"...................."
};
static const char *EAT_1[SPR_H] = {
"....................",
"....................",
".......BBBBBB.......",
".....BBBBBBBBBB.....",
"....BBBBBBBBBBBB....",
"...BBBBBBBBBBBBBB...",
"...BBBBE BBB EBBB...",
"..BBBBBBBBMBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"...BBBBBBBBBBBBB....",
"...BBBBBBBBBBBBB....",
"....BBBBBBBBBBB.....",
".....BBBBBBBBB...FFB",
"......BBBBBBB.FFFFFB",
".......BBBBB.....FFB",
"........BBB.......BB",
".........B........BB",
"...................."
};
static const char *PLAY_0[SPR_H] = {
"....................",
"....................",
".......BBBBBB.......",
".....BBBBBBBBBB.....",
"....BBBBBBBBBBBB....",
"...BBBBBBBBBBBBBB...",
"...BBBBE BBB EBBB...",
"..BBBBBBBB BBBBBB...",
"..BBBBBBBBMBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"...BBBBBBBBBBBBB....",
"...BBBBBBBBBBBBB....",
"....BBBBBBBBBBB..P.B",
".....BBBBBBBBB..PPP.",
"......BBBBBBB..PPPPB",
".......BBBBB....PPP.",
"........BBB......P.B",
".........B........B.",
"...................."
};
static const char *PLAY_1[SPR_H] = {
"....................",
"....................",
".......BBBBBB.......",
".....BBBBBBBBBB.....",
"....BBBBBBBBBBBB....",
"...BBBBBBBBBBBBBB...",
"...BBBBE BBB EBBB...",
"..BBBBBBBB BBBBBB...",
"..BBBBBBBBMBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"...BBBBBBBBBBBBB....",
"...BBBBBBBBBBBBB....",
"....BBBBBBBBBBB.....",
".....BBBBBBBBB......",
"......BBBBBBB....P.B",
".......BBBBB....PPP.",
"........BBB....PPPPB",
".........B......PPP.",
".................PB."
};

// WAVE ANIMATION SPRITES

static const char *WAVE_0[SPR_H] = {
"....................",
"....................",
".......BBBBBB.......",
".....BBBBBBBBBB.....",
"....BBBBBBBBBBBB....",
"...BBBBBBBBBBBBBB...",
"...BBBBE BBB EBBB...",
".BBBBBBBBMBBBBBBB...",
".BBBBBBBBBBBBBBBB...",
".BBBBBBBBBBBBBBBB...",
".BBBBBBBBBBBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"..BBBBBBBBBBBBBBB...",
"...BBBBBBBBBBBBB....",
"....BBBBBBBBBBB.....",
".....BBBBBBBBB......",
"......BBBBBBB.......",
".......BBBBB........",
"........BBB.........",
".........B.........."
};

static const char *WAVE_1[SPR_H] = {
"....................",
"....................",
".......BBBBBB.......",
".....BBBBBBBBBB.....",
"....BBBBBBBBBBBB....",
"...BBBBBBBBBBBBBB...",
"...BBBBE BBB EBBB...",
".BBB.BBBBMB BBBBB...",
".BBB.BBBBBBBBBBBBB..",
".BBB.BBBBBBBBBBBBB..",
".BBB.BBBBBBBBBBBBB..",
"..BBB.BBBBBBBBBBB...",
"..BBB.BBBBBBBBBBB...",
"...BBBBBBBBBBBBB....",
"....BBBBBBBBBBB.....",
".....BBBBBBBBB......",
"......BBBBBBB.......",
".......BBBBB........",
"........BBB.........",
".........B.........."
};

// Color mapping to RGB565
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b){
return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
static uint16_t SpriteColor(char c){
switch(c){
case 'B': return rgb565(0x80,0x00,0x00); // body changed from green to dark red(Problem 1)
case 'E': return rgb565(0x00,0x00,0x00); // eye black
case 'M': return rgb565(0x00,0x00,0x00); // mouth black
case 'F': return rgb565(0xC0,0x80,0x20); // food (brownish)
case 'P': return rgb565(0xC0,0x20,0xC0); // play ball (magenta)
case '.': default: return rgb565(0x00,0x00,0x00); // background black
}
}
#define SPR_SCALE 8 // 2× size (20×20 → 40×40)
// Convert ASCII sprite -> RGB565 buffer then blit
static void DrawSpriteXPM(const char **sprite, uint16_t x, uint16_t y)
{
for (int r = 0; r < SPR_H; r++) {
const char *row = sprite[r];
for (int c = 0; c < SPR_W; c++) {
uint16_t color = SpriteColor(row[c]);
// top-left pixel of the enlarged block
uint16_t px = x + c * SPR_SCALE;
uint16_t py = y + r * SPR_SCALE;
// draw a SPR_SCALE × SPR_SCALE block
for (int dy = 0; dy < SPR_SCALE; dy++) {
for (int dx = 0; dx < SPR_SCALE; dx++) {
LCD_DrawImage(&color, px + dx, py + dy, 1, 1);
}
}
}
}
}

// ===================== UI Helpers =====================
static inline int clamp(int v, int lo, int hi){ return (v<lo)?lo : (v>hi)?hi : v; }
static void DrawMenu(void) {
osMutexAcquire(lcdMutex, osWaitForever);
//LCD_Clear(0x0000);
LCD_DrawString("== MENU ==", 24, 52, 0xFFFF, 0x0000);
const char* items[4] = {"Feed", "Play", "Wave", "Exit"};
for (int i=0;i<3;i++){
uint16_t fg = (i==pet.menuIndex) ? 0x0000 : 0xFFFF;
uint16_t bg = (i==pet.menuIndex) ? 0xFFFF : 0x0000;
LCD_DrawString(items[i], 24, 72 + 14*i, fg, bg);
}
osMutexRelease(lcdMutex);
}
static void DrawIdle(int frame) {
osMutexAcquire(lcdMutex, osWaitForever);
DrawSpriteXPM(frame ? IDLE_1 : IDLE_0, 54, 84); // roughly centered
osMutexRelease(lcdMutex);
}
static void DrawAnimFeed(int frame){
osMutexAcquire(lcdMutex, osWaitForever);
DrawSpriteXPM(frame ? EAT_1 : EAT_0, 54, 84);
osMutexRelease(lcdMutex);
}
static void DrawAnimPlay(int frame){
osMutexAcquire(lcdMutex, osWaitForever);
DrawSpriteXPM(frame ? PLAY_1 : PLAY_0, 54, 84);
osMutexRelease(lcdMutex);
}
//Drawing function for wave
static void DrawAnimWave(int frame){
    osMutexAcquire(lcdMutex, osWaitForever);
    DrawSpriteXPM(frame ? WAVE_1 : WAVE_0, 54, 84);
    osMutexRelease(lcdMutex);
}
static void DrawEndScreen(const char* title, uint16_t color) {
osMutexAcquire(lcdMutex, osWaitForever);
LCD_DrawString(title, 10, 60, color, 0x0000);
LCD_DrawString("Press CENTER to reset", 10, 80, 0xFFFF, 0x0000);
osMutexRelease(lcdMutex);
}
static void ThreadSafeClearScreen(){
osMutexAcquire(lcdMutex, osWaitForever);
LCD_Clear(0x0000);
osMutexRelease(lcdMutex);
}

// ===================== Input Task =====================
static uint8_t readDebounced(void){
uint8_t s1 = Joystick_Read();
osDelay(10);//changed from 20 to 10
uint8_t s2 = Joystick_Read();
return (s1==s2) ? s1 : 0;
}
static BtnEvent mapButtons(uint8_t state){
if (state & JOY_UP) return BTN_UP;
if (state & JOY_DOWN) return BTN_DOWN;
if (state & JOY_LEFT) return BTN_LEFT;
if (state & JOY_RIGHT) return BTN_RIGHT;
if (state & JOY_CENTER) return BTN_CENTER;
return BTN_NONE;
}
static void InputTask(void *arg){
while(1){
uint8_t s = readDebounced();
BtnEvent e = mapButtons(s);
if (e != BTN_NONE) {
osMessageQueuePut(inputQueue, &e, 0, 0);
while (Joystick_Read()) { osDelay(15); } // wait release
}
osDelay(15);
}
}

// ===================== Logic Task =====================
#define MAX_HAPPINESS 100000
#define MAX_HUNGER 100000
static void StartAction(AppState action){
// Apply stat increase once on start, then render task plays the animation.
if (action == APP_FEEDING) pet.hunger = clamp(pet.hunger + 20, 0,
MAX_HUNGER);
if (action == APP_PLAYING) pet.happiness = clamp(pet.happiness + 20, 0,
MAX_HAPPINESS);
if (action == APP_WAVING) {
    pet.happiness = clamp(pet.happiness + 10, 0, MAX_HAPPINESS);
}
pet.state = action;
}
static void LogicTask(void *arg){
uint32_t nextTick = osKernelGetTickCount() + 2000U; // first wake
const uint32_t decayPeriodMs = 100U;
while(1){
osDelayUntil(nextTick); // CMSIS expects absolute time
nextTick += decayPeriodMs; // schedule next wake
if (pet.state == APP_IDLE || pet.state == APP_MENU) {
pet.happiness = clamp(pet.happiness - 1, 0, MAX_HAPPINESS);
pet.hunger = clamp(pet.hunger - 1, 0, MAX_HUNGER);
}
if (pet.state != APP_DEAD && pet.state != APP_RAN_AWAY) {
if (pet.hunger == 0){
clear_screen_flag = 1;
pet.state = APP_DEAD;
}
if (pet.happiness == 0) {
clear_screen_flag = 1;
pet.state = APP_RAN_AWAY;
}
}

// Drain queued input (non-blocking)
BtnEvent e;
while (osMessageQueueGet(inputQueue, &e, NULL, 0) == osOK) {
if (pet.state == APP_DEAD || pet.state == APP_RAN_AWAY) {
if (e == BTN_CENTER) {
clear_screen_flag = 1;
pet.happiness = MAX_HAPPINESS;
pet.hunger = MAX_HUNGER;
pet.menuIndex = 0;
pet.state = APP_IDLE;
}
continue;
}
switch (pet.state) {
case APP_IDLE:
if (e == BTN_CENTER){
clear_screen_flag = 1;
pet.state = APP_MENU;
}
break;
case APP_MENU:
if (e == BTN_UP) {
pet.menuIndex = (pet.menuIndex + 4 - 1) % 4; // up
}else if (e == BTN_DOWN) {
pet.menuIndex = (pet.menuIndex + 1) % 4; // Down
}else if (e == BTN_CENTER) {
clear_screen_flag = 1;
if (pet.menuIndex == 0)
StartAction(APP_FEEDING);
else if (pet.menuIndex == 1)
StartAction(APP_PLAYING);
else if(pet.menuIndex == 2)
StartAction(APP_WAVING);
else
pet.state = APP_IDLE;
}
break;
default:
break;
}
}
}
}

// ===================== Render Task =====================
#define ANIMATION_TICKS 4000
static void RenderTask(void *arg){
int frame = 0;
while(1){
if(clear_screen_flag){
ThreadSafeClearScreen();
clear_screen_flag = 0;
}
switch (pet.state) {
case APP_IDLE:
DrawIdle(frame ^= 1);
osDelay(320);
break;
case APP_MENU:
DrawMenu();
osDelay(60);
break;
case APP_FEEDING: {

	// Show 2-frame “eating” animation for ~4s
uint32_t start = osKernelGetTickCount();
while ((osKernelGetTickCount() - start) < ANIMATION_TICKS) {
DrawAnimFeed(frame ^= 1);
osDelay(180);
}
pet.state = APP_IDLE;
break;
}
case APP_PLAYING: {

	// Show 2-frame “playing” animation for ~4s
uint32_t start = osKernelGetTickCount();
while ((osKernelGetTickCount() - start) < ANIMATION_TICKS) {
DrawAnimPlay(frame ^= 1);
osDelay(180);
}
pet.state = APP_IDLE;
break;
}
case APP_WAVING: {
    uint32_t start = osKernelGetTickCount();
    while ((osKernelGetTickCount() - start) < ANIMATION_TICKS) {
        DrawAnimWave(frame ^= 1);
        osDelay(180);
    }
    pet.state = APP_IDLE;
    break;
}
case APP_RAN_AWAY:
DrawEndScreen("Your pet ran away :(", 0xF800);
osDelay(140);
break;
case APP_DEAD:
DrawEndScreen("Your pet died :(", 0xF800);
osDelay(140);
break;
}
}
}

// ===================== HAL + System =====================
void SystemClock_Config(void)
{
RCC_OscInitTypeDef RCC_OscInitStruct = {0};
RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

/** Initializes the RCC Oscillators according to the specified parameters
* in the RCC_OscInitTypeDef structure.
*/
RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
RCC_OscInitStruct.HSIState = RCC_HSI_ON;
RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
{
while(1);
}

/** Initializes the CPU, AHB and APB buses clocks
*/
RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK |
RCC_CLOCKTYPE_PCLK1;
RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
{
while(1);
}
PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
{
while(1);
}
}

// ===================== CMSIS-RTOS2 Bootstrap =====================
int main(void) {
HAL_Init();
SystemClock_Config();
LCD_Init();
LCD_Clear(0x0000);
osKernelInitialize();
lcdMutex = osMutexNew(NULL);
inputQueue = osMessageQueueNew(8, sizeof(BtnEvent), NULL);
const osThreadAttr_t render_attr =
{ .name="Render", .stack_size=1024, .priority=osPriorityNormal };
const osThreadAttr_t input_attr =
{ .name="Input", .stack_size=512, .priority=osPriorityAboveNormal };
const osThreadAttr_t logic_attr =
{ .name="Logic", .stack_size=768, .priority=osPriorityNormal };
renderTaskHandle = osThreadNew(RenderTask, NULL, &render_attr);
inputTaskHandle = osThreadNew(InputTask, NULL, &input_attr);
logicTaskHandle = osThreadNew(LogicTask, NULL, &logic_attr);
osKernelStart();
while(1);
}
