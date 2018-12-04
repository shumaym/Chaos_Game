#include <SDL2/SDL.h>

#include <iostream>
#include <string>
#include <vector>
#include <signal.h>
#include "math.h"
#include <unordered_map>
#include <unistd.h>

typedef struct { uint16_t x; uint16_t y; } Vertex;

// Screen properties
const uint16_t SCREEN_WIDTH = 1000;
const uint16_t SCREEN_HEIGHT = 1000;
const float SCREEN_MARGINS = 0.05;

// Parameters of the Chaos Game
uint16_t num_vertices = 3;
float factor = 0.5;

// Drawn colours
uint8_t colour_background[3] = {0x00, 0x00, 0x00};
uint8_t colour_vertices[3] = {0xFF, 0x10, 0x10};
uint8_t colour_points[3] = {0x30, 0x90, 0x80};

// Drawn rectangle properties
const uint16_t RECTS_WIDTH = 1;
const uint16_t RECTS_HEIGHT = 1;

// Primes used in hashing function
const double PRIME_1 = 20011;
const double PRIME_2 = 24481;

// Rate of progress
uint64_t stepping = 2500;
uint16_t frame_delay_ms = 50;

// flag_continue dictates the continuation of computation
bool flag_continue = true;

/**
* Log an SDL error with some error message
* @param msg The error message to write, format will be msg error: SDL_GetError()
*/
void log_SDL_error(const std::string &msg){
	std::cerr << msg << " error: " << SDL_GetError() << std::endl;
}

/**
* Handles interrupts, sets a flag to discontinue computation.
*/
void signal_interrupt(int _)
{
	std::cout << std::endl << "Exiting." << std::endl;
	flag_continue = false;
}

