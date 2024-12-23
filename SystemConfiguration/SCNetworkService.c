/*
 * Copyright (c) 2004-2020 Apple Inc. All rights reserved.
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
 * May 13, 2004		Allan Nathanson <ajn@apple.com>
 * - initial revision
 */


#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFRuntime.h>
#include "SCNetworkConfigurationInternal.h"
#include "SCPreferencesInternal.h"

#include <net/if.h>
#include <pthread.h>

#include <CommonCrypto/CommonDigest.h>

#define EXTERNAL_ID_DOMAIN_PREFIX	"_"

static CFStringRef	__SCNetworkServiceCopyDescription	(CFTypeRef cf);
static void		__SCNetworkServiceDeallocate		(CFTypeRef cf);
static Boolean		__SCNetworkServiceEqual			(CFTypeRef cf1, CFTypeRef cf2);
static CFHashCode	__SCNetworkServiceHash			(CFTypeRef cf);


static CFTypeID __kSCNetworkServiceTypeID		= _kCFRuntimeNotATypeID;


static const CFRuntimeClass __SCNetworkServiceClass = {
	0,					// version
	"SCNetworkService",			// className
	NULL,					// init
	NULL,					// copy
	__SCNetworkServiceDeallocate,		// dealloc
	__SCNetworkServiceEqual,		// equal
	__SCNetworkServiceHash,			// hash
	NULL,					// copyFormattingDesc
	__SCNetworkServiceCopyDescription	// copyDebugDesc
};


static pthread_once_t		initialized		= PTHREAD_ONCE_INIT;


static CFStringRef
__SCNetworkServiceCopyDescription(CFTypeRef cf)
{
	CFAllocatorRef			allocator	= CFGetAllocator(cf);
	CFMutableStringRef		result;
	SCNetworkServiceRef		service		= (SCNetworkServiceRef)cf;
	SCNetworkServicePrivateRef	servicePrivate	= (SCNetworkServicePrivateRef)service;

	result = CFStringCreateMutable(allocator, 0);
	CFStringAppendFormat(result, NULL, CFSTR("<SCNetworkService %p [%p]> {"), service, allocator);
	CFStringAppendFormat(result, NULL, CFSTR("id = %@"), servicePrivate->serviceID);
	if (servicePrivate->prefs != NULL) {
		CFStringAppendFormat(result, NULL, CFSTR(", prefs = %p"), servicePrivate->prefs);
	} else if (servicePrivate->store != NULL) {
		CFStringAppendFormat(result, NULL, CFSTR(", store = %p"), servicePrivate->store);
	}
	if (servicePrivate->name != NULL) {
		CFStringAppendFormat(result, NULL, CFSTR(", name = %@"), servicePrivate->name);
	}
	if (!__SCNetworkServiceExists(service)) {
		CFStringAppendFormat(result, NULL, CFSTR(", REMOVED"));
	}
	CFStringAppendFormat(result, NULL, CFSTR("}"));

	return result;
}


static void
__SCNetworkServiceDeallocate(CFTypeRef cf)
{
	SCNetworkServicePrivateRef	servicePrivate	= (SCNetworkServicePrivateRef)cf;

	/* release resources */

	CFRelease(servicePrivate->serviceID);
	if (servicePrivate->interface != NULL) CFRelease(servicePrivate->interface);
	if (servicePrivate->prefs != NULL) CFRelease(servicePrivate->prefs);
	if (servicePrivate->store != NULL) CFRelease(servicePrivate->store);
	if (servicePrivate->name != NULL) CFRelease(servicePrivate->name);
	if (servicePrivate->externalIDs != NULL) CFRelease(servicePrivate->externalIDs);

	return;
}


static Boolean
__SCNetworkServiceEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	SCNetworkServicePrivateRef	s1	= (SCNetworkServicePrivateRef)cf1;
	SCNetworkServicePrivateRef	s2	= (SCNetworkServicePrivateRef)cf2;

	if (s1 == s2)
		return TRUE;

	if (s1->prefs != s2->prefs)
		return FALSE;   // if not the same prefs

	if (!CFEqual(s1->serviceID, s2->serviceID))
		return FALSE;	// if not the same service identifier

	return TRUE;
}


static CFHashCode
__SCNetworkServiceHash(CFTypeRef cf)
{
	SCNetworkServicePrivateRef	servicePrivate	= (SCNetworkServicePrivateRef)cf;

	return CFHash(servicePrivate->serviceID);
}


static void
__SCNetworkServiceInitialize(void)
{
	__kSCNetworkServiceTypeID = _CFRuntimeRegisterClass(&__SCNetworkServiceClass);
	return;
}


