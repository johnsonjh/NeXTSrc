extern void Wait(void);
extern void DoneWaiting(void);

extern void notImp(char *name);

extern int interrupted(void);
extern int sampledInterrupt(void);
extern void flushMouseEvents(void);
extern void setupInterruptable(id button, const char *altHighlight);
