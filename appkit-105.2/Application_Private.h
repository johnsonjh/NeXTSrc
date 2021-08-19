#import "Application.h"

@interface Application(Private)

- _commonAppInit;
- _setUp;
- _removeHiddenWindow:(int)windowNum;
- _focusStack;
- (BOOL)_flipState;
- _setMainWindow:newMainWindow;
- _mainWindow;
- _setKeyWindow:newKeyWindow;
- _keyWindow;
- _setInvalid:(BOOL)flag;
- (int)_doOpenFile:(const char *)fullPath ok:(int *)flag tryTemp:(BOOL)tryTemp;
- (BOOL)_isInvalid;
- (BOOL)_isDeactPending;
- (BOOL)_isInvalidEvent;
- (BOOL)_isRunningModal;
- _replyToLaunch;
- _replyToOpen:(int)ok;
- _removeServicesMenuPort:(const char *)name;
- _initServicesMenu;
- _doUpdateServicesMenu:sender;
- (BOOL)_updateFontMenu:cell;
- _orderFrontModalWindow:theWindow;
- _updateModalMenus;
- _removeWindow:aWindow;
- _addWindow:aWindow;
- _loadNibSegment:(const char *)segmentName section:(const char *)sectionName owner:anOwner withNames:(BOOL)flag fromShlib:(BOOL)fromShlib fromHeader:(const struct mach_header *)mhp fromZone:(NXZone *)zone;
- _restoreCursor;
- (int)_currentActivation;
- _setCurrentActivation:(int)activationNum;
- (const NXScreen *)_zeroScreen;
- _setSlaveJournaler:newJournaler;
- _setMasterJournaler:newJournaler;
- (BOOL)_isDoingOpenFile;
- (BOOL)_servicesMenuIsVisible;
@end


@interface Application (WindowCache)
- findWindowUsingCache:(int)windowNum;
- updateWindowUsingCache;
- setWindowNum:(int)windowNum forWindow:window;
- setVisible:(BOOL)visible forWindow:window;
- setOverridesUpdate:(BOOL)overridesUpdate forWindow:window;
- setDelegateUpdates:(BOOL)delegateUpdates forWindow:window;
- removeWindowFromCache:window;
@end