__private_extern__ SCNetworkServicePrivateRef
__SCNetworkServiceCreatePrivate(CFAllocatorRef		allocator,
				SCPreferencesRef	prefs,
				CFStringRef		serviceID,
				SCNetworkInterfaceRef   interface)
{
	SCNetworkServicePrivateRef		servicePrivate;
	uint32_t				size;

	/* initialize runtime */
	pthread_once(&initialized, __SCNetworkServiceInitialize);

	/* allocate target */
	size           = sizeof(SCNetworkServicePrivate) - sizeof(CFRuntimeBase);
	servicePrivate = (SCNetworkServicePrivateRef)_CFRuntimeCreateInstance(allocator,
									      __kSCNetworkServiceTypeID,
									      size,
									      NULL);
	if (servicePrivate == NULL) {
		return NULL;
	}

	/* initialize non-zero/NULL members */
	servicePrivate->prefs		= (prefs != NULL) ? CFRetain(prefs): NULL;
	servicePrivate->serviceID	= CFStringCreateCopy(NULL, serviceID);
	servicePrivate->interface       = (interface != NULL) ? CFRetain(interface) : NULL;

	return servicePrivate;
}


#pragma mark -
#pragma mark SCNetworkService APIs


#define	N_QUICK	64


__private_extern__ CFArrayRef /* of SCNetworkServiceRef's */
__SCNetworkServiceCopyAllEnabled(SCPreferencesRef prefs)
{
	CFMutableArrayRef	array	= NULL;
	CFIndex			i_sets;
	CFIndex			n_sets;
	CFArrayRef		sets;

	sets = SCNetworkSetCopyAll(prefs);
	if (sets == NULL) {
		return NULL;
	}

	n_sets = CFArrayGetCount(sets);
	for (i_sets = 0; i_sets < n_sets; i_sets++) {
		CFIndex		i_services;
		CFIndex		n_services;
		CFArrayRef	services;
		SCNetworkSetRef	set;

		set = CFArrayGetValueAtIndex(sets, i_sets);
		services = SCNetworkSetCopyServices(set);
		if (services == NULL) {
			continue;
		}

		n_services = CFArrayGetCount(services);
		for (i_services = 0; i_services < n_services; i_services++) {
			SCNetworkServiceRef service;

			service = CFArrayGetValueAtIndex(services, i_services);
			if (!SCNetworkServiceGetEnabled(service)) {
				// if not enabled
				continue;
			}

			if ((array == NULL) ||
			    !CFArrayContainsValue(array,
						  CFRangeMake(0, CFArrayGetCount(array)),
						  service)) {
				if (array == NULL) {
					array = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
				}
				CFArrayAppendValue(array, service);
			}
		}
		CFRelease(services);
	}
	CFRelease(sets);

	return array;
}


__private_extern__ Boolean
__SCNetworkServiceExistsForInterface(CFArrayRef services, SCNetworkInterfaceRef interface)
{
	CFIndex	i;
	CFIndex	n;

	n = isA_CFArray(services) ? CFArrayGetCount(services) : 0;
	for (i = 0; i < n; i++) {
		SCNetworkServiceRef	service;
		SCNetworkInterfaceRef	service_interface;

		service = CFArrayGetValueAtIndex(services, i);

		service_interface = SCNetworkServiceGetInterface(service);
		while (service_interface != NULL) {
			if (CFEqual(interface, service_interface)) {
				return TRUE;
			}

			service_interface = SCNetworkInterfaceGetInterface(service_interface);
		}
	}

	return FALSE;
}


static void
mergeDict(const void *key, const void *value, void *context)
{
	CFMutableDictionaryRef	newDict	= (CFMutableDictionaryRef)context;

	CFDictionarySetValue(newDict, key, value);
	return;
}


static CF_RETURNS_RETAINED CFDictionaryRef
_protocolTemplate(SCNetworkServiceRef service, CFStringRef protocolType)
{
	SCNetworkInterfaceRef		interface;
	SCNetworkServicePrivateRef      servicePrivate  = (SCNetworkServicePrivateRef)service;
	CFDictionaryRef			template	= NULL;

	interface = servicePrivate->interface;
	if (interface != NULL) {
		SCNetworkInterfaceRef   childInterface;
		CFStringRef		childInterfaceType      = NULL;
		CFStringRef		interfaceType;

		// get the template
		interfaceType = SCNetworkInterfaceGetInterfaceType(servicePrivate->interface);
		childInterface = SCNetworkInterfaceGetInterface(servicePrivate->interface);
		if (childInterface != NULL) {
			childInterfaceType = SCNetworkInterfaceGetInterfaceType(childInterface);
		}

		template = __copyProtocolTemplate(interfaceType, childInterfaceType, protocolType);
		if (template != NULL) {
			CFDictionaryRef		overrides;

			// move to the interface at the lowest layer
			while (childInterface != NULL) {
				interface = childInterface;
				childInterface = SCNetworkInterfaceGetInterface(interface);
			}

			overrides = __SCNetworkInterfaceGetTemplateOverrides(interface, protocolType);
			if (isA_CFDictionary(overrides)) {
				CFMutableDictionaryRef	newTemplate;

				newTemplate = CFDictionaryCreateMutableCopy(NULL, 0, template);
				CFDictionaryApplyFunction(overrides, mergeDict, newTemplate);
				CFRelease(template);
				template = newTemplate;
			}
		}
	}

	if (template == NULL) {
		template = CFDictionaryCreate(NULL,
					      NULL,
					      NULL,
					      0,
					      &kCFTypeDictionaryKeyCallBacks,
					      &kCFTypeDictionaryValueCallBacks);
	}

	return template;
}


