// Derived from lcandy2's PR #1995; licensed under the unchanged MMF License.
#import <Foundation/Foundation.h>
NS_ASSUME_NONNULL_BEGIN
/// Opt-in HID++ support for MX Master 3 B023 over Bluetooth only.
@interface LogitechButtonDiverter : NSObject
/// Called on MMF's config/event-tap run loop. Snapshots policy at startup.
+ (void)load_Manual;
/// Serialize cleanup onto the HID++ worker and wait up to four seconds.
/// Call from normal application code, never from a POSIX signal handler.
+ (void)restoreNativeBehavior;
@end
NS_ASSUME_NONNULL_END
