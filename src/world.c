#include "world.h"

world_t *create_world() {
    world_t *world = malloc(sizeof(world_t));
    world->asteroids = create_asteroid_list();
    world->dust_cloud = create_dust_cloud();
    world->bullets = create_bullet_list();
    world->ship = create_ship((vec3) {0.0f, 0.0f, -1.0f});
    world->score = 0;
    world->running = true;

    return world;
}

dust_cloud_t *create_dust_cloud() {
    dust_cloud_t *dust_cloud = malloc(sizeof(dust_cloud_t));
    dust_cloud->vertices_length = 25000;
    dust_cloud->vertices = malloc(sizeof(vec3)*dust_cloud->vertices_length);

    for (int i = 0; i < dust_cloud->vertices_length; i++) {
        float longitude = rand() / (float) RAND_MAX * 3.14159 * 2;
        float colatitude = rand() / (float) RAND_MAX * 3.14159;
        float distance = cbrt(rand() / (float) RAND_MAX) * max_distance;
        vec3 spawn_location = { distance * cos(longitude) * sin(colatitude),
                                distance * sin(longitude) * sin(colatitude),
                                distance * cos(colatitude) };
        glm_vec3_copy(spawn_location, dust_cloud->vertices[i]);
    }

    return dust_cloud;
}

void make_normal(vec3 a, vec3 b, vec3 c, vec3 n) {
    // Given vertices of triangle, creates normal that points away from origin
    vec3 v, w;
    glm_vec3_sub(c, a, v);
    glm_vec3_sub(c, b, w);
    glm_vec3_crossn(v, w, n);
    if (glm_vec3_angle(a,n) > 3.1415926535f/2.0f) {
        glm_vec3_negate(n);
    }
}

void make_vertex(float longitude, float colatitude, float radius, float variation, vec3 vec) {
    radius = radius + rand() / (float) RAND_MAX * variation - variation / 2;
    vec[0] = radius*cos(longitude)*sin(colatitude);
    vec[1] = radius*cos(colatitude);
    vec[2] = radius*sin(longitude)*sin(colatitude);
}

asteroid_t *create_asteroid(vec3 location, float radius, float variation) {
    asteroid_t *asteroid = malloc(sizeof(asteroid_t));

    asteroid->vertices_length = 48*3;
    asteroid->vertices = malloc(asteroid->vertices_length * sizeof(vec3));
    asteroid->normals = malloc(asteroid->vertices_length * sizeof(vec3));
    
    vec3 top;
    make_vertex(0.0f, 0.0f, radius, variation, top);
    vec3 first_band[6];
    for (int i = 0; i < 6; i++) {
        float longitude = 3.14159f / 3 * i;
        float colatitude = 3.14159f / 4;
        make_vertex(longitude, colatitude, radius, variation, first_band[i]);
    }
    vec3 second_band[12];
    for (int i = 0; i < 12; i++) {
        float longitude = 3.14159f / 6 * i;
        float colatitude = 3.14159f / 2;
        make_vertex(longitude, colatitude, radius, variation, second_band[i]);
    }
    vec3 third_band[6];
    for (int i = 0; i < 6; i++) {
        float longitude = 3.14159f / 3 * i;
        float colatitude = 3 * 3.14159f / 4;
        make_vertex(longitude, colatitude, radius, variation, third_band[i]);
    }
    vec3 bottom;
    make_vertex(0.0f, 3.14159f, radius, variation, bottom);

    // First triangle band
    int v = 0;
    for (int i = 0; i < 6; i++) {
        glm_vec3_copy(top, asteroid->vertices[v++]);
        glm_vec3_copy(first_band[i], asteroid->vertices[v++]);
        glm_vec3_copy(first_band[(i+1)%6], asteroid->vertices[v++]);
    }

    // Second triangle band */
    for (int i = 0; i < 6; i++) {
        glm_vec3_copy(first_band[i], asteroid->vertices[v++]);
        glm_vec3_copy(second_band[i*2+1], asteroid->vertices[v++]);
        glm_vec3_copy(first_band[(i+1)%6], asteroid->vertices[v++]);
        
        glm_vec3_copy(second_band[i*2], asteroid->vertices[v++]);
        glm_vec3_copy(second_band[i*2+1], asteroid->vertices[v++]);
        glm_vec3_copy(first_band[i], asteroid->vertices[v++]);

        glm_vec3_copy(second_band[i*2+1], asteroid->vertices[v++]);
        glm_vec3_copy(second_band[(i*2+2)%12], asteroid->vertices[v++]);
        glm_vec3_copy(first_band[(i+1)%6], asteroid->vertices[v++]);
    }
    
    // Third triangle band
    for (int i = 0; i < 6; i++) {
        glm_vec3_copy(third_band[i], asteroid->vertices[v++]);
        glm_vec3_copy(second_band[i*2+1], asteroid->vertices[v++]);
        glm_vec3_copy(third_band[(i+1)%6], asteroid->vertices[v++]);
        
        glm_vec3_copy(second_band[i*2], asteroid->vertices[v++]);
        glm_vec3_copy(second_band[i*2+1], asteroid->vertices[v++]);
        glm_vec3_copy(third_band[i], asteroid->vertices[v++]);

        glm_vec3_copy(second_band[i*2+1], asteroid->vertices[v++]);
        glm_vec3_copy(second_band[(i*2+2)%12], asteroid->vertices[v++]);
        glm_vec3_copy(third_band[(i+1)%6], asteroid->vertices[v++]);
    }

    // Fourth triangle band
    for (int i = 0; i < 6; i++) {
        glm_vec3_copy(bottom, asteroid->vertices[v++]);
        glm_vec3_copy(third_band[i], asteroid->vertices[v++]);
        glm_vec3_copy(third_band[(i+1)%6], asteroid->vertices[v++]);
    }

    // Make normals
    for (int i = 0; i < 36; i++)
        for (int j = 0; j < 3; j++)
            make_normal(asteroid->vertices[i*3],
                        asteroid->vertices[i*3+1],
                        asteroid->vertices[i*3+2],
                        asteroid->normals[i*3+j]);

    glm_vec3_copy(location, asteroid->location);

    asteroid->rotation_speed = rand() / (float) RAND_MAX * 0.25f;
    for (int i = 0; i < 3; i++)
        asteroid->axis[i] = rand() / (float) RAND_MAX;
    glm_vec3_normalize(asteroid->axis);
    asteroid->angle = rand() / (float) RAND_MAX * 3.14159 * 2;

    glm_vec3_copy((vec3) {rand() / (float) RAND_MAX,
                              rand() / (float) RAND_MAX,
                              rand() / (float) RAND_MAX},
        asteroid->direction);
    glm_vec3_normalize(asteroid->direction);
    asteroid->speed = rand() / (float) RAND_MAX * 250;

    glGenBuffers(1, &(asteroid->vbo));
    glGenBuffers(1, &(asteroid->nbo));

    glBindBuffer(GL_ARRAY_BUFFER, asteroid->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*asteroid->vertices_length, asteroid->vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, asteroid->nbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*asteroid->vertices_length, asteroid->normals, GL_DYNAMIC_DRAW);

    asteroid->size = 1.0f;

    return asteroid;
}

