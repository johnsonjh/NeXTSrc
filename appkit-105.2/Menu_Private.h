#import "Menu.h"
#import "MenuCell.h"

@interface Menu(Private)

+ _repairBackups;
+ _newTitleFromNibSection:(const char *)aTitle withMatrix:aMatrix;
- _initTitleFromNibSection:(const char *)aTitle withMatrix:aMatrix;
- _supermenu;
- _removeFromHierarchy;
- _unattachSubmenu;
- _highlightSubmenuEntry:submenu lit:(BOOL)flag;
- _highlightSupermenu:(BOOL)lit;
- _makeServicesMenu;
- _commonAwake;

@end

@interface MenuCell(Private)

- (BOOL)_isEditing;
- _setEditing:(BOOL)flag;

@end