int main(int argc, char *argv[]) {
	signal(SIGINT, signal_interrupt);
	srand(time(0));

	// Process arguments
	int opt, prev_ind;
	while(prev_ind = optind, (opt = getopt(argc, argv, ":hs:d:v:f:")) != EOF)
	{
		if (optind == prev_ind + 2 && *optarg == '-') {
			opt = ':';
			--optind;
		}
		switch (opt)
		{
			case 'h':
				std::cout << "Options:" << std::endl;
				std::cout << " -v N    number of vertices in the polygon (default: " << num_vertices << ")" << std::endl;
				std::cout << " -f N    fraction of distance between the current point and chosen vertex to place a new point (default: " << factor << ")" << std::endl;
				std::cout << " -s N    number of points to generate before refreshing the window (default: " << stepping << ")" << std::endl;
				std::cout << " -d N    delay in ms after the window is refreshed (default: " << frame_delay_ms << ")" << std::endl;
				std::cout << " -h      display this help page and exit" << std::endl;
				std::cout << std::endl << std::endl;
				return 0;

			case 's':
				if (std::atoi(optarg) > 0){
					stepping = std::atoi(optarg);
					std::cout << "Stepping set to " << stepping << "." << std::endl;
				}
				else{
					std::cout << "Invalid stepping value. Defaulting to " << stepping << "." << std::endl;
				}
				break;

			case 'd':
				if (std::atoi(optarg) >= 0){
					frame_delay_ms = std::atoi(optarg);
					std::cout << "Frame delay set to " << frame_delay_ms << " ms." << std::endl;
				}
				else{
					std::cout << "Invalid frame delay. Defaulting to " << frame_delay_ms << " ms." << std::endl;
				}
				break;

			case 'v':
				if (std::atoi(optarg) >= 3 && std::atoi(optarg) <= 255){
					num_vertices = std::atoi(optarg);
					std::cout << "Number of vertices set to " << num_vertices << "." << std::endl;
				}
				else{
					std::cout << "Invalid number of vertices. Defaulting to " << num_vertices << "." << std::endl;
				}
				break;

			case 'f':
				if (std::atof(optarg) > 0.0 && std::atof(optarg) < 1.0){
					factor = std::atof(optarg);
					std::cout << "Factor set to " << factor << std::endl;
				}
				else{
					std::cout << "Invalid factor value. Defaulting to " << factor << std::endl;
				}
				break;

			case ':':
				std::cerr << "Missing option. Exiting." << std::endl;
				return 1;
		}
	}

	// Storage for vertices and points
	std::vector<SDL_Rect> rects;
	std::unordered_map<double, bool> rects_hashmap;
	Vertex vertices[num_vertices];
	SDL_Rect vertice_rects[num_vertices];

	// Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		log_SDL_error("SDL_Init");
		return 1;
	}

	// Create window
	SDL_Window *window = SDL_CreateWindow("Chaos Game",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		0);
	if (window == nullptr){
		log_SDL_error("CreateWindow");
		SDL_Quit();
		return 1;
	}

	// Create renderer
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);	
	if (renderer == nullptr){
		log_SDL_error("CreateRenderer");
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	// Create the vertices of the polygon
	uint16_t x_point, y_point;
	float theta;
	for (int i = 0; i < num_vertices; i++){
		theta = 270 + float (i * 360) / float (num_vertices);
		x_point = SCREEN_WIDTH/2 + cos(theta*M_PI/180) * (SCREEN_WIDTH * (1.0 - SCREEN_MARGINS) / 2);
		y_point = SCREEN_HEIGHT/2 + sin(theta*M_PI/180) * (SCREEN_HEIGHT * (1.0 - SCREEN_MARGINS) / 2);
		vertices[i] = {uint16_t (x_point), uint16_t (y_point)};
		vertice_rects[i] = SDL_Rect {x_point, y_point, RECTS_WIDTH, RECTS_HEIGHT};
	}

	// Reserve some space in the hashmap, which will likely be expanded
	rects_hashmap.reserve(0.05 * SCREEN_WIDTH * SCREEN_HEIGHT);

	// Create the first point
	uint16_t last_x_point, last_y_point;
	last_x_point = rand() % SCREEN_WIDTH;
	last_y_point = rand() % SCREEN_HEIGHT;
	rects.push_back(SDL_Rect {last_x_point, last_y_point, RECTS_WIDTH, RECTS_HEIGHT});
	
	// Keep generating points until flag_continue is set to false
	uint8_t die_roll;
	double hash;
	uint32_t i = 0;
	uint32_t num_rects = 0;
	while (flag_continue){
		// Roll the die and determine the next point's position
		die_roll = rand() % num_vertices;
		x_point = round(last_x_point * (1.0 - factor) + vertices[die_roll].x * factor);
		y_point = round(last_y_point * (1.0 - factor) + vertices[die_roll].y * factor);
		
		// Hash the new point's position and check if it has already been created.
		// If the point does not exist, create it.
		hash = x_point/PRIME_1 + y_point/PRIME_2;
		if (rects_hashmap.find(hash) == rects_hashmap.end()){
			rects_hashmap[hash] = true;
			num_rects++;
			rects.push_back(SDL_Rect {x_point, y_point, RECTS_WIDTH, RECTS_HEIGHT});
		}

		last_x_point = x_point;
		last_y_point = y_point;

		if (i % stepping == 0){
			// Clear renderer
			SDL_SetRenderDrawColor(renderer, colour_background[0], colour_background[1] ,colour_background[2], 0xFF);
			SDL_RenderClear(renderer);

			// Draw generated points
			SDL_SetRenderDrawColor(renderer, colour_points[0], colour_points[1], colour_points[2], 0xFF);
			SDL_RenderFillRects(renderer, &rects[0], num_rects);

			// Draw vertices
			SDL_SetRenderDrawColor(renderer, colour_vertices[0], colour_vertices[1], colour_vertices[2], 0xFF);
			SDL_RenderFillRects(renderer, vertice_rects, num_vertices);

			// Update screen
			SDL_RenderPresent(renderer);
			SDL_Delay(frame_delay_ms);
		}

		i++;
	}

	// Free and destroy
	std::vector<SDL_Rect>().swap(rects);
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
	return 0;
}