typedef struct Texture Texture;

Texture* texture_create(const char* file);

void texture_destroy(Texture* tex);