# Comets
Comets is an attempt at making a 3D game based on the 2D game 'Asteroids'.

## Screenshots
![screen1](readme_assets/screen1.png)
![screen2](readme_assets/screen2.png)

## Architecture
- Asteroids are simple 3D models, floating in space. They spawn at some distance from the player and loop around when far enough away from the player, to create a compact but "open" world. (This actually makes the world into a three-torus, which ironically is compact)

## Plan:
It's working decently well now, but these are some possible points of improvement:
- Add realistic lighting to dust particles
- More precise ship-asteroid collision check
- Implement a proper sun (visual only, should be drawn out of accessible map)
- Add skybox
- Add local score saving
- Add shadows, especially from asteroids to dust

## Libraries

Comets makes use of GLFW, OpenGL, [cglm](https://github.com/recp/cglm) and [glText](https://github.com/vallentin/glText).
