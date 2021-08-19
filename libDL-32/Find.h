#define FORWARD	 1
#define BACKWARD 0

@interface Find:Object
{
    id searchStr, panel, iconButton;
}

- enterFirstWord:(char const *)str;
- enterSelection:(id)sender;
- (const char *)searchStr;
- showPanel:(id)sender;
- search:(id)sender in:(id)textObj;

extern id buildFindMenu(void);
extern id findPanel(void);
@end
