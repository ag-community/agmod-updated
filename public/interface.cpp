#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "interface.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#else

#include <dlfcn.h> // dlopen,dlclose, et al
#include <unistd.h>

#define HMODULE void *
#define GetProcAddress dlsym

// Linux doesn't have this function so this emulates its functionality
//
//
void *GetModuleHandle(const char *name)
{
        void *handle;


        if( name == NULL )
        {
                // hmm, how can this be handled under linux....
                // is it even needed?
                return NULL;
        }

        if( (handle=dlopen(name, RTLD_NOW))==NULL)
        {   
		//printf("Error:%s\n",dlerror());
                // couldn't open this file
                return NULL;
        }

        // read "man dlopen" for details
        // in short dlopen() inc a ref count
        // so dec the ref count by performing the close
        dlclose(handle);
       return handle;
}
#endif

// ------------------------------------------------------------------------------------ //
// InterfaceReg.
// ------------------------------------------------------------------------------------ //
InterfaceReg *InterfaceReg::s_pInterfaceRegs = NULL;


InterfaceReg::InterfaceReg( InstantiateInterfaceFn fn, const char *pName ) :
	m_pName(pName)
{
	m_CreateFn = fn;
	m_pNext = s_pInterfaceRegs;
	s_pInterfaceRegs = this;
}



// ------------------------------------------------------------------------------------ //
// CreateInterface.
// ------------------------------------------------------------------------------------ //
DLLEXPORT IBaseInterface *CreateInterface( const char *pName, int *pReturnCode )
{
	InterfaceReg *pCur;
	
	for(pCur=InterfaceReg::s_pInterfaceRegs; pCur; pCur=pCur->m_pNext)
	{
		if(strcmp(pCur->m_pName, pName) == 0)
		{
			if ( pReturnCode )
			{
				*pReturnCode = IFACE_OK;
			}
			return pCur->m_CreateFn();
		}
	}
	
	if ( pReturnCode )
	{
		*pReturnCode = IFACE_FAILED;
	}
	return NULL;	
}

//Local version of CreateInterface, marked hidden so that it is never merged with the version in other libraries
static DLLHIDDEN IBaseInterface *CreateInterfaceLocal( const char *pName, int *pReturnCode )
{
	InterfaceReg *pCur;
	
	for(pCur=InterfaceReg::s_pInterfaceRegs; pCur; pCur=pCur->m_pNext)
	{
		if(strcmp(pCur->m_pName, pName) == 0)
		{
			if ( pReturnCode )
			{
				*pReturnCode = IFACE_OK;
			}
			return pCur->m_CreateFn();
		}
	}
	
	if ( pReturnCode )
	{
		*pReturnCode = IFACE_FAILED;
	}
	return NULL;	
}


//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : pModuleName - module name
//			*pName - proc name
//-----------------------------------------------------------------------------
//static hlds_run wants to use this function 
static void *Sys_GetProcAddress( const char *pModuleName, const char *pName )
{
	return GetProcAddress( GetModuleHandle(pModuleName), pName );
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : pModuleName - module name
//			*pName - proc name
//-----------------------------------------------------------------------------
// hlds_run wants to use this function 
void *Sys_GetProcAddress( void *pModuleHandle, const char *pName )
{
	return GetProcAddress( (HMODULE)pModuleHandle, pName );
}

//-----------------------------------------------------------------------------
// Purpose: Loads a DLL/component from disk and returns a handle to it
// Input  : *pModuleName - filename of the component
// Output : opaque handle to the module (hides system dependency)
//-----------------------------------------------------------------------------
CSysModule	*Sys_LoadModule( const char *pModuleName )
{
#if defined ( _WIN32 )
	HMODULE hDLL = LoadLibrary( pModuleName );
#else
	HMODULE hDLL  = NULL;
	char szAbsoluteModuleName[1024];
	szAbsoluteModuleName[0] = 0;
	if ( pModuleName[0] != '/' )
	{
		char szCwd[1024];
		char szAbsoluteModuleName[1024];

		getcwd( szCwd, sizeof( szCwd ) );
		if ( szCwd[ strlen( szCwd ) - 1 ] == '/' )
			szCwd[ strlen( szCwd ) - 1 ] = 0;

		_snprintf( szAbsoluteModuleName, sizeof(szAbsoluteModuleName), "%s/%s", szCwd, pModuleName );

		hDLL = dlopen( szAbsoluteModuleName, RTLD_NOW );
	}
	else
	{
		_snprintf( szAbsoluteModuleName, sizeof(szAbsoluteModuleName), "%s", pModuleName );
		 hDLL = dlopen( pModuleName, RTLD_NOW );
	}
#endif

	if( !hDLL )
	{
		char str[512];
#if defined ( _WIN32 )
		_snprintf( str, sizeof(str), "%s.dll", pModuleName );
		hDLL = LoadLibrary( str );
#elif defined(OSX)
		printf("Error:%s\n",dlerror());
		_snprintf( str, sizeof(str), "%s.dylib", szAbsoluteModuleName );
		hDLL = dlopen(str, RTLD_NOW);		
#else
		printf("Error:%s\n",dlerror());
		_snprintf( str, sizeof(str), "%s.so", szAbsoluteModuleName );
		hDLL = dlopen(str, RTLD_NOW);
#endif
	}

	return reinterpret_cast<CSysModule *>(hDLL);
}

//-----------------------------------------------------------------------------
// Purpose: Unloads a DLL/component from
// Input  : *pModuleName - filename of the component
// Output : opaque handle to the module (hides system dependency)
//-----------------------------------------------------------------------------
void Sys_UnloadModule( CSysModule *pModule )
{
	if ( !pModule )
		return;

	HMODULE	hDLL = reinterpret_cast<HMODULE>(pModule);
#if defined ( _WIN32 )
	FreeLibrary( hDLL );
#else
	dlclose((void *)hDLL);
#endif

}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : module - windows HMODULE from Sys_LoadModule() 
//			*pName - proc name
// Output : factory for this module
//-----------------------------------------------------------------------------
CreateInterfaceFn Sys_GetFactory( CSysModule *pModule )
{
	if ( !pModule )
		return NULL;

	HMODULE	hDLL = reinterpret_cast<HMODULE>(pModule);

	//This used to cause problems when compiling with GCC,
	//but it is now allowed to convert between pointer-to-object to pointer-to-function
	//See https://en.cppreference.com/w/cpp/language/reinterpret_cast for more information
	return reinterpret_cast<CreateInterfaceFn>(GetProcAddress( hDLL, CREATEINTERFACE_PROCNAME ));
}

//-----------------------------------------------------------------------------
// Purpose: returns the instance of this module
// Output : interface_instance_t
//-----------------------------------------------------------------------------
CreateInterfaceFn Sys_GetFactoryThis( void )
{
	return CreateInterfaceLocal;
}

//-----------------------------------------------------------------------------
// Purpose: returns the instance of the named module
// Input  : *pModuleName - name of the module
// Output : interface_instance_t - instance of that module
//-----------------------------------------------------------------------------
CreateInterfaceFn Sys_GetFactory( const char *pModuleName )
{
	//See Sys_GetFactory for why it's now safe to use reinterpret_cast
	return reinterpret_cast<CreateInterfaceFn>( Sys_GetProcAddress( pModuleName, CREATEINTERFACE_PROCNAME ) );
}