__private_extern__
CFArrayRef /* of SCNetworkInterfaceRef's */
__SCNetworkServiceCopyAllInterfaces(SCPreferencesRef prefs)
{
	CFMutableArrayRef interfaces = NULL;
	CFArrayRef services = NULL;
	CFIndex servicesCount = 0;
	SCNetworkServiceRef service = NULL;
	SCNetworkInterfaceRef interface = NULL;

	services = SCNetworkServiceCopyAll(prefs);
	if (services == NULL) {
		goto done;
	}

	servicesCount = CFArrayGetCount(services);
	if (servicesCount == 0) {
		goto done;
	}

	interfaces = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);

	for (CFIndex idx = 0; idx < servicesCount; idx++) {
		service = CFArrayGetValueAtIndex(services, idx);
		interface = SCNetworkServiceGetInterface(service);

		if (!isA_SCNetworkInterface(interface)) {
			continue;
		}
		CFArrayAppendValue(interfaces, interface);
	}

	if (CFArrayGetCount(interfaces) == 0) {
		// Do not return an empty array
		CFRelease(interfaces);
		interfaces = NULL;
	}

    done:

	if (services != NULL) {
		CFRelease(services);
	}
	return  interfaces;
}


/*
 * build a list of all of a services entity types that are associated
 * with the services interface.  The list will include :
 *
 * - entity types associated with the interface type (Ethernet, FireWire, PPP, ...)
 * - entity types associated with the interface sub-type (PPPSerial, PPPoE, L2TP, PPTP, ...)
 * - entity types associated with the hardware device (Ethernet, AirPort, FireWire, Modem, ...)
 */
static CFSetRef
_copyInterfaceEntityTypes(CFDictionaryRef protocols)
{
	CFDictionaryRef interface;
	CFMutableSetRef interface_entity_types;

	interface_entity_types = CFSetCreateMutable(NULL, 0, &kCFTypeSetCallBacks);

	interface = CFDictionaryGetValue(protocols, kSCEntNetInterface);
	if (isA_CFDictionary(interface)) {
		CFStringRef	entities[]	= { kSCPropNetInterfaceType,
						    kSCPropNetInterfaceSubType,
						    kSCPropNetInterfaceHardware };

		// include the "Interface" entity itself
		CFSetAddValue(interface_entity_types, kSCEntNetInterface);

		// include the entities associated with the interface
		for (size_t i = 0; i < sizeof(entities)/sizeof(entities[0]); i++) {
			CFStringRef     entity;

			entity = CFDictionaryGetValue(interface, entities[i]);
			if (isA_CFString(entity)) {
				CFSetAddValue(interface_entity_types, entity);
			}
		}

		/*
		 * and, because we've found some misguided network preference code
		 * developers leaving [PPP] entity dictionaries around even though
		 * they are unused and/or unneeded...
		 */
		CFSetAddValue(interface_entity_types, kSCEntNetPPP);
	}

	return interface_entity_types;
}

static Boolean
__SCNetworkServiceSetInterfaceEntity(SCNetworkServiceRef     service,
				     SCNetworkInterfaceRef   interface)
{
	CFDictionaryRef			entity;
	Boolean				ok;
	CFStringRef			path;
	SCNetworkServicePrivateRef      servicePrivate		= (SCNetworkServicePrivateRef)service;

	path = SCPreferencesPathKeyCreateNetworkServiceEntity(NULL,				// allocator
							      servicePrivate->serviceID,	// service
							      kSCEntNetInterface);		// entity
	entity = __SCNetworkInterfaceCopyInterfaceEntity(interface);
	ok = SCPreferencesPathSetValue(servicePrivate->prefs, path, entity);
	CFRelease(entity);
	CFRelease(path);

	return ok;
}

