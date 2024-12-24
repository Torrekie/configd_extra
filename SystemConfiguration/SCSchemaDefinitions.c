#include <TargetConditionals.h>
#include <CoreFoundation/CFString.h>

#if	!TARGET_OS_IPHONE
const CFStringRef kSCEntNetAppleTalk                               = CFSTR("AppleTalk");
#endif	// !TARGET_OS_IPHONE

#if	!TARGET_OS_IPHONE
const CFStringRef kSCEntNetNetInfo                                 = CFSTR("NetInfo");
#endif	// !TARGET_OS_IPHONE

#if	!TARGET_OS_IPHONE
const CFStringRef kSCEntNetSMB                                     = CFSTR("SMB");
#endif	// !TARGET_OS_IPHONE

#if	!TARGET_OS_IPHONE
const CFStringRef kSCPropNetAppleTalkConfigMethod                  = CFSTR("ConfigMethod");
const CFStringRef kSCPropNetAppleTalkDefaultZone                   = CFSTR("DefaultZone");
const CFStringRef kSCPropNetAppleTalkNetworkID                     = CFSTR("NetworkID");
const CFStringRef kSCPropNetAppleTalkNodeID                        = CFSTR("NodeID");
const CFStringRef kSCValNetAppleTalkConfigMethodNode               = CFSTR("Node");
#endif	// !TARGET_OS_IPHONE

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
#endif	// !TARGET_OS_IPHONE

const CFStringRef kSCEntUsersConsoleUser                           = CFSTR("ConsoleUser");

#if	!TARGET_OS_IPHONE
const CFStringRef kSCPropUsersConsoleUserName                      = CFSTR("Name");
const CFStringRef kSCPropUsersConsoleUserUID                       = CFSTR("UID");
const CFStringRef kSCPropUsersConsoleUserGID                       = CFSTR("GID");
#endif	// !TARGET_OS_IPHONE

