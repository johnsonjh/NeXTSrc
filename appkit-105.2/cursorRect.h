
#import "chunk.h"
#import "Window.h"

typedef struct _NXCursorRect {
    NXRect cursorRect;
    id cursor;
    id view;
    BOOL isRectUp;
} NXCursorRect;

typedef struct _NXCursorArray {
    NXChunk chunk;
    NXCursorRect cursorRects[1];
} NXCursorArray;

typedef struct _NXCursorStack {
    NXChunk chunk;
    NXCursorRect *rects[1];
} NXCursorStack;

enum NXCursorRemove {eventsFor, otherEvents, allEvents};
typedef enum NXCursorRemove NXCursorRemove;


extern NXCursorRect *_NXIndexToCursorRect(Window *self, int index);
extern void _NXResetCursorState(int windowNum, NXCursorRemove which, BOOL show);