static CF_RETURNS_RETAINED CFStringRef
copyInterfaceUUID(CFStringRef bsdName)
{
	union {
		unsigned char	sha256_bytes[CC_SHA256_DIGEST_LENGTH];
		CFUUIDBytes	uuid_bytes;
	} bytes;
	CC_SHA256_CTX	ctx;
	char		if_name[IF_NAMESIZE];
	CFUUIDRef	uuid;
	CFStringRef	uuid_str;

	// start with interface name
	memset(&if_name, 0, sizeof(if_name));
	(void) _SC_cfstring_to_cstring(bsdName,
				       if_name,
				       sizeof(if_name),
				       kCFStringEncodingASCII);

	// create SHA256 hash
	memset(&bytes, 0, sizeof(bytes));
	CC_SHA256_Init(&ctx);
	CC_SHA256_Update(&ctx,
			 if_name,
			 sizeof(if_name));
	CC_SHA256_Final(bytes.sha256_bytes, &ctx);

	// create UUID string
	uuid = CFUUIDCreateFromUUIDBytes(NULL, bytes.uuid_bytes);
	uuid_str = CFUUIDCreateString(NULL, uuid);
	CFRelease(uuid);

	return uuid_str;
}

#ifdef FULL_COMP
CFStringRef
SCNetworkServiceGetName(SCNetworkServiceRef service)
{
	CFDictionaryRef			entity;
	SCNetworkInterfaceRef		interface;
	CFStringRef			name		= NULL;
	CFStringRef			path;
	SCNetworkServicePrivateRef	servicePrivate	= (SCNetworkServicePrivateRef)service;
	Boolean				useSystemInterfaces = TRUE;

	if (!isA_SCNetworkService(service) || (servicePrivate->prefs == NULL)) {
		_SCErrorSet(kSCStatusInvalidArgument);
		return NULL;
	}

	if (servicePrivate->name != NULL) {
		return servicePrivate->name;
	}

	path = SCPreferencesPathKeyCreateNetworkServiceEntity(NULL,				// allocator
							      servicePrivate->serviceID,	// service
							      NULL);				// entity
	entity = SCPreferencesPathGetValue(servicePrivate->prefs, path);
	CFRelease(path);

	useSystemInterfaces = !_SCNetworkConfigurationBypassSystemInterfaces(servicePrivate->prefs);

	if (isA_CFDictionary(entity)) {
		name = CFDictionaryGetValue(entity, kSCPropUserDefinedName);
		if (isA_CFString(name)) {
			servicePrivate->name = CFRetain(name);
			if (!useSystemInterfaces) {
				return servicePrivate->name;
			}
		}
	}

	interface = SCNetworkServiceGetInterface(service);
	while (interface != NULL) {
		SCNetworkInterfaceRef   childInterface;
		CFStringRef		interfaceType;

		interfaceType = SCNetworkInterfaceGetInterfaceType(interface);
		if (CFEqual(interfaceType, kSCNetworkInterfaceTypeVPN)) {
			break;
		}

		childInterface = SCNetworkInterfaceGetInterface(interface);
		if ((childInterface == NULL) ||
		    CFEqual(childInterface, kSCNetworkInterfaceIPv4)) {
			break;
		}

		interface = childInterface;
	}

	if (interface != NULL) {
		int		i;
		CFStringRef	interface_name	= NULL;
		CFStringRef	suffix		= NULL;

		//
		// check if the [stored] service name matches the non-localized interface
		// name.  If so, return the localized name.
		//
		// Also, the older "Built-in XXX" interface names are too long for the
		// current UI. If we find that the [stored] service name matches the older
		// name, return the newer (and shorter) localized name.
		//
		// Note: the user/admin will no longer be able to set the service name
		//       to "Built-in Ethernet".
		//
		for (i = 0; i < 3; i++) {
			if (servicePrivate->name == NULL) {
				// if no [stored] service name to compare
				break;
			}

			switch (i) {
				case 0 :
					// compare the non-localized interface name
					interface_name = __SCNetworkInterfaceGetNonLocalizedDisplayName(interface);
					if (interface_name != NULL) {
						CFRetain(interface_name);
					}
					break;
				case 1 :
					// compare the older "Built-in XXX" localized name
					interface_name = __SCNetworkInterfaceCopyXLocalizedDisplayName(interface);
					break;
				case 2 :
					// compare the older "Built-in XXX" non-localized name
					interface_name = __SCNetworkInterfaceCopyXNonLocalizedDisplayName(interface);
					break;
			}

			if (interface_name != NULL) {
				Boolean	match	= FALSE;

				if (CFEqual(name, interface_name)) {
					// if service name matches the OLD localized
					// interface name
					match = TRUE;
				} else if (CFStringHasPrefix(name, interface_name)) {
					CFIndex	prefixLen	= CFStringGetLength(interface_name);
					CFIndex	suffixLen	= CFStringGetLength(name);

					suffix = CFStringCreateWithSubstring(NULL,
									     name,
									     CFRangeMake(prefixLen, suffixLen - prefixLen));
					match = TRUE;
				}
				CFRelease(interface_name);

				if (match) {
					CFRelease(servicePrivate->name);
					servicePrivate->name = NULL;
					break;
				}
			}
		}

		//
		// if the service name has not been set, use the localized interface name
		//
		if (servicePrivate->name == NULL) {
			interface_name = SCNetworkInterfaceGetLocalizedDisplayName(interface);
			if (interface_name != NULL) {
				if (suffix != NULL) {
					servicePrivate->name = CFStringCreateWithFormat(NULL,
											NULL,
											CFSTR("%@%@"),
											interface_name,
											suffix);
				} else {
					servicePrivate->name = CFRetain(interface_name);
				}
			}
		}
		if (suffix != NULL) CFRelease(suffix);
	}

	return servicePrivate->name;
}
#endif

