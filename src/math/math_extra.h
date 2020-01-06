float clampf(float f, float min, float max)
{
    return (f < min ? min : f > max ? max : f);
}

int clampi(int f, int min, int max)
{
	return (f < min ? min : f > max ? max : f);
}

#define DEG_360 (2*M_PI)
#define DEG_180 (M_PI)
#define DEG_90 (M_PI/2)
#define DEG_45 (M_PI/4)