#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include "font.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/fb.h>

#include <stdio.h>
#include <stdlib.h>

#include <linux/limits.h>

static const int SDL_WAKEUPEVENT = SDL_USEREVENT+1;

#ifdef DEBUG
	#define system(x) printf(x); printf("\n")
#endif

#ifdef DEBUG
	#define DBG(x) printf("%s:%d %s %s\n", __FILE__, __LINE__, __func__, x);
#else
	#define DBG(x)
#endif


#define WIDTH  320
#define HEIGHT 240

#define GPIO_BASE		0x10010000
#define PAPIN			((0x10010000 - GPIO_BASE) >> 2)
#define PBPIN			((0x10010100 - GPIO_BASE) >> 2)
#define PCPIN			((0x10010200 - GPIO_BASE) >> 2)
#define PDPIN			((0x10010300 - GPIO_BASE) >> 2)
#define PEPIN			((0x10010400 - GPIO_BASE) >> 2)
#define PFPIN			((0x10010500 - GPIO_BASE) >> 2)

#if defined(TARGET_MIYOO)
#define BTN_X			SDLK_LSHIFT
#define BTN_A			SDLK_LALT
#define BTN_B			SDLK_LCTRL
#define BTN_Y			SDLK_SPACE
#define BTN_L1			SDLK_TAB
#define BTN_R1			SDLK_BACKSPACE
#define BTN_L2			SDLK_PAGEUP
#define BTN_R2			SDLK_PAGEDOWN
#define BTN_L3			SDLK_RALT
#define BTN_R3			SDLK_RSHIFT
#define BTN_START		SDLK_RETURN
#define BTN_SELECT		SDLK_ESCAPE
#define BTN_BACKLIGHT	SDLK_3
#define BTN_POWER		SDLK_RCTRL
#define BTN_UP			SDLK_UP
#define BTN_DOWN		SDLK_DOWN
#define BTN_LEFT		SDLK_LEFT
#define BTN_RIGHT		SDLK_RIGHT
#define GPIO_TV			SDLK_WORLD_0
#define GPIO_MMC		SDLK_WORLD_1
#define GPIO_USB		SDLK_WORLD_2
#define GPIO_PHONES		SDLK_WORLD_3
#elif defined(TARGET_RETROFW)
#define BTN_X			SDLK_SPACE
#define BTN_A			SDLK_LCTRL
#define BTN_B			SDLK_LALT
#define BTN_Y			SDLK_LSHIFT
#define BTN_L1			SDLK_TAB
#define BTN_R1			SDLK_BACKSPACE
#define BTN_START		SDLK_RETURN
#define BTN_SELECT		SDLK_ESCAPE
#define BTN_BACKLIGHT	SDLK_3
#define BTN_POWER		SDLK_END
#define BTN_UP			SDLK_UP
#define BTN_DOWN		SDLK_DOWN
#define BTN_LEFT		SDLK_LEFT
#define BTN_RIGHT		SDLK_RIGHT
#define GPIO_TV			SDLK_WORLD_0
#define GPIO_MMC		SDLK_WORLD_1
#define GPIO_USB		SDLK_WORLD_2
#define GPIO_PHONES		SDLK_WORLD_3
#else
#define BTN_X			SDLK_LSHIFT
#define BTN_A			SDLK_LALT
#define BTN_B			SDLK_LCTRL
#define BTN_Y			SDLK_SPACE
#define BTN_L1			SDLK_TAB
#define BTN_R1			SDLK_BACKSPACE
#define BTN_L2			SDLK_PAGEUP
#define BTN_R2			SDLK_PAGEDOWN
#define BTN_L3			SDLK_RALT
#define BTN_R3			SDLK_RSHIFT
#define BTN_START		SDLK_RETURN
#define BTN_SELECT		SDLK_ESCAPE
#define BTN_BACKLIGHT	SDLK_3
#define BTN_POWER		SDLK_RCTRL
#define BTN_UP			SDLK_UP
#define BTN_DOWN		SDLK_DOWN
#define BTN_LEFT		SDLK_LEFT
#define BTN_RIGHT		SDLK_RIGHT
#define GPIO_TV			SDLK_WORLD_0
#define GPIO_MMC		SDLK_WORLD_1
#define GPIO_USB		SDLK_WORLD_2
#define GPIO_PHONES		SDLK_WORLD_3
#endif

const int	HAlignLeft		= 1,
			HAlignRight		= 2,
			HAlignCenter	= 4,
			VAlignTop		= 8,
			VAlignBottom	= 16,
			VAlignMiddle	= 32;

SDL_RWops *rw;
TTF_Font *font = NULL;
SDL_Surface *screen = NULL;
SDL_Surface* img = NULL;
SDL_Rect bgrect;
SDL_Event event;

