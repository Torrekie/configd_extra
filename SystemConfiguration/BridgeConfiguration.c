#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFRuntime.h>

#include "SCNetworkConfigurationInternal.h"
#include "SCPreferencesInternal.h"

__private_extern__ void
__SCBridgeInterfaceListCollectMembers(CFArrayRef interfaces, CFMutableSetRef set)
{
	CFIndex	i;
	CFIndex	n;

	n = CFArrayGetCount(interfaces);
	for (i = 0; i < n; i++) {
		SCBridgeInterfaceRef	bridgeInterface;
		CFArrayRef		members;

		bridgeInterface = CFArrayGetValueAtIndex(interfaces, i);
		members = SCBridgeInterfaceGetMemberInterfaces(bridgeInterface);
		if (members != NULL) {
			CFIndex	j;
			CFIndex	n_members;

			// exclude the member interfaces of this bridge
			n_members = CFArrayGetCount(members);
			for (j = 0; j < n_members; j++) {
				SCNetworkInterfaceRef	member;

				member = CFArrayGetValueAtIndex(members, j);
				CFSetAddValue(set, member);
			}
		}

	}
	return;
}
