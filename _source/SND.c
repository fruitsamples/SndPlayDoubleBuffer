/*	File:		SND.c	Contains:	Routines demonstrating how to parse 'snd ' resource files.	Written by: Mark Cookson		Copyright:	Copyright � 1996-1999 by Apple Computer, Inc., All Rights Reserved.				You may incorporate this Apple sample source code into your program(s) without				restriction. This Apple sample source code has been provided "AS IS" and the				responsibility for its operation is yours. You are not permitted to redistribute				this Apple sample source code as "Apple sample source code" after having made				changes. If you're going to re-distribute the source, we require that you make				it clear in the source that the code was descended from Apple sample source				code, but that you've made changes.	Change History (most recent first):				8/31/1999	Karl Groethe	Updated for Metrowerks Codewarror Pro 2.1				*/#include "SND.h"/*	There are lots of places for reading a resource to go wrong.  This is	why I currently have the paranoid error checking.	This routine works like the other parse header routines, except that	since it doesn't do any Resource Manager calls, it expects the file	pointer to be positioned right at the first byte of the data of the	'snd ' resource.  This way the FSRead that reads the header will work.	The length calculation could probably use some work, but that would	require modifying the read routines.  Right now we just fudge the length	so that we will read all of a resource.*//*-----------------------------------------------------------------------*/		OSErr	ASoundGetSNDHeader		(SoundInfoPtr theSoundInfo,										long *dataStart,										long *length)/*-----------------------------------------------------------------------*/{	SoundComponentData		sndInfo;	Ptr						theSoundHeader	= nil;	unsigned long			numFrames		= 0,							dataOffset		= 0;	long					headerBytes		= kMaxSNDHeaderSize;	short					resNum			= 0;	OSErr					theErr			= noErr;	*dataStart = kInit;	*length = kInit;	theSoundHeader = NewPtr (headerBytes);	theErr = MemError();	if (theSoundHeader != nil || theErr == noErr) {		/* This returns the number of the first resource of type 'snd ' */		theErr = MyGetFirstResource (theSoundInfo->refNum, 'snd ', &resNum);		if (theErr == noErr) {			theErr = MyGetResourcePosition (theSoundInfo->refNum, 'snd ', resNum, dataStart);			if (theErr == noErr) {				theErr = FSRead (theSoundInfo->refNum, &headerBytes, theSoundHeader);				if (theErr == noErr) {					/* Could use SM 3.2's ParseSndHeader, but this is (supposed to be) educational */					theErr = MyParseSndHeader (&(SndListResource*)theSoundHeader, &sndInfo, &numFrames, &dataOffset);					if (theErr == noErr) {						DisposePtr (theSoundHeader);				/* It's work is done */						theErr = SetupDBHeader (theSoundInfo,												sndInfo.sampleRate,												sndInfo.sampleSize,												sndInfo.numChannels,												fixedCompression,	/* fixedCompression will work for uncompressed */												sndInfo.format);	/* format is the really important field */						theSoundInfo->needsMasking = false;			/* 'snd ' resources never need masking */						*length = numFrames * theSoundInfo->doubleHeader.dbhPacketSize * sndInfo.numChannels;						*dataStart += dataOffset;					}					else {						DebugPrint ("\pParseSndHeader failed");					}				}				else {					DebugPrint ("\pFSRead failed");				}			}			else {				DebugPrint ("\pMyGetResourcePosition failed");			}		}		else {			DebugPrint ("\pMyGetFirstResource failed");		}	}	else {		DebugPrint ("\pNewPtr failed");	}	if (theErr != noErr) {		DebugPrint ("\pError in ASoundGetSNDHeader");	}	*length += *dataStart;	/* Otherwise we wouldn't read the last few bytes from the end of the sound. */	return theErr;}/*	This code parses a 'snd ' resource header.  While Sound Manager 3.2 will	do this for you, this is for education (and entertainment).  Just in case	the Sound Manager routine is better, try it first, if it's available.*//*-----------------------------------------------------------------------*/		OSErr	MyParseSndHeader		(SndListHandle theSoundHeader,										SoundComponentData *sndInfo,										unsigned long *numFrames,										unsigned long *dataOffset)/*-----------------------------------------------------------------------*/{	NumVersion				SndManagerVer;	headerTemplate			theHeader;	long					headerOffset	= 0;	OSErr					theErr			= noErr;	short					numChannels		= 0,							numCommands		= 0,							i				= 0;	Boolean					parsed			= false;	unsigned char			headerFormat	= 0;	SndManagerVer = SndSoundManagerVersion ();	/* If SM 3.2 is available, use its ParseSndHeader function */	if (SndManagerVer.majorRev >= 3 && SndManagerVer.minorAndBugRev >= 32) {		theErr = ParseSndHeader (theSoundHeader, sndInfo, numFrames, dataOffset);		parsed = true;	}	/* If it's not available, or it failed, let's try it ourselves */	if (theErr != noErr || parsed == false) {		theErr = noErr;		switch ((*theSoundHeader)->format) {			case firstSoundFormat:					/* Normal 'snd ' resource */				if ((*theSoundHeader)->modifierPart->modNumber != kSampledSound) {					theErr = badFormat;				/* can only deal with sampled-sound data */				}				else {					numCommands = (*theSoundHeader)->numCommands;					if (numCommands != 1) {						theErr = badFormat;			/* can only deal with one sound per resource, for now */					}					else {						for (i = 0; i < numCommands; i++) {							if ((*theSoundHeader)->commandPart->cmd == kBufferCmd) {								headerOffset = (*theSoundHeader)->commandPart->param2;							}							else {								theErr = badFormat;	/* can only deal with sampled-sound data */							}						}					}				}				break;			case secondSoundFormat:					/* Hypercard 'snd ' resource */				numCommands = ((Snd2ListPtr)(*theSoundHeader))->numCommands;				if (numCommands != 1) {					theErr = badFormat;				/* can only deal with one sound per resource, for now */				}				else {					for (i = 0; i < numCommands; i++) {						if (((Snd2ListPtr)(*theSoundHeader))->commandPart->cmd == kBufferCmd) {							headerOffset = ((Snd2ListPtr)(*theSoundHeader))->commandPart->param2;						}						else {							theErr = badFormat;		/* can only deal with sampled-sound data */						}					}				}				break;			default:				theErr = badFormat;					/* unknown resource format */		}		if (theErr == noErr) {			theHeader.standardHeaderPtr = (SoundHeaderPtr)((Ptr)(*theSoundHeader)+headerOffset);			switch (theHeader.standardHeaderPtr->encode) {				case stdSH:					theHeader.standardHeaderPtr = (SoundHeaderPtr)((Ptr)(*theSoundHeader)+headerOffset);					sndInfo->format			= kOffsetBinary;	/* Can only be raw sounds */					switch ((*theSoundHeader)->modifierPart->modInit & kChannelsMask) {						case initMono:							sndInfo->numChannels	= kMono;							break;						case initStereo:							sndInfo->numChannels	= kStereo;							break;						default:							theErr = badFormat;					/* unknown number of channels */					}					sndInfo->sampleSize		= k8BitSample;		/* Can only be 8 bit sounds */					sndInfo->sampleRate		= theHeader.standardHeaderPtr->sampleRate;					sndInfo->sampleCount	= theHeader.standardHeaderPtr->length;					*dataOffset				= headerOffset + sizeof (SoundHeader) - sizeof (short);					break;				case extSH:					theHeader.extendedHeaderPtr = (ExtSoundHeaderPtr)((Ptr)(*theSoundHeader)+headerOffset);					sndInfo->format			= kOffsetBinary;	/* Can only be raw sounds */					sndInfo->numChannels	= theHeader.extendedHeaderPtr->numChannels;					sndInfo->sampleSize		= theHeader.extendedHeaderPtr->sampleSize;					sndInfo->sampleRate		= theHeader.extendedHeaderPtr->sampleRate;					sndInfo->sampleCount	= theHeader.extendedHeaderPtr->numFrames;					*dataOffset				= headerOffset + sizeof (ExtSoundHeader) - sizeof (short);					break;				case cmpSH:					theHeader.compressedHeaderPtr = (CmpSoundHeaderPtr)((Ptr)(*theSoundHeader)+headerOffset);					sndInfo->format			= theHeader.compressedHeaderPtr->format;					if (sndInfo->format == 0) {						switch (theHeader.compressedHeaderPtr->compressionID) {							case twoToOne:								sndInfo->format 	= kULawCompression;								break;							case eightToThree:					/* I don't know what compressor this is */								theErr = badFormat;								break;							case threeToOne:								sndInfo->format 	= kMACE3Compression;								break;							case sixToOne:								sndInfo->format 	= kMACE6Compression;								break;							default:								DebugPrint ("\pUnknown sound format");								theErr = badFormat;						}					}					sndInfo->numChannels	= theHeader.compressedHeaderPtr->numChannels;					sndInfo->sampleSize		= theHeader.compressedHeaderPtr->sampleSize;					sndInfo->sampleRate		= theHeader.compressedHeaderPtr->sampleRate;					sndInfo->sampleCount	= theHeader.compressedHeaderPtr->numFrames;					*dataOffset				= headerOffset + sizeof (CmpSoundHeader) - sizeof (short);					break;				default:					theErr = badFormat;		/* A header format we don't know about */			}			*numFrames				= (unsigned long)sndInfo->sampleCount;			sndInfo->flags			= 0;	/* ?? */			sndInfo->buffer			= 0;	/* should always be 0, data follows header */			sndInfo->reserved		= 0;	/* just set it to 0 */		}	}	return theErr;}