/*
 * Copyright (c) 2013-2018 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
#ifndef _IPMONITOR_CONTROL_H
#define _IPMONITOR_CONTROL_H
/*
 * IPMonitorControl.h
 * - IPC channel to IPMonitor
 * - used to create interface rank assertions
 */

/*
 * Modification History
 *
 * December 16, 2013	Dieter Siegmund (dieter@apple.com)
 * - initial revision
 */

#include <SystemConfiguration/SCNetworkConfigurationPrivate.h>

struct IPMonitorControl;
typedef struct IPMonitorControl * IPMonitorControlRef;

IPMonitorControlRef
IPMonitorControlCreate(void);

Boolean
IPMonitorControlSetInterfacePrimaryRank(IPMonitorControlRef control,
					CFStringRef ifname,
					SCNetworkServicePrimaryRank rank);

SCNetworkServicePrimaryRank
IPMonitorControlGetInterfacePrimaryRank(IPMonitorControlRef control,
					CFStringRef ifname);

Boolean
IPMonitorControlSetInterfaceAdvisory(IPMonitorControlRef control,
				     CFStringRef ifname,
				     SCNetworkInterfaceAdvisory advisory,
				     CFStringRef reason);
Boolean
IPMonitorControlInterfaceAdvisoryIsSet(IPMonitorControlRef control,
				       CFStringRef ifname);
Boolean
IPMonitorControlAnyInterfaceAdvisoryIsSet(IPMonitorControlRef control);

CFStringRef
IPMonitorControlCopyInterfaceAdvisoryNotificationKey(CFStringRef ifname);

#endif /* _IPMONITOR_CONTROL_H */

