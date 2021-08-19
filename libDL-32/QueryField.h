@interface QueryField : TextField
{
	int currSel;
	id textView;
	id clickBtn;
}

- initPerformClickBtn:btn textView:tView queryEndAction:(SEL) action queryEndTarget:target;
- didSelection:sender;
- (char *)getCurrSelection;
- setQuery:(char const *)s;
- nextQ:sender;
- prevQ:sender;
- click:sender:(char *)t:(NXEvent *)e;
- mouseDown:(NXEvent *) theEvent;
@end
