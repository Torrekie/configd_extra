/*
 * Copyright (c) 2000, 2001, 2003-2005, 2007-2009, 2011, 2014-2021 Apple Inc. All rights reserved.
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

/*
 * Modification History
 *
 * June 1, 2001			Allan Nathanson <ajn@apple.com>
 * - public API conversion
 *
 * November 9, 2000		Allan Nathanson <ajn@apple.com>
 * - initial revision
 */

#include "SCPreferencesInternal.h"
#include "SCNetworkConfigurationInternal.h"

#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/param.h>

__private_extern__ CF_RETURNS_RETAINED CFDataRef
__SCPSignatureFromStatbuf(const struct stat *statBuf)
{
	CFMutableDataRef	signature;
	SCPSignatureDataRef	sig;

	signature = CFDataCreateMutable(NULL, sizeof(SCPSignatureData));
	CFDataSetLength(signature, sizeof(SCPSignatureData));

	/* ALIGN: CFDataGetBytePtr aligns to at least 8 bytes */
	sig = (SCPSignatureDataRef)(void *)CFDataGetBytePtr(signature);

	sig->st_dev       = statBuf->st_dev;
	sig->st_ino       = statBuf->st_ino;
	sig->tv_sec       = statBuf->st_mtimespec.tv_sec;
	sig->tv_nsec      = statBuf->st_mtimespec.tv_nsec;
	sig->st_size      = statBuf->st_size;
	return signature;
}


__private_extern__
uint32_t
__SCPreferencesGetNetworkConfigurationFlags(SCPreferencesRef prefs)
{
	SCPreferencesPrivateRef	prefsPrivate	= (SCPreferencesPrivateRef)prefs;

	return (prefs != NULL) ? prefsPrivate->nc_flags : 0;
}

__private_extern__
void
__SCPreferencesSetNetworkConfigurationFlags(SCPreferencesRef prefs, uint32_t nc_flags)
{
	SCPreferencesPrivateRef	prefsPrivate	= (SCPreferencesPrivateRef)prefs;

	if (prefs != NULL) {
		prefsPrivate->nc_flags = nc_flags;
	}

	return;
}


__private_extern__ char *
__SCPreferencesPath(CFAllocatorRef	allocator,
		    CFStringRef		prefsID,
		    Boolean		useNewPrefs)
{
	CFStringRef	path		= NULL;
	char		*pathStr;

	if (prefsID == NULL) {
		/* default preference ID */
		path = CFStringCreateWithFormat(allocator,
						NULL,
						CFSTR("%@/%@"),
						useNewPrefs ? PREFS_DEFAULT_DIR    : PREFS_DEFAULT_DIR_OLD,
						useNewPrefs ? PREFS_DEFAULT_CONFIG : PREFS_DEFAULT_CONFIG_OLD);
	} else if (CFStringHasPrefix(prefsID, CFSTR("/"))) {
		/* if absolute path */
		path = CFStringCreateCopy(allocator, prefsID);
	} else {
		/* relative path */
		path = CFStringCreateWithFormat(allocator,
						NULL,
						CFSTR("%@/%@"),
						useNewPrefs ? PREFS_DEFAULT_DIR : PREFS_DEFAULT_DIR_OLD,
						prefsID);
		if (useNewPrefs && CFStringHasSuffix(path, CFSTR(".xml"))) {
			CFMutableStringRef	newPath;

			newPath = CFStringCreateMutableCopy(allocator, 0, path);
			CFStringReplace(newPath,
					CFRangeMake(CFStringGetLength(newPath)-4, 4),
					CFSTR(".plist"));
			CFRelease(path);
			path = newPath;
		}
	}

	/*
	 * convert CFStringRef path to C-string path
	 */
	pathStr = _SC_cfstring_to_cstring(path, NULL, 0, kCFStringEncodingASCII);
	if (pathStr == NULL) {
		CFIndex pathLen;

		pathLen = CFStringGetMaximumSizeOfFileSystemRepresentation(path);
		pathStr = CFAllocatorAllocate(NULL, pathLen, 0);
		if (!CFStringGetFileSystemRepresentation(path, pathStr, pathLen)) {
			SC_log(LOG_INFO, "could not convert path to C string");
			CFAllocatorDeallocate(NULL, pathStr);
			pathStr = NULL;
		}
	}

	CFRelease(path);
	return pathStr;
}


__private_extern__
Boolean
__SCPreferencesIsEmpty(SCPreferencesRef	prefs)
{
	SCPreferencesPrivateRef	prefsPrivate	= (SCPreferencesPrivateRef)prefs;

	assert(prefs != NULL);
	__SCPreferencesAccess(prefs);

	if ((prefsPrivate->prefs == NULL) ||
	    (CFDictionaryGetCount(prefsPrivate->prefs) == 0)) {
		return TRUE;
	}

	return FALSE;
}


__private_extern__
off_t
__SCPreferencesPrefsSize(SCPreferencesRef prefs)
{
	SCPreferencesPrivateRef	prefsPrivate	= (SCPreferencesPrivateRef)prefs;
	SCPSignatureDataRef	sig;
	CFDataRef		signature;

	signature = prefsPrivate->signature;
	if (signature == NULL) {
		return 0;
	}

	sig = (SCPSignatureDataRef)(void *)CFDataGetBytePtr(signature);
	return sig->st_size;
}


__private_extern__
Boolean
__SCPreferencesUsingDefaultPrefs(SCPreferencesRef prefs)
{
	char			*curPath;
	Boolean			isDefault = FALSE;
	SCPreferencesPrivateRef prefsPrivate = (SCPreferencesPrivateRef)prefs;

	if (prefs == NULL) {
		// if no prefs, assume that we are using the "default" prefs
		return TRUE;
	}

	curPath = prefsPrivate->newPath ? prefsPrivate->newPath : prefsPrivate->path;
	if (curPath != NULL) {
		char*	defPath;

		defPath = __SCPreferencesPath(NULL,
					      NULL,
					      (prefsPrivate->newPath == NULL));
		if (defPath != NULL) {
			if (strcmp(curPath, defPath) == 0) {
				isDefault = TRUE;
			}
			CFAllocatorDeallocate(NULL, defPath);
		}
	}
	return isDefault;
}

__private_extern__ CF_RETURNS_RETAINED CFStringRef
_SCPNotificationKey(CFAllocatorRef	allocator,
		    CFStringRef		prefsID,
		    int			keyType)
{
	CFStringRef	keyStr;
	char		*path;
	CFStringRef	pathStr;
	CFStringRef	storeKey;

	switch (keyType) {
		case kSCPreferencesKeyLock :
			keyStr = CFSTR("lock");
			break;
		case kSCPreferencesKeyCommit :
			keyStr = CFSTR("commit");
			break;
		case kSCPreferencesKeyApply :
			keyStr = CFSTR("apply");
			break;
		default :
			return NULL;
	}

	path = __SCPreferencesPath(allocator, prefsID, TRUE);
	if (path == NULL) {
		return NULL;
	}

	pathStr = CFStringCreateWithCStringNoCopy(allocator,
						  path,
						  kCFStringEncodingASCII,
						  kCFAllocatorNull);

	storeKey = CFStringCreateWithFormat(allocator,
					    NULL,
					    CFSTR("%@%@:%@"),
					    kSCDynamicStoreDomainPrefs,
					    keyStr,
					    pathStr);

	CFRelease(pathStr);
	CFAllocatorDeallocate(NULL, path);
	return storeKey;
}