#pragma mark -
#pragma mark SCNetworkService SPIs


__private_extern__
Boolean
__SCNetworkServiceExists(SCNetworkServiceRef service)
{
	CFDictionaryRef			entity;
	CFStringRef			path;
	SCNetworkServicePrivateRef      servicePrivate	= (SCNetworkServicePrivateRef)service;

	if (servicePrivate->prefs == NULL) {
		// if no prefs
		return FALSE;
	}

	path = SCPreferencesPathKeyCreateNetworkServiceEntity(NULL,				// allocator
							      servicePrivate->serviceID,	// service
							      kSCEntNetInterface);     		 // entity
	entity = SCPreferencesPathGetValue(servicePrivate->prefs, path);
	CFRelease(path);

	if (!isA_CFDictionary(entity)) {
		// a "service" must have an "interface"
		return FALSE;
	}

	return TRUE;
}

typedef struct {
	CFStringRef	oldServiceID;
	CFStringRef	newServiceID;
} serviceContext, *serviceContextRef;


static void
replaceServiceID(const void *value, void *context)
{
	CFStringRef		link		= NULL;
	CFStringRef		oldLink;
	CFMutableArrayRef	newServiceOrder;
	CFStringRef		path;
	serviceContextRef	service_context	= (serviceContextRef)context;
	CFArrayRef		serviceOrder	= NULL;
	SCNetworkSetRef		set		= (SCNetworkSetRef)value;
	SCNetworkSetPrivateRef	setPrivate	= (SCNetworkSetPrivateRef)set;

	// update service order
	serviceOrder = SCNetworkSetGetServiceOrder(set);
	if ((isA_CFArray(serviceOrder) != NULL) &&
	    CFArrayContainsValue(serviceOrder,
				  CFRangeMake(0, CFArrayGetCount(serviceOrder)),
				  service_context->oldServiceID)) {
		CFIndex	count;
		CFIndex	serviceOrderIndex;

		// replacing all instances of old service ID with new one
		newServiceOrder = CFArrayCreateMutableCopy(NULL, 0, serviceOrder);
		count = CFArrayGetCount(newServiceOrder);
		for (serviceOrderIndex = 0; serviceOrderIndex < count; serviceOrderIndex++) {
			CFStringRef	serviceID;

			serviceID = CFArrayGetValueAtIndex(newServiceOrder, serviceOrderIndex);
			if (CFEqual(serviceID, service_context->oldServiceID)) {
				CFArraySetValueAtIndex(newServiceOrder, serviceOrderIndex, service_context->newServiceID);
			}
		}
		SCNetworkSetSetServiceOrder(set, newServiceOrder);
		CFRelease(newServiceOrder);
	}

	// check if service with old serviceID is part of the set
	path = SCPreferencesPathKeyCreateSetNetworkServiceEntity(NULL,				// allocator
								 setPrivate->setID,		// set
								 service_context->oldServiceID,	// service
								 NULL);				// entity
	oldLink = SCPreferencesPathGetLink(setPrivate->prefs, path);
	if (oldLink == NULL) {
		// don't make any changes if service with old serviceID is not found
		goto done;
	}

	// remove link between "set" and old "service"
	(void) SCPreferencesPathRemoveValue(setPrivate->prefs, path);
	CFRelease(path);

	// create the link between "set" and the "service"
	path = SCPreferencesPathKeyCreateSetNetworkServiceEntity(NULL,				// allocator
								 setPrivate->setID,		// set
								 service_context->newServiceID,	// service
								 NULL);				// entity
	link = SCPreferencesPathKeyCreateNetworkServiceEntity(NULL,				// allocator
							      service_context->newServiceID,	// service
							      NULL);				// entity
	(void) SCPreferencesPathSetLink(setPrivate->prefs, path, link);

    done:

	if (path != NULL) {
		CFRelease(path);
	}
	if (link != NULL) {
		CFRelease(link);
	}

	return;
}

#define kVPNProtocolPayloadInfo			CFSTR("com.apple.payload")
#define kSCEntNetLoginWindowEAPOL		CFSTR("EAPOL.LoginWindow")

