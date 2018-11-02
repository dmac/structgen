int i;

struct Entity {
    /* test doc */
    double x;
    double y;
    long   health;
};

typedef struct {
    struct Entity entity;
    char *        name;
} Player;

char *Entity_string(struct Entity *this) {
    char *format = "Entity{x=%g y=%g health=%ld}";
    int   size   = snprintf(NULL, 0, format, this->x, this->y, this->health);
    char *s      = malloc(size + 1);
    snprintf(s, size + 1, format, this->x, this->y, this->health);
    return s;
}

char *Player_string(Player *this) {
    char *format        = "Player{entity=%s name=0x%lx}";
    char *entity_string = Entity_string(&this->entity);
    int   size          = snprintf(NULL, 0, format, entity_string, this->name);
    char *s             = malloc(size + 1);
    snprintf(s, size + 1, format, entity_string, this->name);
    free(entity_string);
    return s;
}
