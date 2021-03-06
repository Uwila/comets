#include "graphics.h"
#include "world.h"
#include <stdlib.h>
#include <time.h>

#define ASTEROID_SIZE 24.0f
#define ASTEROID_VARIATION 12.0f
#define MINIMUM_COLLISION_DISTANCE 36.0f

GLFWwindow *window;
world_t *world;

void move_objects(float delta){
    // Handle rotations
    asteroid_list_t *asteroids_head = world->asteroids;
    while (asteroids_head->next != NULL){
        asteroids_head->this->angle += asteroids_head->this->rotation_speed * delta;
        asteroids_head = asteroids_head->next;
    }

    vec3 ship_diff;
    glm_vec3_scale(world->ship->movement_direction, -delta, ship_diff);

    // Move world->bullets
    bullet_list_t *bullets_head = world->bullets;
    bullet_list_t **link = &(world->bullets);
    while (bullets_head->next != NULL) {
        vec3 diff;
        glm_vec3_scale(bullets_head->this->direction, delta*bullets_head->this->speed, diff);
        glm_vec3_add(bullets_head->this->location, diff, bullets_head->this->location);
        glm_vec3_add(bullets_head->this->location, ship_diff, bullets_head->this->location);

        // Yes officer, this memory leak right here!
        if(glm_vec3_norm(bullets_head->this->location) > max_distance)
            *link = bullets_head->next;
        else
            link = &(bullets_head->next);
        bullets_head = bullets_head->next;
    }

    // Move world->asteroids
    asteroids_head = world->asteroids;
    while (asteroids_head->next != NULL) {
        vec3 diff;
        glm_vec3_scale(asteroids_head->this->direction, delta*asteroids_head->this->speed, diff);
        glm_vec3_add(asteroids_head->this->location, diff, asteroids_head->this->location);
        glm_vec3_add(asteroids_head->this->location, ship_diff, asteroids_head->this->location);
        if (glm_vec3_norm(asteroids_head->this->location) > max_distance) {
            glm_vec3_negate(asteroids_head->this->location);
        }

        asteroids_head = asteroids_head->next;
    }

    // Move dust
    for (int i = 0; i < world->dust_cloud->vertices_length; i++){
        glm_vec3_add(world->dust_cloud->vertices[i], ship_diff, world->dust_cloud->vertices[i]);
        if (glm_vec3_norm(world->dust_cloud->vertices[i]) > max_distance) {
            glm_vec3_negate(world->dust_cloud->vertices[i]);
        }
    }
}

void process_collisions(float delta) {
    // Asteroid-bullet intersections
    // Iterate over asteroids
    asteroid_list_t *asteroids_head = world->asteroids;
    asteroid_list_t **asteroids_link = &(world->asteroids);
    while(asteroids_head->next != NULL) {
        asteroid_t *asteroid = asteroids_head->this;
        int asteroid_destroyed = 0;

        // Iterate over the asteroid's triangles
        for (int i = 0; i < asteroid->vertices_length / 3; i++) {
            vec3 origin, direction, v0, v1, v2;

            // Set v0,v1,v2 to triangle vertices in world space
            glm_vec3_copy(asteroid->vertices[i*3], v0);
            glm_vec3_copy(asteroid->vertices[i*3+1], v1);
            glm_vec3_copy(asteroid->vertices[i*3+2], v2);

            glm_vec3_rotate(v0, asteroid->angle, asteroid->axis);
            glm_vec3_rotate(v1, asteroid->angle, asteroid->axis);
            glm_vec3_rotate(v2, asteroid->angle, asteroid->axis);

            glm_vec3_add(v0, asteroid->location, v0);
            glm_vec3_add(v1, asteroid->location, v1);
            glm_vec3_add(v2, asteroid->location, v2);

            // Iterate over bullets
            bullet_list_t *bullets_head = world->bullets;
            bullet_list_t **bullets_link = &(world->bullets);
            while(bullets_head->next != NULL) {
                bullet_t *bullet = bullets_head->this;

                glm_vec3_add(bullet->location, bullet->vertices[0], origin);
                glm_vec3_copy(bullet->direction, direction);

                float distance;
                bool intersection;

                // Simple but inexact check for collision, only false positives
                if (glm_vec3_distance(asteroid->location, bullet->location) > (bullet->speed + asteroid->speed)*delta + MINIMUM_COLLISION_DISTANCE*asteroid->size)
                    intersection = false;
                else
                    intersection = glm_ray_triangle(origin, direction, v0, v1, v2, &distance);

                // Exact collision check
                if (intersection && distance <= glm_vec3_distance(bullet->vertices[0], bullet->vertices[1]) + bullet->speed*delta) {
                    // Warning: Memory leak; Removes asteroid and bullet
                    *asteroids_link = asteroids_head->next;
                    asteroid_destroyed = 1;
                    *bullets_link = bullets_head->next;
                    i = asteroid->vertices_length;

                    // If asteroid was big enough, split it into two
                    float size = asteroid->size;
                    if (size > 0.24f){
                        world->score++;
                        size /= 2.0f;
                        asteroid_t *asteroid1 = create_asteroid(asteroid->location, ASTEROID_SIZE*size, ASTEROID_VARIATION*size);
                        asteroid_t *asteroid2 = create_asteroid(asteroid->location, ASTEROID_SIZE*size, ASTEROID_VARIATION*size);
                        asteroid1->size = size;
                        asteroid2->size = size;
                        world->asteroids = asteroid_list_cons(asteroid1, world->asteroids);
                        world->asteroids = asteroid_list_cons(asteroid2, world->asteroids);
                        glm_vec3_ortho(bullet->direction, asteroid1->direction);
                        glm_vec3_copy(asteroid1->direction, asteroid2->direction);
                        glm_vec3_negate(asteroid2->direction);

                        float longitude = rand() / (float) RAND_MAX * 3.14159 * 2;
                        float colatitude = rand() / (float) RAND_MAX * 3.14159;
                        float distance = rand() / (float) RAND_MAX * (max_distance - 1000.0f) + 1000.0f;
                        vec3 spawn_location = { distance * cos(longitude) * sin(colatitude),
                                                distance * sin(longitude) * sin(colatitude),
                                                distance * cos(colatitude) };
                        world->asteroids = asteroid_list_cons(create_asteroid(spawn_location,
                                                                              ASTEROID_SIZE,
                                                                              ASTEROID_VARIATION),
                                                              world->asteroids);
                        world->asteroids->this->speed += sqrt((float) world->score)*250;
                    }
                    break;
                } else
                    bullets_link = &(bullets_head->next);
                bullets_head = bullets_head->next;
            }
        }
        if (!asteroid_destroyed)
            asteroids_link = &(asteroids_head->next);
        asteroids_head = asteroids_head->next;
    }

    // Simple but inaccurate asteroid-ship collision check
    asteroids_head = world->asteroids;
    while(asteroids_head->next != NULL) {
        asteroid_t *asteroid = asteroids_head->this;
        if(glm_vec3_norm(asteroid->location) < MINIMUM_COLLISION_DISTANCE*asteroid->size) {
            world->running = false;
            glm_vec3_copy(GLM_VEC3_ZERO, world->ship->movement_direction);
        }
        asteroids_head = asteroids_head->next;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_T && action == GLFW_PRESS){
        bullet_t *bullet = create_bullet((vec3) {0.0f, 0.0f, 0.0f}, world->ship->pointing_direction, 700.0+glm_vec3_norm(world->ship->movement_direction));
        world->bullets = bullet_list_cons(bullet, world->bullets);
    }
}