SDL_Color txtColor = {200, 200, 220};
SDL_Color titleColor = {200, 200, 0};
SDL_Color subTitleColor = {0, 200, 0};
SDL_Color powerColor = {200, 0, 0};

volatile uint32_t *memregs;
volatile uint8_t memdev = 0;
uint16_t mmcPrev, mmcStatus;
uint16_t udcPrev, udcStatus;
uint16_t tvOutPrev, tvOutStatus;
uint16_t phonesPrev, phonesStatus;

static char buf[1024];
uint32_t *mem;

uint8_t *keys;

extern uint8_t rwfont[];

int draw_text(int x, int y, const char buf[64], SDL_Color txtColor, int align) {
	DBG("");

	SDL_Surface *msg = TTF_RenderText_Blended(font, buf, txtColor);

	if (align & HAlignCenter) {
		x -= msg->w / 2;
	} else if (align & HAlignRight) {
		x -= msg->w;
	}

	if (align & VAlignMiddle) {
		y -= msg->h / 2;
	} else if (align & VAlignTop) {
		y -= msg->h;
	}

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = msg->w;
	rect.h = msg->h;
	SDL_BlitSurface(msg, NULL, screen, &rect);
	SDL_FreeSurface(msg);
	return msg->w;
}

void draw_background(const char buf[64]) {
	DBG("");
	bgrect.w = img->w;
	bgrect.h = img->h;
	bgrect.x = (WIDTH - bgrect.w) / 2;
	bgrect.y = (HEIGHT - bgrect.h) / 2;
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
	SDL_BlitSurface(img, NULL, screen, &bgrect);

	// title
#if defined(TARGET_MIYOO)
	draw_text(310, 4, "MiyooCFW", titleColor, VAlignBottom | HAlignRight);
#elif defined(TARGET_RETROFW)
	draw_text(310, 4, "RetroFW", titleColor, VAlignBottom | HAlignRight);
#else
	draw_text(310, 4, "LinuxOS", titleColor, VAlignBottom | HAlignRight);
#endif
	draw_text(10, 4, buf, titleColor, VAlignBottom);
#if defined(TARGET_XYC)
	draw_text(10, 230, "L+R: Exit", txtColor, VAlignMiddle | HAlignLeft);
#else
	draw_text(10, 230, "SELECT+START: Exit", txtColor, VAlignMiddle | HAlignLeft);
#endif
}

void draw_point(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
	// DBG("");
	SDL_Rect rect;
	rect.w = w;
	rect.h = h;
	rect.x = x + bgrect.x;
	rect.y = y + bgrect.y;
	SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 0, 150, 0));
}

void quit(int err) {
	DBG("");
	system("sync");
	if (font) TTF_CloseFont(font);
	font = NULL;
	SDL_Quit();
	TTF_Quit();
	exit(err);
}

uint16_t getMMCStatus() {
	return (memdev > 0 && !(memregs[0x10500 >> 2] >> 0 & 0b1));
}

uint16_t getUDCStatus() {
	return (memdev > 0 && (memregs[0x10300 >> 2] >> 7 & 0b1));
}

uint16_t getTVOutStatus() {
	return (memdev > 0 && !(memregs[0x10300 >> 2] >> 25 & 0b1));
}

uint16_t getPhonesStatus() {
	return (memdev > 0 && !(memregs[0x10300 >> 2] >> 6 & 0b1));
}

void pushEvent() {
	SDL_Event user_event;
	user_event.type = SDL_KEYUP;
	SDL_PushEvent(&user_event);
}


static int hw_input(void *ptr)
{
	while (1) {
		udcStatus = getUDCStatus();
		if (udcPrev != udcStatus) {
			keys[GPIO_USB] = udcPrev = udcStatus;
			pushEvent();
		}
		mmcStatus = getMMCStatus();
		if (mmcPrev != mmcStatus) {
			keys[GPIO_MMC] = mmcPrev = mmcStatus;
			pushEvent();
		}

		tvOutStatus = getTVOutStatus();
		if (tvOutPrev != tvOutStatus) {
			keys[GPIO_TV] = tvOutPrev = tvOutStatus;
			pushEvent();
		}

		phonesStatus = getPhonesStatus();
		if (phonesPrev != phonesStatus) {
			keys[GPIO_PHONES] = phonesPrev = phonesStatus;
			pushEvent();
		}

		SDL_Delay(100);
	}

	return 0;
}