asteroid_list_t *create_asteroid_list() {
    asteroid_list_t *asteroids = malloc(sizeof(asteroid_list_t));

    asteroids->this = NULL;
    asteroids->next = NULL;

    return asteroids;
}

asteroid_list_t *asteroid_list_cons(asteroid_t* asteroid, asteroid_list_t* asteroids) {
    asteroid_list_t *node = malloc(sizeof(asteroid_list_t));

    node->this = asteroid;
    node->next = asteroids;

    return node;
}

bullet_t *create_bullet(vec3 location, vec3 direction, float speed) {
    bullet_t *bullet = malloc(sizeof(bullet_t));
    bullet->vertices = malloc(2 * sizeof(vec3));

    glm_vec3_copy(location, bullet->location);
    glm_vec3_copy(direction, bullet->direction);
    glm_vec3_normalize(direction);
    glm_vec3_scale(direction, 1.0f, bullet->vertices[1]);
    glm_vec3_scale(direction, 0.0f, bullet->vertices[0]);
    bullet->speed = speed;

    return bullet;
}

bullet_list_t *create_bullet_list() {
    bullet_list_t *bullets = malloc(sizeof(bullet_list_t));

    bullets->this = NULL;
    bullets->next = NULL;

    return bullets;
}

bullet_list_t *bullet_list_cons(bullet_t* bullet, bullet_list_t* bullets) {
    bullet_list_t *node = malloc(sizeof(bullet_list_t));

    node->this = bullet;
    node->next = bullets;

    return node;
}

ship_t *create_ship(vec3 direction) {
    ship_t *ship = malloc(sizeof(ship_t));

    glm_vec3_copy(GLM_VEC3_ZERO, ship->movement_direction);
    glm_vec3_copy(direction, ship->pointing_direction);

    vec3 vertices[5] = {{0.0f, 0.0f, -2.0f}, // front       0
                        {-1.0f, 0.0f, 1.0f}, // back, left  1
                        {1.0f, 0.0f, 1.0f}, // back, right  2
                        {0.0f, 0.2f, 1.0f}, // back, up     3
                        {0.0f, -0.2f, 1.0f}}; // back, down 4

    ship->vertices = malloc(6*3*sizeof(vec3)); // 6 surfaces of 3 vertices
    ship->normals = malloc(6*3*sizeof(vec3)); // idem

    glm_vec3_copy(vertices[0], ship->vertices[0]);
    glm_vec3_copy(vertices[1], ship->vertices[1]);
    glm_vec3_copy(vertices[3], ship->vertices[2]);

    glm_vec3_copy(vertices[0], ship->vertices[3]);
    glm_vec3_copy(vertices[2], ship->vertices[4]);
    glm_vec3_copy(vertices[3], ship->vertices[5]);

    glm_vec3_copy(vertices[0], ship->vertices[6]);
    glm_vec3_copy(vertices[1], ship->vertices[7]);
    glm_vec3_copy(vertices[4], ship->vertices[8]);

    glm_vec3_copy(vertices[0], ship->vertices[9]);
    glm_vec3_copy(vertices[2], ship->vertices[10]);
    glm_vec3_copy(vertices[4], ship->vertices[11]);

    glm_vec3_copy(vertices[3], ship->vertices[12]);
    glm_vec3_copy(vertices[1], ship->vertices[13]);
    glm_vec3_copy(vertices[4], ship->vertices[14]);

    glm_vec3_copy(vertices[3], ship->vertices[15]);
    glm_vec3_copy(vertices[2], ship->vertices[16]);
    glm_vec3_copy(vertices[4], ship->vertices[17]);

    for (int i = 0; i < 6; i++) {
        for (int j = i * 3; j < i * 3 + 3; j++) {
            make_normal(ship->vertices[i*3],
                        ship->vertices[i*3+1],
                        ship->vertices[i*3+2],
                        ship->normals[j]);
        }
    }
    
    return ship;
}