int main(int argc, char *argv[]) {
    // Initialize window
    int error = intialize_window(&window);
    if (error)
        return error;
    glfwSetKeyCallback(window, key_callback);

    // Create world
    srand(time(0));
    world = create_world();

    // Generate asteroids
    for (int i = 0; i < 20; i++) {
        // Generate random longitude, colatitude, distance not too close to ship
        float longitude = rand() / (float) RAND_MAX * 3.14159 * 2;
        float colatitude = rand() / (float) RAND_MAX * 3.14159;
        float distance = sqrt(rand() / (float) RAND_MAX) * (max_distance - 750.0f) + 250.0f;
        vec3 spawn_location = { distance * cos(longitude) * sin(colatitude),
                                distance * sin(longitude) * sin(colatitude),
                                distance * cos(colatitude) };

        // Generate asteroid and add to world
        asteroid_t *asteroid = create_asteroid(spawn_location,
                                               ASTEROID_SIZE,
                                               ASTEROID_VARIATION);
        world->asteroids = asteroid_list_cons(asteroid, world->asteroids);
    }

    double new_time = 0.0d;
    double last_time = glfwGetTime();
    double delta = 0.0d;

    while(!glfwWindowShouldClose(window)) {
        // Get delta
        new_time = glfwGetTime();
        delta = new_time - last_time;
        last_time = new_time;

        // Update world
        move_objects(delta);
        process_collisions(delta);
        glm_vec3_scale(world->ship->movement_direction, powf(0.75f, delta), world->ship->movement_direction);

        // Handle keyboard input
        if (world->running) {
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                vec3 speed_diff;
                glm_vec3_scale(world->ship->pointing_direction, delta * 120.0f, speed_diff);
                glm_vec3_add(world->ship->movement_direction, speed_diff, world->ship->movement_direction);
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
                vec3 speed_diff;
                glm_vec3_scale(world->ship->pointing_direction, -delta * 120.0f, speed_diff);
                glm_vec3_add(world->ship->movement_direction, speed_diff, world->ship->movement_direction);
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                glm_vec3_rotate(world->ship->pointing_direction, -delta, (vec3) {0.0f, 1.0f, 0.0f});
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                glm_vec3_rotate(world->ship->pointing_direction, delta, (vec3) {0.0f, 1.0f, 0.0f});
            }
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                vec3 axis;
                glm_vec3_cross((vec3) {0.0f, 1.0f, 0.0f}, world->ship->pointing_direction, axis);
                glm_vec3_rotate(world->ship->pointing_direction, -delta, axis);
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                vec3 axis;
                glm_vec3_cross((vec3) {0.0f, 1.0f, 0.0f}, world->ship->pointing_direction, axis);
                glm_vec3_rotate(world->ship->pointing_direction, delta, axis);
            }
        }

        render(window, world);

        glfwPollEvents();
    }

    return 0;
}
