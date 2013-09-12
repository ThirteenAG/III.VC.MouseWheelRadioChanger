#ifndef __GINPUTAPI
#define __GINPUTAPI

// GInput 1.02 API file

// With this little API prepared for C++, you can take advantage of some GInput features
// For functions list, scroll down to the interface declaration

// As this API file is compatible with both GInputIII and GInputVC, you need to declare
// whether to compile VC version, III version, or a cross-compatible version
// To target GTA VC, define GINPUT_COMPILE_VC_VERSION in your project settings, code headers or just
// uncomment the line below
//#define GINPUT_COMPILE_VC_VERSION

// You can also target both games at once by defining GINPUT_COMPILE_CROSSCOMPATIBLE_VERSION - define it
// in your project settings, code headers or just uncomment the line below
//#define GINPUT_COMPILE_CROSSCOMPATIBLE_VERSION

enum eGInputEvent
{
	GINPUT_NO_EVENT						= -1,	// DON'T USE, internal use only

	GINPUT_EVENT_CHANGE_RADIO_STATION,			// Returns NULL all the time, dwParam specifies whether to set
												// next or previous radio station (0 for next station, 1 for previous)
	NUM_GINPUT_EVENTS
};

// Internal declarations
#ifndef GINPUT_COMPILE_CROSSCOMPATIBLE_VERSION
#ifdef GINPUT_COMPILE_VC_VERSION
#define GINPUT_FILENAME "GInputVC.asi"
#else
#define GINPUT_FILENAME "GInputIII.asi"
#endif
#endif

#define GINPUT_MODVERSION 0x00010002

// The pad interface
// The interface is safe to use without validating a pointer - in case of GInput loading failure,
// these functions are set to return false all the time
// NOTE: Do not use any of these functions before GInput_Load is called on your interface pointer!
class IGInputPad
{
private:
	void					operator delete(void*) { };

protected:
	virtual					~IGInputPad() { };

public:
	// Returns true when XInput compatible pad is connected
	virtual bool			IsPadConnected()=0;

	// Returns true when last input came from a pad, false otherwise
	virtual bool			HasPadInHands()=0;

	// Returns installed GInput version, -1 on failure
	virtual int				GetVersion()=0;

	// Sends an event to GInput, allowing the script to take advantage of GInput features not available
	// through CPad or other original GTA functions
	// See eGInputEvent enum for supported events and their params/return values (if any)
	virtual unsigned int	SendEvent(eGInputEvent eEvent, unsigned int dwParam)=0;
};

// GInput management functions

// Use this function ONCE to initialise GInput API
// Takes a pointer to pad interface pointer as an argument, returns true if succeed and false otherwise
// (GInput not installed or any other error occured)
bool GInput_Load(IGInputPad** pInterfacePtr);

// Releases GInput API
// Call it when your program terminates
void GInput_Release();


// Management functions definitions - internal use only, do not change anything here
#include <windows.h>

static HMODULE		hGInputHandle = NULL;	// GInput ASI handle

// Although these functions may not be inlined, we need to declare them as so
static inline IGInputPad* _GInput_SafeMode()
{
	static class CDummyPad : public IGInputPad
	{
	public:
		virtual bool			IsPadConnected() { return false; };
		virtual bool			HasPadInHands() { return false; };
		virtual int				GetVersion() { return -1; };
		virtual unsigned int	SendEvent(eGInputEvent eEvent, unsigned int dwParam) { UNREFERENCED_PARAMETER(eEvent); UNREFERENCED_PARAMETER(dwParam); return NULL; };
	} DummyClass;
	return &DummyClass;
}

inline bool GInput_Load(IGInputPad** pInterfacePtr)
{
	static IGInputPad*	pCopiedPtr = NULL;		// We keep a backup of the interface pointer in case user calls GInput_Load multiple times
	static bool			bLoadingResult = false;	// Loading result is also cached so GInput_Load always returns the same value when called multiple times

	// Have we attempted to load GInput already? If so, just return a valid interface pointer and return
	// The pointer can be either a GInput interface or a dummy, 'safe-mode' interface which got initialised
	// due to GInput*.asi loading failure
	if ( pCopiedPtr )
	{
		*pInterfacePtr = pCopiedPtr;
		return bLoadingResult;
	}
	// Cross compatible version?
#ifdef GINPUT_COMPILE_CROSSCOMPATIBLE_VERSION
	hGInputHandle = LoadLibraryA("GInputIII.asi");
	if ( !hGInputHandle )
	{
		hGInputHandle = LoadLibraryA("mss\\GInputIII.asi");
		if ( !hGInputHandle )
		{
			// Still failed? Try VC
			hGInputHandle = LoadLibraryA("GInputVC.asi");
			if ( !hGInputHandle )
			{
				hGInputHandle = LoadLibraryA("mss\\GInputVC.asi");
				if ( !hGInputHandle )
				{
					*pInterfacePtr = pCopiedPtr = _GInput_SafeMode();
					bLoadingResult = false;
					return false;
				}
			}
		}
	}
#else
	// If not, compile non-cross compatible code
	hGInputHandle = LoadLibraryA(GINPUT_FILENAME);
	if ( !hGInputHandle )
	{
		// Could not load GInput*.asi, let's try loading from mss folder
		hGInputHandle = LoadLibraryA("mss\\"GINPUT_FILENAME);
		if ( !hGInputHandle )
		{
			// Still failed? The mod is probably not installed, so let's jump into 'safe-mode' and initialise
			// a dummy interface
			*pInterfacePtr = pCopiedPtr = _GInput_SafeMode();
			bLoadingResult = false;
			return false;
		}
	}
#endif
	// Let's call a GInput export to get the proper interface
	IGInputPad*		(*ExportFunc)() = (IGInputPad*(*)())GetProcAddress(hGInputHandle, (LPCSTR)1);

	// Just to make sure function pointer is valid (will not be valid if GInput 1.0 or 1.01 is used)
	if ( !ExportFunc )
	{
		// Probably too old GInput version, no API support yet (applies only to GInputVC, though)
		*pInterfacePtr = pCopiedPtr = _GInput_SafeMode();
		bLoadingResult = false;
		return false;
	}

	*pInterfacePtr = pCopiedPtr = ExportFunc();
	if ( pCopiedPtr )
	{
		bLoadingResult = true;
		return true;
	}
	else
	{
		// GInput loaded, but for some reason there's no valid interface pointer - let's do the same safe-mode trick
		*pInterfacePtr = pCopiedPtr = _GInput_SafeMode();
		bLoadingResult = false;
		return false;
	}
}

inline void GInput_Release()
{
	// Just release the ASI
	if ( hGInputHandle )
		FreeLibrary(hGInputHandle);
}

#endif