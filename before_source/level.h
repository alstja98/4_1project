#ifndef LIB_t
#define LIB_t

#define LEVEL_SIZE 17
#define LEVEL_ROAD 0
#define LEVEL_TRAP 1

extern void levelInit();
extern int	levelGetFirst();
extern void	levelUpdate();
extern int*	levelGetList();

#endif 