static void
copyInterfaceConfiguration(SCNetworkServiceRef oldService, SCNetworkServiceRef newService)
{
	SCNetworkInterfaceRef	oldInterface;
	SCNetworkInterfaceRef	newInterface;

	oldInterface = SCNetworkServiceGetInterface(oldService);
	newInterface = SCNetworkServiceGetInterface(newService);

	while (oldInterface != NULL) {
		CFDictionaryRef	configuration;
		CFStringRef		interfaceType;

		if (newInterface == NULL) {
			// oops ... interface layering does not match
			return;
		}

		// copy interface configuration
		configuration = SCNetworkInterfaceGetConfiguration(oldInterface);

		if ((configuration != NULL) ||
		    (SCError() == kSCStatusOK)) {
			if (!SCNetworkInterfaceSetConfiguration(newInterface, configuration)) {
				SC_log(LOG_INFO, "problem setting interface configuration");
			}

		}

		// special case: PPP/L2TP + IPSec
		interfaceType = SCNetworkInterfaceGetInterfaceType(oldInterface);
		if (CFEqual(interfaceType, kSCNetworkInterfaceTypePPP)) {
			SCNetworkInterfaceRef	childInterface;

			childInterface = SCNetworkInterfaceGetInterface(oldInterface);
			if (childInterface != NULL) {
				CFStringRef		childInterfaceType;

				childInterfaceType = SCNetworkInterfaceGetInterfaceType(childInterface);

				if (CFEqual(childInterfaceType, kSCNetworkInterfaceTypeL2TP)) {
					configuration = SCNetworkInterfaceGetExtendedConfiguration(oldInterface, kSCEntNetIPSec);
					if ((configuration != NULL) ||
					    (SCError() == kSCStatusOK)) {
						if (!SCNetworkInterfaceSetExtendedConfiguration(newInterface, kSCEntNetIPSec, configuration)) {
							SC_log(LOG_INFO, "problem setting child interface configuration");
						}
					}
				}
			}
		}

		// special case: 802.1x
		configuration = SCNetworkInterfaceGetExtendedConfiguration(oldInterface, kSCEntNetEAPOL);
		if ((configuration != NULL) ||
		    (SCError() == kSCStatusOK)) {
			(void) SCNetworkInterfaceSetExtendedConfiguration(newInterface, kSCEntNetEAPOL, configuration);
		}

		// special case: Managed Client
		configuration = SCNetworkInterfaceGetExtendedConfiguration(oldInterface, kVPNProtocolPayloadInfo);
		if ((configuration != NULL) ||
		    (SCError() == kSCStatusOK)) {
			(void) SCNetworkInterfaceSetExtendedConfiguration(newInterface, kVPNProtocolPayloadInfo, configuration);
		}

		// special case: Network Pref
		configuration = SCNetworkInterfaceGetExtendedConfiguration(oldInterface, kSCValNetPPPAuthProtocolEAP);
		if ((configuration != NULL) ||
		    (SCError() == kSCStatusOK)) {
			(void) SCNetworkInterfaceSetExtendedConfiguration(newInterface, kSCValNetPPPAuthProtocolEAP, configuration);
		}

		// special case: Remote Pref
		configuration = SCNetworkInterfaceGetExtendedConfiguration(oldInterface, kSCEntNetLoginWindowEAPOL);
		if ((configuration != NULL) ||
		    (SCError() == kSCStatusOK)) {
			(void) SCNetworkInterfaceSetExtendedConfiguration(newInterface, kSCEntNetLoginWindowEAPOL, configuration);
		}

		// special case: Network Extension
		configuration = SCNetworkInterfaceGetExtendedConfiguration(oldInterface, kSCNetworkInterfaceTypeIPSec);
		if ((configuration != NULL) ||
		    (SCError() == kSCStatusOK)) {
			(void) SCNetworkInterfaceSetExtendedConfiguration(newInterface, kSCNetworkInterfaceTypeIPSec, configuration);
		}

		oldInterface = SCNetworkInterfaceGetInterface(oldInterface);
		newInterface = SCNetworkInterfaceGetInterface(newInterface);
	}

	return;
}

__private_extern__
void
__SCNetworkServiceAddProtocolToService(SCNetworkServiceRef service, CFStringRef protocolType, CFDictionaryRef configuration, Boolean enabled)
{
	Boolean ok;
	SCNetworkProtocolRef protocol;

	protocol = SCNetworkServiceCopyProtocol(service, protocolType);

	if ((protocol == NULL) &&
	    (SCError() == kSCStatusNoKey)) {
		ok = SCNetworkServiceAddProtocolType(service, protocolType);
		if (ok) {
			protocol = SCNetworkServiceCopyProtocol(service, protocolType);
		}
	}
	if (protocol != NULL) {
		SCNetworkProtocolSetConfiguration(protocol, configuration);
		SCNetworkProtocolSetEnabled(protocol, enabled);
		CFRelease(protocol);
	}
	return;
}



