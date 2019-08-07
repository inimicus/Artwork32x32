#import "ApplicationDelegate.h"
#import "gamma.h"

@implementation ApplicationDelegate

@synthesize panelController     = _panelController;
@synthesize menubarController   = _menubarController;

#pragma mark -

- (void)dealloc {
    [_panelController removeObserver:self forKeyPath:@"hasActivePanel"];
}

#pragma mark -

void *kContextActivePanel = &kContextActivePanel;

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if (context == kContextActivePanel) {
        self.menubarController.hasActiveIcon = self.panelController.hasActivePanel;
    }
    else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

#pragma mark - NSApplicationDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Install icon into the menu bar
    self.menubarController = [[MenubarController alloc] init];

    // Listen for notifications of change
    NSDistributedNotificationCenter *dnc = [NSDistributedNotificationCenter defaultCenter];
    [dnc addObserver:self selector:@selector(updateArtworkDisplay:) name:@"com.apple.iTunes.playerInfo" object:nil];

}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {

    // Clear display
    if (rawhid_open(1, 0x16C0, 0x0486, 0xFFAB, 0x0200) <= 0) {
        MWLogError(@"No device found!");
    } else {

        // Start Packet
        UInt8 packetStart[] = {83,84,65,82,84,0};
        rawhid_send(0, packetStart, 6, 10);

        // Then data
        UInt8 clearOutput[4096] = {0};
        rawhid_send(0, clearOutput, 4096, 1);

        // Stop Packet
        UInt8 packetStop[]  = {83,84,79,80,0};
        rawhid_send(0, packetStop, 5, 10);

        // Close device
        rawhid_close(0);

    }

    // Explicitly remove the icon from the menu bar
    self.menubarController = nil;
    return NSTerminateNow;
}

#pragma mark - Actions

- (IBAction)togglePanel:(id)sender {
    self.menubarController.hasActiveIcon = !self.menubarController.hasActiveIcon;
    self.panelController.hasActivePanel = self.menubarController.hasActiveIcon;
}

#pragma mark - Public accessors

- (PanelController *)panelController {

    if (_panelController == nil) {
        _panelController = [[PanelController alloc] initWithDelegate:self];
        [_panelController addObserver:self forKeyPath:@"hasActivePanel" options:0 context:kContextActivePanel];
    }

    return _panelController;
}

#pragma mark - PanelControllerDelegate

- (StatusItemView *)statusItemViewForPanelController:(PanelController *)controller {
    return self.menubarController.statusItemView;
}

