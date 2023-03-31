# ball_in_maze_game
This program implements a 3D game similar to the maze made of segments from the lines_game repository, according to the following implementation requirements.

The goal of the game is for the player, represented by a sphere/ball, to move from the bottom left corner of the cube (0,0,0) to the top right corner (1,1,1), avoiding obstacles, which are randomly rotated pyramids located at the centers of the NxNxN grid in the cube (N should be between 2 and 15, inclusive). The player's movement will be limited by detected collisions with obstacles. The game is launched from the command line with the first optional parameter as a number that is the seed for the pseudorandom number generator, which ensures that the same obstacles are generated with the same seed. The second optional argument can be a number N, which is the size of the grid (a default equal to 10). The main view is from the observer's position, facing forward in the direction of movement. An additional external view is a parallel projections on the right side of a window.

The key controls are as follows:
- use the four arrow keys to change the view direction,
- press A to move forward and Z to move backward,
- right-click the mouse to adjust the horizontal view direction,
- left-click the mouse to adjust the vertical view direction.

Functionalities:
1. In the event of a collision with a cube wall, objects change their color and receive more pink pigment to indicate the collision.
2. In the event of a collision with stationary pyramids, objects change their color and receive more red pigment to indicate the collision.
3. A white sphere periodically moves from the upper left corner to the lower right corner, and if the player collides with it, the game will be lost.
4. The walls of the cube where the game takes place have a texture applied to facilitate orientation in 3D space and indicate the location of an orange pyramid (collision with which leads to a win). The smaller black squares in the background indicate the player's proximity to the finish line.
5. Along the way, there is a possibility to collect yellow spheres, collecting which deducts 5 seconds from the gameplay time.

To compile: make 
To run: ./main <seed> <N>

Where seed and N are optional.
