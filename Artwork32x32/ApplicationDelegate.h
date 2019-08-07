
#import <Cocoa/Cocoa.h>
#import "MenubarController.h"
#import "PanelController.h"
#import "MWLogging.h"
#import "iTunes.h"
#import "hid.h"

@interface ApplicationDelegate : NSObject <NSApplicationDelegate, PanelControllerDelegate>

@property (nonatomic, strong) MenubarController *menubarController;
@property (nonatomic, strong, readonly) PanelController *panelController;

- (IBAction)togglePanel:(id)sender;

@end