int main(int argc, char* argv[]) {
	DBG("");
	signal(SIGINT, &quit);
	signal(SIGSEGV,&quit);
	signal(SIGTERM,&quit);

	char title[64] = "";
	keys = SDL_GetKeyState(NULL);

	sprintf(title, "IO TESTER");
	printf("%s\n", title);

	setenv("SDL_NOMOUSE", "1", 1);

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		printf("Could not initialize SDL: %s\n", SDL_GetError());
		return -1;
	}
	SDL_PumpEvents();
	SDL_ShowCursor(0);

	screen = SDL_SetVideoMode(WIDTH, HEIGHT, 16, SDL_SWSURFACE);

	SDL_EnableKeyRepeat(1, 1);

	if (TTF_Init() == -1) {
		printf("failed to TTF_Init\n");
		return -1;
	}
	rw = SDL_RWFromMem(rwfont, sizeof(rwfont));
	font = TTF_OpenFontRW(rw, 1, 8);
	TTF_SetFontHinting(font, TTF_HINTING_NORMAL);
	TTF_SetFontOutline(font, 0);

#if defined(TARGET_XYC)
	SDL_Surface* _img = IMG_Load("backdrop_xyc.png");
#elif defined(TARGET_BITTBOY)
	SDL_Surface* _img = IMG_Load("backdrop_bittboy.png");
#else
	SDL_Surface* _img = IMG_Load("backdrop_default.png");
#endif
	img = SDL_DisplayFormat(_img);
	SDL_FreeSurface(_img);

#if defined(TARGET_RETROFW)
	memdev = open("/dev/mem", O_RDWR);
	if (memdev > 0) {
		memregs = (uint32_t*)mmap(0, 0x20000, PROT_READ | PROT_WRITE, MAP_SHARED, memdev, 0x10000000);
	
		SDL_Thread *thread = SDL_CreateThread(hw_input, (void *)NULL);
	
		if (memregs == MAP_FAILED) {
			close(memdev);
		}
	}
#endif

	int loop = 1, running = 0;
#if defined(TARGET_XYC) || defined(TARGET_BITTBOY)
	do {
		draw_background(title);

		int nextline = 15;	

		if (event.key.keysym.sym) {
			sprintf(buf, "Last key: %s", SDL_GetKeyName(event.key.keysym.sym));
			draw_text(bgrect.x + 30, bgrect.y + nextline, buf, subTitleColor, VAlignBottom);
			nextline += 16;

			sprintf(buf, "Keysym.sym: %d", event.key.keysym.sym);
			draw_text(bgrect.x + 30, bgrect.y + nextline, buf, subTitleColor, VAlignBottom);
			nextline += 16;

			sprintf(buf, "Keysym.scancode: %d", event.key.keysym.scancode);
			draw_text(bgrect.x + 30, bgrect.y + nextline, buf, subTitleColor, VAlignBottom);
			nextline += 16;
		}
#else
	do {
		draw_background(title);

		int nextline = 20;	

		if (event.key.keysym.sym) {
			sprintf(buf, "Last key: %s", SDL_GetKeyName(event.key.keysym.sym));
			draw_text(bgrect.x + 104, bgrect.y + nextline, buf, subTitleColor, VAlignBottom);
			nextline += 16;

			sprintf(buf, "Keysym.sym: %d", event.key.keysym.sym);
			draw_text(bgrect.x + 104, bgrect.y + nextline, buf, subTitleColor, VAlignBottom);
			nextline += 16;

			sprintf(buf, "Keysym.scancode: %d", event.key.keysym.scancode);
			draw_text(bgrect.x + 104, bgrect.y + nextline, buf, subTitleColor, VAlignBottom);
			nextline += 16;
		}
#endif

		if (udcStatus) {
			draw_point(84, 0, 20, 10);
		
			SDL_Rect rect;
			rect.w = 10;
			rect.h = 10;
			rect.x = 310 + bgrect.x;
			rect.y = 40 + bgrect.y;
			SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 150, 0, 0));

			draw_text(bgrect.x + 104, bgrect.y + nextline, "USB Connected", subTitleColor, VAlignBottom);
			nextline += 16;
		}
		if (tvOutStatus) {
			draw_point(206, 0, 10, 10);
			draw_text(bgrect.x + 104, bgrect.y + nextline, "TV-Out Connected", subTitleColor, VAlignBottom);
			nextline += 16;
		}
		if (mmcStatus) {
			draw_point(125, 150, 30, 10);
			draw_text(bgrect.x + 104, bgrect.y + nextline, "SD Card Connected", subTitleColor, VAlignBottom);
			nextline += 16;
		}

		if (phonesStatus) {
			draw_point(260, 150, 10, 10);
			draw_text(bgrect.x + 104, bgrect.y + nextline, "Phones Connected", subTitleColor, VAlignBottom);
			nextline += 16;
		}