- (void) updateArtworkDisplay:(NSNotification *)notification {

    // =====================================================================
    // Retreive iTunes Information
    // =====================================================================

        // Get active track
        iTunesApplication   *iTunes     = [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
        iTunesTrack         *track      = iTunes.currentTrack;

        // Get artwork
        iTunesArtwork       *artwork    = track.artworks[0];
        NSImage             *image      = [[NSImage alloc] initWithData:[artwork rawData]];


    // =====================================================================
    // Resize Artwork
    // =====================================================================

        // Setup Contexts
        CGSize          size            = CGSizeMake(32, 32);
        CGColorSpaceRef rgbColorspace   = CGColorSpaceCreateDeviceRGB();
        CGBitmapInfo    bitmapInfo      = (CGBitmapInfo) kCGImageAlphaNoneSkipLast;
        CGContextRef    context         = CGBitmapContextCreate(
                                            NULL,           // Data
                                            size.width,     // Width
                                            size.height,    // Height
                                            8,              // Bits per component
                                            size.width * 4, // Bytes per row
                                            rgbColorspace,  // Colorspace
                                            bitmapInfo      // Alpha settings
                                        );

        NSGraphicsContext *graphicsContext = [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:NO];
        [NSGraphicsContext setCurrentContext:graphicsContext];

        // Resize
        [image drawInRect:NSMakeRect(0, 0, size.width, size.height) fromRect:NSMakeRect(0, 0, image.size.width, image.size.height) operation:NSCompositeCopy fraction:1.0];

        // Save File
        CGImageRef  imageData   = CGBitmapContextCreateImage(context);
        CFDataRef   imagePixels = CGDataProviderCopyData(CGImageGetDataProvider(imageData));

        // Cleanup
        CGImageRelease(imageData);
        CGContextRelease(context);
        CGColorSpaceRelease(rgbColorspace);


    // =====================================================================
    // Interleave Image
    // =====================================================================

        // Pixel data pointer
        UInt8   *pixelData = (UInt8 *) CFDataGetBytePtr(imagePixels);

        UInt8   upperRed, upperGreen, upperBlue,
                lowerRed, lowerGreen, lowerBlue;

        UInt16  readPosition,
                maxLength       = 2048,
                writePosition   = 0,
                outputSize      = 4096;

        UInt8   output[outputSize];

        for (readPosition = 0; readPosition < maxLength; readPosition += 4) {

            if(writePosition != 0 && ((writePosition % 32) == 0)) {
                writePosition += 224;
            }

            upperRed    = gammaCorrection[pixelData[readPosition]];
            upperGreen  = gammaCorrection[pixelData[readPosition + 1]];
            upperBlue   = gammaCorrection[pixelData[readPosition + 2]];

            lowerRed    = gammaCorrection[pixelData[readPosition + 2048]];
            lowerGreen  = gammaCorrection[pixelData[readPosition + 2048 + 1]];
            lowerBlue   = gammaCorrection[pixelData[readPosition + 2048 + 2]];

            output[writePosition]           = ((lowerBlue & 1  ) << 5) + ((lowerGreen & 1  ) << 4) + ((lowerRed & 1  ) << 3) + ((upperBlue & 1  ) << 2) + ((upperGreen & 1  ) << 1) + ( upperRed & 1        );
            output[writePosition+32]        = ((lowerBlue & 2  ) << 4) + ((lowerGreen & 2  ) << 3) + ((lowerRed & 2  ) << 2) + ((upperBlue & 2  ) << 1) + ( upperGreen & 2        ) + ((upperRed & 2  ) >> 1);
            output[writePosition+(32*2)]    = ((lowerBlue & 4  ) << 3) + ((lowerGreen & 4  ) << 2) + ((lowerRed & 4  ) << 1) + ( upperBlue & 4        ) + ((upperGreen & 4  ) >> 1) + ((upperRed & 4  ) >> 2);
            output[writePosition+(32*3)]    = ((lowerBlue & 8  ) << 2) + ((lowerGreen & 8  ) << 1) + ( lowerRed & 8        ) + ((upperBlue & 8  ) >> 1) + ((upperGreen & 8  ) >> 2) + ((upperRed & 8  ) >> 3);
            output[writePosition+(32*4)]    = ((lowerBlue & 16 ) << 1) + ( lowerGreen & 16       ) + ((lowerRed & 16 ) >> 1) + ((upperBlue & 16 ) >> 2) + ((upperGreen & 16 ) >> 3) + ((upperRed & 16 ) >> 4);
            output[writePosition+(32*5)]    = ( lowerBlue & 32       ) + ((lowerGreen & 32 ) >> 1) + ((lowerRed & 32 ) >> 2) + ((upperBlue & 32 ) >> 3) + ((upperGreen & 32 ) >> 4) + ((upperRed & 32 ) >> 5);
            output[writePosition+(32*6)]    = ((lowerBlue & 64 ) >> 1) + ((lowerGreen & 64 ) >> 2) + ((lowerRed & 64 ) >> 3) + ((upperBlue & 64 ) >> 4) + ((upperGreen & 64 ) >> 5) + ((upperRed & 64 ) >> 6);
            output[writePosition+(32*7)]    = ((lowerBlue & 128) >> 2) + ((lowerGreen & 128) >> 3) + ((lowerRed & 128) >> 4) + ((upperBlue & 128) >> 5) + ((upperGreen & 128) >> 6) + ((upperRed & 128) >> 7);

            writePosition++;

        }

    
    // =====================================================================
    // Notify Device
    // =====================================================================

        // Attempt to open device
        if (rawhid_open(1, 0x16C0, 0x0486, 0xFFAB, 0x0200) <= 0) {
            MWLogError(@"No device found!");
        } else {

            // Start Packet
            UInt8 packetStart[] = {83,84,65,82,84,0};
            rawhid_send(0, packetStart, 6, 10);

            // Then data
            rawhid_send(0, output, outputSize, 1);

            // Stop Packet
            UInt8 packetStop[]  = {83,84,79,80,0};
            rawhid_send(0, packetStop, 5, 10);

            // Close device
            rawhid_close(0);

        }

}

@end