__private_extern__
Boolean
__SCNetworkServiceMigrateNew(SCPreferencesRef		prefs,
			     SCNetworkServiceRef	service,
			     CFDictionaryRef		bsdNameMapping,
			     CFDictionaryRef		setMapping,
			     CFDictionaryRef		serviceSetMapping)
{
	CFStringRef			deviceName			= NULL;
	Boolean				enabled;
	SCNetworkInterfaceRef		interface			= NULL;
	CFDictionaryRef			interfaceEntity			= NULL;
	SCNetworkSetRef			newSet				= NULL;
	SCNetworkInterfaceRef		newInterface			= NULL;
	SCNetworkServiceRef		newService			= NULL;
	SCNetworkSetRef			oldSet				= NULL;
	Boolean				serviceAdded			= FALSE;
	CFStringRef			serviceID			= NULL;
	SCNetworkServicePrivateRef	servicePrivate			= (SCNetworkServicePrivateRef)service;
	CFArrayRef			setList				= NULL;
	Boolean				success				= FALSE;
	CFStringRef			targetDeviceName		= NULL;
	CFArrayRef			protocols			= NULL;

	if (!isA_SCNetworkService(service) ||
	    !isA_SCNetworkInterface(servicePrivate->interface) ||
	    (servicePrivate->prefs == NULL)) {
		goto done;
	}
	serviceID = servicePrivate->serviceID;

	newService = SCNetworkServiceCopy(prefs, serviceID);
	if (newService != NULL) {
		// Cannot add service if it already exists
		SC_log(LOG_INFO, "Service already exists");
		goto done;
	}

	interface = SCNetworkServiceGetInterface(service);
	if (interface == NULL) {
		SC_log(LOG_INFO, "No interface");
		goto done;
	}

	interfaceEntity = __SCNetworkInterfaceCopyInterfaceEntity(interface);
	if (interfaceEntity == NULL) {
		SC_log(LOG_INFO, "No interface entity");
		goto done;
	}

	if (bsdNameMapping != NULL) {
		deviceName = CFDictionaryGetValue(interfaceEntity, kSCPropNetInterfaceDeviceName);
		if (deviceName != NULL) {
			targetDeviceName = CFDictionaryGetValue(bsdNameMapping, deviceName);
			if (targetDeviceName != NULL) {
				CFStringRef		name;
				CFMutableDictionaryRef	newInterfaceEntity;

				SC_log(LOG_INFO, "  mapping \"%@\" --> \"%@\"", deviceName, targetDeviceName);

				newInterfaceEntity = CFDictionaryCreateMutableCopy(NULL, 0, interfaceEntity);

				// update mapping
				CFDictionarySetValue(newInterfaceEntity, kSCPropNetInterfaceDeviceName, targetDeviceName);

				// update interface name
				name = CFDictionaryGetValue(newInterfaceEntity, kSCPropUserDefinedName);
				if (name != NULL) {
					CFMutableStringRef	newName;

					/*
					 * the interface "entity" can include the a UserDefinedName
					 * like "Ethernet Adapter (en4)".  If this interface name is
					 * mapped to another then we want to ensure that corresponding
					 * UserDefinedName is also updated to match.
					 */
					newName = CFStringCreateMutableCopy(NULL, 0, name);
					CFStringFindAndReplace(newName,
							       deviceName,
							       targetDeviceName,
							       CFRangeMake(0, CFStringGetLength(newName)),
							       0);
					CFDictionarySetValue(newInterfaceEntity, kSCPropUserDefinedName, newName);
					CFRelease(newName);
				}

				CFRelease(interfaceEntity);
				interfaceEntity = newInterfaceEntity;
			}
		}
	}

	newInterface = _SCNetworkInterfaceCreateWithEntity(NULL,
							   interfaceEntity,
							   __kSCNetworkInterfaceSearchExternal);

	if ((setMapping == NULL) ||
	    (serviceSetMapping == NULL) ||
	    !CFDictionaryGetValueIfPresent(serviceSetMapping, service, (const void **)&setList)) {
		// if no mapping
		SC_log(LOG_INFO, "No mapping");
		goto done;
	}

	newService = SCNetworkServiceCreate(prefs, newInterface);
	if (newService == NULL) {
		SC_log(LOG_INFO, "SCNetworkServiceCreate() failed");
		goto done;
	}

	enabled = SCNetworkServiceGetEnabled(service);
	if (!SCNetworkServiceSetEnabled(newService, enabled)) {
		SCNetworkServiceRemove(newService);
		SC_log(LOG_INFO, "SCNetworkServiceSetEnabled() failed");
		goto done;
	}

	if (!SCNetworkServiceEstablishDefaultConfiguration(newService)) {
		SCNetworkServiceRemove(newService);
		SC_log(LOG_INFO, "SCNetworkServiceEstablishDefaultConfiguration() failed");
		goto done;
	}

	// Set service ID
	_SCNetworkServiceSetServiceID(newService, serviceID);

	// Determine which sets to add service
	for (CFIndex idx = 0; idx < CFArrayGetCount(setList); idx++) {
		oldSet = CFArrayGetValueAtIndex(setList, idx);
		newSet = CFDictionaryGetValue(setMapping, oldSet);
		if (newSet == NULL) {
			continue;
		}

		SC_log(LOG_INFO, "  adding service to set: %@", SCNetworkSetGetSetID(newSet));
		if (SCNetworkSetAddService(newSet, newService)) {
			serviceAdded = TRUE;
		} else {
			SC_log(LOG_INFO, "SCNetworkSetAddService() failed");
		}
	}

	if (!serviceAdded) {
		SCNetworkServiceRemove(newService);
		SC_log(LOG_INFO, "  service not added to any sets");
		goto done;
	}

	protocols = SCNetworkServiceCopyProtocols(service);
	if (protocols != NULL) {

		for (CFIndex idx = 0; idx < CFArrayGetCount(protocols); idx++) {
			SCNetworkProtocolRef protocol = CFArrayGetValueAtIndex(protocols, idx);
			CFDictionaryRef configuration = SCNetworkProtocolGetConfiguration(protocol);
			CFStringRef protocolType = SCNetworkProtocolGetProtocolType(protocol);
			enabled = SCNetworkProtocolGetEnabled(protocol);
			__SCNetworkServiceAddProtocolToService(newService, protocolType, configuration, enabled);
		}
		CFRelease(protocols);
	}

	copyInterfaceConfiguration(service, newService);

	success = TRUE;

    done:

	if (interfaceEntity != NULL) {
		CFRelease(interfaceEntity);
	}
	if (newInterface != NULL) {
		CFRelease(newInterface);
	}
	if (newService != NULL) {
		CFRelease(newService);
	}
	return success;
}


