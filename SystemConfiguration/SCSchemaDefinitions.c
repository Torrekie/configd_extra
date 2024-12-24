#include <TargetConditionals.h>
#include <CoreFoundation/CFString.h>

const CFStringRef kSCEntNetNetInfo                                 = CFSTR("NetInfo");

// IPMonitor
#if	!TARGET_OS_IPHONE
const CFStringRef kSCEntNetSMB                                     = CFSTR("SMB");
#endif	// !TARGET_OS_IPHONE

// IPMonitor
#if	!TARGET_OS_IPHONE
const CFStringRef kSCPropNetSMBNetBIOSName                         = CFSTR("NetBIOSName");
const CFStringRef kSCPropNetSMBNetBIOSNodeType                     = CFSTR("NetBIOSNodeType");
const CFStringRef kSCPropNetSMBNetBIOSScope                        = CFSTR("NetBIOSScope");
const CFStringRef kSCPropNetSMBWINSAddresses                       = CFSTR("WINSAddresses");
const CFStringRef kSCPropNetSMBWorkgroup                           = CFSTR("Workgroup");
const CFStringRef kSCValNetSMBNetBIOSNodeTypeBroadcast             = CFSTR("Broadcast");
const CFStringRef kSCValNetSMBNetBIOSNodeTypePeer                  = CFSTR("Peer");
const CFStringRef kSCValNetSMBNetBIOSNodeTypeMixed                 = CFSTR("Mixed");
const CFStringRef kSCValNetSMBNetBIOSNodeTypeHybrid                = CFSTR("Hybrid");
#endif

// SCDConsoleUser.c
const CFStringRef kSCEntUsersConsoleUser                           = CFSTR("ConsoleUser");
