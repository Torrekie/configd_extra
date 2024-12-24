#include <SystemConfiguration/SystemConfiguration.h>
#include <SystemConfiguration/SCValidation.h>
#include <SystemConfiguration/SCPrivate.h>

#pragma mark -
#pragma mark DOS encoding/codepage


void
_SC_dos_encoding_and_codepage(CFStringEncoding	macEncoding,
			      UInt32		macRegion,
			      CFStringEncoding	*dosEncoding,
			      UInt32		*dosCodepage)
{
	switch (macEncoding) {
	case kCFStringEncodingMacRoman:
		if (macRegion != 0) /* anything non-zero is not US */
		*dosEncoding = kCFStringEncodingDOSLatin1;
		else /* US region */
		*dosEncoding = kCFStringEncodingDOSLatinUS;
		break;

	case kCFStringEncodingMacJapanese:
		*dosEncoding = kCFStringEncodingDOSJapanese;
		break;

	case kCFStringEncodingMacChineseTrad:
		*dosEncoding = kCFStringEncodingDOSChineseTrad;
		break;

	case kCFStringEncodingMacKorean:
		*dosEncoding = kCFStringEncodingDOSKorean;
		break;

	case kCFStringEncodingMacArabic:
		*dosEncoding = kCFStringEncodingDOSArabic;
		break;

	case kCFStringEncodingMacHebrew:
		*dosEncoding = kCFStringEncodingDOSHebrew;
		break;

	case kCFStringEncodingMacGreek:
		*dosEncoding = kCFStringEncodingDOSGreek;
		break;

	case kCFStringEncodingMacCyrillic:
		*dosEncoding = kCFStringEncodingDOSCyrillic;
		break;

	case kCFStringEncodingMacThai:
		*dosEncoding = kCFStringEncodingDOSThai;
		break;

	case kCFStringEncodingMacChineseSimp:
		*dosEncoding = kCFStringEncodingDOSChineseSimplif;
		break;

	case kCFStringEncodingMacCentralEurRoman:
		*dosEncoding = kCFStringEncodingDOSLatin2;
		break;

	case kCFStringEncodingMacTurkish:
		*dosEncoding = kCFStringEncodingDOSTurkish;
		break;

	case kCFStringEncodingMacCroatian:
		*dosEncoding = kCFStringEncodingDOSLatin2;
		break;

	case kCFStringEncodingMacIcelandic:
		*dosEncoding = kCFStringEncodingDOSIcelandic;
		break;

	case kCFStringEncodingMacRomanian:
		*dosEncoding = kCFStringEncodingDOSLatin2;
		break;

	case kCFStringEncodingMacFarsi:
		*dosEncoding = kCFStringEncodingDOSArabic;
		break;

	case kCFStringEncodingMacUkrainian:
		*dosEncoding = kCFStringEncodingDOSCyrillic;
		break;

	default:
		*dosEncoding = kCFStringEncodingDOSLatin1;
		break;
	}

	*dosCodepage = CFStringConvertEncodingToWindowsCodepage(*dosEncoding);
	return;
}
