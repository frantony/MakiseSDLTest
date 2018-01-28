#include <stdlib.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include "makise.h"
#include "makise_gui.h"
#include "makise_sdl2.h"
#include "tests.h"

SDL_Renderer *renderer;
SDL_Window *window;
SDL_Surface *screen;

MakiseGUI* mGui;
MHost *host;
MakiseGUI    Gu;
MakiseBuffer Bu;
MakiseDriver Dr;
MHost hs;
MContainer co;

uint32_t bufff[960000] = {0};

void start_m();

int x = 0;
int y = 0;
char bu[100] = {0};

//Define user keys
#define M_KEY_USER_ESC        M_KEY_USER_0
#define M_KEY_USER_FOCUS_NEXT M_KEY_USER_0+1
#define M_KEY_USER_FOCUS_PREV M_KEY_USER_0+2
#define M_KEY_USER_TREE       M_KEY_USER_0+3

//input result handler. That will be called by gui thread after recieving result from input
MInputData inp_handler(MInputData d, MInputResultEnum res)
{
    if(d.event == M_INPUT_CLICK && res == M_INPUT_NOT_HANDLED)
    {
	//when click wasn't handled
	
	//printf("not h %d\n", d.key);
	if(d.key == M_KEY_LEFT) //if left wasn't handled - we'll switch focus
	    makise_g_host_focus_prev(host);
	if(d.key == M_KEY_RIGHT) //also
	    makise_g_host_focus_next(host);
	if(d.key == M_KEY_UP) //the same
	    makise_g_host_focus_prev(host);
	if(d.key == M_KEY_DOWN)
	    makise_g_host_focus_next(host);
	if(d.key == M_KEY_TAB_NEXT)
	    makise_g_host_focus_next(host);
	if(d.key == M_KEY_TAB_BACK)
	    makise_g_host_focus_prev(host);		    
	if(d.key == M_KEY_USER_ESC)
	    tests_exit();
	if(d.key == M_KEY_USER_FOCUS_NEXT)
	    makise_g_host_focus_next(host);
	if(d.key == M_KEY_USER_FOCUS_PREV)
	    makise_g_host_focus_prev(host);
	if(d.key == M_KEY_USER_TREE)
	    makise_g_print_tree(host);
    }
    return (MInputData){0};
}

static void handle_keydown(MHost *host, SDL_Event *event)
{
	MInputKeyEnum inp_key = M_KEY_NONE;

	/*
	 * Arrows - arrows
	 * Tab - switch tabs or focus
	 * Return - OK
	 * Minus/Equals - switch focus
	 * Esc - exit from test
	 * Delete - print tree of elements
	 */

	switch (event->key.keysym.sym) {

	case SDLK_LEFT:
		inp_key = M_KEY_LEFT;
		break;

	case SDLK_RIGHT:
		inp_key = M_KEY_RIGHT;
		break;

	case SDLK_UP:
		inp_key = M_KEY_UP;
		break;

	case SDLK_DOWN:
		inp_key = M_KEY_DOWN;
		break;

	case SDLK_TAB:
		inp_key = M_KEY_TAB_NEXT;
		break;

	case SDLK_RETURN:
		inp_key = M_KEY_OK;
		break;

	//DO NOT Interact with gui directly. It may cause errors in multithread applications. Use custom events
	case SDLK_DELETE:
		inp_key = M_KEY_USER_TREE;
		break;

	case SDLK_ESCAPE:
		inp_key = M_KEY_USER_ESC;
		break;

	case SDLK_EQUALS:
		inp_key = M_KEY_USER_FOCUS_NEXT;
		break;

	case SDLK_MINUS:
		inp_key = M_KEY_USER_FOCUS_PREV;
		break;
	}

	if (inp_key != M_KEY_NONE)
		makise_gui_input_send_button(host, inp_key, M_INPUT_CLICK, 100);
}

uint8_t prsed = 0;
int main(void) {
    SDL_Event event;

    //init SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(320, 240, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    screen = SDL_GetWindowSurface(window);

    
    //init makise GUI & start
    start_m();

    
    while (1) {
	//draw & predraw gui
	makise_g_host_call(host, M_G_CALL_PREDRAW);
	makise_g_host_call(host, M_G_CALL_DRAW);

	//call driver
	makise_sdl2_draw(mGui);

	//update sdl
	SDL_UpdateWindowSurface(window);

	
	//input system
	if(SDL_WaitEventTimeout(&event, 50))
	{
	    do
	    {
		x = event.motion.x;
		y = event.motion.y;

		if(prsed)
		{
		    MInputData d;
		    d.event = M_INPUT_PRESSING;
		    d.key = M_KEY_CURSOR;
		    d.time = 100;
		    d.cursor.x = x;
		    d.cursor.y = y;
		    makise_gui_input_send(host, d);
		}
		switch(event.type)
		{
		case SDL_QUIT:
		    SDL_DestroyRenderer(renderer);
		    SDL_DestroyWindow(window);
		    SDL_Quit();
		    return EXIT_SUCCESS;
		    break;
		case SDL_MOUSEBUTTONDOWN:
		    if(event.button.button == SDL_BUTTON_LEFT)
		    {
			//printf("down\n");
			prsed = 1;
			MInputData d;
			d.event = M_INPUT_PRESSING;
			d.key = M_KEY_CURSOR;
			d.time = 100;
			d.cursor.x = x;
			d.cursor.y = y;
			makise_gui_input_send(host, d);
		    }
		    break;
		case SDL_MOUSEBUTTONUP:
		    if(event.button.button == SDL_BUTTON_LEFT &&
			prsed)
		    {
			//printf("up\n");
			prsed = 0;
			MInputData d;
			d.event = M_INPUT_CLICK;
			d.key = M_KEY_CURSOR;
			d.time = 100;
			d.cursor.x = x;
			d.cursor.y = y;
			makise_gui_input_send(host, d);
		    }
		    break;
		case SDL_KEYDOWN:
			handle_keydown(host, &event);
			break;
		}
	    }
	    while (SDL_PollEvent(&event));
	    //perform input
	    makise_gui_input_perform(host);
	}
    }
}

MPosition ma_g_hpo;
void start_m()
{
    MakiseGUI    * gu = &Gu;
    MakiseBuffer * bu = &Bu;
    MakiseDriver * dr = &Dr;
    host = &hs;
    host->host = &co;
    host->host->gui = gu;
    makise_gui_init(host); //init gui host
    //if input event wasn't handled by gui. We need to handle it
    host->input.result_handler = &inp_handler;

    ma_g_hpo = mp_rel(0,0,320,240);
    ma_g_hpo.real_x = 0;
    ma_g_hpo.real_y = 0;
    host->host->position = &ma_g_hpo;
    
    
    //init driver structure
    makise_sdl2_driver(dr, 320, 240, screen);
    
    uint32_t sz = makise_init(gu, dr, bu);
    bu->buffer = bufff;
    memset(bu->buffer, 0, sz);
    //printf("%d\n", (uint32_t)(sz));

    
    mGui = gu;
    makise_start(gu);

    mGui->predraw = 0; //we don't need driver to execute those methods
    mGui->draw = 0; //we'll execute them by ourself in loop. It required when app is multitheading or uses interrupts

    
    //run tests
    tests_init(host);
}