__private_extern__
Boolean
__SCNetworkServiceCreate(SCPreferencesRef	prefs,
			 SCNetworkInterfaceRef	interface,
			 CFStringRef		userDefinedName)
{
	SCNetworkSetRef currentSet = NULL;
	Boolean ok = FALSE;
	SCNetworkServiceRef service = NULL;

	if (interface == NULL) {
		goto done;
	}

	if (userDefinedName == NULL) {
		userDefinedName = __SCNetworkInterfaceGetUserDefinedName(interface);
		if (userDefinedName == NULL) {
			SC_log(LOG_INFO, "No userDefinedName");
			goto done;
		}
	}
	service = SCNetworkServiceCreate(prefs, interface);
	if (service == NULL) {
		SC_log(LOG_INFO, "SCNetworkServiceCreate() failed: %s", SCErrorString(SCError()));
	} else {
		ok = SCNetworkServiceSetName(service, userDefinedName);
		if (!ok) {
			SC_log(LOG_INFO, "SCNetworkServiceSetName() failed: %s", SCErrorString(SCError()));
			SCNetworkServiceRemove(service);
			goto done;
		}

		ok = SCNetworkServiceEstablishDefaultConfiguration(service);
		if (!ok) {
			SC_log(LOG_INFO, "SCNetworkServiceEstablishDefaultConfiguration() failed: %s", SCErrorString(SCError()));
			SCNetworkServiceRemove(service);
			goto done;
		}
	}
	currentSet = SCNetworkSetCopyCurrent(prefs);
	if (currentSet == NULL) {
		SC_log(LOG_INFO, "No current set");
		if (service != NULL) {
			SCNetworkServiceRemove(service);
		}
		goto done;
	}
	if (service != NULL) {
		ok = SCNetworkSetAddService(currentSet, service);
		if (!ok) {
			SC_log(LOG_INFO, "Could not add service to the current set");
			SCNetworkServiceRemove(service);
			goto done;
		}
	}

    done:
	if (service != NULL) {
		CFRelease(service);
	}
	if (currentSet != NULL) {
		CFRelease(currentSet);
	}
	return ok;
}

__private_extern__ Boolean
__SCNetworkServiceIsPPTP(SCNetworkServiceRef	service)
{
	CFStringRef intfSubtype;
	SCNetworkServicePrivateRef servicePrivate = (SCNetworkServicePrivateRef)service;

	if (servicePrivate == NULL || servicePrivate->interface == NULL) {
		return FALSE;
	}

	intfSubtype = __SCNetworkInterfaceGetEntitySubType(servicePrivate->interface);
	if (intfSubtype == NULL) {
		return FALSE;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
	if (CFEqual(intfSubtype, kSCValNetInterfaceSubTypePPTP)) {
		return TRUE;
	}
#pragma GCC diagnostic pop

	return FALSE;
}
