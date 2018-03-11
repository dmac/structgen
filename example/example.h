int i;

struct Entity {
    /* test doc */
    double x;
    double y;
    long   health;
};

typedef struct {
    Entity entity;
    char * name;
} Player;
