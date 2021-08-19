typedef struct {
	NXRect	mainWindow;
	int		numSubviews;
	float	*subviews;
} frames;	

extern frames Frames;

extern void readFrames(char *frameStr);
extern void writeFrames(char *frameStr);
