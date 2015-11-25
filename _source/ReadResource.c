/*	File:		ReadResource.c	Contains:	Routines demonstrating how to read resource files without using				the Resource Manager.	Written by: Mark Cookson		Copyright:	Copyright � 1996-1999 by Apple Computer, Inc., All Rights Reserved.				You may incorporate this Apple sample source code into your program(s) without				restriction. This Apple sample source code has been provided "AS IS" and the				responsibility for its operation is yours. You are not permitted to redistribute				this Apple sample source code as "Apple sample source code" after having made				changes. If you're going to re-distribute the source, we require that you make				it clear in the source that the code was descended from Apple sample source				code, but that you've made changes.	Change History (most recent first):				8/31/1999	Karl Groethe	Updated for Metrowerks Codewarror Pro 2.1				*/#include "ReadResource.h"/*	Purpose:		This finds the number of the first resource of chosen					type and returns that number, along with any error.	Side Effects:	None.*//*-----------------------------------------------------------------------*/		OSErr	MyGetFirstResource		(short refNum,										OSType targetType,										short *targetID)/*-----------------------------------------------------------------------*/{	ResReference	theRes;	long			dataOffset		= 0,					typeOffset		= 0,					ResRefSize		= 0,					oldPos			= 0;	short			numResources	= 0;	OSErr			theErr			= noErr;	theErr = GetFPos (refNum, &oldPos);		/* Be nice and don't move the file pointer without puting it back */	if (theErr == noErr) {		/* Find the location of the first resource of the type we want */		theErr = MyGetTypesPosition (refNum, targetType, &numResources, &dataOffset, &typeOffset);	}	if (theErr == noErr) {		/* Position the file to the start of the list of resources */		theErr = SetFPos (refNum, fsFromStart, typeOffset);	}	if (theErr == noErr) {		ResRefSize = sizeof (theRes);		if (numResources != 0) {			theErr = FSRead (refNum, &ResRefSize, &theRes);			*targetID = theRes.ID;			if (theErr == noErr) {				theErr = SetFPos (refNum, fsFromStart, oldPos);			}		}		else {			theErr = resNotFound;		}	}	return theErr;}/*	Purpose:		This finds the requested resource and positions the file					pointer to point to the first byte of that resource.  It					returns the position, inside the resource fork, of that					resource, or if unsuccessful the error.	Side Effects:	None.*//*-----------------------------------------------------------------------*/		OSErr	MyGetResourcePosition	(short refNum,										OSType targetType,										short targetID,										long *firstByte)/*-----------------------------------------------------------------------*/{	ResReference	theRes;	long			position		= 0,					dataOffset		= 0,					typeOffset		= 0,					ResRefSize		= 0;	short			numResources	= 0;	OSErr			theErr			= noErr;	Boolean			found			= false;	/* Find the location of the first resource of the type we want */	theErr = MyGetTypesPosition (refNum, targetType, &numResources, &dataOffset, &typeOffset);	if (theErr == noErr) {		/* Position the file to the start of the list of resources */		theErr = SetFPos (refNum, fsFromStart, typeOffset);	}	if (theErr == noErr) {		ResRefSize = sizeof (theRes);	}	while (theErr == noErr && numResources-- && found == false) {		theErr = FSRead (refNum, &ResRefSize, &theRes);		if (theErr == noErr && theRes.ID == targetID) {			position = (theRes.dataOffset & kDataOffset) + dataOffset;			*firstByte = position + sizeof (long);			found = true;		}	}	if (theErr == noErr) {		theErr = SetFPos (refNum, fsFromStart, *firstByte);	}	if (found == false) {		theErr = resNotFound;	}	return theErr;}/*	Purpose:		This finds the beginning of the list of requested					resource types (i.e. all 'snd ' resources).  It returns					the offset to these resources, or if unsuccessful the					error.	Side Effects:	None.*//*-----------------------------------------------------------------------*/		OSErr	MyGetTypesPosition	(short refNum,									OSType targetType,									short *numResources,									long *dataOffset,									long *firstByteOfTypeList)/*-----------------------------------------------------------------------*/{	ResourceHeader			ResForkHeader;	ResourceMap				ResMap;	ResourceTypeListEntry	typeListEntry;	long					numTypes		= 0,							numBytes		= 0,							numBytesToRead	= 0,							position		= 0,							oldPos			= 0;	OSErr					theErr			= noErr;	Boolean					found			= false;	theErr = GetFPos (refNum, &oldPos);		/* Be nice and don't move the file pointer without puting it back */	if (theErr == noErr) {		theErr = SetFPos (refNum, fsFromStart, 0);	}	if (theErr == noErr) {		/* Read the resource file header */		numBytesToRead = sizeof (ResForkHeader);		theErr = FSRead (refNum, &numBytesToRead, &ResForkHeader);	}	if (theErr == noErr) {		/* Read the resource map */		theErr = SetFPos (refNum, fsFromStart, ResForkHeader.resMapOffset);	}	if (theErr == noErr) {		numBytesToRead = sizeof (ResMap);		theErr = FSRead (refNum, &numBytesToRead, &ResMap);	}	if (theErr == noErr) {		/* Find the type list */		position = ResMap.typesListOffset + ResForkHeader.resMapOffset + 2;		theErr = SetFPos (refNum, fsFromStart, position);	}	if (theErr == noErr) {		numTypes = ResMap.numTypesInMap + 1;		position += sizeof (numTypes);		numBytes = sizeof (typeListEntry);	}	while (theErr == noErr && numTypes-- && found == false) {		theErr = FSRead (refNum, &numBytes, &typeListEntry);		if (theErr == noErr && typeListEntry.resType == targetType) {			*firstByteOfTypeList = ResForkHeader.resMapOffset + ResMap.typesListOffset + typeListEntry.referenceOffset;			*numResources = typeListEntry.numEntries + 1;			*dataOffset = ResForkHeader.resDataOffset;			found = true;		}	}	theErr = SetFPos (refNum, fsFromStart, oldPos);	if (found == false) {		theErr = resNotFound;	}	return theErr;}