#if defined(TARGET_XYC)
		// if (keys[BTN_SELECT] && keys[BTN_START]) loop = 0;
		if (keys[BTN_START]) draw_point(90, 143, 24, 10);
		if (keys[BTN_SELECT]) draw_point(60, 143, 24, 10);
		if (keys[BTN_POWER]) draw_point(118, 143, 22, 10);
		if (keys[BTN_BACKLIGHT]) draw_point(150, 0, 20, 10);
		if (keys[BTN_L1]) draw_point(166, 169, 20, 20);
		if (keys[BTN_R1]) draw_point(166, 198, 20, 20);
		if (keys[BTN_L2]) draw_point(0, 0, 10, 10);
		if (keys[BTN_R2]) draw_point(190, 0, 10, 10);
		if (keys[BTN_L3]) draw_point(40, 190, 10, 10);
		if (keys[BTN_R3]) draw_point(145, 185, 10, 10);
		if (keys[BTN_LEFT]) draw_point(15, 185, 20, 20);
		if (keys[BTN_RIGHT]) draw_point(55, 185, 20, 20);
		if (keys[BTN_UP]) draw_point(35, 165, 20, 20);
		if (keys[BTN_DOWN]) draw_point(35, 205, 20, 20);
		if (keys[BTN_A]) draw_point(111, 201, 20, 20);
		if (keys[BTN_B]) draw_point(139, 193, 20, 20);
		if (keys[BTN_X]) draw_point(139, 164, 20, 20);
		if (keys[BTN_Y]) draw_point(111, 172, 20, 20);
#elif defined(TARGET_BITTBOY)
		// if (keys[BTN_SELECT] && keys[BTN_START]) loop = 0;
		if (keys[BTN_START]) draw_point(81, 206, 20, 10);
		if (keys[BTN_SELECT]) draw_point(58, 206, 20, 10);
		if (keys[BTN_POWER]) draw_point(81, 128, 12, 12);
		if (keys[BTN_BACKLIGHT]) draw_point(150, 0, 20, 10);
		if (keys[BTN_L1]) draw_point(0, 0, 17, 17);
		if (keys[BTN_R1]) draw_point(153, 0, 17, 17);
		if (keys[BTN_L2]) draw_point(17, 0, 17, 17);
		if (keys[BTN_R2]) draw_point(136, 0, 17, 17);
		if (keys[BTN_L3]) draw_point(50, 60, 10, 10);
		if (keys[BTN_R3]) draw_point(265, 65, 10, 10);
		if (keys[BTN_LEFT]) draw_point(14, 153, 20, 20);
		if (keys[BTN_RIGHT]) draw_point(54, 153, 20, 20);
		if (keys[BTN_UP]) draw_point(34, 133, 20, 20);
		if (keys[BTN_DOWN]) draw_point(34, 173, 20, 20);
		if (keys[BTN_A]) draw_point(115, 170, 20, 20);
		if (keys[BTN_B]) draw_point(95, 150, 20, 20);
		if (keys[BTN_X]) draw_point(135, 150, 20, 20);
		if (keys[BTN_Y]) draw_point(115, 130, 20, 20);
#else
		// if (keys[BTN_SELECT] && keys[BTN_START]) loop = 0;
		if (keys[BTN_START]) draw_point(70, 100, 10, 10);
		if (keys[BTN_SELECT]) draw_point(70, 120, 10, 10);
		if (keys[BTN_POWER]) draw_point(230, 0, 10, 10);
		if (keys[BTN_BACKLIGHT]) draw_point(150, 0, 20, 10);
		if (keys[BTN_L1]) draw_point(5, 5, 35, 15);
		if (keys[BTN_R1]) draw_point(280, 5, 35, 15);
		if (keys[BTN_L2]) draw_point(40, 5, 20, 15);
		if (keys[BTN_R2]) draw_point(260, 5, 20, 15);
		if (keys[BTN_L3]) draw_point(50, 60, 10, 10);
		if (keys[BTN_R3]) draw_point(265, 65, 10, 10);
		if (keys[BTN_LEFT]) draw_point(25, 55, 20, 20);
		if (keys[BTN_RIGHT]) draw_point(65, 55, 20, 20);
		if (keys[BTN_UP]) draw_point(45, 35, 20, 20);
		if (keys[BTN_DOWN]) draw_point(45, 75, 20, 20);
		if (keys[BTN_A]) draw_point(280, 60, 20, 20);
		if (keys[BTN_B]) draw_point(260, 80, 20, 20);
		if (keys[BTN_X]) draw_point(260, 40, 20, 20);
		if (keys[BTN_Y]) draw_point(240, 60, 20, 20);
#endif

		SDL_Flip(screen);

		while (SDL_WaitEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
#if defined(TARGET_XYC)
				if (keys[BTN_L1] && keys[BTN_R1]) loop = 0;
#else
				if (keys[BTN_SELECT] && keys[BTN_START]) loop = 0;
#endif
				break;
			}

			if (event.type == SDL_KEYUP) {
				break;
			}
		}
	} while (loop);

	if (memdev > 0) close(memdev);

	quit(0);
	return 0;
}
